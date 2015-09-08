#!/usr/bin/env python

#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_session_test
#

import unittest
import cStringIO
import sys
import os
import socket
import test_utils

sys.path.insert(1, sys.path[0]+'/../../../python')

from pysandesh import sandesh_session
from pysandesh.sandesh_session import *
from pysandesh.sandesh_base import *
from pysandesh.test.gen_py.msg_test.ttypes import *

sandesh_test_started = False

def create_fake_sandesh(size):
    min_sandesh_len = len(sandesh_session._XML_SANDESH_OPEN) + \
        len(sandesh_session._XML_SANDESH_CLOSE)
    assert size > min_sandesh_len, \
        'Sandesh message length should be more than %s' % (min_sandesh_len)
    len_width = len(sandesh_session._XML_SANDESH_OPEN) - \
        (len(sandesh_session._XML_SANDESH_OPEN_ATTR_LEN) + len(sandesh_session._XML_SANDESH_OPEN_END))
    sandesh = sandesh_session._XML_SANDESH_OPEN_ATTR_LEN + (str(size)).zfill(len_width) + \
        sandesh_session._XML_SANDESH_OPEN_END + ''.zfill(size - min_sandesh_len) + \
        sandesh_session._XML_SANDESH_CLOSE
    return sandesh
#end create_fake_sandesh

class SandeshSessionTestHelper(sandesh_session.SandeshSession):

    def __init__(self):
        sandesh_session.SandeshSession.__init__(self, sandesh_global, ('127.0.0.1', 8086), None, None)
        self.send_buf_list = []
    #end __init__

    def write(self, send_buf):
        self.send_buf_list.append(send_buf)
        print 'Send message of length [%s]' % (len(send_buf))
        return len(send_buf)
    #end write

#end class SandeshSessionTestHelper

class SandeshReaderTestHelper(sandesh_session.SandeshReader):
    def __init__(self, session):
        sandesh_session.SandeshReader.__init__(self, session, self._sandesh_msg_handler)
        self.sandesh_msg_size_list = []
    #end __init__

    def _sandesh_msg_handler(self, session, sandesh):
        msg_len = len(sandesh_session._XML_SANDESH_OPEN) + len(sandesh) + \
            len(sandesh_session._XML_SANDESH_CLOSE)
        self.sandesh_msg_size_list.append(msg_len)
        print 'Received message of length [%s]' % (msg_len)
    #end _sandesh_msg_handler

#end class SandeshReaderTestHelper

class SandeshReaderTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        global sandesh_test_started
        if (not sandesh_test_started):
            http_port = test_utils.get_free_port()
            sandesh_global.init_generator('sandesh_session_test', 
                socket.gethostname(), 'Test', 'Test', None,  
                'sandesh_msg_test_ctxt', http_port)
            sandesh_test_started = True

    def setUp(self):
        self.setUpClass()
        self._session = SandeshSessionTestHelper()
        self._sandesh_reader = SandeshReaderTestHelper(self._session)
    #end setUp

    def test_read_msg(self):
        print '-------------------------'
        print '       Test Read Msg     '
        print '-------------------------'
        msg_stream = cStringIO.StringIO()
        msg_size_list = [100, 400, 80, 110, 70, 80, 100]
        msg_segment_list = [
            100 + 60,      # complete msg + start (with header)
            200,           # mid part
            140 + 80 + 10, # end + full msg + start (no header)
            7,             # still no header
            50,            # header but not end
            43,            # end of message
            70,            # complete msg
            80 + 101       # two complete messages
        ]

        for size in msg_size_list:
            sandesh = create_fake_sandesh(size)
            msg_stream.write(sandesh)
        stream = msg_stream.getvalue()
        msg_stream.close()

        for segment in msg_segment_list:
            self._sandesh_reader.read_msg(stream[:segment])
            stream = stream[segment:]

        self.assertEqual(len(msg_size_list), \
            len(self._sandesh_reader.sandesh_msg_size_list))
        for i in range(len(msg_size_list)):
            self.assertEqual(msg_size_list[i], self._sandesh_reader.sandesh_msg_size_list[i])
    #end test_read_msg

#end class SandeshReaderTest

class SandeshWriterTestHelper(sandesh_session.SandeshWriter):
    def __init__(self, session):
        sandesh_session.SandeshWriter.__init__(self, session)
    #end __init__

    def send_sandesh(self, sandesh, more=True):
        if more:
            self._send_msg_more(sandesh)
        else:
            self._send_msg_all(sandesh)
    #end send_sandesh

    def get_send_buf_list(self):
        return self._session.send_buf_list
    #end get_send_buf_list

