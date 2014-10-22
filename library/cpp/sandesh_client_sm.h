/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_client_sm.h
//
// Sandesh Client State Machine
//

#ifndef __SANDESH_CLIENT_SM_H__
#define __SANDESH_CLIENT_SM_H__

#include <string>
#include <boost/function.hpp>
#include <tbb/mutex.h>
#include <io/tcp_server.h>
#include <sandesh/sandesh_session.h>

class SandeshHeader;
class SandeshSession;


// This is the interface for the Sandesh Client State Machine.
//
// The user of the state machine instantiates it using
// the static function "CreateClientSM"
//
// The user must provide callbacks by implementing the 
// SandeshClientSM::Mgr class, which has to be passed in
// at creation time.

class SandeshClientSM {
public:    
    class Mgr {
        public:
            virtual bool ReceiveMsg(const std::string& msg, 
                        const SandeshHeader &header, const std::string &sandesh_name,
                        const uint32_t header_offset) = 0;
            virtual void SendUVE(int count,
                        const std::string & stateName, const std::string & server,
                        TcpServer::Endpoint pri = TcpServer::Endpoint(), 
                        TcpServer::Endpoint sec = TcpServer::Endpoint()) = 0;
            virtual SandeshSession *CreateSMSession(
                        TcpSession::EventObserver eocb,
                        SandeshReceiveMsgCb rmcb,
                        TcpServer::Endpoint ep) = 0;
            virtual void InitializeSMSession(int connects) = 0;
            virtual void DeleteSMSession(SandeshSession * session) = 0;
        protected:
            Mgr() {}
            virtual ~Mgr() {}
    };

    typedef enum {
        IDLE = 0,
        DISCONNECT = 1,
        CONNECT = 2,
        CLIENT_INIT = 3,
        ESTABLISHED = 4
    } State;

    static SandeshClientSM * CreateClientSM(EventManager *evm, Mgr *mgr,
            int sm_task_instance, int sm_task_id);
    State state() const { return state_; }
    virtual const std::string &StateName() const = 0;
    SandeshSession *session() {
        return session_;
    } 
    TcpServer::Endpoint server() {
    	tbb::mutex::scoped_lock l(mtex_); return server_;
    }

    // This function is used to start and stop the state machine
    virtual void SetAdminState(bool down) = 0;

    // This function should be called when there is a discovery service update
    virtual void SetCandidates(TcpServer::Endpoint active, TcpServer::Endpoint backup) = 0;

    // This function is used to send UVE sandesh's to the server
    virtual bool SendSandeshUVE(Sandesh* snh) = 0;

    virtual ~SandeshClientSM() {}

protected:
    SandeshClientSM(Mgr *mgr):  mgr_(mgr), session_(), server_() { state_ = IDLE; }

    virtual void EnqueDelSession(SandeshSession * session) = 0;

    void set_session(SandeshSession * session, bool enq) {
        if (session_ != NULL) {
            session_->set_observer(NULL);
            session_->SetReceiveMsgCb(NULL);
            session_->Close();
            session_->Shutdown();
            if (enq) EnqueDelSession(session_);	
        }
        session_ = session;
    }

    bool send_session(Sandesh *snh) {
        return snh->Enqueue(session_->send_queue());
    }
   
    void set_server(TcpServer::Endpoint e) {
        tbb::mutex::scoped_lock l(mtex_); server_ = e;
    }

    Mgr * const mgr_;
    tbb::atomic<State> state_;

private:
    tbb::mutex mtex_;
    tbb::atomic<SandeshSession *> session_;
    TcpServer::Endpoint server_;

    friend class SandeshClientStateMachineTest;
};

#endif

