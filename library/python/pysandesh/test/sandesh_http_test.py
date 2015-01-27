#!/usr/bin/env python

#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_http_test
#

import unittest
import cStringIO
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
    @classmethod
    def setUpClass(self):
        self.http_port = test_utils.get_free_port()
        sandesh_global.init_generator('sandesh_http_test', socket.gethostname(),
                'Test', 'Test', None, 'sandesh_http_test_ctxt', self.http_port,
                sandesh_req_uve_pkg_list = [],
                connect_to_collector=False)

    def setUp(self):
        self.setUpClass()
        time.sleep(1) # Let http server up
        from sandesh_req_impl import SandeshHttpRequestImp
        self.sandesh_req_impl = SandeshHttpRequestImp(sandesh_global)
        self.assertTrue(sandesh_global.client() is None)
    #end setUp

    def test_http_url_1(self):
        print '-------------------------'
        print '     Test Http URL 1     '
        print '-------------------------'
        http_url_ = "http://127.0.0.1:" + str(self.http_port) + '/'
        http_url = http_url_ + "Snh_SandeshHttpRequest?" + \
             "testID=1&teststring1=<one&two>" + \
             "&teststring2=&1%2&"
        try:
            data = urllib2.urlopen(http_url)
        except urllib2.HTTPError, e:
            print "HTTP error: " + str(e.code)
            assert(False)
        except urllib2.URLError, e:
            print "Network error: " +  str(e.reason.args[1])
            assert(False)
        assert(self.sandesh_req_impl.dict['testID'] == 1)
        assert(self.sandesh_req_impl.dict['teststring1'] == '<one&two>')
        assert(self.sandesh_req_impl.dict['teststring2'] == '&1%2&')

    def test_http_url_2(self):
        print '-------------------------'
        print '     Test Http URL 2     '
        print '-------------------------'
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
        assert(self.sandesh_req_impl.dict['testID'] == 2)
        assert(self.sandesh_req_impl.dict['teststring1'] == '')
        assert(self.sandesh_req_impl.dict['teststring2'] == 'string')

    def test_http_url_2(self):
        print '-------------------------'
        print '     Test Http URL 2     '
        print '-------------------------'
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
        assert(self.sandesh_req_impl.dict['testID'] == 2)
        assert(self.sandesh_req_impl.dict['teststring1'] == '')
        assert(self.sandesh_req_impl.dict['teststring2'] == 'string')

    def test_http_url_3(self):
        print '-------------------------'
        print '     Test Http URL 3     '
        print '-------------------------'
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
        assert(self.sandesh_req_impl.dict['testID'] == 0)
        assert(self.sandesh_req_impl.dict['teststring1'] == '')
        assert(self.sandesh_req_impl.dict['teststring2'] == '')

    def test_http_url_4(self):
        print '-------------------------'
        print '     Test Http URL 4     '
        print '-------------------------'
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
        assert(self.sandesh_req_impl.dict['testID'] == 3)
        assert(self.sandesh_req_impl.dict['teststring1'] == '')
        assert(self.sandesh_req_impl.dict['teststring2'] == '')

if __name__ == '__main__':
    unittest.main()