#end class SandeshWriterTestHelper

class SandeshWriterTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        global sandesh_test_started
        if (not sandesh_test_started):
            http_port = test_utils.get_free_port()
            sandesh_global.init_generator('sandesh_session_test', 
                socket.gethostname(), None, 
                'sandesh_msg_test_ctxt', http_port)
            sandesh_test_started = True

    def setUp(self):
        self._session = SandeshSessionTestHelper()
        self._sandesh_writer = SandeshWriterTestHelper(self._session)
        self._max_msg_size = sandesh_session.SandeshWriter._MAX_SEND_BUF_SIZE
    #end setUp

    def test_send_msg_more(self):
        print '-------------------------'
        print '    Test Send Msg More   '
        print '-------------------------'
        msg_info_list = [
            dict(msg=create_fake_sandesh(self._max_msg_size)),
            dict(msg=create_fake_sandesh(self._max_msg_size/2)),
            dict(msg=create_fake_sandesh(self._max_msg_size/4)),
            dict(msg=create_fake_sandesh((self._max_msg_size/4)+1)),
            dict(msg=create_fake_sandesh(self._max_msg_size-1)),
            dict(msg=create_fake_sandesh(2*self._max_msg_size)),
        ]

        # Fill the send_count and the send_buf_cache_size expected @
        # the end of each iteration
        exp_at_each_it = [
            dict(send_count=1, send_buf_cache_size=0),
            dict(send_count=1, send_buf_cache_size=self._max_msg_size/2),
            dict(send_count=1, send_buf_cache_size=3*(self._max_msg_size/4)),
            dict(send_count=2, send_buf_cache_size=0),
            dict(send_count=2, send_buf_cache_size=self._max_msg_size-1),
            dict(send_count=3, send_buf_cache_size=0)
        ]
        self.assertEqual(len(msg_info_list), len(exp_at_each_it))

        # Fill the send_buf expected @ the end of the test
        exp_at_end = [
            dict(msg=msg_info_list[0]['msg']),
            dict(msg=msg_info_list[1]['msg'] + msg_info_list[2]['msg'] + msg_info_list[3]['msg']),
            dict(msg=msg_info_list[4]['msg'] + msg_info_list[5]['msg'])
        ]
        self.assertEqual(exp_at_each_it[len(exp_at_each_it)-1]['send_count'], len(exp_at_end))

        for i in range(len(msg_info_list)):
            self._sandesh_writer.send_sandesh(msg_info_list[i]['msg'])
            self.assertEqual(exp_at_each_it[i]['send_count'],
                len(self._sandesh_writer.get_send_buf_list()))
            self.assertEqual(exp_at_each_it[i]['send_buf_cache_size'],
                len(self._sandesh_writer._send_buf_cache))

        send_buf_list = self._sandesh_writer.get_send_buf_list()
        for i in range(len(send_buf_list)):
            self.assertEqual(exp_at_end[i]['msg'], send_buf_list[i])
    #end test_send_more

    def test_send_msg_all(self):
        print '-------------------------'
        print '    Test Send Msg All    '
        print '-------------------------'
        msg_info_list = [
            dict(msg=create_fake_sandesh(self._max_msg_size), send_all=True),
            dict(msg=create_fake_sandesh(self._max_msg_size/2), send_all=False),
            dict(msg=create_fake_sandesh(self._max_msg_size/4), send_all=True),
            dict(msg=create_fake_sandesh(self._max_msg_size/2), send_all=False),
            dict(msg=create_fake_sandesh(self._max_msg_size/2), send_all=True),
            dict(msg=create_fake_sandesh((self._max_msg_size/2)+1), send_all=False),
            dict(msg=create_fake_sandesh(self._max_msg_size/2), send_all=True)
        ]

        # Fill the send_count and the send_buf_cache_size expected @
        # the end of each iteration
        exp_at_each_it = [
            dict(send_count=1, send_buf_cache_size=0),
            dict(send_count=1, send_buf_cache_size=self._max_msg_size/2),
            dict(send_count=2, send_buf_cache_size=0),
            dict(send_count=2, send_buf_cache_size=self._max_msg_size/2),
            dict(send_count=3, send_buf_cache_size=0),
            dict(send_count=3, send_buf_cache_size=(self._max_msg_size/2)+1),
            dict(send_count=4, send_buf_cache_size=0)
        ]
        self.assertEqual(len(msg_info_list), len(exp_at_each_it))

        # Fill the send_buf expected @ the end of the test
        exp_at_end = [
            dict(msg=msg_info_list[0]['msg']),
            dict(msg=msg_info_list[1]['msg'] + msg_info_list[2]['msg']),
            dict(msg=msg_info_list[3]['msg'] + msg_info_list[4]['msg']),
            dict(msg=msg_info_list[5]['msg'] + msg_info_list[6]['msg']),
        ]
        self.assertEqual(exp_at_each_it[len(exp_at_each_it)-1]['send_count'], len(exp_at_end))

        for i in range(len(msg_info_list)):
            if True == msg_info_list[i]['send_all']:
                self._sandesh_writer.send_sandesh(msg_info_list[i]['msg'], False)
            else:
                self._sandesh_writer.send_sandesh(msg_info_list[i]['msg'], True)
            self.assertEqual(exp_at_each_it[i]['send_count'],
                len(self._sandesh_writer.get_send_buf_list()))
            self.assertEqual(exp_at_each_it[i]['send_buf_cache_size'],
                len(self._sandesh_writer._send_buf_cache))

        send_buf_list = self._sandesh_writer.get_send_buf_list()
        for i in range(len(send_buf_list)):
            self.assertEqual(exp_at_end[i]['msg'], send_buf_list[i])
    #end test_send_msg_all

