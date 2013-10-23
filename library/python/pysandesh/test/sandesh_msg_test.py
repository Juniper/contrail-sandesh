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

sys.path.insert(1, sys.path[0]+'/../../../python')

from pysandesh.sandesh_base import *
from pysandesh.sandesh_client import *
from pysandesh.sandesh_session import *
from gen_py.msg_test.ttypes import *

class SandeshMsgTestHelper(SandeshSession):

    def __init__(self):
        SandeshSession.__init__(self, sandesh_global, ('127.0.0.1', 8086), None, None)
        self.write_buf = None
    #end __init__

    def write(self, send_buf):
        self.write_buf = send_buf
        return len(send_buf)
    #end write

#end SandeshMsgTestHelper

class SandeshMsgTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        http_port = test_utils.get_free_port()
        sandesh_global.init_generator('sandesh_msg_test', socket.gethostname(), 
                [('127.0.0.1', 8086)], 'sandesh_msg_test_ctxt', http_port)

    def setUp(self):
        self.setUpClass()
        self._session = SandeshMsgTestHelper()
        self._writer = SandeshWriter(session=self._session)
        self._reader = SandeshReader(self._session, self.sandesh_read_handler)
    #end setUp

    def sandesh_read_handler(self, session, sandesh_xml):
        (hdr, hdr_len, sandesh_name) = SandeshReader.extract_sandesh_header(sandesh_xml)
        self.assertEqual(self._expected_type, hdr.Type)
        self.assertEqual(self._expected_hints, hdr.Hints)
    #end sandesh_read_handler

    def test_systemlog_msg(self):
        print '-------------------------'
        print '  Test SystemLog Msg     '
        print '-------------------------'
        systemlog_msg = SystemLogTest()
        self._expected_type = SandeshType.SYSTEM
        self._expected_hints = 0
        self.assertNotEqual(-1, self._writer.send_msg(systemlog_msg, False))
        self.assertNotEqual(-1, self._reader.read_msg(self._session.write_buf))
    #end test_systemlog_msg

    def test_objectlog_msg(self):
        print '------------------------------'
        print '  Test ObjectLog Msg          '
        print '------------------------------'
        objectlog_msg = ObjectLogTest(StructObject())
        self._expected_type = SandeshType.OBJECT
        self._expected_hints = 0
        self.assertNotEqual(-1, self._writer.send_msg(objectlog_msg, False))
        self.assertNotEqual(-1, self._reader.read_msg(self._session.write_buf))
    #end test_objectlog_msg

    def test_objectlog_msg_key_hint(self):
        print '------------------------------'
        print ' Test ObjectLog Msg Key Hint  '
        print '------------------------------'
        objectlog_msg = ObjectLogAnnTest(StructKeyHint("VM1"))
        self._expected_type = SandeshType.OBJECT
        self._expected_hints = SANDESH_KEY_HINT
        self.assertNotEqual(-1, self._writer.send_msg(objectlog_msg, False))
        self.assertNotEqual(-1, self._reader.read_msg(self._session.write_buf))
    #end test_objectlog_msg_key_hint

#end class SandeshMsgTest

if __name__ == '__main__':
    unittest.main()
