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
#include <sandesh/sandesh_uve_types.h>
#include <sandesh/sandesh_message_builder.h>
#include "sandesh_statistics.h"
#include "sandesh_connection.h"
#include "sandesh_state_machine.h"

using namespace std;
using boost::system::error_code;

namespace mpl = boost::mpl;
namespace sc = boost::statechart;

#define SM_LOG(_Level, _Msg)                                                   \
    do {                                                                       \
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
    EvIdleHoldTimerExpired(Timer *timer)  : timer_(timer) {
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
        SESSION_LOG(session);
    }
    static const char *Name() {
        return "EvTcpDeleteSession";
    }
    SandeshSession *session;
};

struct EvSandeshMessageRecv : sc::event<EvSandeshMessageRecv> {
    EvSandeshMessageRecv(const SandeshMessage *msg) :
        msg(msg) {
    };
    static const char * Name() {
        return "EvSandeshMessageRecv";
    }
    boost::shared_ptr<const SandeshMessage> msg;
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

struct EvResourceUpdate : sc::event<EvResourceUpdate> {
    EvResourceUpdate(bool rsc) :
        rsc(rsc) {
    };
    static const char * Name() {
        return "EvResourceUpdate";
    }
    bool rsc;
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
struct ProcessMessage {
    typedef sc::in_state_reaction<Ev, SandeshStateMachine,
            &SandeshStateMachine::ProcessMessage<Ev> > reaction;
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
        return transit<Idle>();
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
        state_machine->set_session(NULL);
        return transit<Idle>();
    }

    sc::result react(const EvSandeshCtrlMessageRecv &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        SandeshConnection *connection = state_machine->connection();
        if (!connection->ProcessSandeshCtrlMessage(event.msg, event.header,
                event.msg_type, event.header_offset)) {
            state_machine->set_session(NULL);
            return transit<Idle>();
        }
        return transit<Established>();
    }

    sc::result react(const EvSandeshMessageRecv &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        SandeshConnection *connection = state_machine->connection();
        if (!connection->ProcessSandeshMessage(event.msg.get(), true)) {
            state_machine->set_session(NULL);
            return transit<Idle>();
        }
        return discard_event();
    }
};
            
struct Established : public sc::state<Established, SandeshStateMachine> {
    typedef mpl::list<
        TransitToIdle<EvStop>::reaction,
        sc::custom_reaction<EvTcpClose>,
        sc::custom_reaction<EvSandeshMessageRecv>,
        DeleteTcpSession<EvTcpDeleteSession>::reaction,
        sc::custom_reaction<EvResourceUpdate>
    > reactions;

    Established(my_context ctx) : my_base(ctx) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        state_machine->set_state(ssm::ESTABLISHED);
        state_machine->set_resource(true);
        SM_LOG(DEBUG, state_machine->StateName());
    }

    ~Established() {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        state_machine->set_resource(false);
    }

    sc::result react(const EvTcpClose &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SM_LOG(DEBUG, state_machine->StateName() << " : " << event.Name());
        // Process disconnect
        SandeshConnection *connection = state_machine->connection();
        connection->ProcessDisconnect(state_machine->session());
        // Reset the session
        state_machine->set_session(NULL);
        return transit<Idle>();
    }

    sc::result react(const EvResourceUpdate &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SandeshConnection *connection = state_machine->connection();
     
        state_machine->set_resource(event.rsc);

        if (!connection->ProcessResourceUpdate(event.rsc)) {
            state_machine->set_session(NULL);
            return transit<Idle>();              
        }
        return discard_event();
    }

    sc::result react(const EvSandeshMessageRecv &event) {
        SandeshStateMachine *state_machine = &context<SandeshStateMachine>();
        SandeshConnection *connection = state_machine->connection();
        if (!connection->ProcessSandeshMessage(event.msg.get(),
                state_machine->get_resource())) {
            state_machine->set_session(NULL);
            return transit<Idle>();
        }
        return discard_event();
    }
};

} // namespace ssm

SandeshStateMachine::SandeshStateMachine(const char *prefix, SandeshConnection *connection)
    : prefix_(prefix),
      work_queue_(connection->GetTaskId(),
                  connection->GetTaskInstance(),
                  boost::bind(&SandeshStateMachine::DequeueEvent, this, _1)),
      connection_(connection),
      session_(),
      idle_hold_timer_(TimerManager::CreateTimer(
              *connection->server()->event_manager()->io_service(),
              "Idle hold timer", 
              connection->GetTaskId(),
              connection->GetTaskInstance())),
      idle_hold_time_(0),
      deleted_(false),
      resource_(false),
      builder_(SandeshMessageBuilder::GetInstance(SandeshMessageBuilder::XML)),
      message_drop_level_(SandeshLevel::INVALID) {
    state_ = ssm::IDLE;
    initiate();
}

