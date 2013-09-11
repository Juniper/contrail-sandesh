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
#include <base/lifetime.h>
#include "sandesh_state_machine.h"

class SandeshSession;
class TcpSession;
class TcpServer;
class SandeshUVE;
class SandeshHeader;

class SandeshConnection {
public:
    typedef boost::asio::ip::tcp::endpoint Endpoint;
    SandeshConnection(const char *prefix, TcpServer *server, Endpoint endpoint,
                      int task_instance, int task_id);
    virtual ~SandeshConnection();

    // Invoked from server when a session is accepted.
    void AcceptSession(SandeshSession *session);
    virtual void ReceiveMsg(const std::string &msg, SandeshSession *session);

    TcpServer *server() { return server_; }
    SandeshSession *CreateSession();

    Endpoint endpoint() { return endpoint_; }
    bool SendSandesh(Sandesh *snh);

    void set_session(SandeshSession *session);

    SandeshSession *session() const;
    SandeshStateMachine *state_machine() const;

    std::string StateName() const { return state_machine_->StateName(); }

    int GetTaskInstance() const {
        // Parallelism between connections
        return task_instance_;
    }

    int GetTaskId() const {
        return task_id_;
    }

    void Initialize() {
        state_machine_->Initialize();
    }

    void Shutdown();
    bool MayDelete() const;

    void SetAdminState(bool down);

    virtual bool ProcessSandeshMessage(const std::string &msg,
            const SandeshHeader &header, const std::string sandesh_name,
            const uint32_t header_offset) = 0;
    virtual bool ProcessSandeshCtrlMessage(const std::string &msg,
            const SandeshHeader &header, const std::string sandesh_name,
            const uint32_t header_offset) = 0;
    virtual bool ProcessMessage(ssm::Message *msg) = 0;
    virtual void ProcessDisconnect(SandeshSession * sess) = 0;

    virtual void ManagedDelete() = 0;
    virtual LifetimeActor *deleter() = 0;
    virtual LifetimeManager *lifetime_manager() = 0;
    virtual void Destroy() = 0;

protected:
    bool is_deleted_;

private:
    friend class SandeshServerStateMachineTest;

    TcpServer *server_;
    Endpoint endpoint_;
    bool admin_down_;
    SandeshSession *session_;
    int task_instance_;
    int task_id_;
    boost::scoped_ptr<SandeshStateMachine> state_machine_;
    
    DISALLOW_COPY_AND_ASSIGN(SandeshConnection);
};

class SandeshServerConnection : public SandeshConnection {
public:
    SandeshServerConnection(TcpServer *server, Endpoint endpoint,
        int task_instance, int task_id);
    virtual ~SandeshServerConnection();

    virtual bool ProcessSandeshMessage(const std::string &msg,
            const SandeshHeader &header, const std::string sandesh_name,
            const uint32_t header_offset);
    virtual bool ProcessSandeshCtrlMessage(const std::string &msg,
            const SandeshHeader &header, const std::string sandesh_name,
            const uint32_t header_offset);
    virtual bool ProcessMessage(ssm::Message *msg);
    virtual void ProcessDisconnect(SandeshSession *session);

    virtual void ManagedDelete();
    virtual LifetimeActor *deleter();
    virtual LifetimeManager *lifetime_manager();
    virtual void Destroy();

private:
    class DeleteActor;
    boost::scoped_ptr<DeleteActor> deleter_;
    LifetimeRef<SandeshServerConnection> server_delete_ref_;

    DISALLOW_COPY_AND_ASSIGN(SandeshServerConnection);
};

#endif // __SANDESH_CONNECTION_H__
