#!/usr/bin/env python

#
# Copyright (c) 2014 Juniper Networks, Inc. All rights reserved.
#

#
# conn_info_test
#

import sys
import socket
import unittest
import test_utils

sys.path.insert(1, sys.path[0]+'/../../../python')
from pysandesh.gen_py.process_info.constants import ConnectionTypeNames, \
    ConnectionStatusNames, ProcessStateNames
from pysandesh.gen_py.process_info.ttypes import ConnectionInfo, \
    ProcessStatus, ProcessState, ConnectionType, ConnectionStatus
from pysandesh.connection_info import ConnectionState
from gen_py.nodeinfo.ttypes import NodeStatusUVE, NodeStatus
from pysandesh.sandesh_base import Sandesh

class ConnInfoTest(unittest.TestCase):

    def setUp(self):
        self.maxDiff = None
        self._sandesh = Sandesh()
        http_port = test_utils.get_free_port()
        self._sandesh.init_generator('conn_info_test', socket.gethostname(),
            'Test', 'Test', None, 'conn_info_test_ctxt', http_port)
    #end setUp

    def _check_conn_status_cb(self, vcinfos):
        self._expected_vcinfos.sort()
        vcinfos.sort()
        self.assertEqual(self._expected_vcinfos, vcinfos)
        return (ProcessState.FUNCTIONAL, '')
    #end _check_conn_status_cb

    def _update_conn_info(self, name, status, description, vcinfos):
        cinfo = ConnectionInfo()
        cinfo.name = name
        cinfo.status = ConnectionStatusNames[status]
        cinfo.description = description
        cinfo.type = ConnectionTypeNames[ConnectionType.TEST]
        cinfo.server_addrs = ['127.0.0.1:0']
        vcinfos.append(cinfo)
    #end _update_conn_info

    def _update_conn_state(self, name, status, description, vcinfos):
        self._expected_vcinfos = vcinfos
        ConnectionState.update(ConnectionType.TEST, name, status,
            ['127.0.0.1:0'], description)
    #end _update_conn_state

    def _delete_conn_info(self, name, vcinfos):
        return [cinfo for cinfo in vcinfos if cinfo.name != name]
    #end _delete_conn_info

    def _delete_conn_state(self, name, vcinfos):
        self._expected_vcinfos = vcinfos
        ConnectionState.delete(ConnectionType.TEST, name)
    #end _delete_conn_state

    def test_basic(self):
        ConnectionState.init(sandesh = self._sandesh, hostname = "TestHost",
            module_id = "TestModule", instance_id = "0",
            conn_status_cb = self._check_conn_status_cb,
            uve_type_cls = NodeStatusUVE,
            uve_data_type_cls = NodeStatus)
        vcinfos = []
        self._update_conn_info("Test1", ConnectionStatus.UP, "Test1 UP",
            vcinfos)
        self._update_conn_state("Test1", ConnectionStatus.UP, "Test1 UP",
            vcinfos)
        self._update_conn_info("Test2", ConnectionStatus.UP, "Test2 UP",
            vcinfos)
        self._update_conn_state("Test2", ConnectionStatus.UP, "Test2 UP",
            vcinfos)
        vcinfos = self._delete_conn_info("Test2", vcinfos)
        self._delete_conn_state("Test2", vcinfos)
    #end test_basic

    def test_callback(self):
        vcinfos = []
        self._update_conn_info("Test1", ConnectionStatus.UP, "Test1 UP",
            vcinfos);
        (pstate, message) = ConnectionState.get_conn_state_cb(vcinfos)
        self.assertEqual(ProcessState.FUNCTIONAL, pstate)
        self.assertEqual('', message)
        self._update_conn_info("Test2", ConnectionStatus.DOWN, "Test2 DOWN",
            vcinfos);
        (pstate, message) = ConnectionState.get_conn_state_cb(vcinfos)
        self.assertEqual(ProcessState.NON_FUNCTIONAL, pstate)
        self.assertEqual("Test:Test2[Test2 DOWN] connection down", message);
        self._update_conn_info("Test3", ConnectionStatus.DOWN, "Test3 DOWN",
            vcinfos);
        (pstate, message) = ConnectionState.get_conn_state_cb(vcinfos);
        self.assertEqual(ProcessState.NON_FUNCTIONAL, pstate);
        self.assertEqual("Test:Test2[Test2 DOWN], Test:Test3[Test3 DOWN] connection down", message);
    #end test_callback

#end ConnInfoTest

if __name__ == '__main__':
    unittest.main()
