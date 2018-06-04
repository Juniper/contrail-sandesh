/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __SANDESH_STATISTICS_H__
#define __SANDESH_STATISTICS_H__

#include <boost/ptr_container/ptr_map.hpp>
#include <sandesh/sandesh_uve_types.h>

class SandeshMessageStatistics {
public:
    SandeshMessageStatistics() {deleted_ = false;}

    void UpdateSend(const std::string &msg_name, uint64_t bytes);
    void UpdateSendFailed(const std::string &msg_name, uint64_t bytes,
                          SandeshTxDropReason::type dreason);
    void UpdateRecv(const std::string &msg_name, uint64_t bytes);
    void UpdateRecvFailed(const std::string &msg_name, uint64_t bytes,
                          SandeshRxDropReason::type dreason);
    void Clear();

    // Detail statistics
    typedef boost::ptr_map<std::string, SandeshMessageTypeStats>
        DetailStatsMap;
    typedef std::vector<SandeshMessageTypeStats> DetailStatsList;

    void Get(DetailStatsList *v_detail_type_stats,
        SandeshMessageStats *detail_agg_stats) const;
    void Get(DetailStatsMap *m_detail_type_stats,
        SandeshMessageStats *detail_agg_stats) const;

    // Basic statistics
    typedef std::vector<SandeshMessageTypeBasicStats> BasicStatsList;

    void Get(BasicStatsList *v_basic_type_stats,
        SandeshMessageBasicStats *basic_agg_stats) const;
    void Shutdown();

private:
    bool deleted_;
    void UpdateInternal(const std::string &msg_name,
                        uint64_t bytes, bool is_tx, bool dropped,
                        SandeshTxDropReason::type send_dreason,
                        SandeshRxDropReason::type recv_dreason);

    DetailStatsMap detail_type_stats_map_;
    SandeshMessageStats detail_agg_stats_;
};

class SandeshEventStatistics {
public:
    SandeshEventStatistics() {deleted_ = false;}

    void Update(std::string &event_name, bool enqueue, bool fail);
    void Get(std::vector<SandeshStateMachineEvStats> *ev_stats) const;
    void Shutdown();

    typedef boost::ptr_map<std::string, SandeshStateMachineEvStats> EventStatsMap;
    EventStatsMap event_stats_;
    SandeshStateMachineEvStats agg_stats_;
private:
    bool deleted_;
};
    
#endif // __SANDESH_STATISTICS_H__
