//
// Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
//

//
// sandesh_statistics.cc
// Sandesh Statistics Implementation
//

#include <boost/foreach.hpp>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_statistics.h>

//
// SandeshMessageStatistics
//

// Detail stats
void SandeshMessageStatistics::Get(DetailStatsMap *m_detail_type_stats,
    SandeshMessageStats *detail_agg_stats) const {
    *m_detail_type_stats = detail_type_stats_map_;
    *detail_agg_stats = detail_agg_stats_;
}

void SandeshMessageStatistics::Get(DetailStatsList *v_detail_type_stats,
    SandeshMessageStats *detail_agg_stats) const {
    BOOST_FOREACH(DetailStatsMap::const_iterator::value_type it,
        detail_type_stats_map_) {
        v_detail_type_stats->push_back(*it.second);
    }
    *detail_agg_stats = detail_agg_stats_;
}

// Basic stats
static void PopulateBasicStats(const SandeshMessageStats &detail_stats,
    SandeshMessageBasicStats *basic_stats) {
    basic_stats->set_messages_sent(detail_stats.get_messages_sent());
    basic_stats->set_bytes_sent(detail_stats.get_bytes_sent());
    basic_stats->set_messages_received(detail_stats.get_messages_received());
    basic_stats->set_bytes_received(detail_stats.get_bytes_received());
    basic_stats->set_messages_sent_dropped(
        detail_stats.get_messages_sent_dropped());
    basic_stats->set_messages_received_dropped(
        detail_stats.get_messages_received_dropped());
}

static void PopulateBasicTypeStats(const SandeshMessageTypeStats &detail_stats,
    SandeshMessageTypeBasicStats *basic_stats) {
    basic_stats->message_type = detail_stats.message_type;
    PopulateBasicStats(detail_stats.stats, &basic_stats->stats);
}

void SandeshMessageStatistics::Get(BasicStatsList *v_basic_type_stats,
    SandeshMessageBasicStats *basic_agg_stats) const {
    BOOST_FOREACH(DetailStatsMap::const_iterator::value_type it,
        detail_type_stats_map_) {
        const SandeshMessageTypeStats *detail_stats(it.second);
        SandeshMessageTypeBasicStats basic_stats;
        PopulateBasicTypeStats(*detail_stats, &basic_stats);
        v_basic_type_stats->push_back(basic_stats);
    }
    PopulateBasicStats(detail_agg_stats_, basic_agg_stats);
}

