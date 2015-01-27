#
# Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
#

#
# Sandesh Request Implementation
#

from pysandesh.test.gen_py.sandesh_http_test.ttypes import SandeshHttpRequest

class SandeshHttpRequestImp():

    def __init__(self, sandesh):
        SandeshHttpRequest.handle_request = self.sandesh_http_request_handle_request

    def sandesh_http_request_handle_request(self, sandesh_req):
        self.sandesh_req = sandesh_req
