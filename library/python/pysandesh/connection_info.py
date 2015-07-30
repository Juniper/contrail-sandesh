#
# Copyright (c) 2014 Juniper Networks, Inc. All rights reserved.
#

#
# Connection State 
#

import gevent

from gen_py.process_info.constants import ConnectionTypeNames, \
    ConnectionStatusNames, ProcessStateNames
from gen_py.process_info.ttypes import ConnectionInfo, \
    ProcessStatus, ProcessState, ConnectionStatus

class ConnectionState(object):
    _sandesh = None 
    _connection_map = {}
    _hostname = None 
    _module_id = None
    _instance_id = None
    _status_cb = None
    _uve_type_cls = None
    _uve_data_type_cls = None

    @staticmethod
    def _send_uve():
        if not ConnectionState._status_cb:
            return
        conn_infos = ConnectionState._connection_map.values()
        (process_state, message) = \
            ConnectionState._status_cb(conn_infos)
        process_status = ProcessStatus(
            module_id = ConnectionState._module_id,
            instance_id = ConnectionState._instance_id,
            state = ProcessStateNames[process_state],
            connection_infos = conn_infos,
            description = message)
        uve_data = ConnectionState._uve_data_type_cls(
            name = ConnectionState._hostname,
            process_status = [process_status])
        uve = ConnectionState._uve_type_cls(data = uve_data,
                sandesh = ConnectionState._sandesh)
        uve.send(sandesh = ConnectionState._sandesh)
    #end _send_uve

    @staticmethod
    def init(sandesh, hostname, module_id, instance_id, status_cb,
             uve_type_cls, uve_data_type_cls):
        ConnectionState._sandesh = sandesh
        ConnectionState._hostname = hostname
        ConnectionState._module_id = module_id
        ConnectionState._instance_id = instance_id
        ConnectionState._status_cb = status_cb
        ConnectionState._uve_type_cls = uve_type_cls
        ConnectionState._uve_data_type_cls = uve_data_type_cls
    #end init

    @staticmethod
    def get_process_state_cb(conn_infos):
        is_cup = True
        message = ''
        for conn_info in conn_infos:
            if conn_info.status != ConnectionStatusNames[ConnectionStatus.UP]:
                if message == '':
                    message = conn_info.type
                else:
                    message += ', ' + conn_info.type
                if conn_info.name is not None and conn_info.name is not '':
                    message += ':' + conn_info.name
                is_cup = False
        if is_cup:
            return (ProcessState.FUNCTIONAL, '')
        else:
            message += ' connection down'
            return (ProcessState.NON_FUNCTIONAL, message)
    #end get_process_state_cb

    @staticmethod     
    def update(conn_type, name, status, server_addrs = None, message = None):
        conn_key = (conn_type, name)
        conn_info = ConnectionInfo(type = ConnectionTypeNames[conn_type],
                                   name = name,
                                   status = ConnectionStatusNames[status],
                                   description = message,
                                   server_addrs = server_addrs)
        if ConnectionState._connection_map.has_key(conn_key):
            if ConnectionStatusNames[status] == ConnectionState._connection_map[conn_key].status and \
                    server_addrs == ConnectionState._connection_map[conn_key].server_addrs and \
                    message == ConnectionState._connection_map[conn_key].description:
		return
        ConnectionState._connection_map[conn_key] = conn_info
        ConnectionState._send_uve()
    #end update

    @staticmethod
    def delete(conn_type, name):
        conn_key = (conn_type, name)
        ConnectionState._connection_map.pop(conn_key, 'None')
        ConnectionState._send_uve()
    #end delete

#end class ConnectionState
