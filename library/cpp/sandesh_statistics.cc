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
    *mtype_stats = type_stats_;
    *magg_stats = agg_stats_;
}

void SandeshMessageStatistics::Get(
    std::vector<SandeshMessageTypeStats> *mtype_stats,
    SandeshMessageStats *magg_stats) const {
    for (boost::ptr_map<std::string,
             SandeshMessageTypeStats>::const_iterator it = type_stats_.begin();
         it != type_stats_.end();
         it++) {
        mtype_stats->push_back(*it->second);
    }
    *magg_stats = agg_stats_;
}

static void UpdateSandeshMessageStatsDrops(SandeshMessageStats *smstats,
    bool sent, uint64_t bytes, Sandesh::DropReason::Send::type send_dreason,
    Sandesh::DropReason::Recv::type recv_dreason) {
    if (sent) {
        switch (send_dreason) {
          case Sandesh::DropReason::Send::QueueLevel:
            smstats->messages_sent_dropped_queue_level++;
            smstats->bytes_sent_dropped_queue_level += bytes;
            break;
          case Sandesh::DropReason::Send::NoClient:
            smstats->messages_sent_dropped_no_client++;
            smstats->bytes_sent_dropped_no_client += bytes;
            break;
          case Sandesh::DropReason::Send::NoSession:
            smstats->messages_sent_dropped_no_session++;
            smstats->bytes_sent_dropped_no_session += bytes;
            break;
          case Sandesh::DropReason::Send::NoQueue:
            smstats->messages_sent_dropped_no_queue++;
            smstats->bytes_sent_dropped_no_queue += bytes;
            break;
          case Sandesh::DropReason::Send::ClientSendFailed:
            smstats->messages_sent_dropped_client_send_failed++;
            smstats->bytes_sent_dropped_client_send_failed += bytes;
            break;
          case Sandesh::DropReason::Send::WrongClientSMState:
            smstats->messages_sent_dropped_wrong_client_sm_state++;
            smstats->bytes_sent_dropped_wrong_client_sm_state += bytes;
            break;
          case Sandesh::DropReason::Send::WriteFailed:
            smstats->messages_sent_dropped_write_failed++;
            smstats->bytes_sent_dropped_write_failed += bytes;
            break;
          case Sandesh::DropReason::Send::HeaderWriteFailed:
            smstats->messages_sent_dropped_header_write_failed++;
            smstats->bytes_sent_dropped_header_write_failed += bytes;
            break;
          case Sandesh::DropReason::Send::SessionNotConnected:
            smstats->messages_sent_dropped_session_not_connected++;
            smstats->bytes_sent_dropped_session_not_connected += bytes;
            break;
          default:
            break;
        }
        smstats->messages_sent_dropped++;
        smstats->bytes_sent_dropped += bytes;
    } else {
        switch (recv_dreason) {
          case Sandesh::DropReason::Recv::QueueLevel:
            smstats->messages_received_dropped_queue_level++;
            smstats->bytes_received_dropped_queue_level += bytes;
            break;
          case Sandesh::DropReason::Recv::NoQueue:
            smstats->messages_received_dropped_no_queue++;
            smstats->bytes_received_dropped_no_queue += bytes;
            break;
          case Sandesh::DropReason::Recv::DecodingFailed:
            smstats->messages_received_dropped_decoding_failed++;
            smstats->bytes_received_dropped_decoding_failed += bytes;
            break;
          case Sandesh::DropReason::Recv::ControlMsgFailed:
            smstats->messages_received_dropped_control_msg_failed++;
            smstats->bytes_received_dropped_control_msg_failed += bytes;
            break;
          case Sandesh::DropReason::Recv::CreateFailed:
            smstats->messages_received_dropped_create_failed++;
            smstats->bytes_received_dropped_create_failed += bytes;
            break;
          default:
            break;
        }
        smstats->messages_received_dropped++;
        smstats->bytes_received_dropped += bytes;
    }
}

void SandeshMessageStatistics::UpdateSend(const std::string &msg_name,
    uint64_t bytes) {
    UpdateInternal(msg_name, bytes, true, false,
        Sandesh::DropReason::Send::NoDrop, Sandesh::DropReason::Recv::NoDrop);
}

void SandeshMessageStatistics::UpdateSendFailed(const std::string &msg_name,
    uint64_t bytes, Sandesh::DropReason::Send::type dreason) {
    UpdateInternal(msg_name, bytes, true, true, dreason,
        Sandesh::DropReason::Recv::NoDrop);
}

void SandeshMessageStatistics::UpdateRecv(const std::string &msg_name,
    uint64_t bytes) {
    UpdateInternal(msg_name, bytes, false, false,
        Sandesh::DropReason::Send::NoDrop, Sandesh::DropReason::Recv::NoDrop);
}

void SandeshMessageStatistics::UpdateRecvFailed(const std::string &msg_name,
    uint64_t bytes, Sandesh::DropReason::Recv::type dreason) {
    UpdateInternal(msg_name, bytes, false, true,
        Sandesh::DropReason::Send::NoDrop, dreason);
}

void SandeshMessageStatistics::UpdateInternal(const std::string &msg_name,
    uint64_t bytes, bool is_tx, bool dropped,
    Sandesh::DropReason::Send::type send_dreason,
    Sandesh::DropReason::Recv::type recv_dreason) {
    boost::ptr_map<std::string, SandeshMessageTypeStats>::iterator it =
        type_stats_.find(msg_name);
    if (it == type_stats_.end()) {
        std::string name(msg_name);
        SandeshMessageTypeStats *nmtstats = new SandeshMessageTypeStats;
        nmtstats->message_type = name;
        it = (type_stats_.insert(name, nmtstats)).first;
    }
    SandeshMessageTypeStats *mtstats = it->second;
    if (is_tx) {
        if (dropped) {
            UpdateSandeshMessageStatsDrops(&mtstats->stats, is_tx,
                bytes, send_dreason, recv_dreason);
            UpdateSandeshMessageStatsDrops(&agg_stats_, is_tx,
                bytes, send_dreason, recv_dreason);
        } else {
            mtstats->stats.messages_sent++;
            mtstats->stats.bytes_sent += bytes;
            agg_stats_.messages_sent++;
            agg_stats_.bytes_sent += bytes;
        }
    } else {
        if (dropped) {
            UpdateSandeshMessageStatsDrops(&mtstats->stats, is_tx,
                bytes, send_dreason, recv_dreason);
            UpdateSandeshMessageStatsDrops(&agg_stats_, is_tx,
                bytes, send_dreason, recv_dreason);
        } else {
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
