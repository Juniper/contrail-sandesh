from pysandesh.test.gen_py.sandesh_http_test.ttypes import SandeshHttpRequest

class SandeshHttpRequestImp():

    def __init__(self, sandesh):
        SandeshHttpRequest.handle_request = self.sandesh_http_request_handle_request

    def sandesh_http_request_handle_request(self, sandesh_req):
        dict = {}
        if sandesh_req.testID:
            dict['testID'] = sandesh_req.testID
        else:
            dict['testID'] = 0
        if sandesh_req.teststring1:
            dict['teststring1'] = sandesh_req.teststring1
        else:
            dict['teststring1'] = ''
        if sandesh_req.teststring2:
            dict['teststring2'] = sandesh_req.teststring2
        else:
            dict['teststring2'] = ''
        self.dict = dict

