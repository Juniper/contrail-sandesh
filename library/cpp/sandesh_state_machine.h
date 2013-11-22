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
class SandeshStateMachineStats;

typedef boost::function<bool(SandeshStateMachine *)> EvValidate;

class SandeshStateMachine :
        public sc::state_machine<SandeshStateMachine, ssm::Idle> {
public:
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
    template <class Ev> void ProcessMessage(const Ev &event);

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

    // In established state, the SM accepts updates to resource state
    void ResourceUpdate(bool rsc);

    const std::string &StateName() const;
    const std::string &LastStateName() const;

    // getters and setters
    SandeshConnection *connection() { return connection_; }

    void DeleteSession(SandeshSession *session);
    void set_session(SandeshSession *session);
    void clear_session();
    SandeshSession *session();

    void set_state(ssm::SsmState state) {
        last_state_ = state;
        state_ = state;
        state_since_ = UTCTimestampUsec();
    }
    ssm::SsmState get_state() const { return state_; }

    bool get_resource() const { return resource_; }
    void set_resource(bool r) { resource_ = r; }

    int idle_hold_time() const { return idle_hold_time_; }
    void reset_idle_hold_time() { idle_hold_time_ = 0; }
    void set_idle_hold_time(int idle_hold_time) {
        idle_hold_time_ = idle_hold_time;
    }

    void set_last_event(const std::string &event) {
        last_event_ = event;
        last_event_at_ = UTCTimestampUsec();
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

    void SetGeneratorKey(const std::string &generator) {
        generator_key_ = generator;
    }
    const std::string &generator_key() const {
        return generator_key_;
    }
    bool GetQueueCount(uint64_t &queue_count) const;
    bool GetStatistics(SandeshStateMachineStats &stats);

private:
    friend class SandeshServerStateMachineTest;
    friend class SandeshClientStateMachineTest;

    struct EventContainer {
        boost::intrusive_ptr<const sc::event_base> event;
        EvValidate validate;
    };

    void TimerErrorHandler(std::string name, std::string error);
    bool IdleHoldTimerExpired();

    template <typename Ev> void Enqueue(const Ev &event);
    bool DequeueEvent(EventContainer ec);
    bool LogEvent(const sc::event_base *event);
    void UpdateEventDequeue(const sc::event_base &event);
    void UpdateEventDequeueFail(const sc::event_base &event);
    void UpdateEventEnqueue(const sc::event_base &event);
    void UpdateEventEnqueueFail(const sc::event_base &event);
    void UpdateEventStats(const sc::event_base &event, bool enqueue, bool fail);

    const char *prefix_;
    WorkQueue<EventContainer> work_queue_;
    SandeshConnection *connection_;
    SandeshSession *session_;
    Timer *idle_hold_timer_;
    int idle_hold_time_;
    bool deleted_;
    tbb::atomic<ssm::SsmState> state_;
    bool resource_;
    ssm::SsmState last_state_;
    uint64_t state_since_;
    std::string last_event_;
    uint64_t last_event_at_;

    struct EventStats {
        EventStats() :
            enqueues(0),
            enqueue_fails(0),
            dequeues(0),
            dequeue_fails(0) {
        }
        uint64_t enqueues;
        uint64_t enqueue_fails;
        uint64_t dequeues;
        uint64_t dequeue_fails;
    };
    typedef boost::ptr_map<std::string, EventStats> EventStatsMap;
    EventStatsMap event_stats_;
    uint64_t enqueues_;
    uint64_t enqueue_fails_;
    uint64_t dequeues_;
    uint64_t dequeue_fails_;
    tbb::mutex stats_mutex_;
    std::string generator_key_;
            
    DISALLOW_COPY_AND_ASSIGN(SandeshStateMachine);
};

#endif // __SANDESH_STATE_MACHINE_H__
