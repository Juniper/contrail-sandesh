//
// Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
//

//
// sandesh_statistics.cc
// Sandesh Statistics Implementation
//

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_statistics.h>

//
// SandeshMessageStatistics
//
void SandeshMessageStatistics::Get(
    boost::ptr_map<std::string, SandeshMessageTypeStats> *mtype_stats,
    SandeshMessageStats *magg_stats) const {
    *mtype_stats = type_all_stats_;
    *magg_stats = agg_all_stats_;
}

void SandeshMessageStatistics::Get(
    std::vector<SandeshMessageTypeStats> *mtype_stats,
    SandeshMessageStats *magg_stats) const {
    for (boost::ptr_map<std::string,
             SandeshMessageTypeStats>::const_iterator it = type_all_stats_.begin();
         it != type_all_stats_.end();
         it++) {
        mtype_stats->push_back(*it->second);
    }
    *magg_stats = agg_all_stats_;
}

void SandeshMessageStatistics::Get(
    boost::ptr_map<std::string, SandeshMessageTypeBasicStats> *mtype_stats,
    SandeshMessageBasicStats *magg_stats) const {
    *mtype_stats = type_stats_;
    *magg_stats = agg_stats_;
}

void SandeshMessageStatistics::Get(
    std::vector<SandeshMessageTypeBasicStats> *mtype_stats,
    SandeshMessageBasicStats *magg_stats) const {
    for (boost::ptr_map<std::string,
             SandeshMessageTypeBasicStats>::const_iterator it = type_stats_.begin();
         it != type_stats_.end();
         it++) {
        mtype_stats->push_back(*it->second);
    }
    *magg_stats = agg_stats_;
}

static void UpdateSandeshMessageStatsDrops(SandeshMessageStats *smstats,
    SandeshMessageBasicStats *stats, bool sent, uint64_t bytes, 
    SandeshTxDropReason::type send_dreason,
    SandeshRxDropReason::type recv_dreason) {
    if (sent) {
        switch (send_dreason) {
          case SandeshTxDropReason::ValidationFailed:
            smstats->messages_sent_dropped_validation_failed++;
            smstats->bytes_sent_dropped_validation_failed += bytes;
            break;
          case SandeshTxDropReason::RatelimitDrop:
            smstats->messages_sent_dropped_rate_limited++;
            smstats->bytes_sent_dropped_rate_limited += bytes;
            break;
          case SandeshTxDropReason::QueueLevel:
            smstats->messages_sent_dropped_queue_level++;
            smstats->bytes_sent_dropped_queue_level += bytes;
            break;
          case SandeshTxDropReason::NoClient:
            smstats->messages_sent_dropped_no_client++;
            smstats->bytes_sent_dropped_no_client += bytes;
            break;
          case SandeshTxDropReason::NoSession:
            smstats->messages_sent_dropped_no_session++;
            smstats->bytes_sent_dropped_no_session += bytes;
            break;
          case SandeshTxDropReason::NoQueue:
            smstats->messages_sent_dropped_no_queue++;
            smstats->bytes_sent_dropped_no_queue += bytes;
            break;
          case SandeshTxDropReason::ClientSendFailed:
            smstats->messages_sent_dropped_client_send_failed++;
            smstats->bytes_sent_dropped_client_send_failed += bytes;
            break;
          case SandeshTxDropReason::WrongClientSMState:
            smstats->messages_sent_dropped_wrong_client_sm_state++;
            smstats->bytes_sent_dropped_wrong_client_sm_state += bytes;
            break;
          case SandeshTxDropReason::WriteFailed:
            smstats->messages_sent_dropped_write_failed++;
            smstats->bytes_sent_dropped_write_failed += bytes;
            break;
          case SandeshTxDropReason::HeaderWriteFailed:
            smstats->messages_sent_dropped_header_write_failed++;
            smstats->bytes_sent_dropped_header_write_failed += bytes;
            break;
          case SandeshTxDropReason::SessionNotConnected:
            smstats->messages_sent_dropped_session_not_connected++;
            smstats->bytes_sent_dropped_session_not_connected += bytes;
            break;
          default:
            assert(0);
        }
        smstats->messages_sent_dropped++;
        smstats->bytes_sent_dropped += bytes;
        stats->messages_sent_dropped++;
    } else {
        switch (recv_dreason) {
          case SandeshRxDropReason::QueueLevel:
            smstats->messages_received_dropped_queue_level++;
            smstats->bytes_received_dropped_queue_level += bytes;
            break;
          case SandeshRxDropReason::NoQueue:
            smstats->messages_received_dropped_no_queue++;
            smstats->bytes_received_dropped_no_queue += bytes;
            break;
          case SandeshRxDropReason::DecodingFailed:
            smstats->messages_received_dropped_decoding_failed++;
            smstats->bytes_received_dropped_decoding_failed += bytes;
            break;
          case SandeshRxDropReason::ControlMsgFailed:
            smstats->messages_received_dropped_control_msg_failed++;
            smstats->bytes_received_dropped_control_msg_failed += bytes;
            break;
          case SandeshRxDropReason::CreateFailed:
            smstats->messages_received_dropped_create_failed++;
            smstats->bytes_received_dropped_create_failed += bytes;
            break;
          default:
            assert(0);
        }
        smstats->messages_received_dropped++;
        smstats->bytes_received_dropped += bytes;
        stats->messages_received_dropped++;
    }
}

