#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# Work Queue
#

import gevent
from gevent.queue import Queue, Empty

class Runner(object):
    def __init__(self, work_queue, max_work_load):
        self._work_q = work_queue
        self._max_work_load = max_work_load
        self._running = False
    #end __init__

    def start(self):
        if not self._running:
            self._running = True
            gevent.spawn(self._do_work)
    #end start

    def _do_work(self):
        while True:
            work_load = self._max_work_load
            work_item = self._work_q.dequeue()
            while work_item:
                self._work_q.worker(work_item)
                work_load -= 1
                if work_load == 0:
                    break
                work_item = self._work_q.dequeue()
                
            if self._work_q.runner_done():
                self._running = False
                break
            else:
                # yield
                gevent.sleep()
    #end _do_work

#end class Runner

class WorkQueue(object):
    def __init__(self, worker, start_runner=None, max_work_load=16):
        if not worker:
            # TODO raise exception
            pass
        self.worker = worker
        self._start_runner = start_runner
        self._max_work_load = max_work_load
        # do we want to limit the size of the queue?
        self._queue = Queue()
        self._runner = Runner(self, self._max_work_load)
    #end __init__

    def enqueue(self, work_item):
        self._queue.put(work_item)
        self.may_be_start_runner()
    #end enqueue

    def dequeue(self):
        try:
            work_item = self._queue.get_nowait()
        except Empty:
            work_item = None
        return work_item
    #end dequeue

    def may_be_start_runner(self):
        if self._queue.empty() or \
           (self._start_runner and not self._start_runner()):
            return
        self._runner.start()
    #end may_be_start_runner

    def runner_done(self):
        if self._queue.empty() or \
           (self._start_runner and not self._start_runner()):
            return True
        return False
    #end runner_done

    def is_queue_empty(self):
        if self._queue.empty():
            return True
        return False

#end class WorkQueue