static void UpdateDetailStatsDrops(SandeshMessageStats *smstats,
    bool sent, uint64_t bytes, SandeshTxDropReason::type send_dreason,
    SandeshRxDropReason::type recv_dreason) {
    if (sent) {
        switch (send_dreason) {
          case SandeshTxDropReason::ValidationFailed:
            smstats->set_messages_sent_dropped_validation_failed(
                smstats->get_messages_sent_dropped_validation_failed() + 1);
            smstats->set_bytes_sent_dropped_validation_failed(
                smstats->get_bytes_sent_dropped_validation_failed() + bytes);
            break;
          case SandeshTxDropReason::RatelimitDrop:
            smstats->set_messages_sent_dropped_rate_limited(
                smstats->get_messages_sent_dropped_rate_limited() + 1);
            smstats->set_bytes_sent_dropped_rate_limited(
                smstats->get_bytes_sent_dropped_rate_limited() + bytes);
            break;
          case SandeshTxDropReason::QueueLevel:
            smstats->set_messages_sent_dropped_queue_level(
                smstats->get_messages_sent_dropped_queue_level() + 1);
            smstats->set_bytes_sent_dropped_queue_level(
                smstats->get_bytes_sent_dropped_queue_level() + bytes);
            break;
          case SandeshTxDropReason::NoClient:
            smstats->set_messages_sent_dropped_no_client(
                smstats->get_messages_sent_dropped_no_client() + 1);
            smstats->set_bytes_sent_dropped_no_client(
                smstats->get_bytes_sent_dropped_no_client() + bytes);
            break;
          case SandeshTxDropReason::NoSession:
            smstats->set_messages_sent_dropped_no_session(
                smstats->get_messages_sent_dropped_no_session() + 1);
            smstats->set_bytes_sent_dropped_no_session(
                smstats->get_bytes_sent_dropped_no_session() + bytes);
            break;
          case SandeshTxDropReason::NoQueue:
            smstats->set_messages_sent_dropped_no_queue(
                smstats->get_messages_sent_dropped_no_queue() + 1);
            smstats->set_bytes_sent_dropped_no_queue(
                smstats->get_bytes_sent_dropped_no_queue() + bytes);
            break;
          case SandeshTxDropReason::ClientSendFailed:
            smstats->set_messages_sent_dropped_client_send_failed(
                smstats->get_messages_sent_dropped_client_send_failed() + 1);
            smstats->set_bytes_sent_dropped_client_send_failed(
                smstats->get_bytes_sent_dropped_client_send_failed() + bytes);
            break;
          case SandeshTxDropReason::WrongClientSMState:
            smstats->set_messages_sent_dropped_wrong_client_sm_state(
                smstats->get_messages_sent_dropped_wrong_client_sm_state() + 1);
            smstats->set_bytes_sent_dropped_wrong_client_sm_state(
                smstats->get_bytes_sent_dropped_wrong_client_sm_state() +
                bytes);
            break;
          case SandeshTxDropReason::WriteFailed:
            smstats->set_messages_sent_dropped_write_failed(
                smstats->get_messages_sent_dropped_write_failed() + 1);
            smstats->set_bytes_sent_dropped_write_failed(
                smstats->get_bytes_sent_dropped_write_failed() + bytes);
            break;
          case SandeshTxDropReason::HeaderWriteFailed:
            smstats->set_messages_sent_dropped_header_write_failed(
                smstats->get_messages_sent_dropped_header_write_failed() + 1);
            smstats->set_bytes_sent_dropped_header_write_failed(
                smstats->get_bytes_sent_dropped_header_write_failed() + bytes);
            break;
          case SandeshTxDropReason::SessionNotConnected:
            smstats->set_messages_sent_dropped_session_not_connected(
                smstats->get_messages_sent_dropped_session_not_connected() + 1);
            smstats->set_bytes_sent_dropped_session_not_connected(
                smstats->get_bytes_sent_dropped_session_not_connected() +
                bytes);
            break;
          default:
            assert(0);
        }
        smstats->set_messages_sent_dropped(
            smstats->get_messages_sent_dropped() + 1);
        smstats->set_bytes_sent_dropped(
            smstats->get_bytes_sent_dropped() + bytes);
    } else {
        switch (recv_dreason) {
          case SandeshRxDropReason::QueueLevel:
            smstats->set_messages_received_dropped_queue_level(
                smstats->get_messages_received_dropped_queue_level() + 1);
            smstats->set_bytes_received_dropped_queue_level(
                smstats->get_bytes_received_dropped_queue_level() + bytes);
            break;
          case SandeshRxDropReason::NoQueue:
            smstats->set_messages_received_dropped_no_queue(
                smstats->get_messages_received_dropped_no_queue() + 1);
            smstats->set_bytes_received_dropped_no_queue(
                smstats->get_bytes_received_dropped_no_queue() + bytes);
            break;
          case SandeshRxDropReason::DecodingFailed:
            smstats->set_messages_received_dropped_decoding_failed(
                smstats->get_messages_received_dropped_decoding_failed() + 1);
            smstats->set_bytes_received_dropped_decoding_failed(
                smstats->get_bytes_received_dropped_decoding_failed() + bytes);
            break;
          case SandeshRxDropReason::ControlMsgFailed:
            smstats->set_messages_received_dropped_control_msg_failed(
                smstats->get_messages_received_dropped_control_msg_failed() +
                1);
            smstats->set_bytes_received_dropped_control_msg_failed(
                smstats->get_bytes_received_dropped_control_msg_failed() +
                bytes);
            break;
          case SandeshRxDropReason::CreateFailed:
            smstats->set_messages_received_dropped_create_failed(
                smstats->get_messages_received_dropped_create_failed() + 1);
            smstats->set_bytes_received_dropped_create_failed(
                smstats->get_bytes_received_dropped_create_failed() + bytes);
            break;
          default:
            assert(0);
        }
        smstats->set_messages_received_dropped(
            smstats->get_messages_received_dropped() + 1);
        smstats->set_bytes_received_dropped(
            smstats->get_bytes_received_dropped() + bytes);
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
    // Update detail stats
    DetailStatsMap::iterator it = detail_type_stats_map_.find(msg_name);
    if (it == detail_type_stats_map_.end()) {
        std::string name(msg_name);
        SandeshMessageTypeStats *n_detail_mtstats(new SandeshMessageTypeStats);
        n_detail_mtstats->message_type = name;
        it = (detail_type_stats_map_.insert(name, n_detail_mtstats)).first;
    }
    SandeshMessageTypeStats *detail_mtstats = it->second;
    if (dropped) {
        UpdateDetailStatsDrops(&detail_mtstats->stats, is_tx,
            bytes, send_dreason, recv_dreason);
        UpdateDetailStatsDrops(&detail_agg_stats_, is_tx,
            bytes, send_dreason, recv_dreason);
    } else {
        SandeshMessageStats *d_smstats(&detail_mtstats->stats);
        if (is_tx) {
            d_smstats->set_messages_sent(d_smstats->get_messages_sent() + 1);
            d_smstats->set_bytes_sent(d_smstats->get_bytes_sent() + bytes);
            detail_agg_stats_.set_messages_sent(
                detail_agg_stats_.get_messages_sent() + 1);
            detail_agg_stats_.set_bytes_sent(
                detail_agg_stats_.get_bytes_sent() + bytes);
        } else {
            d_smstats->set_messages_received(
                d_smstats->get_messages_received() + 1);
            d_smstats->set_bytes_received(
                d_smstats->get_bytes_received() + bytes);
            detail_agg_stats_.set_messages_received(
                detail_agg_stats_.get_messages_received() + 1);
            detail_agg_stats_.set_bytes_received(
                detail_agg_stats_.get_bytes_received() + bytes);
        }
    }
}

//
// SandeshEventStatistics
//
void SandeshEventStatistics::Get(
    std::vector<SandeshStateMachineEvStats> *ev_stats) const {
    if (deleted_) {
        return;
    }
    BOOST_FOREACH(EventStatsMap::const_iterator::value_type it,
        event_stats_) {
        ev_stats->push_back(*it.second);
    }
    ev_stats->push_back(agg_stats_);
}

void SandeshEventStatistics::Update(std::string &event_name, bool enqueue,
    bool fail) {
    if (deleted_) {
        return;
    }
    EventStatsMap::iterator it = event_stats_.find(event_name);
    if (it == event_stats_.end()) {
        it = (event_stats_.insert(event_name, new SandeshStateMachineEvStats)).first;
    }
    SandeshStateMachineEvStats *es = it->second;
    es->set_event(event_name);
    if (enqueue) {
        if (fail) {
            es->set_enqueue_fails(es->get_enqueue_fails() + 1);
            agg_stats_.set_enqueue_fails(agg_stats_.get_enqueue_fails() + 1);
        } else {
            es->set_enqueues(es->get_enqueues() + 1);
            agg_stats_.set_enqueues(agg_stats_.get_enqueues() + 1);
        }
    } else {
        if (fail) {
            es->set_dequeue_fails(es->get_dequeue_fails() + 1);
            agg_stats_.set_dequeue_fails(agg_stats_.get_dequeue_fails() + 1);
        } else {
            es->set_dequeues(es->get_dequeues() + 1);
            agg_stats_.set_dequeues(agg_stats_.get_dequeues() + 1);
        }
    }
}

void SandeshEventStatistics::Shutdown() {
    deleted_ = true;
}
