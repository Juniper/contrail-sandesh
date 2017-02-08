/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_server.cc
//
// Sandesh server implementation
//

#include <boost/bind.hpp>
#include <boost/assign.hpp>

#include <sandesh/protocol/TXMLProtocol.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_ctrl_types.h>
#include "sandesh_connection.h"
#include "sandesh_session.h"
#include "sandesh_server.h"

using namespace std;
using namespace boost::asio;

const std::string SandeshServer::kStateMachineTask = "sandesh::SandeshStateMachine";
const std::string SandeshServer::kLifetimeMgrTask = "sandesh::LifetimeMgr";
const std::string SandeshServer::kSessionReaderTask = "io::ReaderTask";

class SandeshServer::DeleteActor : public LifetimeActor {
public:
    DeleteActor(SandeshServer *server) :
        LifetimeActor(server->lifetime_manager()), server_(server) { }
    virtual bool MayDelete() const {
        return true;
    }
    virtual void Shutdown() {
        server_->SessionShutdown();
    }
    virtual void Destroy() {
    }
private:
    SandeshServer *server_;
};

bool SandeshServer::task_policy_set_ = false;

SandeshServer::SandeshServer(EventManager *evm, const SandeshConfig &config)
    : SslServer(evm, boost::asio::ssl::context::tlsv1_server,
                config.sandesh_ssl_enable),
      sm_task_id_(TaskScheduler::GetInstance()->GetTaskId(kStateMachineTask)),
      session_reader_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSessionReaderTask)),
      lifetime_mgr_task_id_(TaskScheduler::GetInstance()->GetTaskId(kLifetimeMgrTask)),
      lifetime_manager_(new LifetimeManager(lifetime_mgr_task_id_)),
      deleter_(new DeleteActor(this)) {
    // Set task policy for exclusion between :
    // 1. State machine and lifetime mgr since state machine delete happens
    //    in lifetime mgr task
    if (!task_policy_set_) {
        TaskPolicy lm_task_policy = boost::assign::list_of
                (TaskExclusion(sm_task_id_))
                (TaskExclusion(session_reader_task_id_));
        TaskScheduler::GetInstance()->SetPolicy(lifetime_mgr_task_id_, lm_task_policy);
        task_policy_set_ = true;
    }
    if (config.sandesh_ssl_enable) {
        boost::asio::ssl::context *ctx = context();
        boost::system::error_code ec;
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::no_sslv2, ec);
        if (ec.value() != 0) {
            SANDESH_LOG(ERROR, "Error setting ssl options: " << ec.message());
            exit(EINVAL);
        }
        // CA certificate
        if (!config.ca_cert.empty()) {
            // Verify that the peer certificate is signed by a trusted CA
            ctx->set_verify_mode(boost::asio::ssl::verify_peer |
                                 boost::asio::ssl::verify_fail_if_no_peer_cert,
                                 ec);
            if (ec.value() != 0) {
                SANDESH_LOG(ERROR, "Error setting verification mode: " <<
                            ec.message());
                exit(EINVAL);
            }
            ctx->load_verify_file(config.ca_cert, ec);
            if (ec.value() != 0) {
                SANDESH_LOG(ERROR, "Error loading CA certificate: " <<
                            ec.message());
                exit(EINVAL);
            }
        }
        // Server certificate
        ctx->use_certificate_chain_file(config.certfile, ec);
        if (ec.value() != 0) {
            SANDESH_LOG(ERROR, "Error using server certificate: " <<
                        ec.message());
            exit(EINVAL);
        }
        // Server private key
        ctx->use_private_key_file(config.keyfile,
                                  boost::asio::ssl::context::pem, ec);
        if (ec.value() != 0) {
            SANDESH_LOG(ERROR, "Error using server private key file: " <<
                        ec.message());
            exit(EINVAL);
        }
    }
}

SandeshServer::~SandeshServer() {
    TcpServer::ClearSessions();
}

int SandeshServer::lifetime_mgr_task_id() {
    return lifetime_mgr_task_id_;
}

void SandeshServer::SessionShutdown() {
    TcpServer::Shutdown();
}

bool SandeshServer::Initialize(short port) {
    int count = 0;

    while (count++ < kMaxInitRetries) {
        if (TcpServer::Initialize(port))
            break;
        sleep(1);
    }
    if (!(count < kMaxInitRetries)) {
        SANDESH_LOG(ERROR, "Process EXITING: TCP Server initialization failed for port " << port);
        exit(1);
    }
}

int SandeshServer::AllocConnectionIndex() {
    tbb::mutex::scoped_lock lock(mutex_);
    size_t bit = conn_bmap_.find_first();
    if (bit == conn_bmap_.npos) {
        bit = conn_bmap_.size();
        conn_bmap_.resize(bit + 1, true);
    }
    conn_bmap_.reset(bit);
    return bit;
}

