/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_client_sm_priv.h
//
// Sandesh Client State Machine
//

#ifndef __SANDESH_CLIENT_SM_PRIV_H__
#define __SANDESH_CLIENT_SM_PRIV_H__

#include <boost/asio.hpp>
#include <boost/statechart/state_machine.hpp>
#include <tbb/mutex.h>
#include <tbb/atomic.h>

#include "base/queue_task.h"
#include "base/timer.h"
#include "io/tcp_session.h"
#include "sandesh_client_sm.h"

namespace sc = boost::statechart;

namespace scm {

// states
struct Idle;
struct Disconnect;
struct Connect;
struct ClientInit;
struct Established;
struct EvStart;

} // namespace scm

class SandeshClientSMImpl;
class SandeshEventStatistics;
typedef boost::function<bool(SandeshClientSMImpl *)> EvValidate;

class SandeshClientSMImpl : public SandeshClientSM,
        public sc::state_machine<SandeshClientSMImpl, scm::Idle> {
 
public:
    static const int kConnectInterval = 30;
    static const int kIdleHoldTime = 5000; //5 sec .. specified in milliseconds

    SandeshClientSMImpl(EventManager *evm, Mgr *mgr, int sm_task_instance,
            int sm_task_id, bool periodicuve);
    virtual ~SandeshClientSMImpl();

    Mgr * const GetMgr() { return mgr_; }
    void set_session(SandeshSession * session, bool enq = true) {
        SandeshClientSM::set_session(session, enq);
    }
    bool send_session(Sandesh *snh) {
        return SandeshClientSM::send_session(snh);
    }

    void SetAdminState(bool down);
    void SetCandidates(TcpServer::Endpoint active, TcpServer::Endpoint backup);
    void GetCandidates(TcpServer::Endpoint& active,
            TcpServer::Endpoint &backup) const;
    bool SendSandeshUVE(Sandesh* snh);
    bool SendSandesh(Sandesh* snh);
    void EnqueDelSession(SandeshSession * session);

    // Feed session events into the state machine.
    void OnSessionEvent(TcpSession *session, TcpSession::Event event);

    // Receive incoming message
    bool OnMessage(SandeshSession *session, const std::string &msg);

    // State transitions
    template <class Ev> void OnIdle(const Ev &event);

    // In state reactions
    template <class Ev> void ReleaseSandesh(const Ev &event);
    template <class Ev> void DeleteTcpSession(const Ev &event);

    void StartConnectTimer(int seconds);
    void CancelConnectTimer();
    bool ConnectTimerRunning();
    void FireConnectTimer();

    void StartIdleHoldTimer();
    void CancelIdleHoldTimer();
    bool IdleHoldTimerRunning();
    void IdleHoldTimerFired();

    bool DiscUpdate(State from_state, bool update,
            TcpServer::Endpoint active, TcpServer::Endpoint backup);

    // Calculate Timer value for active to connect transition.
    int GetConnectTime() const;

    const std::string &StateName() const;
    const std::string &StateName(SandeshClientSM::State state) const;
    const std::string &LastStateName() const;

    void set_state(State state) {
        last_state_ = state;
        state_ = state;
        state_since_ = UTCTimestampUsec();
    }
    State get_state() const { return state_; }

    int connects_inc() { connects_++; return connects_; }
    int connects() const { return connects_; }
    void connect_attempts_inc() { attempts_++; }
    void connect_attempts_clear() { attempts_ = 0; }
    int idle_hold_time() const { return idle_hold_time_; }
    void reset_idle_hold_time() { idle_hold_time_ = 0; }
    void set_idle_hold_time(int idle_hold_time) {
        idle_hold_time_ = idle_hold_time;
    }

    void set_last_event(const std::string &event) {
        tbb::mutex::scoped_lock lock(mutex_);
        last_event_ = event;
        last_event_at_ = UTCTimestampUsec();
    }
    const std::string last_event() const {
        tbb::mutex::scoped_lock lock(mutex_);
        return last_event_;
    }
    void reset_last_info() {
        tbb::mutex::scoped_lock lock(mutex_);
        last_state_ = IDLE;
        last_event_ = "";
    }

    void set_collector_name(const std::string& cname) { coll_name_ = cname; }
    std::string collector_name() { return coll_name_; }
    
    void unconsumed_event(const sc::event_base &event);
    void SendUVE () {
        mgr_->SendUVE(connects(), StateName(), collector_name(), active_, backup_);
    }
private:
    static const int kStatisticsSendInterval = 30000; // 30 sec .. specified in milliseconds
    struct EventContainer {
        boost::intrusive_ptr<const sc::event_base> event;
        EvValidate validate;
    };

    void StartStatisticsTimer();
    bool StatisticsTimerExpired();
    void TimerErrorHanlder(std::string name, std::string error);
    bool ConnectTimerExpired();
    bool IdleHoldTimerExpired();

    template <typename Ev> void Enqueue(const Ev &event);
    bool DequeueEvent(EventContainer ec);
    void UpdateEventDequeue(const sc::event_base &event);
    void UpdateEventDequeueFail(const sc::event_base &event);
    void UpdateEventEnqueue(const sc::event_base &event);
    void UpdateEventEnqueueFail(const sc::event_base &event);
    void UpdateEventStats(const sc::event_base &event, bool enqueue, bool fail);

    TcpServer::Endpoint active_;
    TcpServer::Endpoint backup_;
    WorkQueue<EventContainer> work_queue_;
    Timer *connect_timer_;
    Timer *idle_hold_timer_;
    Timer *statistics_timer_;
    int idle_hold_time_;
    int statistics_timer_interval_;
    bool periodicuve_;
    int attempts_;
    bool deleted_;
    bool in_dequeue_;
    int connects_;
    State last_state_;
    uint64_t state_since_;
    std::string last_event_;
    uint64_t last_event_at_;
    std::string coll_name_;
    mutable tbb::mutex mutex_;
    std::string generator_key_;
    SandeshEventStatistics event_stats_;

    friend class SandeshClientStateMachineTest;

    DISALLOW_COPY_AND_ASSIGN(SandeshClientSMImpl);
};

#endif // __SANDESH_CLIENT_SM_PRIV_H__

