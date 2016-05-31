/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_client_sm.cc
//
// Sandesh Client State Machine
//

#include <typeinfo>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/statechart/custom_reaction.hpp>
#include <boost/statechart/event.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/transition.hpp>
#include <boost/statechart/in_state_reaction.hpp>

#include <base/logging.h>
#include <base/timer.h>
#include <base/task_annotations.h>
#include <base/connection_info.h>
#include <io/event_manager.h>

#include <sandesh/sandesh_constants.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_uve_types.h>
#include <sandesh/sandesh_statistics.h>
#include "sandesh_client_sm_priv.h"

using boost::system::error_code;
using std::string;
using std::map;
using std::vector;
using process::ConnectionState;
using process::ConnectionType;
using process::ConnectionStatus;

namespace mpl = boost::mpl;
namespace sc = boost::statechart;

#define SM_LOG(_Level, _Msg) \
    do { \
        if (LoggingDisabled()) break;                                          \
        log4cplus::Logger _Xlogger = Sandesh::logger();                        \
        if (_Xlogger.isEnabledFor(log4cplus::_Level##_LOG_LEVEL)) {            \
            log4cplus::tostringstream _Xbuf;                                   \
            _Xbuf << _Msg;                                                     \
            _Xlogger.forcedLog(log4cplus::_Level##_LOG_LEVEL,                  \
                               _Xbuf.str());                                   \
        }                                                                      \
    } while (false)

#define SESSION_LOG(session) \
    SANDESH_LOG(DEBUG, ((session) ? (session)->ToString() : "*") << ":" << Name())


namespace scm {

// events
struct EvStart : sc::event<EvStart> {
    static const char * Name() {
        return "EvStart";
    }
};

struct EvStop : sc::event<EvStop> {
    EvStop() : enq_(true) {}
    EvStop(bool enq) : enq_(enq) {}
    static const char * Name() {
        return "EvStop";
    }
    bool enq_;
};

struct EvDiscUpdate : sc::event<EvDiscUpdate> {
    EvDiscUpdate(TcpServer::Endpoint active, TcpServer::Endpoint backup) :
            active_(active), backup_(backup) {
    }
    static const char * Name() {
        return "EvDiscUpdate";
    }
    TcpServer::Endpoint active_,backup_;
};

struct EvIdleHoldTimerExpired : sc::event<EvIdleHoldTimerExpired> {
    EvIdleHoldTimerExpired(Timer *timer)  : timer_(timer){
    }
    static const char * Name() {
        return "EvIdleHoldTimerExpired";
    }
    bool validate() const {
        return !timer_->cancelled();
    }
    Timer *timer_;
};

struct EvConnectTimerExpired : sc::event<EvConnectTimerExpired> {
    EvConnectTimerExpired(Timer *timer) : timer_(timer) {
    }
    static const char * Name() {
        return "EvConnectTimerExpired";
    }
    bool validate(SandeshClientSMImpl *state_machine) const {
        if (timer_->cancelled()) {
            return false;
        }
        return true;
    }
    Timer *timer_;
};

struct EvTcpConnected : sc::event<EvTcpConnected> {
    EvTcpConnected(SandeshSession *session) : session(session) {
        SESSION_LOG(session);
    }
    static const char * Name() {
        return "EvTcpConnected";
    }

    SandeshSession *session;
};

struct EvTcpConnectFail : sc::event<EvTcpConnectFail> {
    EvTcpConnectFail(SandeshSession *session) : session(session) {
        SESSION_LOG(session);
    }
    static const char * Name() {
        return "EvTcpConnectFail";
    }

    SandeshSession *session;
};


struct EvTcpClose : sc::event<EvTcpClose> {
    EvTcpClose(SandeshSession *session) : session(session) {
        SESSION_LOG(session);
    };
    static const char * Name() {
        return "EvTcpClose";
    }

    SandeshSession *session;
};

// Used to defer the session delete after all events currently on the queue.
struct EvTcpDeleteSession : sc::event<EvTcpDeleteSession> {
    EvTcpDeleteSession(SandeshSession *session) : session(session) {
    }
    static const char *Name() {
        return "EvTcpDeleteSession";
    }
    SandeshSession *session;
};

struct EvSandeshSend : sc::event<EvSandeshSend> {
    EvSandeshSend(Sandesh* snh) :
        snh(snh) {
    }
    static const char * Name() {
        return "EvSandeshSend";
    }    
    Sandesh* snh;
};

struct EvSandeshMessageRecv : sc::event<EvSandeshMessageRecv> {
    EvSandeshMessageRecv(const std::string &msg, const SandeshHeader& header,
            const std::string &msg_type, const uint32_t &header_offset) :
        msg(msg), header(header), msg_type(msg_type), header_offset(header_offset) {
    };
    static const char * Name() {
        return "EvSandeshMessageRecv";
    }
    const std::string msg;
    const SandeshHeader header;
    const std::string msg_type;
    const uint32_t header_offset;
};


// states
struct Idle;
struct Connect;
struct Disconnect;
struct ClientInit;
struct Established;

template <class Ev>
struct TransitToIdle {
    typedef sc::transition<Ev, Idle, SandeshClientSMImpl,
            &SandeshClientSMImpl::OnIdle<Ev> > reaction;
};

template <class Ev>
struct ReleaseSandesh {
    typedef sc::in_state_reaction<Ev, SandeshClientSMImpl,
            &SandeshClientSMImpl::ReleaseSandesh<Ev> > reaction;
};

template <class Ev>
struct DeleteTcpSession {
    typedef sc::in_state_reaction<Ev, SandeshClientSMImpl,
            &SandeshClientSMImpl::DeleteTcpSession<Ev> > reaction;
};


struct Idle : public sc::state<Idle, SandeshClientSMImpl> {
    typedef mpl::list<
            sc::custom_reaction<EvStart>,
            sc::custom_reaction<EvStop>,
            sc::custom_reaction<EvIdleHoldTimerExpired>,
            sc::custom_reaction<EvDiscUpdate>,
            ReleaseSandesh<EvSandeshSend>::reaction,
            DeleteTcpSession<EvTcpDeleteSession>::reaction
        > reactions;

    Idle(my_context ctx) : my_base(ctx) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->set_state(SandeshClientSM::IDLE);
        SM_LOG(DEBUG, state_machine->StateName());
        state_machine->SendUVE();
    }

    ~Idle() {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->CancelIdleHoldTimer();
    }

    sc::result react(const EvStart &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        if (state_machine->idle_hold_time()) {
            // Update connection info
            ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
                std::string(), ConnectionStatus::INIT,
                state_machine->server(),
                state_machine->StateName() + " : " + event.Name());
            state_machine->StartIdleHoldTimer();
        } else {
            // Update connection info
            ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
                std::string(), ConnectionStatus::DOWN,
                state_machine->server(),
                state_machine->StateName() + " : " + event.Name() + 
                    " -> Disconnect");
            return transit<Disconnect>();
        }
        return discard_event();
    }

    sc::result react(const EvStop &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->CancelIdleHoldTimer();
        return discard_event();
    }

    sc::result react(const EvDiscUpdate &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->DiscUpdate(
                SandeshClientSM::IDLE, true,
                event.active_, event.backup_);
        return discard_event();
    }
    sc::result react(const EvIdleHoldTimerExpired &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        if (state_machine->DiscUpdate(
                SandeshClientSM::IDLE, false,
                TcpServer::Endpoint(), TcpServer::Endpoint())) {
            // Update connection info
            ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
                std::string(), ConnectionStatus::INIT,
                state_machine->server(),
                state_machine->StateName() + " : " + event.Name() + 
                    " -> Connect");
            return transit<Connect>();
        } 
        // Update connection info
        ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
            std::string(), ConnectionStatus::DOWN,
            state_machine->server(),
            state_machine->StateName() + " : " + event.Name() + 
                " -> Disconnect");
        return transit<Disconnect>();
    }
};