void SandeshMessageStatistics::UpdateSend(const std::string &msg_name,
    uint64_t bytes) {
    UpdateInternal(msg_name, bytes, true, false,
        SandeshTxDropReason::NoDrop, SandeshRxDropReason::NoDrop);
}

void SandeshMessageStatistics::UpdateSendFailed(const std::string &msg_name,
    uint64_t bytes, SandeshTxDropReason::type dreason) {
    UpdateInternal(msg_name, bytes, true, true, dreason,
        SandeshRxDropReason::NoDrop);
}

void SandeshMessageStatistics::UpdateRecv(const std::string &msg_name,
    uint64_t bytes) {
    UpdateInternal(msg_name, bytes, false, false,
        SandeshTxDropReason::NoDrop, SandeshRxDropReason::NoDrop);
}

void SandeshMessageStatistics::UpdateRecvFailed(const std::string &msg_name,
    uint64_t bytes, SandeshRxDropReason::type dreason) {
    UpdateInternal(msg_name, bytes, false, true,
        SandeshTxDropReason::NoDrop, dreason);
}

void SandeshMessageStatistics::UpdateInternal(const std::string &msg_name,
    uint64_t bytes, bool is_tx, bool dropped,
    SandeshTxDropReason::type send_dreason,
    SandeshRxDropReason::type recv_dreason) {
    boost::ptr_map<std::string, SandeshMessageTypeStats>::iterator it =
        type_all_stats_.find(msg_name);
    if (it == type_all_stats_.end()) {
        std::string name(msg_name);
        SandeshMessageTypeStats *nmtastats = new SandeshMessageTypeStats;
        nmtastats->message_type = name;
        it = (type_all_stats_.insert(name, nmtastats)).first;
    }
    SandeshMessageTypeStats *mtastats = it->second;
    boost::ptr_map<std::string, SandeshMessageTypeBasicStats>::iterator ir =
        type_stats_.find(msg_name);
    if (ir == type_stats_.end()) {
        std::string name(msg_name);
        SandeshMessageTypeBasicStats *nmtstats = new SandeshMessageTypeBasicStats;
        nmtstats->message_type = name;
        ir = (type_stats_.insert(name, nmtstats)).first;
    }
    SandeshMessageTypeBasicStats *mtstats = ir->second;
    if (is_tx) {
        if (dropped) {
            UpdateSandeshMessageStatsDrops(&mtastats->stats, &mtstats->stats, is_tx,
                bytes, send_dreason, recv_dreason);
            UpdateSandeshMessageStatsDrops(&agg_all_stats_, &agg_stats_, is_tx,
                bytes, send_dreason, recv_dreason);
        } else {
            mtastats->stats.messages_sent++;
            mtastats->stats.bytes_sent += bytes;
            agg_all_stats_.messages_sent++;
            agg_all_stats_.bytes_sent += bytes;
            mtstats->stats.messages_sent++;
            mtstats->stats.bytes_sent += bytes;
            agg_stats_.messages_sent++;
            agg_stats_.bytes_sent += bytes;
        }
    } else {
        if (dropped) {
            UpdateSandeshMessageStatsDrops(&mtastats->stats, &mtstats->stats, is_tx,
                bytes, send_dreason, recv_dreason);
            UpdateSandeshMessageStatsDrops(&agg_all_stats_, &agg_stats_, is_tx,
                bytes, send_dreason, recv_dreason);
        } else {
            mtastats->stats.messages_received++;
            mtastats->stats.bytes_received += bytes;
            agg_all_stats_.messages_received++;
            agg_all_stats_.bytes_received += bytes;
            mtstats->stats.messages_received++;
            mtstats->stats.bytes_received += bytes;
            agg_stats_.messages_received++;
            agg_stats_.bytes_received += bytes;
        }
    }
}

//
// SandeshEventStatistics
//
void SandeshEventStatistics::Get(
    std::vector<SandeshStateMachineEvStats> *ev_stats) const {
    for (EventStatsMap::const_iterator it = event_stats.begin();
            it != event_stats.end(); ++it) {
        const EventStats *es = it->second;
        SandeshStateMachineEvStats ev_stat;
        ev_stat.event = it->first;
        ev_stat.enqueues = es->enqueues;
        ev_stat.dequeues = es->dequeues;
        ev_stat.enqueue_fails = es->enqueue_fails;
        ev_stat.dequeue_fails = es->dequeue_fails;
        ev_stats->push_back(ev_stat);
    }
    SandeshStateMachineEvStats ev_agg_stat;
    ev_agg_stat.enqueues = agg_stats.enqueues;
    ev_agg_stat.dequeues = agg_stats.dequeues;
    ev_agg_stat.enqueue_fails = agg_stats.enqueue_fails;
    ev_agg_stat.dequeue_fails = agg_stats.dequeue_fails;
    ev_stats->push_back(ev_agg_stat);
}

void SandeshEventStatistics::Update(std::string &event_name, bool enqueue,
    bool fail) {
    EventStatsMap::iterator it = event_stats.find(event_name);
    if (it == event_stats.end()) {
        it = (event_stats.insert(event_name, new EventStats)).first;
    }
    EventStats *es = it->second;
    if (enqueue) {
        if (fail) {
            es->enqueue_fails++;
            agg_stats.enqueue_fails++;
        } else {
            es->enqueues++;
            agg_stats.enqueues++;
        }
    } else {
        if (fail) {
            es->dequeue_fails++;
            agg_stats.dequeue_fails++;
        } else {
            es->dequeues++;
            agg_stats.dequeues++;
        }
    }
}
