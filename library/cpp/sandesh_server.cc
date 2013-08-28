/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_server.cc
//
// Sandesh server implementation
//

#include <boost/bind.hpp>

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

const std::string SandeshServer::kSessionTask = "sandesh::SandeshSession";

SandeshServer::SandeshServer(EventManager *evm)
    : TcpServer(evm),
      session_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSessionTask)) {
}

SandeshServer::~SandeshServer() {
}

void SandeshServer::Initialize(short port) {
    TcpServer::Initialize(port);
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
    TcpServer::Shutdown();
}

bool SandeshServer::Compare(const Endpoint &peer_addr,
                            const SandeshConnectionPair &p) const {
    return (peer_addr == p.second->endpoint() ?  false : true);
}

SandeshConnection *SandeshServer::FindConnection(const Endpoint &peer_addr) {
    SandeshConnectionMap::iterator loc = find_if(connection_.begin(),
            connection_.end(), boost::bind(&SandeshServer::Compare, this,
                                           boost::ref(peer_addr), _1));
    if (loc != connection_.end()) {
        return loc->second;
    }
    return NULL;
}

TcpSession *SandeshServer::AllocSession(Socket *socket) {
    TcpSession *session = new SandeshSession(this, socket,
            Task::kTaskInstanceAny, session_task_id_);
    return session;
}

void SandeshServer::RemoveConnection(SandeshConnection *connection) {
    boost::asio::ip::tcp::endpoint endpoint = connection->endpoint();
    connection_.erase(endpoint);
}

SandeshConnection *SandeshServer::CreateConnection(SandeshServer *server,
        ip::tcp::endpoint remote) {
    SandeshConnection *connection = new SandeshServerConnection(this, remote);
    return connection;
}

bool SandeshServer::AcceptSession(TcpSession *session) {
    SandeshConnection *connection;
    ip::tcp::endpoint remote = session->remote_endpoint();
    SandeshConnectionMap::iterator loc = connection_.find(remote);

    if (loc == connection_.end()) {
        LOG(INFO, "Server: " << __func__ << " " << "Create Connection");
        //create a connection_
        connection = CreateConnection(this, remote);
        connection->Initialize();
        connection_.insert(remote, connection);
    } else {
        connection = loc->second;
        if (connection->session() != NULL) {
            return false;
        }
    }
    connection->AcceptSession(static_cast<SandeshSession *>(session));
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