struct Disconnect : public sc::state<Disconnect, SandeshClientSMImpl> {
    typedef mpl::list<
            TransitToIdle<EvStop>::reaction,
            sc::custom_reaction<EvDiscUpdate>,
            ReleaseSandesh<EvSandeshSend>::reaction,
            DeleteTcpSession<EvTcpDeleteSession>::reaction
        > reactions;

    Disconnect(my_context ctx) : my_base(ctx) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->set_state(SandeshClientSM::DISCONNECT);
        SM_LOG(DEBUG, state_machine->StateName());
        state_machine->SendUVE();
    }

    sc::result react(const EvDiscUpdate &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        if (state_machine->DiscUpdate(
                SandeshClientSM::DISCONNECT, true,
                event.active_, event.backup_)) {
            // Update connection info
            ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
                std::string(), ConnectionStatus::INIT,
                state_machine->server(),
                state_machine->StateName() + " : " + event.Name() + 
                    " -> Connect");
            return transit<Connect>();
        } 
        return discard_event();
    }
};

struct Connect : public sc::state<Connect, SandeshClientSMImpl> {
    typedef mpl::list<
            TransitToIdle<EvStop>::reaction,
            sc::custom_reaction<EvConnectTimerExpired>,
            sc::custom_reaction<EvTcpConnected>,
            sc::custom_reaction<EvTcpConnectFail>,
            sc::custom_reaction<EvTcpClose>,
            sc::custom_reaction<EvDiscUpdate>,
            ReleaseSandesh<EvSandeshSend>::reaction,
            DeleteTcpSession<EvTcpDeleteSession>::reaction
        > reactions;

