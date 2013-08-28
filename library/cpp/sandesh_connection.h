/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_connection.h
//
// Sandesh connection class
//

#ifndef __SANDESH_CONNECTION_H__
#define __SANDESH_CONNECTION_H__

#include <boost/asio/ip/tcp.hpp>

#include <base/util.h>
#include "sandesh_state_machine.h"

class SandeshSession;
class TcpSession;
class TcpServer;
class SandeshUVE;
class SandeshHeader;

class SandeshConnection {
public:
    typedef boost::asio::ip::tcp::endpoint Endpoint;
    SandeshConnection(const char *prefix, TcpServer *server, Endpoint endpoint);
    virtual ~SandeshConnection();

    // Invoked from server when a session is accepted.
    void AcceptSession(SandeshSession *session);
    virtual void ReceiveMsg(const std::string &msg, SandeshSession *session);

    TcpServer *server() { return server_; }
    SandeshSession *CreateSession();

    Endpoint endpoint() { return endpoint_; }
    bool Send(const uint8_t *data, size_t size);

    void set_session(SandeshSession *session);

    SandeshSession *session() const;
    ssm::SsmState GetStateMachineState() const;

    std::string StateName() const { return state_machine_->StateName(); }
    TcpSession *GetTcpSession() const { return state_machine_->session(); }

    int GetIndex() const {
        // Parallelism between connections
        return -1;
    }

    void Initialize() {
        state_machine_->Initialize();
    }

    void SetAdminState(bool down);

    virtual bool HandleSandeshMessage(const std::string &msg,
            const SandeshHeader &header, const std::string sandesh_name,
            const uint32_t header_offset) = 0;
    virtual bool HandleSandeshCtrlMessage(const std::string &msg,
            const SandeshHeader &header, const std::string sandesh_name,
            const uint32_t header_offset) = 0;
    virtual bool HandleMessage(ssm::Message *msg) { return true; }
    virtual void HandleDisconnect(SandeshSession * sess) {}
    SandeshStateMachine *state_machine() const;

protected:

private:
    friend class SandeshServerStateMachineTest;
    friend class SandeshClientStateMachineTest;
    TcpServer *server_;
    Endpoint endpoint_;
    bool admin_down_;
    SandeshSession *session_;
    boost::scoped_ptr<SandeshStateMachine> state_machine_;
    
    DISALLOW_COPY_AND_ASSIGN(SandeshConnection);
};

class SandeshServerConnection : public SandeshConnection {
public:
    SandeshServerConnection(TcpServer *server, Endpoint endpoint);
    virtual ~SandeshServerConnection();
    virtual bool IsClient() const;
    virtual bool HandleSandeshMessage(const std::string &msg,
            const SandeshHeader &header, const std::string sandesh_name,
            const uint32_t header_offset);
    virtual bool HandleSandeshCtrlMessage(const std::string &msg,
            const SandeshHeader &header, const std::string sandesh_name,
            const uint32_t header_offset);
    virtual bool HandleMessage(ssm::Message *msg);
    virtual void HandleDisconnect(SandeshSession * sess);

private:
    DISALLOW_COPY_AND_ASSIGN(SandeshServerConnection);
};

#endif // __SANDESH_CONNECTION_H__
