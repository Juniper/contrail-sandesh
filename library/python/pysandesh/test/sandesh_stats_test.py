#!/usr/bin/env python

#
# Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_stats_test
#

import unittest
import sys

sys.path.insert(1, sys.path[0]+'/../../../python')

from pysandesh.sandesh_base import Sandesh
from pysandesh.sandesh_stats import SandeshMessageStatistics
from pysandesh.gen_py.sandesh.ttypes import SandeshTxDropReason, \
    SandeshRxDropReason

class SandeshMessageStatisticsTest(unittest.TestCase):

    def setUp(self):
        self.maxDiff = None
        self.sandesh_msg_stats = SandeshMessageStatistics()
    # end setUp

    def tearDown(self):
        pass
    # end tearDown

    def _verify_tx_drop_stats(self, stats, tmsg, tbytes, nmsg, nbytes):
        self.assertEqual(tmsg, stats.messages_sent_dropped)
        self.assertEqual(tbytes, stats.bytes_sent_dropped)
        self.assertEqual(nmsg, stats.messages_sent_dropped_validation_failed)
        self.assertEqual(nbytes, stats.bytes_sent_dropped_validation_failed)
        self.assertEqual(nmsg, stats.messages_sent_dropped_queue_level)
        self.assertEqual(nbytes, stats.bytes_sent_dropped_queue_level)
        self.assertEqual(nmsg, stats.messages_sent_dropped_no_client)
        self.assertEqual(nbytes, stats.bytes_sent_dropped_no_client)
        self.assertEqual(nmsg, stats.messages_sent_dropped_no_session)
        self.assertEqual(nbytes, stats.bytes_sent_dropped_no_session)
        self.assertEqual(nmsg, stats.messages_sent_dropped_no_queue)
        self.assertEqual(nbytes, stats.bytes_sent_dropped_no_queue)
        self.assertEqual(nmsg, stats.messages_sent_dropped_client_send_failed)
        self.assertEqual(nbytes, stats.bytes_sent_dropped_client_send_failed)
        self.assertEqual(nmsg, stats.messages_sent_dropped_header_write_failed)
        self.assertEqual(nbytes, stats.bytes_sent_dropped_header_write_failed)
        self.assertEqual(nmsg, stats.messages_sent_dropped_write_failed)
        self.assertEqual(nbytes, stats.bytes_sent_dropped_write_failed)
        self.assertEqual(nmsg, stats.messages_sent_dropped_session_not_connected)
        self.assertEqual(nbytes, stats.bytes_sent_dropped_session_not_connected)
        self.assertEqual(nmsg, stats.messages_sent_dropped_wrong_client_sm_state)
        self.assertEqual(nbytes, stats.bytes_sent_dropped_wrong_client_sm_state)
    # end _verify_tx_drop_stats

    def test_update_tx_stats(self):
        # update tx stats -> default reason - SandeshTxDropReason.NoDrop
        self.assertTrue(self.sandesh_msg_stats.update_tx_stats('Message1', 64))
        self.assertTrue(self.sandesh_msg_stats.update_tx_stats('Message2', 192))
        
        # update tx stats - drops
        # for all SandeshTxDropReason values except SandeshTxDropReason.NoDrop
        drop_nmsg = 0
        drop_nbytes = 0
        ndrops_per_reason = 2
        nbytes = 128
        for n in range(ndrops_per_reason):
            for reason in range(SandeshTxDropReason.MinDropReason+1,
                                SandeshTxDropReason.MaxDropReason):
                if reason is not SandeshTxDropReason.NoDrop:
                    self.assertTrue(self.sandesh_msg_stats.update_tx_stats(
                        'Message2', nbytes, reason))
                    drop_nmsg += 1
                    drop_nbytes += nbytes

        # verify aggregate stats
        agg_stats = self.sandesh_msg_stats.aggregate_stats()
        self.assertEqual(2, agg_stats.messages_sent)
        self.assertEqual(256, agg_stats.bytes_sent)
        self._verify_tx_drop_stats(agg_stats, drop_nmsg, drop_nbytes,
            ndrops_per_reason, ndrops_per_reason*nbytes)

        # verify per msg type stats
        # Message1
        msg_type_stats = self.sandesh_msg_stats.message_type_stats()['Message1']
        self.assertEqual(1, msg_type_stats.messages_sent)
        self.assertEqual(64, msg_type_stats.bytes_sent)
        self._verify_tx_drop_stats(msg_type_stats, None, None, None, None)
        # Message2
        msg_type_stats = self.sandesh_msg_stats.message_type_stats()['Message2']
        self.assertEqual(1, msg_type_stats.messages_sent)
        self.assertEqual(192, msg_type_stats.bytes_sent)
        self._verify_tx_drop_stats(msg_type_stats, drop_nmsg, drop_nbytes,
            ndrops_per_reason, ndrops_per_reason*nbytes)

        # Invalid tx drop reason
        self.assertFalse(self.sandesh_msg_stats.update_tx_stats('Message1', 64,
            SandeshTxDropReason.MinDropReason))
        self.assertFalse(self.sandesh_msg_stats.update_tx_stats('Message1', 64,
            SandeshTxDropReason.MinDropReason-1))
        self.assertFalse(self.sandesh_msg_stats.update_tx_stats('Message1', 64,
            SandeshTxDropReason.MaxDropReason))
        self.assertFalse(self.sandesh_msg_stats.update_tx_stats('Message1', 64,
            SandeshTxDropReason.MaxDropReason+1))
    # end test_update_tx_stats

    def _verify_rx_drop_stats(self, stats, tmsg, tbytes, nmsg, nbytes):
        self.assertEqual(tmsg, stats.messages_received_dropped)
        self.assertEqual(tbytes, stats.bytes_received_dropped)
        self.assertEqual(nmsg, stats.messages_received_dropped_queue_level)
        self.assertEqual(nbytes, stats.bytes_received_dropped_queue_level)
        self.assertEqual(nmsg, stats.messages_received_dropped_no_queue)
        self.assertEqual(nbytes, stats.bytes_received_dropped_no_queue)
        self.assertEqual(nmsg, stats.messages_received_dropped_control_msg_failed)
        self.assertEqual(nbytes, stats.bytes_received_dropped_control_msg_failed)
        self.assertEqual(nmsg, stats.messages_received_dropped_create_failed)
        self.assertEqual(nbytes, stats.bytes_received_dropped_create_failed)
        self.assertEqual(nmsg, stats.messages_received_dropped_decoding_failed)
        self.assertEqual(nbytes, stats.bytes_received_dropped_decoding_failed)
    # end _verify_rx_drop_stats

    def test_update_rx_stats(self):
        # update rx stats -> default reason - SandeshRxDropReason.NoDrop
        self.assertTrue(self.sandesh_msg_stats.update_rx_stats('Message1', 64))
        self.assertTrue(self.sandesh_msg_stats.update_rx_stats('Message2', 192))
        
        # update rx stats - drops
        # for all SandeshRxDropReason values except SandeshRxDropReason.NoDrop
        drop_nmsg = 0
        drop_nbytes = 0
        ndrops_per_reason = 2
        nbytes = 128
        for n in range(ndrops_per_reason):
            for reason in range(SandeshRxDropReason.MinDropReason+1,
                                SandeshRxDropReason.MaxDropReason):
                if reason is not SandeshRxDropReason.NoDrop:
                    self.assertTrue(self.sandesh_msg_stats.update_rx_stats(
                        'Message2', nbytes, reason))
                    drop_nmsg += 1
                    drop_nbytes += nbytes

        # verify aggregate stats
        agg_stats = self.sandesh_msg_stats.aggregate_stats()
        self.assertEqual(2, agg_stats.messages_received)
        self.assertEqual(256, agg_stats.bytes_received)
        self._verify_rx_drop_stats(agg_stats, drop_nmsg, drop_nbytes,
            ndrops_per_reason, ndrops_per_reason*nbytes)

        # verify per msg type stats
        # Message1
        msg_type_stats = self.sandesh_msg_stats.message_type_stats()['Message1']
        self.assertEqual(1, msg_type_stats.messages_received)
        self.assertEqual(64, msg_type_stats.bytes_received)
        self._verify_rx_drop_stats(msg_type_stats, None, None, None, None)
        # Message2
        msg_type_stats = self.sandesh_msg_stats.message_type_stats()['Message2']
        self.assertEqual(1, msg_type_stats.messages_received)
        self.assertEqual(192, msg_type_stats.bytes_received)
        self._verify_rx_drop_stats(msg_type_stats, drop_nmsg, drop_nbytes,
            ndrops_per_reason, ndrops_per_reason*nbytes)

        # Invalid rx drop reason
        self.assertFalse(self.sandesh_msg_stats.update_rx_stats('Message1', 32,
            SandeshRxDropReason.MinDropReason))
        self.assertFalse(self.sandesh_msg_stats.update_rx_stats('Message1', 64,
            SandeshRxDropReason.MinDropReason-1))
        self.assertFalse(self.sandesh_msg_stats.update_rx_stats('Message1', 96,
            SandeshRxDropReason.MaxDropReason))
        self.assertFalse(self.sandesh_msg_stats.update_rx_stats('Message1', 64,
            SandeshRxDropReason.MaxDropReason+1))
    # end test_update_rx_stats

# end class SandeshMessageStatisticsTest

if __name__ == '__main__':
    unittest.main(verbosity=2, catchbreak=True)