    static const int kConnectTimeout = 60;  // seconds

    Connect(my_context ctx) : my_base(ctx) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->set_state(SandeshClientSM::CONNECT);
        StartSession(state_machine);
        state_machine->connect_attempts_inc();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << "Start Connect timer " <<
            state_machine->server());
        state_machine->StartConnectTimer(state_machine->GetConnectTime());
        state_machine->SendUVE();
    }

    ~Connect() {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->CancelConnectTimer();
    }

    sc::result react(const EvTcpConnectFail &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        state_machine->DiscUpdate(
                SandeshClientSM::CONNECT, false,
                TcpServer::Endpoint(), TcpServer::Endpoint());
        return ToIdle(state_machine, event.Name());
    }


    sc::result react(const EvConnectTimerExpired &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        // Flip the active and backup collector
        state_machine->DiscUpdate(
                SandeshClientSM::CONNECT, false,
                TcpServer::Endpoint(), TcpServer::Endpoint());
        return ToIdle(state_machine, event.Name());
    }

    sc::result react(const EvTcpConnected &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " <<
                event.Name() << " : " << "Cancelling Connect timer");
        state_machine->CancelConnectTimer();
        SandeshSession *session = event.session;
        // Update connection info
        ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
            std::string(), ConnectionStatus::INIT, session->remote_endpoint(),
            state_machine->StateName() + " : " + event.Name());       
        // Start the send queue runner XXX move this to Established or later
        session->send_queue()->MayBeStartRunner();
        return transit<ClientInit>();
    }

    sc::result react(const EvTcpClose &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        return ToIdle(state_machine, event.Name());
    }

    sc::result react(const EvDiscUpdate &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        if (state_machine->DiscUpdate(
                SandeshClientSM::CONNECT, true,
                event.active_, event.backup_)) {
            return ToIdle(state_machine, event.Name());
        }  
        return discard_event();
    }

     // Create an active connection request.
    void StartSession(SandeshClientSMImpl *state_machine) {
        SandeshSession *session = static_cast<SandeshSession *>(
                state_machine->GetMgr()->CreateSMSession(
                        boost::bind(&SandeshClientSMImpl::OnSessionEvent,
                                          state_machine, _1, _2),
                        boost::bind(&SandeshClientSMImpl::OnMessage,
                                          state_machine, _2, _1),
                        state_machine->server()));
        state_machine->set_session(session);
    }

    sc::result ToIdle(SandeshClientSMImpl *state_machine,
        const char *event_name) {
        // Update connection info
        ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
            std::string(), ConnectionStatus::DOWN,
            state_machine->session()->remote_endpoint(),
            state_machine->StateName() + " : " + event_name);       
        state_machine->set_idle_hold_time(state_machine->GetConnectTime() * 1000);
        state_machine->OnIdle<EvStop>(EvStop());
        state_machine->StartIdleHoldTimer();
        return transit<Idle>();
    }
};

struct ClientInit : public sc::state<ClientInit, SandeshClientSMImpl> {
    typedef mpl::list<
        TransitToIdle<EvStop>::reaction,
        sc::custom_reaction<EvConnectTimerExpired>,
        sc::custom_reaction<EvTcpClose>,
        sc::custom_reaction<EvSandeshMessageRecv>,
        sc::custom_reaction<EvSandeshSend>,
        sc::custom_reaction<EvDiscUpdate>,
        DeleteTcpSession<EvTcpDeleteSession>::reaction
    > reactions;