#end class SandeshWriterTest


class SandeshSendQueueTest(unittest.TestCase):

    def setUp(self):
        self.maxDiff = None
        self.sandesh_sendq = SandeshSendQueue(self.sandesh_queue_callback)
    # end setUp

    def tearDown(self):
        pass
    # end tearDown

    def sandesh_queue_callback(self):
        pass
    # end sandesh_queue_callback

    def test_sandesh_send_queue_size(self):
        # enqueue sandesh and verify that the queue size is incremented
        # by sandesh size
        systemlog = SystemLogTest()
        self.sandesh_sendq.enqueue(SandeshSendQueue.Element(systemlog))
        expected_qsize = sys.getsizeof(systemlog)
        self.assertEqual(expected_qsize, self.sandesh_sendq.size())

        # enqueue one more entry
        objectlog = ObjectLogTest(StructObject())
        self.sandesh_sendq.enqueue(SandeshSendQueue.Element(objectlog))
        expected_qsize += sys.getsizeof(objectlog)
        self.assertEqual(expected_qsize, self.sandesh_sendq.size())

        # dequeue sandesh and verify that the queue size is decremented
        # by sandesh size
        self.sandesh_sendq.dequeue()
        expected_qsize -= sys.getsizeof(systemlog)
        self.assertEqual(expected_qsize, self.sandesh_sendq.size())

        # dequeue last entry
        self.sandesh_sendq.dequeue()
        expected_qsize = 0
        self.assertEqual(expected_qsize, self.sandesh_sendq.size())
    # end test_sandesh_send_queue_size

# end class SandeshSendQueueTest


class SandeshSessionTest(unittest.TestCase):

    def setUp(self):
        self.sandesh_instance = Sandesh()
        self.sandesh_instance.init_generator('SandeshSessionTest', 'localhost',
            'UT', 0, None, 'context', -1, connect_to_collector=False)
    # end setUp

    def tearDown(self):
        pass
    # end tearDown

    def verify_watermarks(self, expected_wms, actual_wms):
        expected_wms.sort()
        actual_wms.sort()
        print '== verify watermarks =='
        print expected_wms
        self.assertEqual(len(expected_wms), len(actual_wms))
        for i in range(len(expected_wms)):
            self.assertEqual(expected_wms[i][0], actual_wms[i].size)
            # Invoke the watermark callback and verify that the
            # send_level is set correctly
            actual_wms[i].callback(expected_wms[i][0])
            self.assertEqual(expected_wms[i][1],
                             self.sandesh_instance.send_level())
    # end verify_watermarks

    def test_send_queue_watermarks(self):
        session = SandeshSession(self.sandesh_instance, None, None, None)
        wms = SandeshSendQueue._SENDQ_WATERMARKS
        # verify high watermarks are set properly in sandesh send queue
        high_wms = [wm for wm in wms if wm[2] is True]
        sendq_hwms = session.send_queue().high_watermarks()
        self.verify_watermarks(high_wms, sendq_hwms)

        # verify low watermarks are set properly in sandesh send queue
        low_wms = [wm for wm in wms if wm[2] is False]
        sendq_lwms = session.send_queue().low_watermarks()
        self.verify_watermarks(low_wms, sendq_lwms)
    # end test_send_queue_watermarks

# end class SandeshSessionTest


if __name__ == '__main__':
    unittest.main(verbosity=2, catchbreak=True)
