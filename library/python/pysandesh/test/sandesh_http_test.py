#!/usr/bin/env python

#
# Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_http_test
#

import unittest
import sys
import os
import socket
import test_utils
import time
import urllib2
import gevent
from gevent import monkey; monkey.patch_all()

sys.path.insert(1, sys.path[0]+'/../../../python')

from pysandesh.sandesh_base import *

class SandeshHttpTest(unittest.TestCase):

    def setUp(self):
        self.setUpClass()
        self.http_port = test_utils.get_free_port()
        sandesh_global.init_generator('sandesh_http_test', socket.gethostname(),
                'Test', 'Test', None, 'sandesh_http_test_ctxt', self.http_port,
                sandesh_req_uve_pkg_list = [],
                connect_to_collector=False)
        time.sleep(1) # Let http server up
        from sandesh_req_impl import SandeshHttpRequestImp
        self.sandesh_req_impl = SandeshHttpRequestImp(sandesh_global)
        self.assertTrue(sandesh_global.client() is None)
    #end setUp

    def test_validate_url_fields_with_special_char(self):
        print '----------------------------------------------'
        print '    Validate URL Fields With Special Char     '
        print '----------------------------------------------'
        http_url_ = "http://127.0.0.1:" + str(self.http_port) + '/'
        string1 = urllib2.quote("<one&two>")
        string2 = urllib2.quote("&1%2&")
        http_url = http_url_ + "Snh_SandeshHttpRequest?" + \
             "testID=1&teststring1=" + string1 + \
             "&teststring2=" + string2
        try:
            data = urllib2.urlopen(http_url)
        except urllib2.HTTPError, e:
            print "HTTP error: " + str(e.code)
            assert(False)
        except urllib2.URLError, e:
            print "Network error: " +  str(e.reason.args[1])
            assert(False)
        assert(self.sandesh_req_impl.sandesh_req.testID == 1)
        assert(self.sandesh_req_impl.sandesh_req.teststring1 == '<one&two>')
        assert(self.sandesh_req_impl.sandesh_req.teststring2 == '&1%2&')

    def test_validate_url_with_one_empty_field(self):
        print '-------------------------------------------'
        print '     Validate URL With One Empty Field     '
        print '-------------------------------------------'
        http_url_ = "http://127.0.0.1:" + str(self.http_port) + '/'
        http_url = http_url_ + "Snh_SandeshHttpRequest?" + \
                   "testID=2&teststring1=&teststring2=string"
        try:
            data = urllib2.urlopen(http_url)
        except urllib2.HTTPError, e:
            print "HTTP error: " + str(e.code)
            assert(False)
        except urllib2.URLError, e:
            print "Network error: " +  str(e.reason.args[1])
            assert(False)
        assert(self.sandesh_req_impl.sandesh_req.testID == 2)
        assert(self.sandesh_req_impl.sandesh_req.teststring1 == None)
        assert(self.sandesh_req_impl.sandesh_req.teststring2 == 'string')

    def test_validate_url_with_two_empty_fields(self):
        print '--------------------------------------------'
        print '     Validate URL With Two Empty Fields     '
        print '--------------------------------------------'
        http_url_ = "http://127.0.0.1:" + str(self.http_port) + '/'
        http_url = http_url_ + "Snh_SandeshHttpRequest?" + \
                   "testID=0&teststring1=&teststring2="
        try:
            data = urllib2.urlopen(http_url)
        except urllib2.HTTPError, e:
            print "HTTP error: " + str(e.code)
            assert(False)
        except urllib2.URLError, e:
            print "Network error: " +  str(e.reason.args[1])
            assert(False)
        assert(self.sandesh_req_impl.sandesh_req.testID == 0)
        assert(self.sandesh_req_impl.sandesh_req.teststring1 == None)
        assert(self.sandesh_req_impl.sandesh_req.teststring2 == None)

    def test_validate_url_with_x_value(self):
        print '-----------------------------------'
        print '     Validate URL With x Value     '
        print '-----------------------------------'
        http_url_ = "http://127.0.0.1:" + str(self.http_port) + '/'
        http_url = http_url_ + "Snh_SandeshHttpRequest?x=3"
        try:
            data = urllib2.urlopen(http_url)
        except urllib2.HTTPError, e:
            print "HTTP error: " + str(e.code)
            assert(False)
        except urllib2.URLError, e:
            print "Network error: " +  str(e.reason.args[1])
            assert(False)
        assert(self.sandesh_req_impl.sandesh_req.testID == 3)
        assert(self.sandesh_req_impl.sandesh_req.teststring1 == None)
        assert(self.sandesh_req_impl.sandesh_req.teststring2 == None)

if __name__ == '__main__':
    unittest.main()