    ClientInit(my_context ctx) : my_base(ctx) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->set_state(SandeshClientSM::CLIENT_INIT);
        SM_LOG(DEBUG, state_machine->StateName());
        state_machine->CancelConnectTimer();
        state_machine->StartConnectTimer(state_machine->GetConnectTime());
        state_machine->GetMgr()->InitializeSMSession(state_machine->connects_inc());
        state_machine->SendUVE();
    }

    ~ClientInit() {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->CancelConnectTimer();
    }

    sc::result react(const EvTcpClose &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        state_machine->DiscUpdate(
                SandeshClientSM::CLIENT_INIT, false,
                TcpServer::Endpoint(), TcpServer::Endpoint());
        return ToIdle(state_machine, event.Name());
    }

    sc::result react(const EvConnectTimerExpired &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        return ToIdle(state_machine, event.Name());
    }

    sc::result react(const EvSandeshMessageRecv &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());

        if (!state_machine->GetMgr()->ReceiveMsg(event.msg, event.header,
                    event.msg_type, event.header_offset)) {
            return ToIdle(state_machine, event.Name());
        }
        state_machine->set_collector_name(event.header.get_Source());

        if (event.header.get_Hints() & g_sandesh_constants.SANDESH_CONTROL_HINT) {
            // Update connection info
            ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
                std::string(), ConnectionStatus::UP,
                state_machine->session()->remote_endpoint(),
                state_machine->StateName() + " : Control " + event.Name());       
            return transit<Established>();
        }
        return discard_event();
    }

    sc::result react(const EvSandeshSend &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        Sandesh *snh(event.snh);
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name() <<
            " : " << snh->Name());
        if (dynamic_cast<SandeshUVE *>(snh)) {
            if (Sandesh::IsLoggingDroppedAllowed(snh->type())) {
                SANDESH_LOG(ERROR, "SANDESH: Send FAILED: " <<
                    snh->ToString());
            }
            Sandesh::UpdateTxMsgFailStats(snh->Name(), 0,
                SandeshTxDropReason::WrongClientSMState);
            SM_LOG(INFO, "Received UVE message in wrong state : " << snh->Name());
            snh->Release();
            return discard_event(); 
        }
        if (!state_machine->send_session(snh)) {
            SM_LOG(INFO, "Could not EnQ Sandesh :" << snh->Name());
            // If Enqueue encounters an error, it will release the Sandesh
        }
        return discard_event();
    }

    sc::result react(const EvDiscUpdate &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        if (state_machine->DiscUpdate(
                SandeshClientSM::CLIENT_INIT, true,
                event.active_, event.backup_)) {
            return ToIdle(state_machine, event.Name());
        }  
        return discard_event();
    }

    sc::result ToIdle(SandeshClientSMImpl *state_machine,
        const char *event_name) {
        // Update connection info
        ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
            std::string(), ConnectionStatus::DOWN,
            state_machine->session()->remote_endpoint(),
            state_machine->StateName() + " : " + event_name);
        state_machine->OnIdle<EvStop>(EvStop());
        state_machine->StartIdleHoldTimer();
        SM_LOG(INFO, "Return to idle with " << state_machine->idle_hold_time()); 
        return transit<Idle>();
    }
};

struct Established : public sc::state<Established, SandeshClientSMImpl> {
    typedef mpl::list<
        TransitToIdle<EvStop>::reaction,
        sc::custom_reaction<EvTcpClose>,
        sc::custom_reaction<EvSandeshMessageRecv>,
        sc::custom_reaction<EvSandeshSend>,
        sc::custom_reaction<EvDiscUpdate>,
        DeleteTcpSession<EvTcpDeleteSession>::reaction
    > reactions;

