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

static int32_t inner_a(3);
static int16_t inner_b(4);
static int64_t inner_c(5);
static abc_enum inner_d(ABC_ENUM_VAL2);
static uint16_t inner_e(65535);
static uint32_t inner_f(4294967295u);
static uint64_t inner_g(18446744073709551615ull);
static std::string inner_h("abc");
static uint32_t inner_i(4294967295u);
static uuid_t inner_j = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
static std::string inner_k("20.1.1.2");
static std::string inner_l("2001:abce::4");
static std::string inner_m("def");

static void PopulateAbcInner(abc_inner *inner) {
    inner->inner_a = inner_a;
    inner->inner_b = inner_b;
    inner->inner_c = inner_c;
    inner->inner_d = inner_d;
    inner->inner_e = inner_e;
    inner->inner_f = inner_f;
    inner->inner_g = inner_g;
    inner->inner_h = const_cast<char *>(inner_h.c_str());
    inner->inner_i = inner_i;
    memcpy(inner->inner_j, inner_j, sizeof(inner_j));
    inner->inner_k.iptype = AF_INET;
    int success(inet_pton(AF_INET, inner_k.c_str(), &inner->inner_k.ipv4));
    EXPECT_EQ(success, 1);
    inner->inner_l.iptype = AF_INET6;
    success = inet_pton(AF_INET6, inner_l.c_str(), &inner->inner_l.ipv6);
    EXPECT_EQ(success, 1);
    inner->inner_m = const_cast<char *>(inner_m.c_str());
    inner->inner_n = NULL;
}

static void ValidateAbcInner(abc_inner *inner) {
    EXPECT_EQ(inner->inner_a, inner_a);
    EXPECT_EQ(inner->inner_b, inner_b);
    EXPECT_EQ(inner->inner_c, inner_c);
    EXPECT_EQ(inner->inner_d, inner_d);
    EXPECT_EQ(inner->inner_e, inner_e);
    EXPECT_EQ(inner->inner_f, inner_f);
    EXPECT_EQ(inner->inner_g, inner_g);
    EXPECT_STREQ(inner->inner_h, inner_h.c_str());
    EXPECT_EQ(inner->inner_i, inner_i);
    EXPECT_EQ(0, memcmp(inner->inner_j,
        inner_j, sizeof(uuid_t)));
    ipaddr_t inner_k_v4;
    inner_k_v4.iptype = AF_INET;
    int success(inet_pton(AF_INET, inner_k.c_str(), &inner_k_v4.ipv4));
    EXPECT_EQ(success, 1);
    EXPECT_EQ(inner->inner_k.iptype, inner_k_v4.iptype);
    EXPECT_EQ(0, memcmp(&inner->inner_k.ipv4,
                        &inner_k_v4.ipv4, 4));
    ipaddr_t inner_l_v6;
    inner_l_v6.iptype = AF_INET6;
    success = inet_pton(AF_INET6, inner_l.c_str(), &inner_l_v6.ipv6);
    EXPECT_EQ(success, 1);
    EXPECT_EQ(inner->inner_l.iptype, inner_l_v6.iptype);
    EXPECT_EQ(0, memcmp(&inner->inner_l.ipv6,
                        &inner_l_v6.ipv6, 16));
    EXPECT_STREQ(inner->inner_m, inner_m.c_str());
    EXPECT_TRUE(inner->inner_n == NULL);
}

static void ValidateAbc(abc *abc) {
    ValidateAbcInner(abc->nested);
}

static void ValidateAbcSandesh(abc_sandesh *abc_sandesh) {
    ValidateAbc(abc_sandesh->elem);
    EXPECT_EQ(abc_sandesh->rwinfo_size, 5);
    for (int i = 0; i < abc_sandesh->rwinfo_size; i++) {
        EXPECT_EQ(abc_sandesh->rwinfo[i], i);
    }
}

