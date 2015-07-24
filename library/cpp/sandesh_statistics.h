/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __SANDESH_STATISTICS_H__
#define __SANDESH_STATISTICS_H__

#include <boost/ptr_container/ptr_map.hpp>
#include <sandesh/sandesh_uve_types.h>

class SandeshMessageStatistics {
public:
    SandeshMessageStatistics() {}

    void UpdateSend(const std::string &msg_name, uint64_t bytes);
    void UpdateSendFailed(const std::string &msg_name, uint64_t bytes,
                          SandeshTxDropReason::type dreason);
    void UpdateRecv(const std::string &msg_name, uint64_t bytes);
    void UpdateRecvFailed(const std::string &msg_name, uint64_t bytes,
                          SandeshRxDropReason::type dreason);
    void Get(std::vector<SandeshMessageTypeStats> *mtype_stats,
             SandeshMessageStats *magg_stats) const;
    void Get(boost::ptr_map<std::string, SandeshMessageTypeStats> *mtype_stats,
             SandeshMessageStats *magg_stats) const;

private:
    void UpdateInternal(const std::string &msg_name,
                        uint64_t bytes, bool is_tx, bool dropped,
                        SandeshTxDropReason::type send_dreason,
                        SandeshRxDropReason::type recv_dreason);

    boost::ptr_map<std::string, SandeshMessageTypeStats> type_stats_;
    SandeshMessageStats agg_stats_;
};

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

class SandeshEventStatistics {
public:
    SandeshEventStatistics() {}

    void Update(std::string &event_name, bool enqueue, bool fail);
    void Get(std::vector<SandeshStateMachineEvStats> *ev_stats) const;

    typedef boost::ptr_map<std::string, EventStats> EventStatsMap;
    EventStatsMap event_stats;
    EventStats agg_stats;
};
    
#endif // __SANDESH_STATISTICS_H__