    Established(my_context ctx) : my_base(ctx) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->set_state(SandeshClientSM::ESTABLISHED);
        SM_LOG(DEBUG, state_machine->StateName());
        state_machine->connect_attempts_clear();
        // Update connection info
        ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
            std::string(), ConnectionStatus::UP,
            state_machine->session()->remote_endpoint(),
            state_machine->StateName());
        state_machine->SendUVE();
    }

    ~Established() {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        state_machine->set_collector_name(string());
    }

    sc::result react(const EvTcpClose &event) {

        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());

        if (state_machine->DiscUpdate(
                SandeshClientSM::ESTABLISHED, false,
                TcpServer::Endpoint(), TcpServer::Endpoint())) {
            // Update connection info
            ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
                std::string(), ConnectionStatus::INIT,
                state_machine->session()->remote_endpoint(),
                state_machine->StateName() + " : " + event.Name() + 
                    " -> Connect");
            state_machine->OnIdle<EvStop>(EvStop());
            return transit<Connect>();            
        } else {
            return ToIdle(state_machine, event.Name());
        }  
    }

    sc::result react(const EvSandeshMessageRecv &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());

        if (!state_machine->GetMgr()->ReceiveMsg(event.msg, event.header,
                    event.msg_type, event.header_offset)) {
            return ToIdle(state_machine, event.Name());
        }
        return discard_event();
    }

    sc::result react(const EvSandeshSend &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();
        //SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        if (!state_machine->send_session(event.snh)) {
            SM_LOG(ERROR, "Could not EnQ Sandesh :" << event.snh->Name());
            // If Enqueue encounters an error, it will release the Sandesh
        }
        return discard_event();
    }

    sc::result react(const EvDiscUpdate &event) {
        SandeshClientSMImpl *state_machine = &context<SandeshClientSMImpl>();

        if (state_machine->DiscUpdate(
                SandeshClientSM::ESTABLISHED, true,
                event.active_, event.backup_)) {
            // Update connection info
            ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
                std::string(), ConnectionStatus::INIT,
                state_machine->session()->remote_endpoint(),
                state_machine->StateName() + " : " + event.Name() + 
                    " -> Connect");
            state_machine->OnIdle<EvStop>(EvStop());
            return transit<Connect>();            
        }  
        return discard_event();
    }

    sc::result ToIdle(SandeshClientSMImpl *state_machine,
        const char *event_name) {
        // Update connection info
        ConnectionState::GetInstance()->Update(ConnectionType::COLLECTOR,
            std::string(), ConnectionStatus::DOWN,
            state_machine->session()->remote_endpoint(),
            state_machine->StateName() + " : " + event_name);
        state_machine->OnIdle<EvStop>(EvStop());
        state_machine->StartIdleHoldTimer();
        return transit<Idle>();
    }

};

} // namespace scm

void SandeshClientSMImpl::SetAdminState(bool down) {
    if (down) {
        Enqueue(scm::EvStop());
    } else {
        reset_idle_hold_time();
        // On fresh restart of state machine, all previous state should be reset
        reset_last_info();
        Enqueue(scm::EvStart());
    }
}

void SandeshClientSMImpl::EnqueDelSession(SandeshSession * session) {
    Enqueue(scm::EvTcpDeleteSession(session));
}

template <class Ev>
void SandeshClientSMImpl::OnIdle(const Ev &event) {
    // Release all resources
    set_idle_hold_time(idle_hold_time() ? idle_hold_time() : kIdleHoldTime);

    CancelIdleHoldTimer();

    set_session(NULL, event.enq_);
}

template <class Ev>
void SandeshClientSMImpl::ReleaseSandesh(const Ev &event) {
    Sandesh *snh(event.snh);
    if (Sandesh::IsLoggingDroppedAllowed(snh->type())) {
        SANDESH_LOG(ERROR, "SANDESH: Send FAILED: " << snh->ToString());
    }
    Sandesh::UpdateTxMsgFailStats(snh->Name(), 0,
        SandeshTxDropReason::WrongClientSMState);
    SM_LOG(DEBUG, "Wrong state: " << StateName() << " for event: " <<
       event.Name() << " message: " << snh->Name());
    snh->Release();
}

template <class Ev>
void SandeshClientSMImpl::DeleteTcpSession(const Ev &event) {
    GetMgr()->DeleteSMSession(event.session);
}

void SandeshClientSMImpl::StartConnectTimer(int seconds) {
    connect_timer_->Start(seconds * 1000,
            boost::bind(&SandeshClientSMImpl::ConnectTimerExpired, this),
            boost::bind(&SandeshClientSMImpl::TimerErrorHanlder, this, _1, _2));
}

void SandeshClientSMImpl::CancelConnectTimer() {
    connect_timer_->Cancel();
}

bool SandeshClientSMImpl::ConnectTimerRunning() {
    return connect_timer_->running();
}

void SandeshClientSMImpl::StartIdleHoldTimer() {
    if (idle_hold_time_ <= 0)
        return;

    idle_hold_timer_->Start(idle_hold_time_,
            boost::bind(&SandeshClientSMImpl::IdleHoldTimerExpired, this),
            boost::bind(&SandeshClientSMImpl::TimerErrorHanlder, this, _1, _2));
}

