//
// Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
//

//
// sandesh_statistics_test.cc
//
// Sandesh Statistic Test
//

#include "testing/gunit.h"

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_statistics.h>

class SandeshStatisticsTest : public ::testing::Test {
 protected:
    SandeshMessageStatistics msg_stats_;
};

TEST_F(SandeshStatisticsTest, MsgStats) {
    // Update
    // TX
    msg_stats_.UpdateSend("Test", 64);
    msg_stats_.UpdateSend("Test1", 64);
    msg_stats_.UpdateSend("Test1", 64);
    // TX Fail
    for (int i = static_cast<int>(Sandesh::DropReason::Send::MinDropReason);
         i < static_cast<int>(Sandesh::DropReason::Send::MaxDropReason);
         i++) {
        if (i == static_cast<int>(Sandesh::DropReason::Send::NoDrop)) {
            continue;
        }
        msg_stats_.UpdateSendFailed("Test1", 64,
            static_cast<Sandesh::DropReason::Send::type>(i));
    }
    // RX
    msg_stats_.UpdateRecv("Test", 64);
    msg_stats_.UpdateRecv("Test1", 128);
    // RX Fail
    for (int i = static_cast<int>(Sandesh::DropReason::Recv::MinDropReason);
         i < static_cast<int>(Sandesh::DropReason::Recv::MaxDropReason);
         i++) {
        if (i == static_cast<int>(Sandesh::DropReason::Recv::NoDrop)) {
            continue;
        }
        msg_stats_.UpdateRecvFailed("Test1", 64,
            static_cast<Sandesh::DropReason::Recv::type>(i));
    }
    // Get
    boost::ptr_map<std::string, SandeshMessageTypeStats> mt_stats;
    SandeshMessageStats agg_mt_stats;
    msg_stats_.Get(&mt_stats, &agg_mt_stats);
    // Verify Test
    boost::ptr_map<std::string, SandeshMessageTypeStats>::iterator test_it =
        mt_stats.find("Test");
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
    boost::ptr_map<std::string, SandeshMessageTypeStats>::iterator test1_it =
        mt_stats.find("Test1");
    SandeshMessageStats *test1_sms = &test1_it->second->stats;
    EXPECT_EQ(2 , test1_sms->messages_sent);
    EXPECT_EQ(128, test1_sms->bytes_sent);
    int expected_sent_msg_dropped(
        static_cast<int>(Sandesh::DropReason::Send::MaxDropReason) -
        static_cast<int>(Sandesh::DropReason::Send::MinDropReason) - 1);
    int expected_sent_bytes_dropped(expected_sent_msg_dropped * 64);
    EXPECT_EQ(expected_sent_msg_dropped, test1_sms->messages_sent_dropped);
    EXPECT_EQ(expected_sent_bytes_dropped, test1_sms->bytes_sent_dropped);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_no_queue);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_no_client);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_no_session);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_queue_level);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_client_send_failed);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_session_not_connected);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_header_write_failed);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_write_failed);
    EXPECT_EQ(1, test1_sms->messages_sent_dropped_wrong_client_sm_state);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_no_queue);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_no_client);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_no_session);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_queue_level);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_client_send_failed);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_session_not_connected);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_header_write_failed);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_write_failed);
    EXPECT_EQ(64, test1_sms->bytes_sent_dropped_wrong_client_sm_state);
    int expected_recv_msg_dropped(
        static_cast<int>(Sandesh::DropReason::Recv::MaxDropReason) -
        static_cast<int>(Sandesh::DropReason::Recv::MinDropReason) - 1);
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

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    return success;
}
