#!/usr/bin/env python

#
# Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_uve_alarm_test
#

import mock
import unittest
import sys
import socket

sys.path.insert(1, sys.path[0]+'/../../../python')

from pysandesh.sandesh_base import *
from pysandesh.sandesh_client import SandeshClient
from gen_py.uve_alarm_test.ttypes import *

class SandeshUVEAlarmTest(unittest.TestCase):

    def setUp(self):
        self.maxDiff = None
        self.sandesh = Sandesh()
        self.sandesh.init_generator('sandesh_uve_alarm_test',
            socket.gethostname(), 'Test', '0', None, '', -1,
            connect_to_collector=False)
        # mock the sandesh client object
        self.sandesh._client = mock.MagicMock(spec=SandeshClient)
    # end setUp

    def tearDown(self):
        pass
    # end tearDown

    def verify_uve_alarm_sandesh(self, sandesh, seqnum,
                                 sandesh_type, data):
        self.assertEqual(socket.gethostname(), sandesh._source)
        self.assertEqual('Test', sandesh._node_type)
        self.assertEqual('sandesh_uve_alarm_test', sandesh._module)
        self.assertEqual('0', sandesh._instance_id)
        self.assertEqual(SANDESH_KEY_HINT, sandesh._hints)
        self.assertEqual(sandesh_type, sandesh._type)
        self.assertEqual(seqnum, sandesh._seqnum)
        self.assertEqual(data, sandesh.data)
    # end verify_uve_alarm_sandesh

    def test_sandesh_uve(self):
        uve_data = [
            # add uve
            SandeshUVEData(name='uve1'),
            # update uve
            SandeshUVEData(name='uve1', xyz=345),
            # add another uve
            SandeshUVEData(name='uve2', xyz=12),
            # delete uve
            SandeshUVEData(name='uve2', deleted=True),
            # add deleted uve
            SandeshUVEData(name='uve2')
        ]

        # send UVEs
        for i in range(len(uve_data)):
            uve_test = SandeshUVETest(data=uve_data[i], sandesh=self.sandesh)
            uve_test.send(sandesh=self.sandesh)

        expected_data1 = [{'seqnum': i+1, 'data': uve_data[i]} \
                          for i in range(len(uve_data))]

        # verify uve sync
        self.sandesh._uve_type_maps.sync_all_uve_types({}, self.sandesh)

        expected_data2 = [
            {'seqnum': 5, 'data': SandeshUVEData(name='uve2')},
            {'seqnum': 2, 'data': SandeshUVEData(name='uve1', xyz=345)}
        ]

        expected_data = expected_data1 + expected_data2

        # verify the result
        args_list = self.sandesh._client.send_uve_sandesh.call_args_list
        self.assertEqual(len(expected_data), len(args_list),
                         'args_list: %s' % str(args_list))
        for i in range(len(expected_data)):
            self.verify_uve_alarm_sandesh(args_list[i][0][0],
                seqnum=expected_data[i]['seqnum'],
                sandesh_type=SandeshType.UVE,
                data=expected_data[i]['data'])
    # end test_sandesh_uve

    def test_sandesh_alarm(self):
        alarm_data = [
            # add alarm
            SandeshAlarmData(name='alarm1', description='alarm1 generated'),
            # update alarm
            SandeshAlarmData(name='alarm1', acknowledged=True),
            # add another alarm
            SandeshAlarmData(name='alarm2', description='alarm2 generated'),
            # delete alarm
            SandeshAlarmData(name='alarm2', deleted=True),
            # add deleted alarm
            SandeshAlarmData(name='alarm2'),
            # add alarm with deleted flag set
            SandeshAlarmData(name='alarm3', description='alarm3 deleted',
                             deleted=True)
        ]

        # send the alarms
        for i in range(len(alarm_data)):
            alarm_test = SandeshAlarmTest(data=alarm_data[i],
                                          sandesh=self.sandesh)
            alarm_test.send(sandesh=self.sandesh)

        expected_data1 = [{'seqnum': i+1, 'data': alarm_data[i]} \
                          for i in range(len(alarm_data))]

        # verify alarms sync
        self.sandesh._uve_type_maps.sync_all_uve_types({}, self.sandesh)

        expected_data2 = [
            {'seqnum': 6, 'data': SandeshAlarmData(name='alarm3',
                description='alarm3 deleted', deleted=True)},
            {'seqnum': 5, 'data': SandeshAlarmData(name='alarm2')},
            {'seqnum': 2, 'data': SandeshAlarmData(name='alarm1',
                description='alarm1 generated', acknowledged=True)}
        ]

        expected_data = expected_data1 + expected_data2

        # verify the result
        args_list = self.sandesh._client.send_uve_sandesh.call_args_list
        self.assertEqual(len(expected_data), len(args_list),
                         'args_list: %s' % str(args_list))
        for i in range(len(expected_data)):
            self.verify_uve_alarm_sandesh(args_list[i][0][0],
                seqnum=expected_data[i]['seqnum'],
                sandesh_type=SandeshType.ALARM,
                data=expected_data[i]['data'])
    # end test_sandesh_alarm

# end class SandeshUVEAlarmTest


if __name__ == '__main__':
    unittest.main(verbosity=2, catchbreak=True)
