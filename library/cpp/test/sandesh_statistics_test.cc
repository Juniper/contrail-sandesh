//
// Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
//

//
// sandesh_statistics_test.cc
//
// Sandesh Statistic Test
//

#include "testing/gunit.h"

#include <boost/foreach.hpp>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_statistics.h>

class SandeshStatisticsTest : public ::testing::Test {
};

static void UpdateMsgStats(SandeshMessageStatistics *msg_stats) {
    // Update
    // TX
    msg_stats->UpdateSend("Test", 64);
    msg_stats->UpdateSend("Test1", 64);
    msg_stats->UpdateSend("Test1", 64);
    // TX Fail
    for (int i = static_cast<int>(SandeshTxDropReason::MinDropReason) + 1;
         i < static_cast<int>(SandeshTxDropReason::MaxDropReason);
         i++) {
        if (i == static_cast<int>(SandeshTxDropReason::NoDrop)) {
            continue;
        }
        msg_stats->UpdateSendFailed("Test1", 64,
            static_cast<SandeshTxDropReason::type>(i));
    }
    // RX
    msg_stats->UpdateRecv("Test", 64);
    msg_stats->UpdateRecv("Test1", 128);
    // RX Fail
    for (int i = static_cast<int>(SandeshRxDropReason::MinDropReason) + 1;
         i < static_cast<int>(SandeshRxDropReason::MaxDropReason);
         i++) {
        if (i == static_cast<int>(SandeshRxDropReason::NoDrop)) {
            continue;
        }
        msg_stats->UpdateRecvFailed("Test1", 64,
            static_cast<SandeshRxDropReason::type>(i));
    }
}

TEST_F(SandeshStatisticsTest, DetailMsgStats) {
    SandeshMessageStatistics msg_stats;
    UpdateMsgStats(&msg_stats);
    // Get detail
    SandeshMessageStatistics::DetailStatsMap detail_mt_stats;
    SandeshMessageStats detail_agg_mt_stats;
    msg_stats.Get(&detail_mt_stats, &detail_agg_mt_stats);
    // Verify Test
    SandeshMessageStatistics::DetailStatsMap::iterator test_it =
        detail_mt_stats.find("Test");
    SandeshMessageStats *test_sms = &test_it->second->stats;
    EXPECT_EQ(1, test_sms->messages_sent);
    EXPECT_EQ(64, test_sms->bytes_sent);
    EXPECT_EQ(0, test_sms->messages_sent_dropped);
    EXPECT_EQ(0, test_sms->bytes_sent_dropped);
    EXPECT_EQ(1, test_sms->messages_received);
    EXPECT_EQ(64, test_sms->bytes_received);
    EXPECT_EQ(0, test_sms->messages_received_dropped);
    EXPECT_EQ(0, test_sms->bytes_received_dropped);
    // Verify Test1
    SandeshMessageStatistics::DetailStatsMap::iterator test1_it =
        detail_mt_stats.find("Test1");
    SandeshMessageStats *test1_sms = &test1_it->second->stats;
    EXPECT_EQ(2 , test1_sms->messages_sent);
    EXPECT_EQ(128, test1_sms->bytes_sent);
    int expected_sent_msg_dropped(
        static_cast<int>(SandeshTxDropReason::MaxDropReason) -
        static_cast<int>(SandeshTxDropReason::MinDropReason) - 2);
    int expected_sent_bytes_dropped(expected_sent_msg_dropped * 64);
    EXPECT_EQ(expected_sent_msg_dropped, test1_sms->messages_sent_dropped);
    EXPECT_EQ(expected_sent_bytes_dropped, test1_sms->bytes_sent_dropped);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_no_queue);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_no_client);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_no_session);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_queue_level);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_validation_failed);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_client_send_failed);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_session_not_connected);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_header_write_failed);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_write_failed);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_wrong_client_sm_state);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_sending_to_syslog);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_no_queue);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_no_client);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_no_session);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_queue_level);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_client_send_failed);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_session_not_connected);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_header_write_failed);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_write_failed);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_wrong_client_sm_state);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_sending_to_syslog);
    int expected_recv_msg_dropped(
        static_cast<int>(SandeshRxDropReason::MaxDropReason) -
        static_cast<int>(SandeshRxDropReason::MinDropReason) - 2);
    int expected_recv_bytes_dropped(expected_recv_msg_dropped * 64);
    EXPECT_EQ(1, test1_sms->messages_received);
    EXPECT_EQ(128, test1_sms->bytes_received);
    EXPECT_EQ(expected_recv_msg_dropped, test1_sms->messages_received_dropped);
    EXPECT_EQ(expected_recv_bytes_dropped, test1_sms->bytes_received_dropped);
    EXPECT_EQ(1, test1_sms->messages_received_dropped_no_queue);
    EXPECT_EQ(1, test1_sms->messages_received_dropped_queue_level);
    EXPECT_EQ(1, test1_sms->messages_received_dropped_create_failed);
    EXPECT_EQ(1, test1_sms->messages_received_dropped_control_msg_failed);
    EXPECT_EQ(1, test1_sms->messages_received_dropped_decoding_failed);
    EXPECT_EQ(64, test1_sms->bytes_received_dropped_no_queue);
    EXPECT_EQ(64, test1_sms->bytes_received_dropped_queue_level);
    EXPECT_EQ(64, test1_sms->bytes_received_dropped_create_failed);
    EXPECT_EQ(64, test1_sms->bytes_received_dropped_control_msg_failed);
    EXPECT_EQ(64, test1_sms->bytes_received_dropped_decoding_failed);
}

