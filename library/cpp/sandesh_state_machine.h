/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_state_machine.h
//
// Sandesh state machine based on boost::statechart
//

#ifndef __SANDESH_STATE_MACHINE_H__
#define __SANDESH_STATE_MACHINE_H__

#include <boost/asio.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/statechart/state_machine.hpp>
#include <tbb/mutex.h>
#include <tbb/atomic.h>

#include "base/queue_task.h"
#include "base/timer.h"
#include "io/tcp_session.h"

namespace sc = boost::statechart;

namespace ssm {

struct Idle;
struct Active;
struct Established;
struct ServerInit;
struct EvStart;

typedef enum {
    IDLE        = 0,
    ACTIVE      = 1,
    ESTABLISHED = 2,
    SERVER_INIT = 3,
} SsmState;

struct Message {
    virtual ~Message() {}
protected:
    Message() {}
};
} // namespace ssm

class SandeshSession;
class TcpSession;
class SandeshConnection;
class SandeshStateMachine;
class SandeshUVE;

typedef boost::function<bool(SandeshStateMachine *)> EvValidate;

class SandeshStateMachine :
        public sc::state_machine<SandeshStateMachine, ssm::Idle> {
public:
    static const int kConnectInterval = 30;
    static const int kIdleHoldTime = 5000; //5 sec .. specified in milliseconds
        
    SandeshStateMachine(const char *prefix, SandeshConnection *connection);
    ~SandeshStateMachine();

    void Initialize();
    void SetAdminState(bool down);

    // State transitions
    template <class Ev> void OnIdle(const Ev &event);

    // In state reactions
    template <class Ev> void ReleaseSandesh(const Ev &event);
    template <class Ev> void DeleteTcpSession(const Ev &event);
    template <class Ev> void HandleMessage(const Ev &event);

    void StartConnectTimer(int seconds);
    void CancelConnectTimer();
    bool ConnectTimerRunning();
    void FireConnectTimer();

    void StartIdleHoldTimer();
    void CancelIdleHoldTimer();
    bool IdleHoldTimerRunning();
    void IdleHoldTimerFired();

    // Feed session events into the state machine.
    void OnSessionEvent(TcpSession *session, TcpSession::Event event);

    // Receive Passive Open.
    void PassiveOpen(SandeshSession *session);

    // Send SandeshUVEs to collector
    void SandeshUVESend(SandeshUVE *usnh);

    // Receive incoming sandesh message
    void OnSandeshMessage(SandeshSession *session, const std::string &msg);

    // Receive incoming ssm::Message
    void OnMessage(ssm::Message *smsg);

    const std::string &StateName() const;
    const std::string &LastStateName() const;

    // getters and setters
    SandeshConnection *connection() { return connection_; }

    void DeleteSession(SandeshSession *session);
    void set_session(SandeshSession *session);
    TcpSession *session();

    void set_state(ssm::SsmState state) {
        last_state_ = state;
        state_ = state;
    }
    ssm::SsmState get_state() const { return state_; }

    int idle_hold_time() const { return idle_hold_time_; }
    void reset_idle_hold_time() { idle_hold_time_ = 0; }
    void set_idle_hold_time(int idle_hold_time) {
        idle_hold_time_ = idle_hold_time;
    }

    void set_last_event(const std::string &event) {
        last_event_ = event;
    }
    const std::string &last_event() const {
        return last_event_;
    }
    void reset_last_info() {
        last_state_ = ssm::IDLE;
        last_event_ = "";
    }

    void Shutdown(void);
    void unconsumed_event(const sc::event_base &event);
    const char *prefix() { return prefix_; }

    void SetStateMachineKey(const std::string& state_machine_key) {
        state_machine_key_ = state_machine_key;
    }

private:
    friend class SandeshServerStateMachineTest;
    friend class SandeshClientStateMachineTest;

    static const int PeriodicTimeSec = 30;

    struct EventContainer {
        boost::intrusive_ptr<const sc::event_base> event;
        EvValidate validate;
    };

    void TimerErrorHanlder(std::string name, std::string error);
    bool ConnectTimerExpired();
    bool IdleHoldTimerExpired();
    bool PeriodicTimerExpired();
    void PeriodicTimerErrorHandler(std::string name, std::string error);

    template <typename Ev> void Enqueue(const Ev &event);
    bool DequeueEvent(EventContainer ec);
    bool LogEvent(const sc::event_base *event);

    const char *prefix_;
    WorkQueue<EventContainer> work_queue_;
    SandeshConnection *connection_;
    SandeshSession *session_;
    Timer *connect_timer_;
    Timer *idle_hold_timer_;
    Timer *periodic_timer_;
    int idle_hold_time_;
    bool deleted_;
    bool in_dequeue_;
    bool isClient;
    tbb::atomic<ssm::SsmState> state_;
    ssm::SsmState last_state_;
    std::string last_event_;
    tbb::mutex mutex_;

    struct EventStats {
        EventStats() : enqueues(0), dequeues(0) {}
        uint64_t enqueues;
        uint64_t dequeues;
    };
    typedef boost::ptr_map<std::string, EventStats> EventStatsMap;
    EventStatsMap event_stats_;
    uint64_t tot_enqueues_;
    uint64_t tot_dequeues_;
    tbb::mutex stats_mutex_;
    std::string state_machine_key_;
            
    DISALLOW_COPY_AND_ASSIGN(SandeshStateMachine);
};

#endif // __SANDESH_STATE_MACHINE_H__
