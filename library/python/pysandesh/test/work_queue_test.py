#!/usr/bin/env python

#
# Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
#

#
# work_queue_test
#

import gevent
import unittest
import mock
import functools

from pysandesh.work_queue import WorkQueue, WaterMark
from pysandesh import util

class WorkQueueEntry(object):

    def __init__(self, i=None):
        self.item = i
    # end __init__

# end class WorkQueueEntry


class WaterMarkCbInfo(object):
    cb_type = util.enum('INVALID',
                        'HWM1',
                        'HWM2',
                        'HWM3',
                        'LWM1',
                        'LWM2',
                        'LWM3')

    def __init__(self):
        self.wm_cb_type = WaterMarkCbInfo.cb_type.INVALID
        self.wm_cb_qsize = 0
        self.wm_cb_count = 0
    # end __init__

    def callback(self, qsize, wm_cb_type=cb_type):
        self.wm_cb_qsize = qsize
        self.wm_cb_type = wm_cb_type
        self.wm_cb_count += 1
    # end callback

# end class WaterMarkCbInfo


class WorkQueueTest(unittest.TestCase):

    def setUp(self):
        self.maxDiff = None
        self.worker_count = 0
        self.wm_cb_info = WaterMarkCbInfo()
    # end setUp

    def tearDown(self):
        pass
    # end tearDown

    def enqueue_entries(self, workq, count):
        for i in range(count):
            workq.enqueue(WorkQueueEntry(i))
    # end enqueue_entries

    def dequeue_entries(self, workq, count):
        for i in range(count):
            workq.dequeue()
    # end dequeue_entries

    def start_runner_always(self):
        return True
    # end start_runner_always

    def start_runner_never(self):
        return False
    # end start_runner_never

    def worker(self, entry):
        self.worker_count += 1
    # end worker

    @mock.patch.object(WorkQueue, 'may_be_start_runner')
    def test_enqueue_dequeue(self, mock_may_be_start_runner):
        workq = WorkQueue(self.worker, start_runner=None, max_qsize=1)
        entry = WorkQueueEntry()

        # basic enqueue test
        self.assertEqual(True, workq.enqueue(entry))
        self.assertEqual(1, workq.num_enqueues())
        self.assertEqual(0, workq.drops())
        self.assertEqual(1, workq.size())
        self.assertEqual(1, workq.may_be_start_runner.call_count)

        # work queue is not bounded
        # verify that we could enqueue more than max_qsize
        self.assertFalse(workq.bounded())
        self.assertEqual(True, workq.enqueue(entry))
        self.assertEqual(2, workq.num_enqueues())
        self.assertEqual(0, workq.drops())
        self.assertEqual(2, workq.size())
        self.assertEqual(2, workq.may_be_start_runner.call_count)

        # work queue is bounded
        # verify that enqueue fails if qsize > max_qsize
        workq.set_bounded(True)
        self.assertTrue(workq.bounded())
        self.assertEqual(False, workq.enqueue(entry))
        self.assertEqual(2, workq.num_enqueues())
        self.assertEqual(1, workq.drops())
        self.assertEqual(2, workq.size())
        self.assertEqual(2, workq.may_be_start_runner.call_count)

        # basic dequeue test
        self.assertIs(entry, workq.dequeue())
        self.assertEqual(1, workq.num_dequeues())
        self.assertEqual(1, workq.size())
        self.assertEqual(entry, workq.dequeue())
        self.assertEqual(2, workq.num_dequeues())
        self.assertEqual(0, workq.size())

        # call dequeue with no item in the queue
        self.assertEqual(None, workq.dequeue())
        self.assertEqual(2, workq.num_dequeues())
        self.assertEqual(0, workq.size())
    # end test_enqueue_dequeue

    def test_runner(self):
        # Don't set start_runner
        # verify runner is executed and worker() is invoked
        workq = WorkQueue(self.worker)
        self.assertFalse(workq.runner().running())
        self.enqueue_entries(workq, 5)
        self.assertEqual(5, workq.size())
        self.assertEqual(5, workq.num_enqueues())
        self.assertTrue(workq.runner().running())
        gevent.sleep()
        self.assertEqual(0, workq.size())
        self.assertEqual(5, workq.num_dequeues())
        self.assertEqual(5, self.worker_count)

        # set start_runner to return False
        # verify that runner doesn't start
        workq._start_runner = self.start_runner_never
        self.assertFalse(workq.runner().running())
        self.enqueue_entries(workq, 3)
        self.assertFalse(workq.runner().running())
        self.assertEqual(3, workq.size())
        self.assertEqual(8, workq.num_enqueues())
        gevent.sleep()
        self.assertEqual(3, workq.size())
        self.assertEqual(5, workq.num_dequeues())
        self.assertEqual(5, self.worker_count)

        # set start_runner to return True
        # verify that runner gets executed
        workq._start_runner = self.start_runner_always
        workq.may_be_start_runner()
        self.assertTrue(workq.runner().running())
        gevent.sleep()
        self.assertEqual(0, workq.size())
        self.assertEqual(8, workq.num_dequeues())
        self.assertEqual(8, self.worker_count)
    # end test_runner

    def test_max_workload(self):
        max_workload = 2
        workq = WorkQueue(self.worker, max_work_load=max_workload)
        self.assertFalse(workq.runner().running())
        num_iterations = 3
        total_work_items = max_workload*num_iterations
        self.enqueue_entries(workq, total_work_items)
        self.assertEqual(total_work_items, workq.size())
        self.assertEqual(total_work_items, workq.num_enqueues())
        # verify that only max_work_load is executed per cycle
        num_dequeues = 0
        num_work_items = total_work_items
        for i in range(num_iterations):
            self.assertTrue(workq.runner().running())
            gevent.sleep()
            num_work_items -= max_workload
            num_dequeues += max_workload
            self.assertEqual(num_work_items, workq.size())
            self.assertEqual(num_dequeues, workq.num_dequeues())
        self.assertFalse(workq.runner().running())
        self.assertEqual(0, workq.size())
        self.assertEqual(total_work_items, workq.num_dequeues())
    # end test_max_workload

    def test_watermarks(self):
        workq = WorkQueue(self.worker)

        hw1 = WaterMark(5, functools.partial(self.wm_cb_info.callback,
                                wm_cb_type=WaterMarkCbInfo.cb_type.HWM1))
        hw2 = WaterMark(11, functools.partial(self.wm_cb_info.callback,
                                wm_cb_type=WaterMarkCbInfo.cb_type.HWM2))
        hw3 = WaterMark(17, functools.partial(self.wm_cb_info.callback,
                                wm_cb_type=WaterMarkCbInfo.cb_type.HWM3))
        # set high watermarks with duplicates and in unsorted order
        workq.set_high_watermarks([hw1, hw3, hw1, hw2])
        # verify that duplicates are removed and the list is sorted 
        self.assertEqual([hw1, hw2, hw3], workq.high_watermarks())
        self.assertEqual((-1, -1), workq.watermark_indices())

        lw1 = WaterMark(14, functools.partial(self.wm_cb_info.callback,
                                wm_cb_type=WaterMarkCbInfo.cb_type.LWM1))
        lw2 = WaterMark(8, functools.partial(self.wm_cb_info.callback,
                                wm_cb_type=WaterMarkCbInfo.cb_type.LWM2))
        lw3 = WaterMark(2, functools.partial(self.wm_cb_info.callback,
                                wm_cb_type=WaterMarkCbInfo.cb_type.LWM3))
        # set low watermarks with duplicates and in unsorted order
        workq.set_low_watermarks([lw2, lw2, lw1, lw3, lw3])
        # verify that duplicates are removed and the list is sorted
        self.assertEqual([lw3, lw2, lw1], workq.low_watermarks())
        self.assertEqual((-1, -1), workq.watermark_indices())

        # verify the initial values
        self.assertEqual(WaterMarkCbInfo.cb_type.INVALID,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(0, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(0, self.wm_cb_info.wm_cb_count)

        # enqueue 4 entries
        # verify that we did not hit any high watermark
        self.enqueue_entries(workq, 4)
        self.assertEqual(4, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.INVALID,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(0, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(0, self.wm_cb_info.wm_cb_count)
        self.assertEqual((-1, -1), workq.watermark_indices())

        # dequeue 1 entry
        # verify that we hit LWM2
        self.dequeue_entries(workq, 1)
        self.assertEqual(3, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM2,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(3, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(1, self.wm_cb_info.wm_cb_count)
        self.assertEqual((0, 1), workq.watermark_indices())

        # dequeue 1 entry
        # verify that we hit LWM3
        self.dequeue_entries(workq, 1)
        self.assertEqual(2, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM3,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(2, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(2, self.wm_cb_info.wm_cb_count)
        self.assertEqual((-1, 0), workq.watermark_indices())

        # dequeue 2 entries
        # verify that we did not hit any new low watermark
        self.dequeue_entries(workq, 2)
        self.assertEqual(0, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM3,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(2, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(2, self.wm_cb_info.wm_cb_count)
        self.assertEqual((-1, 0), workq.watermark_indices())

        # enqueue 5 entries
        # verify that we hit HWM1
        self.enqueue_entries(workq, 5)
        self.assertEqual(5, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM1,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(5, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(3, self.wm_cb_info.wm_cb_count)
        self.assertEqual((0, 1), workq.watermark_indices())

        # enqueue 2 entries
        # verify that we did not hit any new high watermark
        self.enqueue_entries(workq, 2)
        self.assertEqual(7, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM1,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(5, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(3, self.wm_cb_info.wm_cb_count)
        self.assertEqual((0, 1), workq.watermark_indices())

        # enqueue 11 entries
        # verify that we hit HWM2 and HWM3
        self.enqueue_entries(workq, 11)
        self.assertEqual(18, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM3,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(17, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(5, self.wm_cb_info.wm_cb_count)
        self.assertEqual((2, 3), workq.watermark_indices())

        # dequeue 6 entries
        # verify that we hit LWM1
        self.dequeue_entries(workq, 6)
        self.assertEqual(12, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM1,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(14, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(6, self.wm_cb_info.wm_cb_count)
        self.assertEqual((1, 2), workq.watermark_indices())

        # enqueue 5 entries
        # verify that we hit HWM3
        self.enqueue_entries(workq, 5)
        self.assertEqual(17, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM3,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(17, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(7, self.wm_cb_info.wm_cb_count)
        self.assertEqual((2, 3), workq.watermark_indices())

        # dequeue 1 entry
        # verify that we did not hit any low watermark
        self.dequeue_entries(workq, 1)
        self.assertEqual(16, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM3,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(17, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(7, self.wm_cb_info.wm_cb_count)
        self.assertEqual((2, 3), workq.watermark_indices())

        # dequeue 8 entries
        # verify that we hit LWM1 and LWM2
        self.dequeue_entries(workq, 8)
        self.assertEqual(8, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM2,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(8, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(9, self.wm_cb_info.wm_cb_count)
        self.assertEqual((0, 1), workq.watermark_indices())

        # enqueue 3 entries
        # verify that we hit HWM2
        self.enqueue_entries(workq, 3)
        self.assertEqual(11, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM2,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(11, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(10, self.wm_cb_info.wm_cb_count)
        self.assertEqual((1, 2), workq.watermark_indices())

        # dequeue 1 entry
        # verify that we did not hit any low watermark
        self.dequeue_entries(workq, 1)
        self.assertEqual(10, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM2,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(11, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(10, self.wm_cb_info.wm_cb_count)
        self.assertEqual((1, 2), workq.watermark_indices())

        # dequeue 3 entries
        # verify that we hit LWM2
        self.dequeue_entries(workq, 3)
        self.assertEqual(7, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM2,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(8, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(11, self.wm_cb_info.wm_cb_count)
        self.assertEqual((0, 1), workq.watermark_indices())

        # enqueue 1 entry
        # verify that we did not hit any new high watermark
        self.enqueue_entries(workq, 1)
        self.assertEqual(8, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM2,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(8, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(11, self.wm_cb_info.wm_cb_count)
        self.assertEqual((0, 1), workq.watermark_indices())

        # dequeue 7 entries
        # verify that we hit LWM3
        self.dequeue_entries(workq, 7)
        self.assertEqual(1, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM3,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(2, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(12, self.wm_cb_info.wm_cb_count)
        self.assertEqual((-1, 0), workq.watermark_indices())

        # dequeue 1 entry
        # verify that we did not hit any new low watermark
        self.dequeue_entries(workq, 1)
        self.assertEqual(0, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM3,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(2, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(12, self.wm_cb_info.wm_cb_count)
        self.assertEqual((-1, 0), workq.watermark_indices())

        # enqueue 5 entries
        # verify that we hit HWM1
        self.enqueue_entries(workq, 5)
        self.assertEqual(5, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM1,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(5, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(13, self.wm_cb_info.wm_cb_count)
        self.assertEqual((0, 1), workq.watermark_indices())

        # dequeue 5 entries
        # verify that we hit LWM3
        self.dequeue_entries(workq, 5)
        self.assertEqual(0, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM3,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(2, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(14, self.wm_cb_info.wm_cb_count)
        self.assertEqual((-1, 0), workq.watermark_indices())

        # enqueue 5 entries
        # verify that we hit HWM1
        self.enqueue_entries(workq, 5)
        self.assertEqual(5, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM1,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(5, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(15, self.wm_cb_info.wm_cb_count)
        self.assertEqual((0, 1), workq.watermark_indices())

        # dequeue 2 entries
        # verify that we did not hit any new low watermark
        self.dequeue_entries(workq, 2)
        self.assertEqual(3, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM1,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(5, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(15, self.wm_cb_info.wm_cb_count)
        self.assertEqual((0, 1), workq.watermark_indices())

        # enqueue 1 entry
        # verify that we did not hit any new high watermark
        self.enqueue_entries(workq, 1)
        self.assertEqual(4, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.HWM1,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(5, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(15, self.wm_cb_info.wm_cb_count)
        self.assertEqual((0, 1), workq.watermark_indices())

        # dequeue 3 entries
        # verify that we hit LWM3
        self.dequeue_entries(workq, 3)
        self.assertEqual(1, workq.size())
        self.assertEqual(WaterMarkCbInfo.cb_type.LWM3,
                         self.wm_cb_info.wm_cb_type)
        self.assertEqual(2, self.wm_cb_info.wm_cb_qsize)
        self.assertEqual(16, self.wm_cb_info.wm_cb_count)
        self.assertEqual((-1, 0), workq.watermark_indices())
    # end test_watermarks

# end class WorkQueueTest


if __name__ == '__main__':
    unittest.main(verbosity=2, catchbreak=True)