static void CompareAbcInner(abc_inner *actual, abc_inner *expected) {
    EXPECT_EQ(actual->inner_a, expected->inner_a);
    EXPECT_EQ(actual->inner_b, expected->inner_b);
    EXPECT_EQ(actual->inner_c, expected->inner_c);
    EXPECT_EQ(actual->inner_d, expected->inner_d);
    EXPECT_EQ(actual->inner_e, expected->inner_e);
    EXPECT_EQ(actual->inner_f, expected->inner_f);
    EXPECT_EQ(actual->inner_g, expected->inner_g);
    EXPECT_STREQ(actual->inner_h, expected->inner_h);
    EXPECT_EQ(actual->inner_i, expected->inner_i);
    EXPECT_EQ(0, memcmp(actual->inner_j,
        expected->inner_j, sizeof(uuid_t)));
    EXPECT_EQ(actual->inner_k.iptype, expected->inner_k.iptype);
    EXPECT_EQ(0, memcmp(&actual->inner_k.ipv4,
                        &expected->inner_k.ipv4, 4));
    EXPECT_EQ(actual->inner_l.iptype, expected->inner_l.iptype);
    EXPECT_EQ(0, memcmp(&actual->inner_l.ipv6,
                        &expected->inner_l.ipv6, 16));
    EXPECT_STREQ(actual->inner_m, expected->inner_m);
    EXPECT_STREQ(actual->inner_n, expected->inner_n);
}

static void CompareAbc(abc *actual, abc *expected) {
    CompareAbcInner(actual->nested, expected->nested);
}

static bool CompareAbcSandesh(abc_sandesh *actual, abc_sandesh *expected) {
    CompareAbc(actual->elem, expected->elem);
    EXPECT_EQ(actual->rwinfo_size, expected->rwinfo_size);
    for (int i = 0; i < actual->rwinfo_size; i++) {
        EXPECT_EQ(actual->rwinfo[i], expected->rwinfo[i]);
    }
}

TEST_F(SandeshCTest, ReadWrite) {
    abc wabc, rabc;
    u_int8_t buf[256];
    ThriftBinaryProtocol protocol;
    ThriftMemoryBuffer transport;
    int error = 0, wxfer, rxfer;

    /* Initialize the transport and protocol */
    thrift_memory_buffer_init(&transport, buf, sizeof(buf));
    thrift_protocol_init(&protocol, T_PROTOCOL_BINARY, (ThriftTransport *)&transport);

    /* Write the struct */
    wabc.nested = (abc_inner *)calloc(1, sizeof(*wabc.nested));
    PopulateAbcInner(wabc.nested);
    wxfer = abc_write(&wabc, &protocol, &error);
    EXPECT_GT(wxfer, 0);

    /* Read the struct */
    rxfer = abc_read(&rabc, &protocol, &error);
    EXPECT_GT(rxfer, 0);

    /* Verify */
    EXPECT_EQ(wxfer, rxfer);
    CompareAbcInner(rabc.nested, wabc.nested);

    free(wabc.nested);
    wabc.nested = NULL;
    abc_free(&rabc);
}