void SandeshServer::FreeConnectionIndex(int id) {
    tbb::mutex::scoped_lock lock(mutex_);
    conn_bmap_.set(id);

    for (size_t i = conn_bmap_.size(); i != 0; i--) {
        if (conn_bmap_[i-1] != true) {
            if (i != conn_bmap_.size()) {
                conn_bmap_.resize(i);
            }
            return;
        }
    }
    conn_bmap_.clear();
}

TcpSession *SandeshServer::CreateSession() {
    typedef boost::asio::detail::socket_option::boolean<
#ifdef __APPLE__
        SOL_SOCKET, SO_REUSEPORT> reuse_port_t;
#else
        SOL_SOCKET, SO_REUSEADDR> reuse_addr_t;
#endif
    TcpSession *session = SslServer::CreateSession();
    Socket *socket = session->socket();

    boost::system::error_code err;
    socket->open(ip::tcp::v4(), err);
    if (err) {
        SANDESH_LOG(ERROR, __func__ << " Server Open Fail " << err.message());
    }

#ifdef __APPLE__
    socket->set_option(reuse_port_t(true), err);
#else
    socket->set_option(reuse_addr_t(true), err);
#endif
    if (err) {
        SANDESH_LOG(ERROR, __func__ << " SetSockOpt Fail " << err.message());
        return session;
    }

    socket->bind(LocalEndpoint(), err);
    if (err) {
        SANDESH_LOG(ERROR, __func__ << " Server Bind Failure " <<  err.message());
    }

    return session;
}

void SandeshServer::Shutdown() {
    assert(deleter_.get());
    deleter_->Delete();
}

bool SandeshServer::Compare(const Endpoint &peer_addr,
                            const SandeshConnectionPair &p) const {
    return (peer_addr == p.second->endpoint() ?  false : true);
}

SandeshConnection *SandeshServer::FindConnection(const Endpoint &peer_addr) {
    tbb::mutex::scoped_lock lock(mutex_);
    SandeshConnectionMap::iterator loc = find_if(connection_.begin(),
            connection_.end(), boost::bind(&SandeshServer::Compare, this,
                                           boost::ref(peer_addr), _1));
    if (loc != connection_.end()) {
        return loc->second;
    }
    return NULL;
}

SslSession *SandeshServer::AllocSession(SslSocket *socket) {
    // Use the state machine task to run the session send queue since
    // they need to be exclusive as session delete happens from state
    // machine
    SslSession *session = new SandeshSession(this, socket,
            AllocConnectionIndex(), session_writer_task_id(),
            session_reader_task_id());
    return session;
}

void SandeshServer::RemoveConnection(SandeshConnection *connection) {
    tbb::mutex::scoped_lock lock(mutex_);
    boost::asio::ip::tcp::endpoint endpoint = connection->endpoint();
    connection_.erase(endpoint);
}

bool SandeshServer::AcceptSession(TcpSession *session) {
    tbb::mutex::scoped_lock lock(mutex_);
    SandeshConnection *connection;
    SandeshSession *ssession = dynamic_cast<SandeshSession *>(session);
    assert(ssession);
    ip::tcp::endpoint remote = session->remote_endpoint();
    SandeshConnectionMap::iterator loc = connection_.find(remote);

    if (loc == connection_.end()) {
        SANDESH_LOG(INFO, "Server: " << __func__ << " " << "Create Connection");
        //create a connection_
        connection = new SandeshServerConnection(this, remote,
                         ssession->GetSessionInstance(), 
                         sm_task_id_);
        connection->Initialize();
        connection_.insert(remote, connection);
    } else {
        connection = loc->second;
        if (connection->session() != NULL) {
            return false;
        }
    }
    connection->AcceptSession(ssession);
    return true;
}

bool SandeshServer::ReceiveSandeshCtrlMsg(SandeshStateMachine *sm,
        SandeshSession *session, const Sandesh *sandesh) {
    const SandeshCtrlClientToServer *snh =
            dynamic_cast<const SandeshCtrlClientToServer *>(sandesh);
    if (!snh) {
        SANDESH_LOG(DEBUG, "Received Ctrl Message with wrong type " << sandesh->Name());
        return false;
    }
    SANDESH_LOG(DEBUG, "Received Ctrl Message from " << snh->get_module_name());
    std::vector<UVETypeInfo> vu;
    SandeshCtrlServerToClient::Request(vu, true, "ctrl", session->connection());
    return true;
}

LifetimeActor *SandeshServer::deleter() {
    return deleter_.get();
}

LifetimeManager *SandeshServer::lifetime_manager() {
    return lifetime_manager_.get();
}
