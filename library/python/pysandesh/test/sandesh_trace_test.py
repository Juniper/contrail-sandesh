#!/usr/bin/env python

#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_trace_test
#

import sys
import socket
import unittest
import test_utils

sys.path.insert(1, sys.path[0]+'/../../../python')
from pysandesh.sandesh_base import *
from gen_py.msg_test.ttypes import *

class SandeshTraceTest(unittest.TestCase):

    def setUp(self):
        self._sandesh = Sandesh()
        http_port = test_utils.get_free_port()
        self._sandesh.init_generator('sandesh_trace_test', socket.gethostname(),
            [('127.0.0.1', 8086)], 'trace_test_ctxt', http_port)
        self._trace_read_list = []
    #end setUp

    def sandesh_trace_read_handler(self, trace_msg, more):
        self._trace_read_list.append(trace_msg)
    #end sandesh_trace_read_handler

    def test_create_delete_trace_buffer(self):
        trace_buf_name = 'test_create_delete_trace_buffer'
        trace_buf_size = 5
        self._sandesh.trace_buffer_create(trace_buf_name, trace_buf_size)
        self.assertTrue(trace_buf_name in self._sandesh.trace_buffer_list_get())
        self.assertEqual(trace_buf_size, self._sandesh.trace_buffer_size_get(trace_buf_name))
        
        # Read from empty trace buffer
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='test',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = []
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._sandesh.trace_buffer_delete(trace_buf_name)
        self.assertFalse(trace_buf_name in self._sandesh.trace_buffer_list_get())

        # Read deleted trace buffer
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='test',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = []
        self.assertEqual(exp_trace_list, self._trace_read_list)
    #end test_create_delete_trace_buffer

    def test_enable_disable_trace_buffer(self):
        trace_buf_name = 'test_enable_disable_trace_buffer'
        trace_buf_size = 5
        # Create trace buffer in disabled state
        self._sandesh.trace_buffer_create(trace_buf_name, trace_buf_size, False)
        tmsg1 = TraceTest(magicNo=1234, sandesh=self._sandesh)
        tmsg1.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read1',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        # Trace buffer should be empty
        exp_trace_list = []
        self.assertEqual(exp_trace_list, self._trace_read_list)
       
        # Enable trace buffer
        self._sandesh.trace_buffer_enable(trace_buf_name)
        tmsg1.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        tmsg2 = TraceTest(magicNo=3456, sandesh=self._sandesh)
        tmsg2.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read2',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg1, tmsg2]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []
        
        # Disable trace buffer
        self._sandesh.trace_buffer_disable(trace_buf_name)
        tmsg3 = TraceTest(magicNo=7890, sandesh=self._sandesh)
        tmsg3.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read3',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg1, tmsg2]
        self.assertEqual(exp_trace_list, self._trace_read_list)
    #end test_enable_disable_trace_buffer

    def test_enable_disable_trace(self):
        trace_buf_name = 'test_enable_disable_trace'
        trace_buf_size = 3
        # Disable trace
        self._sandesh.trace_disable()
        self._sandesh.trace_buffer_create(trace_buf_name, trace_buf_size)
        tmsg1 = TraceTest(magicNo=1234, sandesh=self._sandesh)
        tmsg1.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read1',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        # Trace buffer should be empty
        exp_trace_list = []
        self.assertEqual(exp_trace_list, self._trace_read_list)
        
        # Enable trace
        self._sandesh.trace_enable()
        tmsg1.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read2',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg1]
        self.assertEqual(exp_trace_list, self._trace_read_list)
    #end test_enable_disable_trace

    def test_read_count_trace_buffer(self):
        trace_buf_name = 'test_read_count_trace_buffer'
        trace_buf_size = 10
        self._sandesh.trace_buffer_create(trace_buf_name, trace_buf_size)
        tmsg1 = TraceTest(magicNo=1, sandesh=self._sandesh)
        tmsg1.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        tmsg2 = TraceTest(magicNo=2, sandesh=self._sandesh)
        tmsg2.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        tmsg3 = TraceTest(magicNo=3, sandesh=self._sandesh)
        tmsg3.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        
        # Total messages in trace buffer = 3, count = 1
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read1',
                                        count=1, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg1]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []
        
        # count = 0, should read the last two messages
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read1',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg2, tmsg3]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []
        
        # Total messages in trace buffer = 3, count = 5 (< trace_buf_size)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read2',
                                        count=5, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg1, tmsg2, tmsg3]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []

        # Total messages in trace buffer = 3, count = 20 (> trace_buf_size)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read3',
                                        count=20, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg1, tmsg2, tmsg3]
        self.assertEqual(exp_trace_list, self._trace_read_list)
    #end test_read_count_trace_buffer

    def test_overwrite_trace_buffer(self):
        trace_buf_name = 'test_overwrite_trace_buffer'
        trace_buf_size = 3
        self._sandesh.trace_buffer_create(trace_buf_name, trace_buf_size)
        tmsg1 = TraceTest(magicNo=123, sandesh=self._sandesh)
        tmsg1.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        tmsg2 = TraceTest(magicNo=345, sandesh=self._sandesh)
        tmsg2.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        tmsg3 = TraceTest(magicNo=567, sandesh=self._sandesh)
        tmsg3.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read1',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg1, tmsg2, tmsg3]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []
        # Overwrite trace buffer
        tmsg4 = TraceTest(magicNo=789, sandesh=self._sandesh)
        tmsg4.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read2',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg2, tmsg3, tmsg4]
        self.assertEqual(exp_trace_list, self._trace_read_list)
    #end test_overwrite_trace_buffer

    def test_trace_buffer_read_context(self):
        trace_buf_name = 'test_trace_buffer_read_context'
        trace_buf_size = 3
        self._sandesh.trace_buffer_create(trace_buf_name, trace_buf_size)
        tmsg1 = TraceTest(magicNo=123, sandesh=self._sandesh)
        tmsg1.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        tmsg2 = TraceTest(magicNo=345, sandesh=self._sandesh)
        tmsg2.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read1',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg1, tmsg2]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []
        
        # After reading the entire content of trace buffer, 
        # dont delete the read context. Subsequent call to trace_buffer_read()
        # should not read any trace message
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read1',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = []
        self.assertEqual(exp_trace_list, self._trace_read_list)
        
        # We have not deleted the read context. Add more trace messages
        # and make sure we don't read the already read trace messages.
        tmsg3 = TraceTest(magicNo=56, sandesh=self._sandesh)
        tmsg3.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        tmsg4 = TraceTest(magicNo=67, sandesh=self._sandesh)
        tmsg4.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read1',
                                        count=1, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg3]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []
        # Now read the last message
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read1',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg4]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []
        
        # Read the trace buffer with different read_context
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read2',
                                        count=2, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg2, tmsg3]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []

        # Delete read context
        self._sandesh.trace_buffer_read_done(name=trace_buf_name, context='read2')
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read2',
                                        count=1, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg2]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []

        # Interleave reading and writing of trace buffer - invalidate read context
        tmsg5 = TraceTest(magicNo=78, sandesh=self._sandesh)
        tmsg5.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        tmsg6 = TraceTest(magicNo=89, sandesh=self._sandesh)
        tmsg6.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read2',
                                        count=2, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg4, tmsg5]
        self.assertEqual(exp_trace_list, self._trace_read_list)
        self._trace_read_list = []
        tmsg7 = TraceTest(magicNo=98, sandesh=self._sandesh) 
        tmsg7.trace_msg(name=trace_buf_name, sandesh=self._sandesh)
        self._sandesh.trace_buffer_read(name=trace_buf_name, read_context='read1',
                                        count=0, read_cb=self.sandesh_trace_read_handler)
        exp_trace_list = [tmsg5, tmsg6, tmsg7]
        self.assertEqual(exp_trace_list, self._trace_read_list)
    #end test_trace_buffer_read_context

#end SandeshTraceTest

if __name__ == '__main__':
    unittest.main()