void SandeshClientSMImpl::CancelIdleHoldTimer() {
    idle_hold_timer_->Cancel();
}

bool SandeshClientSMImpl::IdleHoldTimerRunning() {
    return idle_hold_timer_->running();
}

//
// Test Only API : Start
//
void SandeshClientSMImpl::FireConnectTimer() {
    connect_timer_->Fire();
}

void SandeshClientSMImpl::IdleHoldTimerFired() {
    idle_hold_timer_->Fire();
}
//
// Test Only API : End
//

void SandeshClientSMImpl::TimerErrorHanlder(std::string name, std::string error) {
    SM_LOG(ERROR, name + " error: " + error);
}

// Client only
bool SandeshClientSMImpl::ConnectTimerExpired() {
    if (!deleted_) {
        SM_LOG(DEBUG, server() << " "
            << "EvConnectTimerExpired in state " << StateName());
        Enqueue(scm::EvConnectTimerExpired(connect_timer_));
    }
    return false;
}

bool SandeshClientSMImpl::IdleHoldTimerExpired() {
    Enqueue(scm::EvIdleHoldTimerExpired(idle_hold_timer_));
    return false;
}

void SandeshClientSMImpl::OnSessionEvent(
        TcpSession *session, TcpSession::Event event) {
    SandeshSession *sandesh_session = dynamic_cast<SandeshSession *>(session);
    assert((session != NULL) == (sandesh_session != NULL));
    std::string session_s = session ? session->ToString() : "*";
    switch (event) {
    case TcpSession::CONNECT_COMPLETE:
        SM_LOG(DEBUG, session_s << " " << __func__ <<
               " " << "TCP Connected");
        Enqueue(scm::EvTcpConnected(sandesh_session));
        break;
    case TcpSession::CONNECT_FAILED:
        SM_LOG(DEBUG, session_s << " " << __func__ <<
               " " << "TCP Connect Failed");
        Enqueue(scm::EvTcpConnectFail(sandesh_session));
        break;
    case TcpSession::CLOSE:
        SM_LOG(DEBUG, session_s << " " << __func__ <<
               " " << "TCP Connection Closed");
        Enqueue(scm::EvTcpClose(sandesh_session));
        break;
    case TcpSession::ACCEPT:
    default:
        SM_LOG(DEBUG, session_s << " " << "Unknown event: " <<
               event);
        break;
    }
}

bool SandeshClientSMImpl::SendSandeshUVE(Sandesh * snh) {
    Enqueue(scm::EvSandeshSend(snh));
    return true;
}

bool SandeshClientSMImpl::SendSandesh(Sandesh * snh) {
    Enqueue(scm::EvSandeshSend(snh));
    return true;
}

bool SandeshClientSMImpl::OnMessage(SandeshSession *session,
                                    const std::string &msg) {
    // Demux based on Sandesh message type
    SandeshHeader header;
    std::string message_type;
    uint32_t xml_offset = 0;

    // Extract the header and message type
    int ret = SandeshReader::ExtractMsgHeader(msg, header, message_type,
        xml_offset);
    if (ret) {
        SM_LOG(ERROR, "OnMessage in state: " << StateName() << ": Extract "
               << " FAILED(" << ret << ")");
        return false;
    }
    
    if (header.get_Hints() & g_sandesh_constants.SANDESH_CONTROL_HINT) {
        SM_LOG(INFO, "OnMessage control in state: " << StateName() );
    } 
    Enqueue(scm::EvSandeshMessageRecv(msg, header, message_type, xml_offset));
    return true;
}

static const std::string state_names[] = {
    "Idle",
    "Disconnect",
    "Connect",
    "ClientInit",
    "Established",
};

const string &SandeshClientSMImpl::StateName(
    SandeshClientSM::State state) const {
    return state_names[state];
}

const string &SandeshClientSMImpl::StateName() const {
    return state_names[state_];
}

const string &SandeshClientSMImpl::LastStateName() const {
    return state_names[last_state_];
}

const int SandeshClientSMImpl::kConnectInterval;

int SandeshClientSMImpl::GetConnectTime() const {
    int backoff = std::min(attempts_, 6);
    return std::min(backoff ? 1 << (backoff - 1) : 0, kConnectInterval);
}

void SandeshClientSMImpl::UpdateEventEnqueue(const sc::event_base &event) {
    UpdateEventStats(event, true, false);
}

