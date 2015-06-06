//
// Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
//

//
// sandesh_test_common.cc
//

#include "testing/gunit.h"

#include <sandesh/protocol/TXMLProtocol.h>
#include <sandesh/transport/TBufferTransports.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>

using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;

static const std::string FakeMessageSandeshBegin("<FakeSandesh type=\"sandesh\"><str1 type=\"string\" identifier=\"1\">");
static const std::string FakeMessageSandeshEnd("</str1></FakeSandesh>");

void CreateFakeMessage(uint8_t *data, size_t length) {
    size_t offset = 0;
    SandeshHeader header;
    // Populate the header
    header.set_Namespace("Test");
    header.set_Timestamp(123456);
    header.set_Module("SandeshStateMachineTest");
    header.set_Source("TestMachine");
    header.set_Context("");
    header.set_SequenceNum(0);
    header.set_VersionSig(0);
    header.set_Type(SandeshType::SYSTEM);
    header.set_Hints(0);
    header.set_Level(SandeshLevel::SYS_DEBUG);
    header.set_Category("SSM");
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(512));
    boost::shared_ptr<TXMLProtocol> prot =
            boost::shared_ptr<TXMLProtocol>(
                    new TXMLProtocol(btrans));
    // Write the sandesh header
    int ret = header.write(prot);
    EXPECT_GT(ret, 0);
    EXPECT_GT(length, offset + ret);
    // Get the buffer
    uint8_t *hbuffer;
    uint32_t hlen;
    btrans->getBuffer(&hbuffer, &hlen);
    EXPECT_EQ(hlen, ret);
    memcpy(data + offset, hbuffer, hlen);
    offset += hlen;
    EXPECT_GT(length, offset + FakeMessageSandeshBegin.size());
    memcpy(data + offset, FakeMessageSandeshBegin.c_str(),
            FakeMessageSandeshBegin.size());
    offset += FakeMessageSandeshBegin.size();
    memset(data + offset, '0', length - offset - FakeMessageSandeshEnd.size());
    offset += length - offset - FakeMessageSandeshEnd.size();
    memcpy(data + offset, FakeMessageSandeshEnd.c_str(),
           FakeMessageSandeshEnd.size());
    offset += FakeMessageSandeshEnd.size();
    EXPECT_EQ(offset, length);
}
