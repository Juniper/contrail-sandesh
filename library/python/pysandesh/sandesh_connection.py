#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# Sandesh Connection
#

import gevent
import os
from sandesh_session import SandeshSession
from sandesh_session import SandeshReader
from sandesh_state_machine import SandeshStateMachine
from transport import TTransport
from protocol import TXMLProtocol
from gen_py.sandesh.constants import *
from sandesh_uve import SandeshUVETypeMaps

class SandeshConnection(object):

    _CONNECT_RETRY_DELAY = 15

    def __init__(self, sandesh_instance, server, client):
        self._sandesh_instance = sandesh_instance
        self._logger = sandesh_instance._logger
        self._client = client
        self._server = server
        # Collector name. Updated upon receiving the control message 
        # from the Collector during connection negotiation.
        self._collector = None 
        self._admin_down = False
        self._state_machine = SandeshStateMachine(self)
        self._state_machine.initialize()
        self._retry_count = 0
        self._session = None
    #end __init__

    # Public methods

    def session(self):
        return self._session
    #end session

    def statemachine(self):
        return self._state_machine
    #end statemachine

    def sandesh_instance(self):
        return self._sandesh_instance
    #end sandesh_instance

    def collector(self):
        return self._collector
    #end collector

    def set_collector(self, collector):
        self._collector = collector
    #end set_collector

    def reset_collector(self):
        self._collector = None
    #end reset_collector

    def connect(self):
        self._session = SandeshSession(self._sandesh_instance, self._server,
                                       self._state_machine.on_session_event,
                                       self._receive_sandesh_msg)
        while self._session.connect() < 0:
            self._retry_count += 1
            gevent.sleep(self._CONNECT_RETRY_DELAY)
        self._session.set_connection(self)
        gevent.spawn(self._session.read)
    #end connect

    def disconnect(self):
        if self._session:
            self._session.close()
            self._session = None
            self._collector = None
    #end disconnect 

    def state(self):
        return self._state_machine.state()
    #end state

    def handle_initialized(self, count):
        uve_types = []
        uve_global_map = self._sandesh_instance._uve_type_maps.get_uve_global_map()
        for uve_type_key in uve_global_map.iterkeys():
            uve_types.append(uve_type_key)
        from gen_py.sandesh_ctrl.ttypes import SandeshCtrlClientToServer
        ctrl_msg = SandeshCtrlClientToServer(self._sandesh_instance.source_id(),
            self._sandesh_instance.module(), count, uve_types, os.getpid(), 0)
        self._logger.debug('Send sandesh control message. uve type count # %d' % (len(uve_types)))
        ctrl_msg.request('ctrl', sandesh=self._sandesh_instance)
    #end handle_initialized

    def handle_sandesh_ctrl_msg(self, ctrl_msg):
        self._client.handle_sandesh_ctrl_msg(ctrl_msg)
    #end handle_sandesh_ctrl_msg

    def handle_sandesh_uve_msg(self, uve_msg):
        self._client.send_sandesh(uve_msg)
    #end handle_sandesh_uve_msg

    def set_admin_state(self, down):
        if self._admin_down != down:
            self._admin_down = down
            self._state_machine.set_admin_state(down)
    #end set_admin_state

    # Private methods

    def _receive_sandesh_msg(self, session, msg):
        (hdr, hdr_len, sandesh_name) = SandeshReader.extract_sandesh_header(msg)
        if sandesh_name is None:
            self._logger.error('Failed to decode sandesh header for "%s"' % (msg))
            return
        # update the sandesh rx stats
        self._sandesh_instance.stats().update_stats(sandesh_name, len(msg), False)
        if hdr.Hints & SANDESH_CONTROL_HINT:
            self._logger.debug('Received sandesh control message [%s]' % (sandesh_name))
            if sandesh_name != 'SandeshCtrlServerToClient':
                self._logger.error('Invalid sandesh control message [%s]' % (sandesh_name))
                return
            transport = TTransport.TMemoryBuffer(msg[hdr_len:])
            protocol_factory = TXMLProtocol.TXMLProtocolFactory()
            protocol = protocol_factory.getProtocol(transport)
            from gen_py.sandesh_ctrl.ttypes import SandeshCtrlServerToClient
            sandesh_ctrl_msg = SandeshCtrlServerToClient()
            if sandesh_ctrl_msg.read(protocol) == -1:
                self._logger.error('Failed to decode sandesh control message "%s"' %(msg))
            else:
                self._state_machine.on_sandesh_ctrl_msg_receive(session, sandesh_ctrl_msg, 
                                                                hdr.Source)
        else:
            self._logger.debug('Received sandesh message [%s]' % (sandesh_name))
            self._client.handle_sandesh_msg(sandesh_name, msg[hdr_len:])
    #end _receive_sandesh_msg

#end class SandeshConnection
