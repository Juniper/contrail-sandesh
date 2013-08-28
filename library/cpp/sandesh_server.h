/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_server.h
//
// Sandesh Analytics Database Server
//

#ifndef __SANDESH_SERVER_H__
#define __SANDESH_SERVER_H__

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <sandesh/sandesh.h>
#include <io/tcp_server.h>
#include <io/tcp_session.h>

class SandeshConnection;
class SandeshSession;
class SandeshStateMachine;

namespace ssm {
    struct Message;
}

class SandeshServer : public TcpServer {
public:
    explicit SandeshServer(EventManager *evm);
    virtual ~SandeshServer();

    virtual void Initialize(short port);
    virtual TcpSession *CreateSession();
    void Initiate();
    void Shutdown();

    SandeshConnection *FindConnection(const Endpoint &peer_addr);
    void RemoveConnection(SandeshConnection *connection);
    virtual bool DisableSandeshLogMessages() { return true; }
    
    virtual bool ReceiveSandeshMsg(SandeshSession *session,
        const std::string& cmsg, const std::string& message_type,
        const SandeshHeader& header, uint32_t xml_offset) = 0;
    virtual bool ReceiveSandeshCtrlMsg(SandeshStateMachine *state_machine,
            SandeshSession *session, const Sandesh *sandesh);
    virtual bool ReceiveMsg(SandeshSession *session, ssm::Message *msg) = 0;
    virtual void DisconnectSession(SandeshSession *session) {}

protected:
    virtual TcpSession *AllocSession(Socket *socket);
    virtual bool AcceptSession(TcpSession *session);
    virtual SandeshConnection *CreateConnection(SandeshServer *server,
            boost::asio::ip::tcp::endpoint remote);

private:
    static const std::string kSessionTask;

    typedef boost::ptr_map<boost::asio::ip::tcp::endpoint,
                           SandeshConnection> SandeshConnectionMap;
    typedef boost::ptr_container_detail::ref_pair<
                   boost::asio::ip::basic_endpoint<boost::asio::ip::tcp>,
                   SandeshConnection *const> SandeshConnectionPair;
    bool Compare(const Endpoint &peer_addr, const SandeshConnectionPair &) const;

    SandeshConnectionMap connection_;
    int session_task_id_;

    DISALLOW_COPY_AND_ASSIGN(SandeshServer);
};

#endif // __SANDESH_SERVER_H__
