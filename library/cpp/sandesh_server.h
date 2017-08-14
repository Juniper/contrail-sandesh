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
#include <boost/scoped_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
#include <base/lifetime.h>
#include <sandesh/sandesh.h>
#include <io/ssl_server.h>
#include <io/tcp_session.h>

class SandeshConnection;
class SandeshSession;
class SandeshStateMachine;
class LifetimeActor;
class LifetimeManager;
class SandeshMessage;

class SandeshServer : public SslServer {
public:
    explicit SandeshServer(EventManager *evm, const SandeshConfig &config);
    virtual ~SandeshServer();

    virtual bool Initialize(short port);
    virtual TcpSession *CreateSession();
    void Initiate();
    void Shutdown();
    virtual void SessionShutdown();

    LifetimeManager *lifetime_manager();
    LifetimeActor *deleter();
    int lifetime_mgr_task_id();

    SandeshConnection *FindConnection(const Endpoint &peer_addr);
    void RemoveConnection(SandeshConnection *connection);
    virtual bool DisableSandeshLogMessages() const { return true; }
   
    virtual bool ReceiveResourceUpdate(SandeshSession *session,
            bool rsc) { return true; } 
    virtual bool ReceiveSandeshMsg(SandeshSession *session,
        const SandeshMessage *msg, bool resource) = 0;
    virtual bool ReceiveSandeshCtrlMsg(SandeshStateMachine *state_machine,
            SandeshSession *session, const Sandesh *sandesh);
    virtual void DisconnectSession(SandeshSession *session) {}
    size_t ConnectionsCount() { return connection_.size(); }
    int AllocConnectionIndex();
    void FreeConnectionIndex(int);

protected:
    virtual SslSession *AllocSession(SslSocket *socket);
    virtual bool AcceptSession(TcpSession *session);
    // Session read, write, and state machine tasks run exclusively
    int session_writer_task_id() const { return sm_task_id_; }
    int session_reader_task_id() const { return session_reader_task_id_; }

private:
    static const int kMaxInitRetries = 5;
    static const std::string kSessionReaderTask;
    static const std::string kStateMachineTask;
    static const std::string kLifetimeMgrTask;
    static bool task_policy_set_;
    
    class DeleteActor;
    friend class DeleteActor;

    typedef boost::ptr_map<boost::asio::ip::tcp::endpoint,
                           SandeshConnection> SandeshConnectionMap;
    typedef boost::ptr_container_detail::ref_pair<
                   boost::asio::ip::basic_endpoint<boost::asio::ip::tcp>,
                   SandeshConnection *const> SandeshConnectionPair;
    bool Compare(const Endpoint &peer_addr, const SandeshConnectionPair &) const;

    SandeshConnectionMap connection_;
    boost::dynamic_bitset<> conn_bmap_;
    int sm_task_id_;
    int session_reader_task_id_;
    int lifetime_mgr_task_id_;
    boost::scoped_ptr<LifetimeManager> lifetime_manager_;
    boost::scoped_ptr<DeleteActor> deleter_;
    // Protect connection map and bmap
    tbb::mutex mutex_;

    DISALLOW_COPY_AND_ASSIGN(SandeshServer);
};

#endif // __SANDESH_SERVER_H__
