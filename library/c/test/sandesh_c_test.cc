/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_c_test.cc
//
// Sandesh C Test
//

#include <inttypes.h>
#include <fstream>

#include "testing/gunit.h"
#include <base/logging.h>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh_constants.h>
#include <sandesh/sandesh.h>

#include "sandesh/library/c/sandesh.h"
#include "gen-c/sandesh_buffer_test_types.h"
#include "sandesh_c_test_types.h"

namespace {
class SandeshCTest : public ::testing::Test {
public:
    static void incr_decoded() {
        num_decoded_++;
    }

protected:
    SandeshCTest() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

    static int num_decoded_;
    static int num_encoded_;
};

int SandeshCTest::num_decoded_ = 0;
int SandeshCTest::num_encoded_ = 0;

TEST_F(SandeshCTest, ReadWrite) {
    abc wabc, rabc;
    u_int8_t buf[256];
    ThriftBinaryProtocol protocol;
    ThriftMemoryBuffer transport;
    int error = 0, wxfer, rxfer;
    std::string inner_h("abc");
    uuid_t uuid_test =
           {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
            0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};

    /* Initialize the transport and protocol */
    memset(buf, 0, sizeof(buf));
    thrift_memory_buffer_init(&transport, buf, sizeof(buf));
    thrift_protocol_init(&protocol, T_PROTOCOL_BINARY, (ThriftTransport *)&transport);

    /* Write the struct */
    wabc.nested = (abc_inner *)calloc(1, sizeof(*wabc.nested));
    wabc.nested->inner_a = 3;
    wabc.nested->inner_b = 4;
    wabc.nested->inner_c = 5;
    wabc.nested->inner_d = ABC_ENUM_VAL2;
    wabc.nested->inner_e = 65535;
    wabc.nested->inner_f = 4294967295u;
    wabc.nested->inner_g = 18446744073709551615ull;
    wabc.nested->inner_h = const_cast<char *>(inner_h.c_str());
    wabc.nested->inner_i = 4294967295u;
    memcpy(wabc.nested->inner_j, uuid_test, sizeof(uuid_t));
    wxfer = abc_write(&wabc, &protocol, &error);
    EXPECT_GT(wxfer, 0);

    /* Read the struct */
    rxfer = abc_read(&rabc, &protocol, &error);
    EXPECT_GT(rxfer, 0);

    /* Verify */
    EXPECT_EQ(wxfer, rxfer);
    EXPECT_EQ(wabc.nested->inner_a, rabc.nested->inner_a);
    EXPECT_EQ(wabc.nested->inner_b, rabc.nested->inner_b);
    EXPECT_EQ(wabc.nested->inner_c, rabc.nested->inner_c);
    EXPECT_EQ(wabc.nested->inner_d, rabc.nested->inner_d);
    EXPECT_EQ(wabc.nested->inner_e, rabc.nested->inner_e);
    EXPECT_EQ(wabc.nested->inner_f, rabc.nested->inner_f);
    EXPECT_EQ(wabc.nested->inner_g, rabc.nested->inner_g);
    EXPECT_STREQ(wabc.nested->inner_h, rabc.nested->inner_h);
    EXPECT_EQ(wabc.nested->inner_i, rabc.nested->inner_i);
    EXPECT_EQ(0, memcmp(wabc.nested->inner_j,
        rabc.nested->inner_j, sizeof(uuid_t)));

    free(wabc.nested);
    wabc.nested = NULL;
    free(rabc.nested->inner_h);
    rabc.nested->inner_h = NULL;
    free(rabc.nested);
    rabc.nested = NULL;
}

TEST_F(SandeshCTest, EncodeDecode) {
    abc_sandesh wabc_sandesh;
    int error = 0, wxfer, wxfer1, rxfer;
    u_int32_t i;
    u_int8_t buf[256];
    std::string inner_h("abc");
    uuid_t uuid_test =
           {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
            0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};

    memset(buf, 0, sizeof(buf));
    wabc_sandesh.elem = (abc *)calloc(1, sizeof(*wabc_sandesh.elem));
    wabc_sandesh.elem->nested = (abc_inner *)calloc(1, sizeof(*wabc_sandesh.elem->nested));
    wabc_sandesh.elem->nested->inner_a = 3;
    wabc_sandesh.elem->nested->inner_b = 4;
    wabc_sandesh.elem->nested->inner_c = 5;
    wabc_sandesh.elem->nested->inner_d = ABC_ENUM_VAL1;
    wabc_sandesh.elem->nested->inner_e = 65535;
    wabc_sandesh.elem->nested->inner_f = 4294967295u;
    wabc_sandesh.elem->nested->inner_g = 18446744073709551615ull;
    wabc_sandesh.elem->nested->inner_h = const_cast<char *>(inner_h.c_str());
    wabc_sandesh.elem->nested->inner_i = 4294967295u;
    memcpy(wabc_sandesh.elem->nested->inner_j, uuid_test, sizeof(uuid_t));
    wabc_sandesh.rwinfo_size = 5;
    wabc_sandesh.rwinfo = (int8_t *)calloc(1, sizeof(*wabc_sandesh.rwinfo) *
            wabc_sandesh.rwinfo_size);
    for (i = 0; i < wabc_sandesh.rwinfo_size; i++) {
        wabc_sandesh.rwinfo[i] = i;
    }

    wxfer = sandesh_encode(&wabc_sandesh, "abc_sandesh",
                           sandesh_c_test_find_sandesh_info,
                           buf, sizeof(buf), &error);
    EXPECT_GT(wxfer, 0);
    EXPECT_EQ(error, 0);
    num_encoded_++;

    wxfer1 = sandesh_encode(&wabc_sandesh, "abc_sandesh",
                            sandesh_c_test_find_sandesh_info,
                            buf + wxfer, sizeof(buf) - wxfer,
                            &error);
    EXPECT_GT(wxfer1, 0);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(wxfer, wxfer1);
    num_encoded_++;
    wxfer += wxfer1;

    rxfer = sandesh_decode(buf, wxfer,
                           sandesh_c_test_find_sandesh_info,
                           &error);
    EXPECT_GT(rxfer, 0);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(wxfer, rxfer);
    EXPECT_EQ(num_decoded_, num_encoded_);

    free(wabc_sandesh.elem->nested);
    wabc_sandesh.elem->nested = NULL;
    free(wabc_sandesh.elem);
    wabc_sandesh.elem = NULL;
    free(wabc_sandesh.rwinfo);
    wabc_sandesh.rwinfo = NULL;
}

TEST_F(SandeshCTest, EncodeDecodeC2CPlus) {
    std::ifstream file("buffersandesh.data", std::ios_base::in |
            std::ios_base::binary);
    // get length of file:
    file.seekg (0, std::ios::end);
    int wxfer = file.tellg();
    EXPECT_GT(wxfer, 0);
    file.seekg (0, std::ios::beg);
    // allocate memory:
    u_int8_t *buf = new u_int8_t [wxfer];
    // read data as a block:
    file.read((char *)buf, wxfer);
    file.close();
    // Decode in C
    int error = 0;
    int rxfer = sandesh_decode(buf, wxfer,
                               sandesh_buffer_test_find_sandesh_info,
                               &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(rxfer, wxfer);
    delete [] buf;
}

TEST_F(SandeshCTest, GetEncodeLength) {

    abc_sandesh wabc_sandesh;
    int error = 0, wxfer, exfer;
    u_int32_t i;
    u_int8_t buf[256];
    std::string inner_h("abc");
    uuid_t uuid_test =
           {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
            0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};

    memset(buf, 0, sizeof(buf));

    wabc_sandesh.elem = (abc *)calloc(1, sizeof(*wabc_sandesh.elem));
    wabc_sandesh.elem->nested = (abc_inner *)calloc(1, sizeof(*wabc_sandesh.elem->nested));
    wabc_sandesh.elem->nested->inner_a = 3;
    wabc_sandesh.elem->nested->inner_b = 4;
    wabc_sandesh.elem->nested->inner_c = 5;
    wabc_sandesh.elem->nested->inner_d = ABC_ENUM_VAL1;
    wabc_sandesh.elem->nested->inner_e = 65535;
    wabc_sandesh.elem->nested->inner_f = 4294967295u;
    wabc_sandesh.elem->nested->inner_g = 18446744073709551615ull;
    wabc_sandesh.elem->nested->inner_h = const_cast<char *>(inner_h.c_str());
    wabc_sandesh.elem->nested->inner_i = 4294967295u;
    memcpy(wabc_sandesh.elem->nested->inner_j, uuid_test, sizeof(uuid_t));
    wabc_sandesh.rwinfo_size = 5;
    wabc_sandesh.rwinfo = (int8_t *)calloc(1, sizeof(*wabc_sandesh.rwinfo) *
            wabc_sandesh.rwinfo_size);
    for (i = 0; i < wabc_sandesh.rwinfo_size; i++) {
        wabc_sandesh.rwinfo[i] = i;
    }

    // Get the encode length first
    exfer = sandesh_get_encoded_length(&wabc_sandesh, "abc_sandesh",
            sandesh_c_test_find_sandesh_info, &error);
    EXPECT_GT(exfer, 0);
    EXPECT_EQ(error, 0);

    // Now write the sandesh
    wxfer = sandesh_encode(&wabc_sandesh, "abc_sandesh",
            sandesh_c_test_find_sandesh_info,
            buf, sizeof(buf), &error);
    EXPECT_GT(wxfer, 0);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(wxfer, exfer);

    free(wabc_sandesh.elem->nested);
    wabc_sandesh.elem->nested = NULL;
    free(wabc_sandesh.elem);
    wabc_sandesh.elem = NULL;
    free(wabc_sandesh.rwinfo);
    wabc_sandesh.rwinfo = NULL;
}
} // namespace

void
abc_sandesh_process (void *pabc) {
    u_int32_t i;
    uuid_t uuid_value =
           {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
            0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    abc_sandesh *psandesh = (abc_sandesh *)pabc;
    assert(psandesh);
    EXPECT_EQ(psandesh->elem->nested->inner_a, 3);
    EXPECT_EQ(psandesh->elem->nested->inner_b, 4);
    EXPECT_EQ(psandesh->elem->nested->inner_c, 5);
    EXPECT_EQ(psandesh->elem->nested->inner_d, ABC_ENUM_VAL1);
    EXPECT_EQ(psandesh->elem->nested->inner_e, 65535);
    EXPECT_EQ(psandesh->elem->nested->inner_f, 4294967295u);
    EXPECT_EQ(psandesh->elem->nested->inner_g, 18446744073709551615ull);
    EXPECT_STREQ(psandesh->elem->nested->inner_h, "abc");
    EXPECT_EQ(psandesh->elem->nested->inner_i, 4294967295u);
    EXPECT_EQ(0, memcmp(psandesh->elem->nested->inner_j,
         uuid_value, sizeof(uuid_t)));

    for (i = 0; i < psandesh->rwinfo_size; i++) {
        EXPECT_EQ(psandesh->rwinfo[i], i);
    }
    SandeshCTest::incr_decoded();
}

void
buffer_test_process (void *ptr) {
    int i;
    uuid_t uuid_value =
           {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
            0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    BufferTest *psandesh = (BufferTest *)ptr;
    assert(psandesh);
    EXPECT_EQ(psandesh->i32Elem1, 1);
    EXPECT_STREQ(psandesh->stringElem1, "test");
    EXPECT_EQ(psandesh->i64Elem1, 1);
    for (i = 0; i < 5; i++) {
        EXPECT_EQ(psandesh->listElem1[i], i);
        EXPECT_EQ(psandesh->listElem2[i], i);
        uuid_t uuid_temp =
               {0x00+i,0x00+i,0x01+i,0x01+i,0x02+i,0x02+i,0x03+i,0x03+i,
                0x04+i,0x04+i,0x05+i,0x05+i,0x06+i,0x06+i,0x07+i,0x07+i};
        EXPECT_EQ(0, memcmp(psandesh->listElem3[i],
             uuid_temp, sizeof(uuid_t)));
    }
    EXPECT_EQ(psandesh->u16Elem1, 65535);
    EXPECT_EQ(psandesh->u32Elem1, 4294967295u);
    EXPECT_EQ(psandesh->u64Elem1, 18446744073709551615ull);
    EXPECT_STREQ(psandesh->xmlElem1, "xmlElem1");
    EXPECT_EQ(psandesh->ipv4Elem1, 4294967295u);
    EXPECT_EQ(0, memcmp(psandesh->uuidElem1, uuid_value, sizeof(uuid_t)));
}

void buffer_update_test_process (void *ptr) {
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return success;
}