void SandeshClientSMImpl::UpdateEventDequeue(const sc::event_base &event) {
    UpdateEventStats(event, false, false);
}

void SandeshClientSMImpl::UpdateEventEnqueueFail(const sc::event_base &event) {
    UpdateEventStats(event, true, true);
}

void SandeshClientSMImpl::UpdateEventDequeueFail(const sc::event_base &event) {
    UpdateEventStats(event, false, true);
}

void SandeshClientSMImpl::UpdateEventStats(const sc::event_base &event,
        bool enqueue, bool fail) {
    std::string event_name(TYPE_NAME(event));
    tbb::mutex::scoped_lock lock(mutex_);
    event_stats_.Update(event_name, enqueue, fail);
    lock.release();
}

bool SandeshClientSMImpl::DequeueEvent(SandeshClientSMImpl::EventContainer ec) {
    if (deleted_) return true;
    in_dequeue_ = true;

    set_last_event(TYPE_NAME(*ec.event));
    if (ec.validate.empty() || ec.validate(this)) {
        if ((state()!=ESTABLISHED)||(TYPE_NAME(*ec.event)!="scm::EvSandeshSend"))
            SM_LOG(DEBUG, "Processing " << TYPE_NAME(*ec.event) << " in state "
                << StateName());
        UpdateEventDequeue(*ec.event);
        process_event(*ec.event);
    } else {
        SM_LOG(DEBUG, "Discarding " << TYPE_NAME(*ec.event) << " in state "
                << StateName());
        UpdateEventDequeueFail(*ec.event);
    }

    ec.event.reset();
    in_dequeue_ = false;
    return true;
}

void SandeshClientSMImpl::unconsumed_event(const sc::event_base &event) {
    SM_LOG(DEBUG, "Unconsumed " << TYPE_NAME(event) << " in state "
            << StateName());
}

// This class determines whether a given class has a method called 'validate'
template<typename Ev>
struct HasValidate
{
    template<typename T, bool (T::*)(SandeshClientSMImpl *) const> struct SFINAE {};
    template<typename T> static char Test(SFINAE<T, &T::validate>*);
    template<typename T> static int Test(...);
    static const bool Has = sizeof(Test<Ev>(0)) == sizeof(char);
};

template <typename Ev, bool has_validate>
struct ValidateFn {
    EvValidate operator()(const Ev *event) { return NULL; }
};

template <typename Ev>
struct ValidateFn<Ev, true> {
    EvValidate operator()(const Ev *event) {
        return boost::bind(&Ev::validate, event, _1);
    }
};

template <typename Ev>
void SandeshClientSMImpl::Enqueue(const Ev &event) {
    if (deleted_) return;

    EventContainer ec;
    ec.event = event.intrusive_from_this();
    ec.validate = ValidateFn<Ev, HasValidate<Ev>::Has>()(static_cast<const Ev *>(ec.event.get()));
    if (!work_queue_.Enqueue(ec)) {
        // XXX - Disable till we implement bounded work queues
        //UpdateEventEnqueueFail(event);
        //return;
    }
    UpdateEventEnqueue(event);
    return;
}


SandeshClientSMImpl::SandeshClientSMImpl(EventManager *evm, Mgr *mgr,
        int sm_task_instance, int sm_task_id)
    :   SandeshClientSM(mgr),
        active_(TcpServer::Endpoint()),
        backup_(TcpServer::Endpoint()),
        work_queue_(sm_task_id, sm_task_instance,
                boost::bind(&SandeshClientSMImpl::DequeueEvent, this, _1)),
        connect_timer_(TimerManager::CreateTimer(*evm->io_service(), "Client Connect timer", sm_task_id, sm_task_instance)),
        idle_hold_timer_(TimerManager::CreateTimer(*evm->io_service(), "Client Idle hold timer", sm_task_id, sm_task_instance)),
        statistics_timer_(TimerManager::CreateTimer(*evm->io_service(), "Client Statistics timer", sm_task_id, sm_task_instance)),
        idle_hold_time_(0),
        statistics_timer_interval_(kStatisticsSendInterval),
        attempts_(0),
        deleted_(false),
        in_dequeue_(false),
        connects_(0),
        coll_name_(string()) {
    state_ = IDLE;
    generator_key_ = Sandesh::source() + ":" + Sandesh::node_type() + ":" + 
        Sandesh::module() + ":" + Sandesh::instance_id();
    initiate();
    StartStatisticsTimer();
}

