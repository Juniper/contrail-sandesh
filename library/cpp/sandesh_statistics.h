/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __SANDESH_STATISTICS_H__
#define __SANDESH_STATISTICS_H__

#include <sandesh/sandesh_uve_types.h>

class SandeshStatistics {
public:
    SandeshStatistics() {}

    void Update(const std::string& sandesh_name,
                uint32_t bytes, bool is_tx, bool dropped);
    void Get(std::vector<SandeshMessageTypeStats> &mtype_stats,
             SandeshMessageStats &magg_stats) const;
    void Get(boost::ptr_map<std::string, SandeshMessageTypeStats> &mtype_stats,
             SandeshMessageStats &magg_stats) const;

    boost::ptr_map<std::string, SandeshMessageTypeStats> type_stats;
    SandeshMessageStats agg_stats;
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
    void Get(std::vector<SandeshStateMachineEvStats> &ev_stats) const;

    typedef boost::ptr_map<std::string, EventStats> EventStatsMap;
    EventStatsMap event_stats;
    EventStats agg_stats;
};
    
#endif // __SANDESH_STATISTICS_H__