TEST_F(SandeshStatisticsTest, BasicMsgStats) {
    SandeshMessageStatistics msg_stats;
    UpdateMsgStats(&msg_stats);
    // Get basic
    SandeshMessageStatistics::BasicStatsList basic_mt_stats;
    SandeshMessageBasicStats basic_agg_mt_stats;
    msg_stats.Get(&basic_mt_stats, &basic_agg_mt_stats);
    EXPECT_EQ(2, basic_mt_stats.size());
    BOOST_FOREACH(const SandeshMessageTypeBasicStats &test_smtbs, basic_mt_stats) {
        // Verify Test
        if (test_smtbs.message_type == "Test") {
            const SandeshMessageBasicStats &test_smbs(test_smtbs.stats);
            EXPECT_EQ(1, test_smbs.messages_sent);
            EXPECT_EQ(64, test_smbs.bytes_sent);
            EXPECT_EQ(0, test_smbs.messages_sent_dropped);
            EXPECT_EQ(1, test_smbs.messages_received);
            EXPECT_EQ(64, test_smbs.bytes_received);
            EXPECT_EQ(0, test_smbs.messages_received_dropped);
        }
        // Verify Test1
        if (test_smtbs.message_type == "Test1") {
            const SandeshMessageBasicStats &test_smbs(test_smtbs.stats);
            EXPECT_EQ(2 , test_smbs.messages_sent);
            EXPECT_EQ(128, test_smbs.bytes_sent);
            int expected_sent_msg_dropped(
                static_cast<int>(SandeshTxDropReason::MaxDropReason) -
                static_cast<int>(SandeshTxDropReason::MinDropReason) - 2);
            EXPECT_EQ(expected_sent_msg_dropped,
                test_smbs.messages_sent_dropped);
            int expected_recv_msg_dropped(
                static_cast<int>(SandeshRxDropReason::MaxDropReason) -
                static_cast<int>(SandeshRxDropReason::MinDropReason) - 2);
            EXPECT_EQ(1, test_smbs.messages_received);
            EXPECT_EQ(128, test_smbs.bytes_received);
            EXPECT_EQ(expected_recv_msg_dropped,
                test_smbs.messages_received_dropped);
        }
    }
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    return success;
}
