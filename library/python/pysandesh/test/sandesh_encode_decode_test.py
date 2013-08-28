#!/usr/bin/env python

#
# Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
#

#
# sandesh_encode_decode_test
#

import unittest
import sys
import os

sys.path.insert(1, sys.path[0]+'/../../../python')

from pysandesh.transport import TTransport
from pysandesh.protocol import TXMLProtocol
from gen_py.encode_decode_test.ttypes import *

class SandeshEncodeDecodeTest(unittest.TestCase):
    def setUp(self):
        self._protocol_factory = TXMLProtocol.TXMLProtocolFactory()
        self.setUpEncode()
    #end setUp

    def setUpEncode(self):
        self._wtransport = TTransport.TMemoryBuffer()
        self._wprotocol = self._protocol_factory.getProtocol(self._wtransport)
    #end setUpEncode

    def setUpDecode(self, buf):
        self._rtransport = TTransport.TMemoryBuffer(buf)
        self._rprotocol = self._protocol_factory.getProtocol(self._rtransport)
    #end setUpDecode

    def test_basic_types(self):
        print '-------------------------'
        print '     Test Basic Types    '
        print '-------------------------'
        # Encode Test
        btype_encode = BasicTypesTest(bool_1=True, byte_1=127, i16_1=4321,
            i32_1=54321, i64_1=654321, double_1=12.345, string_1="Basic Types Test",
            u16_1=65535, u32_1=4294967295, u64_1=18446744073709551615,
            xml_1="<abc>", xml_2 ="abc", xml_3="abc]", xml_4="abc]]")
        self.assertNotEqual(-1, btype_encode.write(self._wprotocol))
        expected_data = '<BasicTypesTest type="sandesh"><bool_1 type="bool" identifier="1">true</bool_1><byte_1 type="byte" identifier="2">127</byte_1><i16_1 type="i16" identifier="3">4321</i16_1><i32_1 type="i32" identifier="4">54321</i32_1><i64_1 type="i64" identifier="5">654321</i64_1><double_1 type="double" identifier="6">12.345</double_1><string_1 type="string" identifier="7">Basic Types Test</string_1><str8 type="string" identifier="8">Last field</str8><u16_1 type="u16" identifier="9">65535</u16_1><u32_1 type="u32" identifier="10">4294967295</u32_1><u64_1 type="u64" identifier="11">18446744073709551615</u64_1><xml_1 type="xml" identifier="12"><![CDATA[<abc>]]></xml_1><xml_2 type="xml" identifier="13"><![CDATA[abc]]></xml_2><xml_3 type="xml" identifier="14"><![CDATA[abc]]]></xml_3><xml_4 type="xml" identifier="15"><![CDATA[abc]]]]></xml_4></BasicTypesTest>'
        actual_data = self._wtransport.getvalue()
        self.assertEqual(expected_data, actual_data)

        # Decode Test
        self.setUpDecode(expected_data)
        btype_decode = BasicTypesTest()
        self.assertNotEqual(-1, btype_decode.read(self._rprotocol))
        self.assertTrue(btype_encode.compare(btype_decode))
    #end test_basic_types

    def test_xml_string(self):
        print '-------------------------'
        print '     Test XML String     '
        print '-------------------------'
        # Encode Test
        xmlmsg = '<sandesh_types><type1>\"systemlog\"</type1><type2>\'objectlog\'</type2><type3>uve & trace</type3></sandesh_types>'
        xml_encode = BasicTypesTest(string_1=xmlmsg)
        self.assertNotEqual(-1, xml_encode.write(self._wprotocol))
        expected_data = '<BasicTypesTest type="sandesh"><string_1 type="string" identifier="7">&lt;sandesh_types&gt;&lt;type1&gt;&quot;systemlog&quot;&lt;/type1&gt;&lt;type2&gt;&apos;objectlog&apos;&lt;/type2&gt;&lt;type3&gt;uve &amp; trace&lt;/type3&gt;&lt;/sandesh_types&gt;</string_1><str8 type="string" identifier="8">Last field</str8></BasicTypesTest>'
        actual_data = self._wtransport.getvalue()
        self.assertEqual(expected_data, actual_data)
       
        # Decode Test
        self.setUpDecode(expected_data)
        xml_decode = BasicTypesTest()
        self.assertNotEqual(-1, xml_decode.read(self._rprotocol))
        self.assertTrue(xml_encode.compare(xml_decode))
    #end test_xml_string
    
    def test_xml(self):
        print '-------------------------'
        print '     Test XML            '
        print '-------------------------'
        # Encode Test
        xmlmsg = '<sandesh_types><type1>\"systemlog\"</type1><type2>\'objectlog\'</type2><type3>uve & trace</type3></sandesh_types>'
        xml_encode = BasicTypesTest(xml_1=xmlmsg)
        self.assertNotEqual(-1, xml_encode.write(self._wprotocol))
        expected_data = '<BasicTypesTest type="sandesh"><str8 type="string" identifier="8">Last field</str8><xml_1 type="xml" identifier="12"><![CDATA[<sandesh_types><type1>\"systemlog\"</type1><type2>\'objectlog\'</type2><type3>uve & trace</type3></sandesh_types>]]></xml_1></BasicTypesTest>'
        actual_data = self._wtransport.getvalue()
        self.assertEqual(expected_data, actual_data)
       
        # Decode Test
        self.setUpDecode(expected_data)
        xml_decode = BasicTypesTest()
        self.assertNotEqual(-1, xml_decode.read(self._rprotocol))
        self.assertTrue(xml_encode.compare(xml_decode))
    #end test_xml

    def test_struct(self):
        print '-------------------------'
        print '     Test Struct Type    '
        print '-------------------------'
        # Encode Test
        btype_struct1 = StructBasicTypes(bool_1=False, i16_1=9876, u16_1=65535)
        btype_struct2 = StructBasicTypes(i16_1=1111, u32_1=4294967295, u64_1=18446744073709551615,
                                         xml_1="<abc>", xml_2 ="abc", xml_3="abc]", xml_4="abc]]")
        struct_encode = StructTest(btype_struct1, btype_struct2)
        self.assertNotEqual(-1, struct_encode.write(self._wprotocol))
        expected_data = '<StructTest type="sandesh"><st_1 type="struct" identifier="1"><StructBasicTypes><str1 type="string" identifier="1">First field</str1><bool_1 type="bool" identifier="2">false</bool_1><i16_1 type="i16" identifier="3">9876</i16_1><str4 type="string" identifier="4">Last field</str4><u16_1 type="u16" identifier="5">65535</u16_1></StructBasicTypes></st_1><st_2 type="struct" identifier="2"><StructBasicTypes><str1 type="string" identifier="1">First field</str1><i16_1 type="i16" identifier="3">1111</i16_1><str4 type="string" identifier="4">Last field</str4><u32_1 type="u32" identifier="6">4294967295</u32_1><u64_1 type="u64" identifier="7">18446744073709551615</u64_1><xml_1 type="xml" identifier="8"><![CDATA[<abc>]]></xml_1><xml_2 type="xml" identifier="9"><![CDATA[abc]]></xml_2><xml_3 type="xml" identifier="10"><![CDATA[abc]]]></xml_3><xml_4 type="xml" identifier="11"><![CDATA[abc]]]]></xml_4></StructBasicTypes></st_2></StructTest>'
        actual_data = self._wtransport.getvalue()
        self.assertEqual(expected_data, actual_data)

        # Decode Test
        self.setUpDecode(expected_data)
        struct_decode = StructTest()
        self.assertNotEqual(-1, struct_decode.read(self._rprotocol))
        self.assertTrue(struct_encode.compare(struct_decode))
    #end test_struct

    def test_container_types(self):
        print '-------------------------'
        print '   Test Container Types  '
        print '-------------------------'
        # Encode Test
        list_bool = [True, True, False, True]
        list_byte = [123, 12, 23]
        btype_st1 = StructBasicTypes(i16_1=5678)
        btype_st2 = StructBasicTypes(bool_1=True)
        list_btype_st = []
        list_btype_st.append(btype_st1)
        list_btype_st.append(btype_st2)
        list_i32_1 = [1111, 2222, 3333]
        list_str1 = ['nicira', 'midokura', 'contrail']
        list_str2 = ['nvgre', 'vxlan']
        ctype_st1 = StructContainerTypes(list_i32_1, list_str1, list_btype_st)
        ctype_st2 = StructContainerTypes(li32_1=[], lstring_1=list_str2)
        list_ctype_st = []
        list_ctype_st.append(ctype_st1)
        list_ctype_st.append(ctype_st2)
        container_encode = ContainerTypesTest(list_bool, list_byte,
                list_btype_st, list_ctype_st)
        self.assertNotEqual(-1, container_encode.write(self._wprotocol))
        expected_data = '<ContainerTypesTest type="sandesh"><lbool_1 type="list" identifier="1"><list type="bool" size="4"><element>true</element><element>true</element><element>false</element><element>true</element></list></lbool_1><lbyte_1 type="list" identifier="2"><list type="byte" size="3"><element>123</element><element>12</element><element>23</element></list></lbyte_1><lbst_1 type="list" identifier="3"><list type="struct" size="2"><StructBasicTypes><str1 type="string" identifier="1">First field</str1><i16_1 type="i16" identifier="3">5678</i16_1><str4 type="string" identifier="4">Last field</str4></StructBasicTypes><StructBasicTypes><str1 type="string" identifier="1">First field</str1><bool_1 type="bool" identifier="2">true</bool_1><str4 type="string" identifier="4">Last field</str4></StructBasicTypes></list></lbst_1><lcst_1 type="list" identifier="4"><list type="struct" size="2"><StructContainerTypes><li32_1 type="list" identifier="1"><list type="i32" size="3"><element>1111</element><element>2222</element><element>3333</element></list></li32_1><lstring_1 type="list" identifier="2"><list type="string" size="3"><element>nicira</element><element>midokura</element><element>contrail</element></list></lstring_1><lbst_1 type="list" identifier="3"><list type="struct" size="2"><StructBasicTypes><str1 type="string" identifier="1">First field</str1><i16_1 type="i16" identifier="3">5678</i16_1><str4 type="string" identifier="4">Last field</str4></StructBasicTypes><StructBasicTypes><str1 type="string" identifier="1">First field</str1><bool_1 type="bool" identifier="2">true</bool_1><str4 type="string" identifier="4">Last field</str4></StructBasicTypes></list></lbst_1></StructContainerTypes><StructContainerTypes><li32_1 type="list" identifier="1"><list type="i32" size="0"></list></li32_1><lstring_1 type="list" identifier="2"><list type="string" size="2"><element>nvgre</element><element>vxlan</element></list></lstring_1></StructContainerTypes></list></lcst_1></ContainerTypesTest>'
        actual_data = self._wtransport.getvalue()
        self.assertEqual(expected_data, actual_data)

        # Decode Test
        self.setUpDecode(expected_data)
        container_decode = ContainerTypesTest()
        self.assertNotEqual(-1, container_decode.read(self._rprotocol))
        self.assertTrue(container_encode.compare(container_decode))
    #end test_container_types

    def test_annotations(self):
        print '-------------------------'
        print '     Test Annotations    '
        print '-------------------------'
        # Encode Test
        anno_struct = StructAnnotation(string_1="VM", i16_1=345)
        anno_encode = AnnotationsTest(anno_struct, i32_1=911, string_1="VN")
        self.assertNotEqual(-1, anno_encode.write(self._wprotocol))
        expected_data = '<AnnotationsTest type="sandesh"><st1_1 type="struct" identifier="1"><StructAnnotation><string_1 type="string" identifier="1" key="Test">VM</string_1><i16_1 type="i16" identifier="2" format="%x">345</i16_1></StructAnnotation></st1_1><i32_1 type="i32" identifier="2" format="%d">911</i32_1><string_1 type="string" identifier="3" key="Contrail">VN</string_1></AnnotationsTest>'
        actual_data = self._wtransport.getvalue()
        self.assertEqual(expected_data, actual_data)

        # Decode Test
        self.setUpDecode(expected_data)
        anno_decode = AnnotationsTest()
        self.assertNotEqual(-1, anno_decode.read(self._rprotocol))
        self.assertTrue(anno_encode.compare(anno_decode))
    #end test_annotations

#end SandeshEncodeDecodeTest

if __name__ == '__main__':
    unittest.main()