SandeshClientSMImpl::~SandeshClientSMImpl() {
    tbb::mutex::scoped_lock lock(mutex_);

    assert(!deleted_);
    deleted_ = true;

    //
    // XXX Temporary hack until config task is made to run exclusively wrt
    // all other threads including main()
    //

    while (!work_queue_.IsQueueEmpty()) usleep(100);

    while (in_dequeue_) usleep(100);

    // The State Machine should have already been shutdown by this point.
    assert(!session());

    work_queue_.Shutdown();

    //
    // Explicitly call the state destructor before the state machine itself.
    // This is needed because some of the destructors access the state machine
    // context.
    //
    terminate();

    //
    // Delete timer after state machine is terminated so that there is no
    // possible reference to the timers being deleted any more
    //
    TimerManager::DeleteTimer(connect_timer_);
    TimerManager::DeleteTimer(idle_hold_timer_);

    statistics_timer_->Cancel();
    TimerManager::DeleteTimer(statistics_timer_);
}

void SandeshClientSMImpl::StartStatisticsTimer() {
    statistics_timer_->Start(statistics_timer_interval_,
            boost::bind(&SandeshClientSMImpl::StatisticsTimerExpired, this),
            boost::bind(&SandeshClientSMImpl::TimerErrorHanlder, this, _1,
                    _2));
}

bool SandeshClientSMImpl::StatisticsTimerExpired() {
    if (deleted_ || generator_key_.empty()) {
        return true;
    }
    std::vector<SandeshStateMachineEvStats> ev_stats;
    tbb::mutex::scoped_lock lock(mutex_);
    event_stats_.Get(&ev_stats);
    lock.release();
    // Send the message
    ModuleClientState mcs;
    mcs.set_name(generator_key_);
    mcs.set_sm_queue_count(work_queue_.Length());
    mcs.set_max_sm_queue_count(work_queue_.max_queue_len());
    // Sandesh state machine statistics
    SandeshStateMachineStats sm_stats;
    sm_stats.set_ev_stats(ev_stats);
    sm_stats.set_state(StateName());
    sm_stats.set_last_state(LastStateName());
    sm_stats.set_last_event(last_event());
    sm_stats.set_state_since(state_since_);
    sm_stats.set_last_event_at(last_event_at_);
    mcs.set_sm_stats(sm_stats);
    // Sandesh session statistics
    SandeshSession *session = this->session();
    if (session) {
        mcs.set_session_stats(session->GetStats());
        SocketIOStats rx_stats;
        session->GetRxSocketStats(rx_stats);
        mcs.set_session_rx_socket_stats(rx_stats);
        SocketIOStats tx_stats;
        session->GetTxSocketStats(tx_stats);
        mcs.set_session_tx_socket_stats(tx_stats);
    }
    SandeshModuleClientTrace::Send(mcs);
    SendUVE();
    return true;
}

void SandeshClientSMImpl::GetCandidates(TcpServer::Endpoint& active,
        TcpServer::Endpoint &backup) const {
    active = active_;
    backup = backup_;   
}

void SandeshClientSMImpl::SetCandidates(
        TcpServer::Endpoint active, TcpServer::Endpoint backup) {
   Enqueue(scm::EvDiscUpdate(active,backup)); 
}

bool SandeshClientSMImpl::DiscUpdate(State from_state, bool update,
        TcpServer::Endpoint active, TcpServer::Endpoint backup) {

    if (update) {
        active_ = active;
        backup_ = backup;        
    }
    if ((from_state == DISCONNECT) ||
        ((from_state == IDLE) && (!update))) {
            set_server(active_);
            SendUVE();
            return true;
    }
    if ((from_state == ESTABLISHED)||(from_state == CONNECT)||(from_state == CLIENT_INIT)) {
        if (!update) {
            if (backup_!=TcpServer::Endpoint()) {
                TcpServer::Endpoint temp = backup_;
                backup_ = active_;
                active_ = temp; 
                set_server(active_);
                SendUVE();
                return true;
            }
        } else {
            if (server()!=active_) {
                set_server(active_);
                SendUVE();
                return true;                
            }
        }
    }
    SendUVE();
    return false;
}

SandeshClientSM * SandeshClientSM::CreateClientSM(
        EventManager *evm, Mgr *mgr, int sm_task_instance,
        int sm_task_id) {
    return new SandeshClientSMImpl(evm, mgr, sm_task_instance, sm_task_id);
}


