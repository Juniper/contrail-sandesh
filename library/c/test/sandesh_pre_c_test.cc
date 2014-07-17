/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_pre_c_test.cc
//
// Prerequisite for running Sandesh C Test
//

#include <fstream>

#include "testing/gunit.h"
#include <base/logging.h>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh_constants.h>
#include <sandesh/sandesh.h>

#include "sandesh_buffer_test_types.h"

namespace {
class SandeshPreCTest : public ::testing::Test {
protected:
    SandeshPreCTest() {
        memset(buf_, 0, sizeof(buf_));
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

    u_int8_t buf_[512];
};

TEST_F(SandeshPreCTest, EncodeDecodeC2CPlus) {
    // First encode in C++
    BufferTest test;
    std::vector<int8_t> rwinfo;
    std::vector<uint32_t> u32_list;
    std::vector<boost::uuids::uuid> uuid_list;
    int32_t wxfer, wxfer1, error = 0;
    // Populate test
    test.set_i32Elem1(1);
    test.set_stringElem1("test");
    for (int i = 0; i < 5; i++) {
        rwinfo.push_back(i);
        u32_list.push_back(i);
        boost::uuids::uuid temp;
        for (int j = 0; j < 8; j++) {
            temp.data[2*j] = i + j;
            temp.data[2*j + 1] = i + j;
        }
        uuid_list.push_back(temp);
    }
    test.set_listElem1(rwinfo);
    test.set_i64Elem1(1);
    test.set_u16Elem1(65535);
    test.set_u32Elem1(4294967295u);
    test.set_listElem2(u32_list);
    test.set_u64Elem1(18446744073709551615ull);
    test.set_xmlElem1("xmlElem1");
    test.set_ipv4Elem1(4294967295u);
    boost::uuids::uuid uuid_test = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    test.set_uuidElem1(uuid_test);
    test.set_listElem3(uuid_list);
    // Write test to buf
    wxfer = test.WriteBinary(buf_, sizeof(buf_), &error);
    EXPECT_GT(wxfer, 0);
    EXPECT_EQ(error, 0);
    // Write test to buf again
    wxfer1 = test.WriteBinary(buf_ + wxfer, sizeof(buf_) - wxfer, &error);
    EXPECT_GT(wxfer1, 0);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(wxfer, wxfer1);
    wxfer += wxfer1;
    // Write buffer to file so that sandesh C library can use it for decoding
    std::ofstream file("buffersandesh.data", std::ios::binary);
    file.write((const char *)buf_, wxfer);
    file.close();
}
} // namespace

void BufferTest::Process(SandeshContext *context) {
}

void BufferUpdateTest::Process(SandeshContext *context) {
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return success;
}