SandeshStateMachine::~SandeshStateMachine() {
    assert(!deleted_);
    deleted_ = true;

    work_queue_.Shutdown();

    assert(session() == NULL);

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

// Note this api does not enqueue the deletion of TCP session
void SandeshStateMachine::clear_session() {
    if (session_ != NULL) {
        session_->set_observer(NULL);
        session_->SetReceiveMsgCb(NULL);
        session_->SetConnection(NULL);
        session_->Close();
        session_->Shutdown();
        connection_->set_session(NULL);
        session_ = NULL;
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
    session->SetReceiveMsgCb(NULL);
    session->SetConnection(NULL);
    session->Close();
    session->Shutdown();
    Enqueue(ssm::EvTcpDeleteSession(session));
}

SandeshSession *SandeshStateMachine::session() {
    return session_;
}

int SandeshStateMachine::task_id() const {
    return connection_->GetTaskId();
}

int SandeshStateMachine::task_instance() const {
    return connection_->GetTaskInstance();
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
    event.session->server()->DeleteSession(event.session);
    SandeshConnection *connection = this->connection();
    if (connection) {
        connection->ManagedDelete();
    }
}

void SandeshStateMachine::StartIdleHoldTimer() {
    if (idle_hold_time_ <= 0)
        return;

    idle_hold_timer_->Start(idle_hold_time_,
            boost::bind(&SandeshStateMachine::IdleHoldTimerExpired, this),
            boost::bind(&SandeshStateMachine::TimerErrorHandler, this, _1,
                    _2));
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

void SandeshStateMachine::TimerErrorHandler(std::string name, std::string error) {
    SM_LOG(ERROR, name + " error: " + error);
}

bool SandeshStateMachine::GetQueueCount(uint64_t &queue_count) const {
    if (deleted_ || generator_key_.empty()) {
        return false;
    }
    queue_count = work_queue_.Length();
    return true;
}

bool SandeshStateMachine::GetMessageDropLevel(
    std::string &drop_level) const {
    if (deleted_ || generator_key_.empty()) {
        return false;
    }
    drop_level = Sandesh::LevelToString(message_drop_level_);
    return true;
} 

bool SandeshStateMachine::GetStatistics(
        SandeshStateMachineStats &sm_stats,
        SandeshGeneratorStats &msg_stats) {
    if (deleted_ || generator_key_.empty()) {
        return false;
    }
    std::vector<SandeshStateMachineEvStats> ev_stats;
    tbb::mutex::scoped_lock elock(smutex_);
    // State machine event statistics
    event_stats_.Get(ev_stats);
    elock.release();
    sm_stats.set_ev_stats(ev_stats);
    sm_stats.set_state(StateName());
    sm_stats.set_last_state(LastStateName());
    sm_stats.set_last_event(last_event());
    sm_stats.set_state_since(state_since_);
    sm_stats.set_last_event_at(last_event_at_);
    // Sandesh message statistics
    std::vector<SandeshMessageTypeStats> mtype_stats;
    SandeshMessageStats magg_stats;
    tbb::mutex::scoped_lock mlock(smutex_);
    message_stats_.Get(mtype_stats, magg_stats);
    mlock.release();
    msg_stats.set_type_stats(mtype_stats);
    msg_stats.set_aggregate_stats(magg_stats);
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
    SandeshMessage *xmessage = builder_->Create(
        reinterpret_cast<const uint8_t *>(msg.c_str()), msg.size());
    const SandeshHeader &header(xmessage->GetHeader());
    const std::string &message_type(xmessage->GetMessageType());
    // Drop ? 
    if (DoDropSandeshMessage(header, message_drop_level_)) {
        // Update message statistics
        tbb::mutex::scoped_lock lock(smutex_);
        message_stats_.Update(message_type, msg.size(), false, true);
        lock.release();
        delete xmessage;
        return;
    }
    // Update message statistics
    tbb::mutex::scoped_lock lock(smutex_);
    message_stats_.Update(message_type, msg.size(), false, false);
    lock.release();
    if (header.get_Hints() & g_sandesh_constants.SANDESH_CONTROL_HINT) {
        SandeshHeader ctrl_header;
        std::string ctrl_message_type;
        uint32_t ctrl_xml_offset = 0;
        // Extract the header and message type
        int ret = SandeshReader::ExtractMsgHeader(msg, ctrl_header,
            ctrl_message_type, ctrl_xml_offset);
        if (ret) {
            SM_LOG(ERROR, "OnMessage control in state: " << StateName() <<
                " session " << session->ToString() << ": Extract FAILED ("
                << ret << ")");
            delete xmessage;
            return;
        } 
        assert(header == ctrl_header);
        assert(message_type == ctrl_message_type);
        SM_LOG(DEBUG, "OnMessage control in state: " << StateName() <<
                " session " << session->ToString());
        Enqueue(ssm::EvSandeshCtrlMessageRecv(msg, ctrl_header,
                ctrl_message_type, ctrl_xml_offset));
        delete xmessage;
    } else {
        Enqueue(ssm::EvSandeshMessageRecv(xmessage));
    }
}

void SandeshStateMachine::ResourceUpdate(bool rsc) {
    Enqueue(ssm::EvResourceUpdate(rsc));
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

void SandeshStateMachine::UpdateEventEnqueue(const sc::event_base &event) {
    UpdateEventStats(event, true, false);
}

void SandeshStateMachine::UpdateEventDequeue(const sc::event_base &event) {
    UpdateEventStats(event, false, false);
}

void SandeshStateMachine::UpdateEventEnqueueFail(const sc::event_base &event) {
    UpdateEventStats(event, true, true);
}

void SandeshStateMachine::UpdateEventDequeueFail(const sc::event_base &event) {
    UpdateEventStats(event, false, true);
}

void SandeshStateMachine::UpdateEventStats(const sc::event_base &event,
        bool enqueue, bool fail) {
    std::string event_name(TYPE_NAME(event));
    tbb::mutex::scoped_lock lock(smutex_);
    event_stats_.Update(event_name, enqueue, fail);
    lock.release();
}

bool SandeshStateMachine::DequeueEvent(SandeshStateMachine::EventContainer &ec) {
    if (deleted_) {
        return true;
    }
    set_last_event(TYPE_NAME(*ec.event));
    if (ec.validate.empty() || ec.validate(this)) {
        // Log only relevant events and states
        if (LogEvent(ec.event.get())) {
            SM_LOG(DEBUG, "Processing " << TYPE_NAME(*ec.event) << " in state "
                   << StateName() << " Key " << generator_key());
        }
        // Update event stats
        UpdateEventDequeue(*ec.event);
        process_event(*ec.event);
    } else {
        SM_LOG(DEBUG, "Discarding " << TYPE_NAME(*ec.event) << " in state "
                << StateName() << " Key " << generator_key());
        // Update event stats
        UpdateEventDequeueFail(*ec.event);
    }
    ec.event.reset();
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
    if (deleted_) {
        return;
    }
    EventContainer ec;
    ec.event = event.intrusive_from_this();
    ec.validate = ValidateFn<Ev, HasValidate<Ev>::Has>()(static_cast<const Ev *>(ec.event.get()));
    if (!work_queue_.Enqueue(ec)) {
        // Update event stats
        // XXX Disable till we implement bounded work queues
        //UpdateEventEnqueueFail(event);
        //return;
    }
    // Update event stats
    UpdateEventEnqueue(event);
    return;
}

void SandeshStateMachine::SetSandeshMessageDropLevel(size_t queue_count,
    SandeshLevel::type level) {
    if (message_drop_level_ != level) {
        SM_LOG(INFO, "SANDESH MESSAGE DROP LEVEL: [" <<
            Sandesh::LevelToString(message_drop_level_) << "] -> [" <<
            Sandesh::LevelToString(level) << "], SM QUEUE COUNT: " <<
            queue_count);
        message_drop_level_ = level;
    }
}

void SandeshStateMachine::SetQueueWaterMarkInfo(
    Sandesh::QueueWaterMarkInfo &wm) {
    bool high(boost::get<2>(wm));
    size_t queue_count(boost::get<0>(wm));
    EventQueue::WaterMarkInfo wmi(queue_count, 
        boost::bind(&SandeshStateMachine::SetSandeshMessageDropLevel, 
            this, _1, boost::get<1>(wm)));
    if (high) {
        work_queue_.SetHighWaterMark(wmi);
    } else {
        work_queue_.SetLowWaterMark(wmi);
    }
}

void SandeshStateMachine::ResetQueueWaterMarkInfo() {
    work_queue_.ResetHighWaterMark();
    work_queue_.ResetLowWaterMark();
}
