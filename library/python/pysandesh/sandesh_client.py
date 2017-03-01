#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# Sandesh Client
#

from sandesh_connection import SandeshConnection
from sandesh_logger import SandeshLogger
from transport import TTransport
from protocol import TXMLProtocol
from sandesh_uve import SandeshUVETypeMaps
from gen_py.sandesh.ttypes import SandeshTxDropReason, SandeshRxDropReason

class SandeshClient(object):

    def __init__(self, sandesh):
        self._sandesh_instance = sandesh
        self._logger = sandesh._logger
        self._connection = None
    #end __init__

    # Public functions

    def initiate(self, collectors):
        self._connection = SandeshConnection(self._sandesh_instance, self,
                                             collectors)
    #end initiate

    def set_collectors(self, collectors):
        self._connection.set_collectors(collectors)
    # end set_collectors

    def connection(self):
        return self._connection
    #end connection

    def send_sandesh(self, sandesh):
        if (self._connection.session() is not None) and \
                (self._sandesh_instance._module is not None) and \
                (self._sandesh_instance._module != ""):
            self._connection.session().enqueue_sandesh(sandesh)
        else:
            if (self._connection.session() is None):
                self._sandesh_instance.drop_tx_sandesh(sandesh,
                    SandeshTxDropReason.NoSession)
            else:
                self._sandesh_instance.drop_tx_sandesh(sandesh,
                    SandeshTxDropReason.ClientSendFailed)
        return 0
    #end send_sandesh

    def send_uve_sandesh(self, uve_sandesh):
        self._connection.statemachine().on_sandesh_uve_msg_send(uve_sandesh)
    #end send_uve_sandesh

    def handle_sandesh_msg(self, sandesh_name, sandesh_xml, msg_len):
        transport = TTransport.TMemoryBuffer(sandesh_xml)
        protocol_factory = TXMLProtocol.TXMLProtocolFactory()
        protocol = protocol_factory.getProtocol(transport)
        sandesh_req = self._sandesh_instance.get_sandesh_request_object(sandesh_name)
        if sandesh_req:
            if sandesh_req.read(protocol) == -1:
                self._sandesh_instance.update_rx_stats(sandesh_name, msg_len,
                    SandeshRxDropReason.DecodingFailed)
                self._logger.error('Failed to decode sandesh request "%s"' \
                    % (sandesh_name))
            else:
                self._sandesh_instance.update_rx_stats(sandesh_name, msg_len)
                self._sandesh_instance.enqueue_sandesh_request(sandesh_req)
        else:
            self._sandesh_instance.update_rx_stats(sandesh_name, msg_len,
                SandeshRxDropReason.CreateFailed)
    #end handle_sandesh_msg

    def handle_sandesh_ctrl_msg(self, sandesh_ctrl_msg):
        uve_type_map = {}
        self._logger.debug('Number of uve types in sandesh control message is %d' % (len(sandesh_ctrl_msg.type_info)))
        for type_info in sandesh_ctrl_msg.type_info:
            uve_type_map[type_info.type_name] = type_info.seq_num
        self._sandesh_instance._uve_type_maps.sync_all_uve_types(uve_type_map, self._sandesh_instance)
    #end handle_sandesh_ctrl_msg

#end class SandeshClient