TEST_F(SandeshCTest, EncodeDecode) {
    abc_sandesh wabc_sandesh;
    int error = 0, wxfer, wxfer1, rxfer;
    u_int32_t i;
    u_int8_t buf[512];

    memset(buf, 0, sizeof(buf));
    wabc_sandesh.elem = (abc *)calloc(1, sizeof(*wabc_sandesh.elem));
    wabc_sandesh.elem->nested = (abc_inner *)calloc(1, sizeof(*wabc_sandesh.elem->nested));
    PopulateAbcInner(wabc_sandesh.elem->nested);
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

    memset(buf, 0, sizeof(buf));

    wabc_sandesh.elem = (abc *)calloc(1, sizeof(*wabc_sandesh.elem));
    wabc_sandesh.elem->nested = (abc_inner *)calloc(1, sizeof(*wabc_sandesh.elem->nested));
    PopulateAbcInner(wabc_sandesh.elem->nested);
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

TEST_F(SandeshCTest, SandeshEncodePerf) {
    abc_inner inner;
    PopulateAbcInner(&inner);
    u_int8_t buf[512];
    ThriftBinaryProtocol protocol;
    ThriftMemoryBuffer transport;
    thrift_protocol_init(&protocol, T_PROTOCOL_BINARY, (ThriftTransport *)&transport);
    for (int i = 0; i < 1000000; i++) {
        /* Initialize the transport and protocol */
        thrift_memory_buffer_init(&transport, buf, sizeof(buf));
        int error(0);
        int32_t xfer(abc_inner_write(&inner, &protocol, &error));
        EXPECT_EQ(error, 0);
        int32_t xfer1(abc_inner_write(&inner, &protocol, &error));
        EXPECT_EQ(error, 0);
    }
}

TEST_F(SandeshCTest, SandeshDecodePerf) {
    abc_inner winner;
    PopulateAbcInner(&winner);
    u_int8_t buf[256];
    ThriftBinaryProtocol protocol;
    ThriftMemoryBuffer transport;
    thrift_memory_buffer_init(&transport, buf, sizeof(buf));
    thrift_protocol_init(&protocol, T_PROTOCOL_BINARY, (ThriftTransport *)&transport);
    int error(0);
    int32_t wxfer(abc_inner_write(&winner, &protocol, &error));
    ASSERT_GT(wxfer, 0);
    ASSERT_EQ(error, 0);

    for (int i = 0; i < 10; i++) {
        abc_inner rinner;
        thrift_memory_buffer_init(&transport, buf, sizeof(buf));
        thrift_protocol_init(&protocol, T_PROTOCOL_BINARY, (ThriftTransport *)&transport);
        thrift_memory_buffer_wrote_bytes(&transport, wxfer);
        int32_t rxfer(abc_inner_read(&rinner, &protocol, &error));
        ASSERT_EQ(error, 0);
        ASSERT_EQ(rxfer, wxfer);
        abc_inner_free(&rinner);
    }
}

TEST_F(SandeshCTest, SandeshEncodeBufferPerf) {
    abc_inner inner;
    PopulateAbcInner(&inner);
    u_int8_t buf[512];
    for (int i = 0; i < 1000000; i++) {
        int error(0);
        int32_t offset(abc_inner_write_binary_to_buffer(&inner, buf,
            sizeof(buf), &error));
        EXPECT_GT(offset, 0);
        int32_t offset1(abc_inner_write_binary_to_buffer(&inner, buf + offset,
            sizeof(buf) - offset, &error));
        EXPECT_GT(offset1, 0);
    }
}

TEST_F(SandeshCTest, SandeshDecodeBufferPerf) {
    abc_inner winner;
    PopulateAbcInner(&winner);
    u_int8_t buf[256];
    int error(0);
    int32_t wxfer(abc_inner_write_binary_to_buffer(&winner, buf, sizeof(buf),
        &error));
    ASSERT_GT(wxfer, 0);
    ASSERT_EQ(error, 0);

    for (int i = 0; i < 10; i++) {
        abc_inner rinner;
        int32_t rxfer(abc_inner_read_binary_from_buffer(&rinner, buf, wxfer,
            &error));
        ASSERT_EQ(error, 0);
        ASSERT_EQ(rxfer, wxfer);
        abc_inner_free(&rinner);
    }
}

TEST_F(SandeshCTest, SandeshGenBufferValidate) {
    // abc_inner
    // Encode
    abc_inner inner;
    PopulateAbcInner(&inner);
    u_int8_t buf[512];
    memset(buf, 0, sizeof(buf));
    int error(0);
    int32_t offset(abc_inner_write_binary_to_buffer(&inner, buf, sizeof(buf),
        &error));
    EXPECT_EQ(error, 0);
    u_int8_t buf1[512];
    memset(buf1, 0, sizeof(buf1));
    ThriftBinaryProtocol protocol;
    ThriftMemoryBuffer transport;
    thrift_protocol_init(&protocol, T_PROTOCOL_BINARY,
        (ThriftTransport *)&transport);
    thrift_memory_buffer_init(&transport, buf1, sizeof(buf1));
    int error1(0);
    int32_t offset1(abc_inner_write(&inner, &protocol, &error1));
    EXPECT_EQ(error1, 0);
    EXPECT_EQ(offset, offset1);
    EXPECT_EQ(0, memcmp(buf, buf1, sizeof(buf)));
    // Decode
    abc_inner rinner;
    int32_t roffset(abc_inner_read_binary_from_buffer(&rinner, buf, offset,
        &error));
    EXPECT_EQ(error, 0);
    EXPECT_EQ(roffset, offset);
    ValidateAbcInner(&rinner);
    CompareAbcInner(&rinner, &inner);
    abc_inner_free(&rinner);
    // abc
    abc wabc;
    wabc.nested = (abc_inner *)calloc(1, sizeof(*wabc.nested));
    PopulateAbcInner(wabc.nested);
    memset(buf, 0, sizeof(buf));
    offset = abc_write_binary_to_buffer(&wabc, buf, sizeof(buf), &error);
    EXPECT_EQ(error, 0);
    memset(buf1, 0, sizeof(buf1));
    thrift_protocol_init(&protocol, T_PROTOCOL_BINARY,
        (ThriftTransport *)&transport);
    thrift_memory_buffer_init(&transport, buf1, sizeof(buf1));
    offset1 = abc_write(&wabc, &protocol, &error1);
    EXPECT_EQ(error1, 0);
    EXPECT_EQ(offset, offset1);
    EXPECT_EQ(0, memcmp(buf, buf1, sizeof(buf)));
    // Decode
    abc rabc;
    roffset = abc_read_binary_from_buffer(&rabc, buf1, offset1, &error1);
    EXPECT_EQ(error1, 0);
    EXPECT_EQ(roffset, offset1);
    ValidateAbc(&rabc);
    CompareAbc(&rabc, &wabc);
    abc_free(&rabc);
    free(wabc.nested);
    wabc.nested = NULL;
    // abc_sandesh
    abc_sandesh wabc_sandesh;
    int i;
    wabc_sandesh.elem = (abc *)calloc(1, sizeof(*wabc_sandesh.elem));
    wabc_sandesh.elem->nested = (abc_inner *)calloc(1,
        sizeof(*wabc_sandesh.elem->nested));
    PopulateAbcInner(wabc_sandesh.elem->nested);
    wabc_sandesh.rwinfo_size = 5;
    wabc_sandesh.rwinfo = (int8_t *)calloc(1, sizeof(*wabc_sandesh.rwinfo) *
            wabc_sandesh.rwinfo_size);
    for (i = 0; i < wabc_sandesh.rwinfo_size; i++) {
        wabc_sandesh.rwinfo[i] = i;
    }
    memset(buf, 0, sizeof(buf));
    offset = abc_sandesh_write_binary_to_buffer(&wabc_sandesh, buf,
        sizeof(buf), &error);
    EXPECT_EQ(error, 0);
    memset(buf1, 0, sizeof(buf1));
    thrift_protocol_init(&protocol, T_PROTOCOL_BINARY,
        (ThriftTransport *)&transport);
    thrift_memory_buffer_init(&transport, buf1, sizeof(buf1));
    offset1 = abc_sandesh_write(&wabc_sandesh, &protocol, &error1);
    EXPECT_EQ(error1, 0);
    EXPECT_EQ(offset, offset1);
    EXPECT_EQ(0, memcmp(buf, buf1, sizeof(buf)));
    // Decode
    abc_sandesh rabc_sandesh;
    roffset = abc_sandesh_read_binary_from_buffer(&rabc_sandesh, buf, offset,
        &error);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(roffset, offset);
    ValidateAbcSandesh(&rabc_sandesh);
    CompareAbcSandesh(&rabc_sandesh, &wabc_sandesh);
    abc_sandesh_free(&rabc_sandesh);
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
    abc_sandesh *psandesh = (abc_sandesh *)pabc;
    assert(psandesh);
    ValidateAbcInner(psandesh->elem->nested);
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
    ipaddr_t ipaddr1;
    inet_pton(AF_INET, "11.11.11.3", &ipaddr1.ipv4);
    EXPECT_EQ(psandesh->ipaddrElem1.iptype, AF_INET);
    EXPECT_EQ(0, memcmp(&psandesh->ipaddrElem1.ipv4, &ipaddr1.ipv4, 4));
    EXPECT_EQ(psandesh->listElem4_size, 2);
    ipaddr_t lipaddr1, lipaddr2;
    inet_pton(AF_INET6, "2001:ab8::2", &lipaddr1.ipv6);
    EXPECT_EQ(psandesh->listElem4[0].iptype, AF_INET6);
    EXPECT_EQ(0, memcmp(&psandesh->listElem4[0].ipv6, &lipaddr1.ipv6, 16));
    inet_pton(AF_INET, "192.168.12.3", &lipaddr2.ipv4);
    EXPECT_EQ(psandesh->listElem4[1].iptype, AF_INET);
    EXPECT_EQ(0, memcmp(&psandesh->listElem4[1].ipv4, &lipaddr2.ipv4, 4));
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
