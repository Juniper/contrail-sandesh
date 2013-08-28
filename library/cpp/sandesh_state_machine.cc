/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_state_machine.cc
//
// Sandesh State Machine Implementation
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
#include <base/task_annotations.h>
#include <io/event_manager.h>
#include <io/tcp_session.h>
#include <io/tcp_server.h>

#include <sandesh/sandesh_constants.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_session.h>
#include <sandesh/sandesh_req_types.h>
#include "sandesh_connection.h"
#include "sandesh_state_machine.h"
#include "../common/sandesh_uve_types.h"

using namespace std;
using boost::system::error_code;

namespace mpl = boost::mpl;
namespace sc = boost::statechart;

#define SM_LOG(_Level, _Msg) \
    do { \
        if (LoggingDisabled()) break;                                          \
        log4cplus::Logger _Xlogger = log4cplus::Logger::getRoot();             \
        if (_Xlogger.isEnabledFor(log4cplus::_Level##_LOG_LEVEL)) {            \
            log4cplus::tostringstream _Xbuf;                                   \
            SandeshStateMachine *_sm = &context<SandeshStateMachine>();        \
            _Xbuf << _sm->prefix() << _Msg;                                    \
            _Xlogger.forcedLog(log4cplus::_Level##_LOG_LEVEL,                  \
                               _Xbuf.str());                                   \
        }                                                                      \
    } while (false)

#define SESSION_LOG(session) \
    LOG(DEBUG, ((session) ? (session)->ToString() : "*") << ":" << Name())


namespace ssm {

// events
struct EvStart : sc::event<EvStart> {
    static const char * Name() {
        return "EvStart";
    }
};

struct EvStop : sc::event<EvStop> {
    static const char * Name() {
        return "EvStop";
    }
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

struct EvTcpPassiveOpen : sc::event<EvTcpPassiveOpen> {
    EvTcpPassiveOpen(SandeshSession *session) : session(session) {
        SESSION_LOG(session);
    };
    static const char * Name() {
        return "EvTcpPassiveOpen";
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
    bool validate(SandeshStateMachine *state_machine) const {
        return ((state_machine->connection()->session() == session) ||
                (state_machine->session() == session));
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

struct EvSandeshMessageRecv : sc::event<EvSandeshMessageRecv> {
    EvSandeshMessageRecv(const std::string &msg, const SandeshHeader& header,
            const std::string &msg_type, const uint32_t &header_offset) :
        msg(msg), header(header), msg_type(msg_type),
        header_offset(header_offset) {
    };
    static const char * Name() {
        return "EvSandeshMessageRecv";
    }
    const std::string msg;
    const SandeshHeader header;
    const std::string msg_type;
    const uint32_t header_offset;
};

struct EvSandeshCtrlMessageRecv : sc::event<EvSandeshCtrlMessageRecv> {
    EvSandeshCtrlMessageRecv(const std::string &msg,
            const SandeshHeader& header,
            const std::string &msg_type, const uint32_t &header_offset) :
        msg(msg), header(header), msg_type(msg_type),
        header_offset(header_offset) {
    };
    static const char * Name() {
        return "EvSandeshCtrlMessageRecv";
    }
    const std::string msg;
    const SandeshHeader header;
    const std::string msg_type;
    const uint32_t header_offset;
};

struct EvMessageRecv : sc::event<EvMessageRecv> {
    EvMessageRecv(ssm::Message *msg) :
        msg(msg) {
    };
    static const char * Name() {
        return "EvMessageRecv";
    }
    boost::shared_ptr<ssm::Message> msg;
};

// states
struct Idle;
struct Active;
struct Established;
struct ServerInit;

template <class Ev>
struct TransitToIdle {
    typedef sc::transition<Ev, Idle, SandeshStateMachine,
            &SandeshStateMachine::OnIdle<Ev> > reaction;
};

template <class Ev>
struct ReleaseSandesh {
    typedef sc::in_state_reaction<Ev, SandeshStateMachine,
            &SandeshStateMachine::ReleaseSandesh<Ev> > reaction;
};

template <class Ev>
struct DeleteTcpSession {
    typedef sc::in_state_reaction<Ev, SandeshStateMachine,
            &SandeshStateMachine::DeleteTcpSession<Ev> > reaction;
};

template <class Ev>
struct HandleMessage {
    typedef sc::in_state_reaction<Ev, SandeshStateMachine,
            &SandeshStateMachine::HandleMessage<Ev> > reaction;
};

struct Idle : public sc::state<Idle, SandeshStateMachine> {
    typedef mpl::list<
            sc::custom_reaction<EvStart>,
            sc::custom_reaction<EvStop>,
            sc::custom_reaction<EvTcpPassiveOpen>,
            sc::custom_reaction<EvIdleHoldTimerExpired>,
            DeleteTcpSession<EvTcpDeleteSession>::reaction
        > reactions;

    Idle(my_context ctx) : my_base(ctx) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        state_machine->set_state(ssm::IDLE);
        SM_LOG(DEBUG, state_machine->StateName());
    }

    ~Idle() {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        state_machine->CancelIdleHoldTimer();
    }

    sc::result react(const EvStart &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        if (state_machine->idle_hold_time()) {
            state_machine->StartIdleHoldTimer();
        } else {
            return transit<Active>();
        }
        return discard_event();
    }

    sc::result react(const EvStop &event) {
        SandeshStateMachine *state_mahine = &context<SandeshStateMachine>();
        state_mahine->CancelIdleHoldTimer();
        return discard_event();
    }

    sc::result react(const EvIdleHoldTimerExpired &event) {
        return transit<Active>();
    }

    // Close the session and ignore event
    sc::result react(const EvTcpPassiveOpen &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SandeshSession *session = event.session;
        state_machine->DeleteSession(session);
        return discard_event();
    }
};

struct Active : public sc::state<Active, SandeshStateMachine> {
    typedef mpl::list<
            TransitToIdle<EvStop>::reaction,
            sc::custom_reaction<EvTcpPassiveOpen>,
            sc::custom_reaction<EvTcpClose>,
            DeleteTcpSession<EvTcpDeleteSession>::reaction
        > reactions;

    Active(my_context ctx) : my_base(ctx) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        state_machine->set_state(ssm::ACTIVE);
        SM_LOG(DEBUG, state_machine->StateName());
    }

    ~Active() {
    }

    sc::result react(const EvTcpPassiveOpen &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        state_machine->set_session(event.session);
        event.session->set_observer(
            boost::bind(&SandeshStateMachine::OnSessionEvent,
                        state_machine, _1, _2));
        return transit<ServerInit>();
    }

    sc::result react(const EvTcpClose &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        state_machine->set_session(NULL);
        return discard_event();
    }
};

struct ServerInit : public sc::state<ServerInit, SandeshStateMachine> {
    typedef mpl::list<
        TransitToIdle<EvStop>::reaction,
        sc::custom_reaction<EvTcpClose>,
        sc::custom_reaction<EvSandeshCtrlMessageRecv>,
        sc::custom_reaction<EvSandeshMessageRecv>,
        DeleteTcpSession<EvTcpDeleteSession>::reaction
    > reactions;

    ServerInit(my_context ctx) : my_base(ctx) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        state_machine->set_state(ssm::SERVER_INIT);
        SM_LOG(DEBUG, state_machine->StateName());
    }

    ~ServerInit() {
    }

    sc::result react(const EvTcpClose &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        return ResetSession(state_machine);
    }

    sc::result react(const EvSandeshCtrlMessageRecv &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        SandeshConnection *connection = state_machine->connection();
        if (!connection->HandleSandeshCtrlMessage(event.msg, event.header,
                event.msg_type, event.header_offset)) {
            return ResetSession(state_machine);
        }
        return transit<Established>();
    }

    sc::result react(const EvSandeshMessageRecv &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        SandeshConnection *connection = state_machine->connection();
        if (!connection->HandleSandeshMessage(event.msg, event.header,
                event.msg_type, event.header_offset)) {
            return ResetSession(state_machine);
        }
        return discard_event();
    }
    //
    // Reset established session
    //
    // Bring down the state to IDLE, notify interested parties, cleanup and
    // reset the state machine
    //
    sc::result ResetSession(SandeshStateMachine *state_machine) {
        // session in the connection will be reset to NULL
        // asynchronously  after cleanup is complete
        state_machine->set_session(NULL);
        state_machine->set_state(ssm::IDLE);
        return discard_event();
    }
};
            
struct Established : public sc::state<Established, SandeshStateMachine> {
    typedef mpl::list<
        TransitToIdle<EvStop>::reaction,
        sc::custom_reaction<EvTcpClose>,
        sc::custom_reaction<EvSandeshMessageRecv>,
        DeleteTcpSession<EvTcpDeleteSession>::reaction,
        HandleMessage<EvMessageRecv>::reaction
    > reactions;

    Established(my_context ctx) : my_base(ctx) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        state_machine->set_state(ssm::ESTABLISHED);
        SM_LOG(DEBUG, state_machine->StateName());
    }

    sc::result react(const EvTcpClose &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        SandeshConnection *connection = state_machine->connection();
        SandeshSession *sess = static_cast<SandeshSession *>(state_machine->session());
        connection->HandleDisconnect(sess);
        return ResetSession(state_machine);
    }

    sc::result react(const EvSandeshMessageRecv &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SandeshConnection *connection = state_machine->connection();
        if (!connection->HandleSandeshMessage(event.msg, event.header,
                event.msg_type, event.header_offset)) {
            return ResetSession(state_machine);
        }
        return discard_event();
    }

    //
    // Reset established session
    //
    sc::result ResetSession(SandeshStateMachine *state_machine) {
        // Release all resources
        state_machine->set_session(NULL);
        state_machine->set_state(ssm::IDLE);
        return discard_event();
    }
};

} // namespace ssm

SandeshStateMachine::SandeshStateMachine(const char *prefix, SandeshConnection *connection)
    : prefix_(prefix),
      work_queue_(TaskScheduler::GetInstance()->GetTaskId("sandesh::StateMachine"),
                  connection->GetIndex(),
                  boost::bind(&SandeshStateMachine::DequeueEvent, this, _1)),
      connection_(connection),
      session_(),
      idle_hold_timer_(TimerManager::CreateTimer(*connection->server()->event_manager()->io_service(), "Idle hold timer")),
      periodic_timer_(TimerManager::CreateTimer(*connection->server()->event_manager()->io_service(), "Periodic timer")),
      idle_hold_time_(0),
      deleted_(false),
      in_dequeue_(false),
      tot_enqueues_(0),
      tot_dequeues_(0) {
    state_ = ssm::IDLE;
    initiate();
    periodic_timer_->Start(PeriodicTimeSec * 1000,
            boost::bind(&SandeshStateMachine::PeriodicTimerExpired, this),
            boost::bind(&SandeshStateMachine::PeriodicTimerErrorHandler, this, _1, _2));
}

SandeshStateMachine::~SandeshStateMachine() {
    tbb::mutex::scoped_lock lock(mutex_);
    TcpSession *sess = session();

    assert(!deleted_);
    deleted_ = true;

    //
    // XXX Temporary hack until config task is made to run exclusively wrt
    // all other threads including main()
    //
    while (in_dequeue_) usleep(100);

    work_queue_.Shutdown();

    //
    // If there is a session assigned to this state machine, reset the observer
    // so that tcp does not have a reference to 'this' which is going away
    //
    if (sess) {
        set_session(NULL);
    }

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
    TimerManager::DeleteTimer(idle_hold_timer_);
    TimerManager::DeleteTimer(periodic_timer_);
}

void SandeshStateMachine::Initialize() {
    Enqueue(ssm::EvStart());
}

void SandeshStateMachine::Shutdown(void) {
    Enqueue(ssm::EvStop());
}

void SandeshStateMachine::SetAdminState(bool down) {
    if (down) {
        Enqueue(ssm::EvStop());
    } else {
        reset_idle_hold_time();
        // On fresh restart of state machine, all previous state should be reset
        reset_last_info();
        Enqueue(ssm::EvStart());
    }
}

void SandeshStateMachine::set_session(SandeshSession *session) {
    if (session_ != NULL) {
        DeleteSession(session_);
    }
    connection_->set_session(session);
    session_ = session;
}

void SandeshStateMachine::DeleteSession(SandeshSession *session) {
    session->set_observer(NULL);
    session->Close();
    Enqueue(ssm::EvTcpDeleteSession(session));
}

TcpSession *SandeshStateMachine::session() {
    return session_;
}

template <class Ev>
void SandeshStateMachine::OnIdle(const Ev &event) {
    // Release all resources
    set_idle_hold_time(idle_hold_time() ? idle_hold_time() : kIdleHoldTime);

    CancelIdleHoldTimer();

    set_session(NULL);
}

template <class Ev>
void SandeshStateMachine::DeleteTcpSession(const Ev &event) {
    event.session->connection()->server()->DeleteSession(event.session);
}

template <class Ev>
void SandeshStateMachine::HandleMessage(const Ev &event) {
    connection()->HandleMessage(event.msg.get());
}

void SandeshStateMachine::StartIdleHoldTimer() {
    if (idle_hold_time_ <= 0)
        return;

    idle_hold_timer_->Start(idle_hold_time_,
            boost::bind(&SandeshStateMachine::IdleHoldTimerExpired, this),
            boost::bind(&SandeshStateMachine::TimerErrorHanlder, this, _1, _2));
}

void SandeshStateMachine::CancelIdleHoldTimer() {
    idle_hold_timer_->Cancel();
}

bool SandeshStateMachine::IdleHoldTimerRunning() {
    return idle_hold_timer_->running();
}

//
// Test Only API : Start
//
void SandeshStateMachine::IdleHoldTimerFired() {
    idle_hold_timer_->Fire();
}
//
// Test Only API : End
//

void SandeshStateMachine::TimerErrorHanlder(std::string name, std::string error) {
    SM_LOG(ERROR, name + " error: " + error);
}

/* TBD restart timer on error */
void SandeshStateMachine::PeriodicTimerErrorHandler(std::string name, std::string error) {
    SM_LOG(ERROR, name + " error: " + error);
}

bool SandeshStateMachine::PeriodicTimerExpired() {
    if (!deleted_ && !state_machine_key_.empty()) {
        std::vector<SandeshStateMachineEvStats> sm_stats;

        tbb::mutex::scoped_lock lock(stats_mutex_);
        EventStatsMap::iterator it;
        for (it = event_stats_.begin(); it != event_stats_.end(); it++) {
            EventStats *es = it->second;
            SandeshStateMachineEvStats ev_stats;
            ev_stats.generator = it->first;
            ev_stats.enqueues = es->enqueues;
            ev_stats.count = es->enqueues - es->dequeues;
            sm_stats.push_back(ev_stats);
        }

        SandeshStateMachineInfo_s sm_info;
        sm_info.set_agg_count(tot_enqueues_ - tot_dequeues_);
        sm_info.set_name(state_machine_key_);
        sm_info.set_sm_stats(sm_stats);
        SandeshStateMachineInfo::Send(sm_info);
    }

    return true;
}

bool SandeshStateMachine::IdleHoldTimerExpired() {
    Enqueue(ssm::EvIdleHoldTimerExpired(idle_hold_timer_));
    return false;
}

void SandeshStateMachine::OnSessionEvent(
        TcpSession *session, TcpSession::Event event) {
    SandeshSession *sandesh_session = dynamic_cast<SandeshSession *>(session);
    assert((session != NULL) == (sandesh_session != NULL));
    std::string session_s = session ? session->ToString() : "*";
    switch (event) {
    case TcpSession::CLOSE:
        SM_LOG(DEBUG, session_s << " " << __func__ <<
               " " << "TCP Connection Closed");
        Enqueue(ssm::EvTcpClose(sandesh_session));
        break;
    case TcpSession::ACCEPT:
        break;
    default:
        SM_LOG(DEBUG, session_s << " " << "Unknown event: " <<
               event);
        break;
    }
}

void SandeshStateMachine::PassiveOpen(SandeshSession *session) {
    session->set_observer(boost::bind(&SandeshStateMachine::OnSessionEvent,
            this, _1, _2));
    SM_LOG(DEBUG, session->ToString() << " " << "PassiveOpen");
    Enqueue(ssm::EvTcpPassiveOpen(session));
}

void SandeshStateMachine::OnSandeshMessage(SandeshSession *session,
                                           const std::string &msg) {
    // Demux based on Sandesh message type
    SandeshHeader header;
    std::string message_type;
    uint32_t xml_offset = 0;

    // Extract the header and message type
    SandeshReader::ExtractMsgHeader(msg, header, message_type, xml_offset);
    
    if (header.get_Hints() & g_sandesh_constants.SANDESH_CONTROL_HINT) {
        SM_LOG(DEBUG, "OnMessage control in state: " << StateName() << " session " << session->ToString());
        Enqueue(ssm::EvSandeshCtrlMessageRecv(msg, header, message_type, xml_offset));
    } else {
        Enqueue(ssm::EvSandeshMessageRecv(msg, header, message_type, xml_offset));
    }
}

void SandeshStateMachine::OnMessage(ssm::Message *msg) {
    Enqueue(ssm::EvMessageRecv(msg));
}

static const std::string state_names[] = {
    "Idle",
    "Active",
    "Established",
    "ServerInit"
};

const string &SandeshStateMachine::StateName() const {
    return state_names[state_];
}

const string &SandeshStateMachine::LastStateName() const {
    return state_names[last_state_];
}

const int SandeshStateMachine::kConnectInterval;

 
bool SandeshStateMachine::LogEvent(const sc::event_base *event) {
    if (state_ == ssm::ESTABLISHED) {
        const ssm::EvSandeshMessageRecv *snh_rcv = 
            dynamic_cast<const ssm::EvSandeshMessageRecv *>(event);
        if (snh_rcv != NULL) {
            return false;
        }
    }
    return true;
}


bool SandeshStateMachine::DequeueEvent(SandeshStateMachine::EventContainer ec) {
    if (deleted_) return true;
    in_dequeue_ = true;

    // update stats
      {
        tbb::mutex::scoped_lock lock(stats_mutex_);
        std::string event_name(TYPE_NAME(*ec.event));
        EventStatsMap::iterator it = event_stats_.find(event_name);
        if (it == event_stats_.end()) {
            it = (event_stats_.insert(event_name, new EventStats)).first;
        }
        EventStats *es = it->second;
        es->dequeues++;
        tot_dequeues_++;
      }

    set_last_event(TYPE_NAME(*ec.event));
    if (ec.validate.empty() || ec.validate(this)) {
        // Log only relevant events and states
        if (LogEvent(ec.event.get())) {
            SM_LOG(DEBUG, "Processing " << TYPE_NAME(*ec.event) << " in state "
                   << StateName());
        } 
        process_event(*ec.event);
    } else {
        SM_LOG(DEBUG, "Discarding " << TYPE_NAME(*ec.event) << " in state "
                << StateName());
    }

    ec.event.reset();
    in_dequeue_ = false;
    return true;
}

void SandeshStateMachine::unconsumed_event(const sc::event_base &event) {
    SM_LOG(DEBUG, "Unconsumed " << TYPE_NAME(event) << " in state "
            << StateName());
}

// This class determines whether a given class has a method called 'validate'
template<typename Ev>
struct HasValidate
{
    template<typename T, bool (T::*)(SandeshStateMachine *) const> struct SFINAE {};
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
void SandeshStateMachine::Enqueue(const Ev &event) {
    if (deleted_) return;

    EventContainer ec;
    ec.event = event.intrusive_from_this();
    ec.validate = ValidateFn<Ev, HasValidate<Ev>::Has>()(static_cast<const Ev *>(ec.event.get()));
    work_queue_.Enqueue(ec);

    tbb::mutex::scoped_lock lock(stats_mutex_);
    std::string event_name(TYPE_NAME(*ec.event));
    EventStatsMap::iterator it = event_stats_.find(event_name);
    if (it == event_stats_.end()) {
        it = (event_stats_.insert(event_name, new EventStats)).first;
    }
    EventStats *es = it->second;
    es->enqueues++;
    tot_enqueues_++;
}
