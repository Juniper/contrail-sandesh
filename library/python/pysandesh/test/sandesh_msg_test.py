#!/usr/bin/env python

#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_msg_test
#

import unittest
import sys
import os
import socket
import test_utils
import time
import uuid
from itertools import chain

sys.path.insert(1, sys.path[0]+'/../../../python')

from pysandesh.sandesh_base import *
from pysandesh.sandesh_client import *
from pysandesh.sandesh_session import *
from gen_py.msg_test.ttypes import *

class SandeshMsgTestHelper(SandeshSession):

    def __init__(self):
        SandeshSession.__init__(self, sandesh_global, ('127.0.0.1', 8086), None, None)
        self.write_buf = None
    # end __init__

    def write(self, send_buf):
        self.write_buf = send_buf
        return len(send_buf)
    # end write

# end SandeshMsgTestHelper

class SandeshMsgTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        http_port = test_utils.get_free_port()
        sandesh_global.init_generator('sandesh_msg_test', socket.gethostname(), 
                'Test', 'Test', None, 'sandesh_msg_test_ctxt', http_port,
                connect_to_collector=False)

    def setUp(self):
        self.setUpClass()
        self.assertTrue(sandesh_global.client() is None)
        self._session = SandeshMsgTestHelper()
        self._writer = SandeshWriter(session=self._session)
        self._reader = SandeshReader(self._session, self.sandesh_read_handler)
    # end setUp

    def sandesh_read_handler(self, session, sandesh_xml):
        (hdr, hdr_len, sandesh_name) = SandeshReader.extract_sandesh_header(sandesh_xml)
        self.assertEqual(self._expected_type, hdr.Type)
        self.assertEqual(self._expected_hints, hdr.Hints)
    # end sandesh_read_handler

    def test_systemlog_msg(self):
        systemlog_msg = SystemLogTest()
        self._expected_type = SandeshType.SYSTEM
        self._expected_hints = 0
        self.assertNotEqual(-1, self._writer.send_msg(systemlog_msg, False))
        self.assertNotEqual(-1, self._reader.read_msg(self._session.write_buf))
    # end test_systemlog_msg

    def test_objectlog_msg(self):
        objectlog_msg = ObjectLogTest(StructObject())
        self._expected_type = SandeshType.OBJECT
        self._expected_hints = 0
        self.assertNotEqual(-1, self._writer.send_msg(objectlog_msg, False))
        self.assertNotEqual(-1, self._reader.read_msg(self._session.write_buf))
    # end test_objectlog_msg

    def test_objectlog_optional_msg(self):
        objectlog_msg = ObjectLogTest(StructObject(), None, 1)
        self._expected_type = SandeshType.OBJECT
        self._expected_hints = 0
        self.assertNotEqual(-1, self._writer.send_msg(objectlog_msg, False))
        self.assertNotEqual(-1, self._reader.read_msg(self._session.write_buf))
    # end test_objectlog_optional_msg

    def test_objectlog_msg_key_hint(self):
        objectlog_msg = ObjectLogAnnTest(StructKeyHint("VM1"))
        self._expected_type = SandeshType.OBJECT
        self._expected_hints = SANDESH_KEY_HINT
        self.assertNotEqual(-1, self._writer.send_msg(objectlog_msg, False))
        self.assertNotEqual(-1, self._reader.read_msg(self._session.write_buf))
    # end test_objectlog_msg_key_hint

    def test_systemlog_msg_buffer_threshold(self):
        systemlog_msg = SystemLogTest()
        self._expected_type = SandeshType.SYSTEM
        self._expected_hints = 0
        SandeshSystem.set_sandesh_send_rate_limit(10)
        time.sleep(1)
        for i in xrange(0,15):
            systemlog_msg.send(sandesh=sandesh_global)
        self.assertEqual(5,sandesh_global.msg_stats(). \
             message_type_stats()['SystemLogTest']. \
             messages_sent_dropped_rate_limited)
    # end test_systemlog_msg_buffer_threshold

    def test_sandesh_queue_level_drop(self):
        levels = list(range(SandeshLevel.SYS_EMERG,SandeshLevel.SYS_DEBUG))
        queue_level_drop = 0
        for send_level in levels:
            sandesh_global.set_send_level(None, send_level)
            for sandesh_level in levels:
                systemlog = SystemLogTest(level=sandesh_level)
                systemlog.send()
                if sandesh_level >= send_level:
                    queue_level_drop += 1
                self.assertEqual(queue_level_drop, sandesh_global.msg_stats().\
                    aggregate_stats().messages_sent_dropped_queue_level)
    # end test_sandesh_queue_level_drop

    def test_sandesh_sizeof(self):
        # test sizeof SystemLogTest sandesh without setting any members
        system_log = SystemLogTest()
        exp_system_log_size = sys.getsizeof('Async Test')
        self.assertEqual(exp_system_log_size, system_log.__sizeof__())

        # test sizeof SystemLogTest by setting list1 and uuid1
        l1 = [10, 10934, 34]
        uuid1 = uuid.UUID('{00010203-0405-0607-0809-0a0b0c0d0e0f}')
        system_log = SystemLogTest(list1=l1, uuid1=uuid1)
        # verify that the size of individual elements in the list is
        # accounted while calclulating the size of a list
        exp_system_log_size += sys.getsizeof(l1)
        exp_system_log_size += sum(map(sys.getsizeof, l1))
        exp_system_log_size += sys.getsizeof(uuid1)
        self.assertEqual(exp_system_log_size, system_log.__sizeof__())

        # test sizeof SystemLogTest by setting list1, uuid1 and map1
        m1 = {1: 'collector', 2: 'query-engine', 3: 'opserver'}
        system_log = SystemLogTest(list1=l1, map1=m1, uuid1=uuid1)
        # verify that the size of individual elements (key, value)
        # in the map is accounted which calculating the size of a map
        exp_system_log_size += sys.getsizeof(m1)
        exp_system_log_size += \
            sum(map(sys.getsizeof, chain.from_iterable(m1.iteritems())))
        self.assertEqual(exp_system_log_size, system_log.__sizeof__())

        # test the size of struct
        exp_struct_obj_size = 0
        f1 = False
        lxml1 = '<sandesh>EOM</sandesh>'
        lxml2 = '<test>sizeof</test>'
        l1 = [lxml1, lxml2]
        struct_obj1 = StructObject(flag1=f1, list1=l1)
        exp_struct_obj_size += sys.getsizeof('Object')
        exp_struct_obj_size += sys.getsizeof(f1)
        exp_struct_obj_size += sys.getsizeof(lxml1)
        exp_struct_obj_size += sys.getsizeof(lxml2)
        exp_struct_obj_size += sys.getsizeof(l1)
        self.assertEqual(exp_struct_obj_size, struct_obj1.__sizeof__())

        # test the size of ObjectLogTest
        objlog = ObjectLogTest(object=struct_obj1)
        exp_objlog_size = sys.getsizeof(struct_obj1)
        self.assertEqual(exp_objlog_size, objlog.__sizeof__())

        # modify objlog and verify the size
        struct_obj2 = StructObject(s1='contrail-collector')
        list_obj = [struct_obj1, struct_obj2]
        objlog.object = None
        objlog.list_obj1 = list_obj
        exp_objlog_size = sys.getsizeof(list_obj)
        exp_objlog_size += sys.getsizeof(struct_obj1)
        exp_objlog_size += sys.getsizeof(struct_obj2)
        self.assertEqual(exp_objlog_size, objlog.__sizeof__())
    # end test_sandesh_sizeof

# end class SandeshMsgTest

if __name__ == '__main__':
    unittest.main(verbosity=2, catchbreak=True)
