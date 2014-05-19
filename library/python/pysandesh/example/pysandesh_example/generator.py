#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

import gevent
from gevent import monkey; monkey.patch_all()
from pysandesh.sandesh_base import *
from gen_py.generator_msg.ttypes import *
from pysandesh_example.gen_py.vn.ttypes import *
from pysandesh_example.gen_py.vm.ttypes import *
import sandesh_req_impl
import socket

class generator(object):
    def __init__(self):
        self._sandesh_instance = Sandesh()
        sandesh_req_impl.bind_handle_request_impl()
    #end __init__

    def run_generator(self):
        self._sandesh_instance.init_generator('PysandeshExample', socket.gethostname(),
                'Example', '0', ['127.0.0.1:8086'], 'generator_context', 9090, 
                ['pysandesh_example'])
        self._sandesh_instance.set_logging_params(enable_local_log = True,
                                                  level = SandeshLevel.SYS_EMERG)
        self._sandesh_instance.trace_buffer_create(name = "PysandeshTraceBuf",
                                                   size = 1000)
        send_uve_task = gevent.spawn(self._send_uve_sandesh)
        gevent.joinall([send_uve_task])
        print 'We shoud not see this message!'
    #end run_generator

    def _send_sandesh(self):
        while True:
            gen_msg = GeneratorMsg(sandesh=self._sandesh_instance)
            gen_msg.send(sandesh=self._sandesh_instance)
            m1 = msg1(1, 2, 3, "msg1_string1", 10, sandesh=self._sandesh_instance)
            m1.send(sandesh=self._sandesh_instance)
            m4 = msg4(name='msg1', sandesh=self._sandesh_instance)
            m4.trace_msg(name='PysandeshTraceBuf',
                         sandesh=self._sandesh_instance)
            st11 = struct1(10, "struct1_string1")
            st12 = struct1(20, "struct1_string2")
            m2 = msg2(st11, st12, sandesh=self._sandesh_instance)
            m2.send(sandesh=self._sandesh_instance)
            m4 = msg4(name='msg2', sandesh=self._sandesh_instance)
            m4.trace_msg(name='PysandeshTraceBuf',
                         sandesh=self._sandesh_instance)
            s_list = ['str1', 'str2', 'str3']
            i_list = [11, 22]
            stl1  = []
            stl1.append(st11)
            stl1.append(st12)
            st21 = struct2(stl1, 44)
            m3 = msg3(s_list, i_list, st21, sandesh=self._sandesh_instance)
            m3.send(sandesh=self._sandesh_instance)
            m4 = msg4(name='msg3', sandesh=self._sandesh_instance)
            m4.trace_msg(name='PysandeshTraceBuf',
                         sandesh=self._sandesh_instance)
            gevent.sleep(20)
    #end _send_sandesh

    def _send_uve_sandesh(self):
        count = 0
        while True:
            count += 1
            
            vn_cfg = UveVirtualNetworkConfig()
            vn_cfg.name = 'sandesh-corp:vn45'
            vn_cfg.total_interfaces = count*2
            vn_cfg.total_virtual_machines = count
            uve_cfg_vn = UveVirtualNetworkConfigTrace(data=vn_cfg, 
                                                      sandesh=self._sandesh_instance)
            uve_cfg_vn.send(sandesh=self._sandesh_instance)
            m4 = msg4(name='UveVirtualNetworkConfigTrace', sandesh=self._sandesh_instance)
            m4.trace_msg(name='PysandeshTraceBuf',
                         sandesh=self._sandesh_instance)

            vm_if = VmInterfaceAgent()
            vm_if.name = 'vhost0'
            vm_if.in_pkts = count;
            vm_if.in_bytes = count*10;
            vm_agent = UveVirtualMachineAgent()
            vm_agent.name = 'sandesh-corp:vm-dns'
            vm_agent.interface_list = []
            vm_agent.interface_list.append(vm_if)
            uve_agent_vm = UveVirtualMachineAgentTrace(data=vm_agent, 
                                                       sandesh=self._sandesh_instance)
            uve_agent_vm.send(sandesh=self._sandesh_instance)
            m4 = msg4(name='UveVirtualMachineAgentTrace', sandesh=self._sandesh_instance)
            m4.trace_msg(name='PysandeshTraceBuf',
                         sandesh=self._sandesh_instance)
            
            gevent.sleep(10)
    #end _send_uve_sandesh

#end class generator

gen = generator()
gen.run_generator()
