#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

from pysandesh_example.gen_py.vn.ttypes import VirtualNetwork, VNInfo, VirtualNetworkResp, \
         VirtualNetworkAll, VNStats, VirtualNetworkAllResp

def VirtualNetwork_handle_request(self, sandesh):
    vn_stats = VNStats(in_pkts=10, out_pkts=20, in_bytes=1024, out_bytes=2048)
    vms = ['vm1', 'vm2', 'vm3']

    if not self.name:
        vn_name = 'vn1'
    else:
        vn_name = self.name

    if not self.id:
        vn_id = 100
    else:
        vn_id = self.id

    vn_info = VNInfo(vn_name, vn_id, vms, vn_stats)
    vn_resp = VirtualNetworkResp(vn_info)
    vn_resp.response(self._context)
#end VirtualNetwork_handle_request

def VirtualNetworkAll_handle_request(self, sandesh):
    range_min = 1
    range_max = 10
    for i in range(1,10):
        vn_stats = VNStats(in_pkts=i*10, out_pkts=i*20,
        in_bytes=i*64, out_bytes=i*128)
        vms = []
        for j in range(1, 4):
            vms.append('vm'+str(j*i))
        vn_info = VNInfo('VN'+str(i), i, vms, vn_stats)
        vn_resp = VirtualNetworkAllResp(vn_info)
        if i != (range_max-1):
            vn_resp.response(self._context, True)
        else:
            vn_resp.response(self._context, False)
#end VirtualNetworkAll_handle_request


def bind_handle_request_impl():
    VirtualNetwork.handle_request = VirtualNetwork_handle_request
    VirtualNetworkAll.handle_request = VirtualNetworkAll_handle_request
