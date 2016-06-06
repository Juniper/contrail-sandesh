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

import test_utils
from pysandesh.sandesh_base import *
from pysandesh.sandesh_client import SandeshClient
from pysandesh.util import UTCTimestampUsec
from pysandesh.gen_py.sandesh_alarm.ttypes import *
from gen_py.sandesh_alarm_base.ttypes import *
from gen_py.uve_alarm_test.ttypes import *

class SandeshUVEAlarmTest(unittest.TestCase):

    def setUp(self):
        self.maxDiff = None
        self.sandesh = Sandesh()
        self.sandesh.init_generator('sandesh_uve_alarm_test',
            socket.gethostname(), 'Test', '0', None, '',
            test_utils.get_free_port(),
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
        self.assertEqual(SANDESH_KEY_HINT, (SANDESH_KEY_HINT & sandesh._hints))
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

        expected_data = [{'seqnum': i+1, 'data': uve_data[i]} \
                         for i in range(len(uve_data))]

        # send UVE with different key
        uve_test_data = SandeshUVEData(name='uve1')
        uve_test = SandeshUVETest(data=uve_test_data, table='CollectorInfo',
                                  sandesh=self.sandesh)
        uve_test.send(sandesh=self.sandesh)

        expected_data.extend([{'seqnum': 6, 'data': uve_test_data}])

        # send dynamic UVEs
        dynamic_uve_data = [
            # add uve
            {
                'type' : (ConfigUVE, Config),
                'table' : 'CollectorInfo',
                'name' : 'node1',
                'elements' : {'log_level' : 'SYS_INFO'},
                'expected_elements' : {'log_level' : 'SYS_INFO'},
                'seqnum' : 1
            },
            # update uve
            {
                'type' : (ConfigUVE, Config),
                'table' : 'CollectorInfo',
                'name' : 'node1',
                'elements' : {'log_local' : 'True'},
                'expected_elements' : {'log_level' : 'SYS_INFO',
                                       'log_local' : 'True'},
                'seqnum' : 2
            },
            # add another uve
            {
                'type' : (ConfigUVE, Config),
                'table' : 'ControlInfo',
                'name' : 'node1',
                'elements' : {'log_category' : 'Redis'},
                'expected_elements' : {'log_category' : 'Redis'},
                'seqnum' : 3
            },
            # delete uve
            {
                'type' : (ConfigUVE, Config),
                'table' : 'ControlInfo',
                'name' : 'node1',
                'deleted' : True,
                'seqnum' : 4
            },
            # add deleted uve
            {
                'type' : (ConfigUVE, Config),
                'table' : 'ControlInfo',
                'name' : 'node1',
                'elements' : {'log_level' : 'SYS_DEBUG',
                              'log_file' : '/var/log/control.log'},
                'expected_elements' : {'log_level' : 'SYS_DEBUG',
                                       'log_file' : '/var/log/control.log'},
                'seqnum' : 5
            },
            #  add another uve - different type
            {
                'type' : (ConfigTestUVE, ConfigTest),
                'table' : 'CollectorInfo',
                'name' : 'node2',
                'elements' : {'param1' : 'val1', 'param2' : 'val2'},
                'expected_elements' : {'param1' : 'val1', 'param2' : 'val2'},
                'seqnum' : 1
            },
            # delete uve, set elements to []
            {
                'type' : (ConfigTestUVE, ConfigTest),
                'table' : 'CollectorInfo',
                'name' : 'node2',
                'deleted' : True,
                'elements' : {},
                'expected_elements' : {},
                'seqnum' : 2
            },
        ]

        for uve in dynamic_uve_data:
            uve_type, uve_data_type = uve['type']
            elts = uve.get('elements')
            uve_data = uve_data_type(name=uve['name'], elements=elts,
                                     deleted=uve.get('deleted'))
            dynamic_uve = uve_type(data=uve_data, table=uve['table'],
                                   sandesh=self.sandesh)
            dynamic_uve.send(sandesh=self.sandesh)
            elts_exp = uve.get('expected_elements')
            if elts_exp is not None:
                uve_data = uve_data_type(name=uve['name'], elements=elts_exp,
                        deleted=uve.get('deleted'))
                uve_data._table = uve['table']
            print uve_data.__dict__
            expected_data.extend([{'seqnum': uve['seqnum'], 'data': uve_data}])

        # sync UVEs
        self.sandesh._uve_type_maps.sync_all_uve_types({}, self.sandesh)

        sync_uve_data = [
            {'data': SandeshUVEData(name='uve1'), 'table': 'CollectorInfo',
             'seqnum': 6},
            {'data': SandeshUVEData(name='uve2'), 'table': 'OpserverInfo',
             'seqnum': 5},
            {'data': SandeshUVEData(name='uve1', xyz=345),
             'table':'OpserverInfo', 'seqnum': 2}
        ]
        for uve_data in sync_uve_data:
            uve_data['data']._table = uve_data['table']
            expected_data.extend([{'seqnum': uve_data['seqnum'],
                                   'data': uve_data['data']}])
        sync_dynamic_uve_data = [
            {
                'type' : (ConfigTestUVE, ConfigTest),
                'table' : 'CollectorInfo',
                'name' : 'node2',
                'deleted' : True,
                'elements' : {},
                'seqnum' : 2
            },
            {
                'type' : (ConfigUVE, Config),
                'table' : 'ControlInfo',
                'name' : 'node1',
                'elements' : {'log_level' : 'SYS_DEBUG',
                              'log_file' : '/var/log/control.log'},
                'seqnum' : 5
            },
            {
                'type' : (ConfigUVE, Config),
                'table' : 'CollectorInfo',
                'name' : 'node1',
                'elements' : {'log_level' : 'SYS_INFO',
                              'log_local' : 'True'},
                'seqnum' : 2
            }
        ]
        for uve in sync_dynamic_uve_data:
            uve_type, uve_data_type = uve['type']
            elts = uve.get('elements')
            uve_data = uve_data_type(name=uve['name'], elements=elts,
                                     deleted=uve.get('deleted'))
            uve_data._table = uve['table']
            expected_data.extend([{'seqnum': uve['seqnum'], 'data': uve_data}])

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

    def _create_uve_alarm_info(self):
        uve_alarm_info = UVEAlarmInfo()
        uve_alarm_info.type = 'ProcessStatus'
        condition = AlarmCondition(operation='==',
            operand1='NodeStatus.process_info.process_state',
            operand2=json.dumps('null'))
        match1 = AlarmMatch(json_operand1_value=json.dumps('null'))
        condition_match = AlarmConditionMatch(condition, [match1])
        and_list = AlarmAndList(and_list=[condition_match])
        uve_alarm_info.alarm_rules = [AlarmRules(or_list=[and_list])]
        uve_alarm_info.ack = False
        uve_alarm_info.timestamp = UTCTimestampUsec()
        uve_alarm_info.severity = 1
        return uve_alarm_info
    # end _create_uve_alarm_info

    def _update_uve_alarm_info(self):
        uve_alarm_info = self._create_uve_alarm_info()
        uve_alarm_info.ack = True
        return uve_alarm_info
    # end _update_uve_alarm_info

    def test_sandesh_alarm(self):
        alarm_data = [
            # add alarm
            (UVEAlarms(name='alarm1', alarms=[self._create_uve_alarm_info()]),
             'ObjectCollectorInfo'),
            # update alarm
            (UVEAlarms(name='alarm1', alarms=[self._update_uve_alarm_info()]),
             'ObjectCollectorInfo'),
            # add another alarm
            (UVEAlarms(name='alarm2', alarms=[self._create_uve_alarm_info()]),
             'ObjectVRouterInfo'),
            # delete alarm
            (UVEAlarms(name='alarm2', deleted=True), 'ObjectVRouterInfo'),
            # add deleted alarm
            (UVEAlarms(name='alarm2', alarms=[self._create_uve_alarm_info()]),
             'ObjectVRouterInfo'),
            # add alarm with deleted flag set
            (UVEAlarms(name='alarm3', alarms=[self._create_uve_alarm_info()],
                      deleted=True), 'ObjectCollectorInfo'),
            # add alarm with same key and different table
            (UVEAlarms(name='alarm3', alarms=[self._create_uve_alarm_info()]),
             'ObjectVRouterInfo')
        ]

        # send the alarms
        for i in range(len(alarm_data)):
            alarm_test = AlarmTrace(data=alarm_data[i][0],
                                    table=alarm_data[i][1],
                                    sandesh=self.sandesh)
            alarm_test.send(sandesh=self.sandesh)

        expected_data1 = [{'seqnum': i+1, 'data': alarm_data[i][0]} \
                          for i in range(len(alarm_data))]

        # verify alarms sync
        self.sandesh._uve_type_maps.sync_all_uve_types({}, self.sandesh)

        expected_data2 = [
            {'seqnum': 7, 'data': alarm_data[6][0]},
            {'seqnum': 5, 'data': alarm_data[4][0]},
            {'seqnum': 6, 'data': alarm_data[5][0]},
            {'seqnum': 2, 'data': alarm_data[1][0]}
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
