/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_rw_test.cc
//
// Sandesh Read-Write Test
//

#include <map>
#include <arpa/inet.h>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/range/iterator_range.hpp>

#include "testing/gunit.h"

#include "base/logging.h"
#include "base/util.h"

#include <sandesh/protocol/TXMLProtocol.h>
#include <sandesh/protocol/TBinaryProtocol.h>
#include <sandesh/transport/TBufferTransports.h>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include "sandesh_rw_test_types.h"

using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;

static const int32_t test_i32 = 0xdeadbeef;
static const uint32_t test_list_size = 5;

class SandeshReadWriteUnitTest : public ::testing::Test {
protected:
    SandeshReadWriteUnitTest() {
    }

    void SandeshReadWriteProcess(boost::shared_ptr<TMemoryBuffer> btrans,
            boost::shared_ptr<TProtocol> prot) {
        uint8_t *buffer;
        uint32_t offset, wxfer, rxfer;
        // Initialize the struct
        wstruct_test_.set_i32Test(test_i32);
        // Create a vector of SandeshStructTest
        std::vector<SandeshListTestElement> test_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            SandeshListTestElement tmp;
            tmp.set_i32Elem(i);
            test_list.push_back(tmp); // 0, 1, 2, 3, 4
        }
        wstruct_test_.set_listTest(test_list);
        // Create a vector of int32_t
        std::vector<int32_t> test_basic_type_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            test_basic_type_list.push_back(i); // 0, 1, 2, 3, 4
        }
        wstruct_test_.set_basicTypeListTest(test_basic_type_list);
        // Create a map of <int32_t, string>
        std::map<int32_t, std::string> test_basic_type_map;
        for (uint32_t i = 0; i < test_list_size; i++) {
            // (0, "0"), (1, "1"), (2, "2"), (3, "3"), (4, "4")
            test_basic_type_map.insert(std::pair<int32_t, std::string>
                                       (i, integerToString(i)));
        }
        wstruct_test_.set_basicTypeMapTest(test_basic_type_map);
        // Create a map of <int32_t, SandeshListTestElement>
        std::map<int32_t, SandeshListTestElement> test_complex_type_map;
        for (uint32_t i = 0; i < test_list_size; i++) {
            SandeshListTestElement tmp;
            tmp.set_i32Elem(i);
            test_complex_type_map.insert(std::pair<int32_t, SandeshListTestElement>
                                         (i, tmp));
        }
        wstruct_test_.set_complexTypeMapTest(test_complex_type_map);
        wstruct_test_.set_u16Test(65535);
        wstruct_test_.set_u32Test(4294967295u);
        wstruct_test_.set_u64Test(18446744073709551615ull);
        wstruct_test_.set_xmlTest("<abc>");
        wstruct_test_.set_xmlTest1("abc");
        wstruct_test_.set_xmlTest2("ab]");
        wstruct_test_.set_xmlTest3("abc]]");
        // Write the struct
        wxfer = wstruct_test_.write(prot);
        // Get the buffer
        btrans->getBuffer(&buffer, &offset);
        EXPECT_EQ(wxfer, offset);
        // Read the struct
        rxfer = rstruct_test_.read(prot);
        // Now compare
        EXPECT_EQ(rxfer, wxfer);
        EXPECT_EQ(wstruct_test_, rstruct_test_);
    }

    SandeshStructTest wstruct_test_;
    SandeshStructTest rstruct_test_;
};


TEST_F(SandeshReadWriteUnitTest, StructXMLReadWrite) {
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(4096));
    boost::shared_ptr<TXMLProtocol> prot =
            boost::shared_ptr<TXMLProtocol>(
                    new TXMLProtocol(btrans));
    SandeshReadWriteProcess(btrans, prot);
}

TEST_F(SandeshReadWriteUnitTest, StructBinaryReadWrite) {
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(4096));
    boost::shared_ptr<TBinaryProtocol> prot =
            boost::shared_ptr<TBinaryProtocol>(
                    new TBinaryProtocol(btrans));
    SandeshReadWriteProcess(btrans, prot);
}

class SandeshPerfTest : public ::testing::Test {
protected:
    SandeshPerfTest() :
        xml_("str1 type=\"string\" identifier=\"1\""),
        cmp_("nomatch") {
    }

    std::string xml_;
    std::string cmp_;
};

TEST_F(SandeshPerfTest, DISABLED_PerfTestTokenizer) {
    typedef boost::tokenizer<boost::char_separator<char> >
        tokenizer;
    for (int i = 0; i < 10000; i++) {
        boost::char_separator<char> sep("=\" ");
        tokenizer tokens(xml_, sep);
        for (tokenizer::iterator it = tokens.begin();
                it != tokens.end(); ++it) {
            if (*it == cmp_) {
            }
        }
    }
}

TEST_F(SandeshPerfTest, DISABLED_PerfTestSplit) {
    typedef std::vector<boost::iterator_range<std::string::iterator> > split_vector_type;
    for (int i = 0; i < 10000; i++) {
        split_vector_type tokens;
        boost::algorithm::split(tokens, xml_, boost::is_any_of("=\" "), boost::token_compress_on);
        for (split_vector_type::const_iterator it = tokens.begin();
                it != tokens.end(); ++it) {
            if (boost::copy_range<std::string>(*it) == cmp_) {
            }
        }
    }
}


TEST_F(SandeshPerfTest, DISABLED_PerfTestSplitIter) {
    typedef boost::algorithm::split_iterator<std::string::iterator> string_split_iterator;
    for (int i = 0; i < 10000; i++) {
        for (string_split_iterator it = 
                 make_split_iterator(xml_, 
                                     token_finder(boost::is_any_of("=\" "), 
                                                  boost::token_compress_on));
             it != string_split_iterator(); ++it) {
            if (*it == cmp_) {
            }
        }
    }
}

class SandeshLogUnitTest : public ::testing::Test {
protected:
    SandeshLogUnitTest() {
    }

    SandeshLogTest ltest_;
};

TEST_F(SandeshLogUnitTest, Basic) {
    ltest_.set_byteTest(255);
    ltest_.set_byteTest1(120);
    ltest_.set_boolTest(true);
    std::string buf = ltest_.log();
    EXPECT_STREQ("byteTest = -1 byteTest1 = 120 boolTest = 1", buf.c_str());
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return success;
}
