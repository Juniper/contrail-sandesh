#!/usr/bin/env python

#
# Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_uve_analytics_api_info_test
#

import mock
import unittest
import logging
import sys
import socket

sys.path.insert(1, sys.path[0]+'/../../../python')

import test_utils
from pysandesh.sandesh_base import *
from pysandesh.sandesh_client import SandeshClient
from pysandesh.util import UTCTimestampUsec
from gen_py.analytics_api_info.ttypes import AnalyticsApiInfo, AnalyticsApiInfoUVE

class SandeshUVEAnalyticsApiInfoTest(unittest.TestCase):

    def setUp(self):
        self.maxDiff = None
        self.sandesh = Sandesh()
        self.sandesh.init_generator('sandesh_uve_analytics_api_info_test',
            socket.gethostname(), 'Test', '0', None, '',
            test_utils.get_free_port(),
            connect_to_collector=False)
        # mock the sandesh client object
        self.sandesh._client = mock.MagicMock(spec=SandeshClient)
    # end setUp

    def tearDown(self):
        pass
    # end tearDown

    def verify_uve_analytics_api_info_sandesh(self, sandesh, seqnum,
                                 sandesh_type, data):
        self.assertEqual(socket.gethostname(), sandesh._source)
        self.assertEqual('Test', sandesh._node_type)
        self.assertEqual('sandesh_uve_analytics_api_info_test', sandesh._module)
        self.assertEqual('0', sandesh._instance_id)
        self.assertEqual(SANDESH_KEY_HINT, (SANDESH_KEY_HINT & sandesh._hints))
        self.assertEqual(sandesh_type, sandesh._type)
        self.assertEqual(seqnum, sandesh._seqnum)
        #sandesh.data referes to UVE.data
        self.assertEqual(data, sandesh.data)
    # end verify_uve_analytics_api_info_sandesh

    def _create_uve_analytics_api_info(self, rest_api_ip, host_ip):
        analytics_api_info = AnalyticsApiInfo()
        analytics_api_info.name = socket.gethostname()
        analytics_api_info.analytics_node_ip = dict()
        analytics_api_info.analytics_node_ip[
                'rest_api_ip'] = rest_api_ip
        analytics_api_info.analytics_node_ip[
                'host_ip'] = host_ip
        if rest_api_ip == "" and host_ip == "":
            analytics_api_info.deleted = True
        return analytics_api_info
    # end _create_uve_analytics_api_info

    def _update_uve_analytics_api_info(self, rest_api_ip, host_ip):
        uve_analytics_api_info = self._create_uve_analytics_api_info(
                rest_api_ip, host_ip)
        return uve_analytics_api_info
    # end _update_uve_analytics_api_info

    def test_sandesh_analytics_api_info(self):
        self.sandesh._logger.info("RUN Case: test_sandesh_analytics_api_info")
        #set debug level of logger for this test
        #self.sandesh._logger.level=logging.DEBUG

        analytics_api_info_data = [
            # add rest_api_ip and host_ip
            (self._create_uve_analytics_api_info("1.2.3.4", "5.6.7.8")),
            # update rest_api_ip
            (self._update_uve_analytics_api_info("0.0.0.0", "5.6.7.8")),
            # delete rest_api_ip and host_ip
            (self._update_uve_analytics_api_info("", "")),
            # add rest_api_ip and host_ip
            (self._create_uve_analytics_api_info("10.10.10.10", "90.90.90.90"))
        ]

        expected_data = [{'seqnum': i+1, 'data': analytics_api_info_data[i]}  \
                        for i in range(len(analytics_api_info_data))]

        # send the rest_api_ip and host_ip
        for i in range(len(analytics_api_info_data)):
            uve_analytics_api_test = AnalyticsApiInfoUVE(
                        data = analytics_api_info_data[i],
                        sandesh = self.sandesh)
            uve_analytics_api_test.send(sandesh = self.sandesh)

            self.sandesh._uve_type_maps.sync_all_uve_types({}, self.sandesh)

            self.sandesh._logger.debug("==============================================\n")
            self.sandesh._logger.debug("All args list\n")
            self.sandesh._logger.debug("+++++++++++++++++++++++++++++++++++++++++++\n")
            self.sandesh._logger.debug(self.sandesh._client.send_uve_sandesh.call_args_list)
            self.sandesh._logger.debug("+++++++++++++++++++++++++++++++++++++++++++\n")
            self.sandesh._logger.debug("Expected Data------------------------\n")
            self.sandesh._logger.debug(expected_data[i]['data'])
            self.sandesh._logger.debug("==============================================\n")

            # verify the result
            args_list = self.sandesh._client.send_uve_sandesh.call_args_list
            #for i in range(len(expected_data)):
            self.verify_uve_analytics_api_info_sandesh(args_list[len(args_list)-1][0][0],
                    seqnum=expected_data[i]['seqnum'],
                    sandesh_type=SandeshType.UVE,
                    data=expected_data[i]['data'])
    # end test_sandesh_analytics_api_info

# end class SandeshUVEAnalyticsApiInfoTest


if __name__ == '__main__':
    unittest.main(verbosity=100, catchbreak=True)
