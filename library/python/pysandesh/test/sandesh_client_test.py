#!/usr/bin/env python

#
# Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_client_test
#

import unittest
import sys
import socket
import mock

sys.path.insert(1, sys.path[0]+'/../../../python')

from pysandesh.sandesh_base import *
from pysandesh.sandesh_client import *
from pysandesh.util import *
from gen_py.msg_test.ttypes import *

class SandeshClientTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        sandesh_global.init_generator('sandesh_client_test', socket.gethostname(),
                'Test', 'Test', None, 'sandesh_client_test_ctxt', -1,
                connect_to_collector=False)
    # end setUpClass

    def test_sandesh_queue_level_drop(self):
        # Increase rate limit
        SandeshSystem.set_sandesh_send_rate_limit(100)
        levels = list(range(SandeshLevel.SYS_EMERG,SandeshLevel.SYS_DEBUG))
        queue_level_drop = 0
        mlevels = list(levels)
        mlevels.append(SandeshLevel.SYS_DEBUG)
        for send_level in levels:
            sandesh_global.send_level = mock.MagicMock(return_value=send_level)
            for sandesh_level in levels:
                systemlog = SystemLogTest(level=sandesh_level)
                systemlog.send()
                if sandesh_level >= send_level:
                    queue_level_drop += 1
                self.assertEqual(queue_level_drop, sandesh_global.\
                    msg_stats().aggregate_stats().\
                    messages_sent_dropped_queue_level)
    # end test_sandesh_queue_level_drop

    def test_close_sm_session(self):
        initial_close_interval_msec = \
            SandeshClient._INITIAL_SM_SESSION_CLOSE_INTERVAL_MSEC
        close_time_usec = UTCTimestampUsec()
        # First time close event
        (close, close_interval_msec) = SandeshClient._do_close_sm_session(
            close_time_usec, 0, 0)
        self.assertTrue(close)
        self.assertEqual(initial_close_interval_msec, close_interval_msec)
        # Close event time same as last close time
        last_close_interval_usec = initial_close_interval_msec * 1000
        (close, close_interval_msec) = SandeshClient._do_close_sm_session(
            close_time_usec, close_time_usec, last_close_interval_usec)
        self.assertFalse(close)
        self.assertEqual(0, close_interval_msec)
        # Close event time is less than last close interval
        (close, close_interval_msec) = SandeshClient._do_close_sm_session(
            close_time_usec, close_time_usec - last_close_interval_usec,
            last_close_interval_usec)
        self.assertFalse(close)
        self.assertEqual(0, close_interval_msec)
        # Close event time is between close interval and 2 * last close
        # interval
        last_close_interval_usec = (initial_close_interval_msec * 2) * 1000
        (close, close_interval_msec) = SandeshClient._do_close_sm_session(
            close_time_usec,
            close_time_usec - (1.5 * last_close_interval_usec),
            last_close_interval_usec)
        self.assertTrue(close)
        self.assertEqual((last_close_interval_usec * 2)/1000,
            close_interval_msec)
        # Close event time is between 2 * last close interval and 4 * last
        # close interval
        last_close_interval_usec = (initial_close_interval_msec * 3) * 1000
        (close, close_interval_msec) = SandeshClient._do_close_sm_session(
            close_time_usec,
            close_time_usec - (3 * last_close_interval_usec),
            last_close_interval_usec)
        self.assertTrue(close)
        self.assertEqual(last_close_interval_usec/1000, close_interval_msec)
        # Close event ime is beyond 4 * last close interval
        last_close_interval_usec = (initial_close_interval_msec * 2) * 1000
        (close, close_interval_msec) = SandeshClient._do_close_sm_session(
            close_time_usec,
            close_time_usec - (5 * last_close_interval_usec),
            last_close_interval_usec)
        self.assertTrue(close)
        self.assertEqual(initial_close_interval_msec, close_interval_msec)
        # Maximum close interval
        last_close_interval_usec = \
            (SandeshClient._MAX_SM_SESSION_CLOSE_INTERVAL_MSEC * 1000)/2
        (close, close_interval_msec) = SandeshClient._do_close_sm_session(
            close_time_usec,
            close_time_usec - (1.5 * last_close_interval_usec),
            last_close_interval_usec)
        self.assertTrue(close)
        self.assertEqual(SandeshClient._MAX_SM_SESSION_CLOSE_INTERVAL_MSEC,
            close_interval_msec)
    # end test_close_sm_session

# end class SandeshClientTest

if __name__ == '__main__':
    unittest.main(verbosity=2, catchbreak=True)
