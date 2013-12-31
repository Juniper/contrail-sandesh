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

#include <base/logging.h>
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

SandeshServer::SandeshServer(EventManager *evm)
    : TcpServer(evm),
      sm_task_id_(TaskScheduler::GetInstance()->GetTaskId(kStateMachineTask)),
      session_reader_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSessionReaderTask)),
      lifetime_mgr_task_id_(TaskScheduler::GetInstance()->GetTaskId(kLifetimeMgrTask)),
      lifetime_manager_(new LifetimeManager(lifetime_mgr_task_id_)),
      deleter_(new DeleteActor(this)) {
    // Set task policy for exclusion between :
    // 1. State machine and lifetime mgr since state machine delete happens
    //    in lifetime mgr task
    if (!task_policy_set_) {
        TaskPolicy sm_task_policy = boost::assign::list_of
                (TaskExclusion(lifetime_mgr_task_id_));
        TaskScheduler::GetInstance()->SetPolicy(sm_task_id_, sm_task_policy);
        task_policy_set_ = true;
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

void SandeshServer::Initialize(short port) {
    TcpServer::Initialize(port);
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
    TcpSession *session = TcpServer::CreateSession();
    Socket *socket = session->socket();

    boost::system::error_code err;
    socket->open(ip::tcp::v4(), err);
    if (err) {
        LOG(ERROR, __func__ << " Server Open Fail " << err.message());
    }

#ifdef __APPLE__
    socket->set_option(reuse_port_t(true), err);
#else
    socket->set_option(reuse_addr_t(true), err);
#endif
    if (err) {
        LOG(ERROR, __func__ << " SetSockOpt Fail " << err.message());
        return session;
    }

    socket->bind(LocalEndpoint(), err);
    if (err) {
        LOG(ERROR, __func__ << " Server Bind Failure " <<  err.message());
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

TcpSession *SandeshServer::AllocSession(Socket *socket) {
    // Use the state machine task to run the session send queue since
    // they need to be exclusive as session delete happens from state
    // machine
    TcpSession *session = new SandeshSession(this, socket,
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
        LOG(INFO, "Server: " << __func__ << " " << "Create Connection");
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
        LOG(DEBUG, "Received Ctrl Message with wrong type " << sandesh->Name());
        return false;
    }
    LOG(DEBUG, "Received Ctrl Message from " << snh->get_module_name());
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
