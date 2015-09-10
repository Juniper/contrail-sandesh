/*
 * Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_send_queue_test.cc
//
// Sandesh send queue Test
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

#include <sandesh/sandesh.h>
#include <sandesh/sandesh_types.h>
#include "sandesh_send_queue_test_types.h"

using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;

static const int32_t test_i32 = 0xdeadbeef;
static const uint32_t test_list_size = 5;

static const int kQueueSize = 200 * 1024 * 1024;

class SandeshSendQueueTest : public ::testing::Test  {
protected:
    virtual void SetUp() {
        sandesh_queue_.reset(new Sandesh::SandeshQueue(TaskScheduler::
            GetInstance()->GetTaskId("sandesh::Test::SendQueueTest"
            "::sandesh_queue"),
            Task::kTaskInstanceAny,
                boost::bind(&SandeshSendQueueTest::DequeueEvent, this, _1),
                kQueueSize));
    }

    //Create a structure of size 278 bytes
    void CreateMessages() {
        uint8_t *buffer;
        uint32_t offset, wxfer, rxfer;
        // Initialize the struct
        msg1.set_i32Test(test_i32); //4 bytes
        // Create a vector of SandeshStructTest
        std::vector<SandeshListTestElement> test_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            SandeshListTestElement tmp;
            tmp.set_i32Elem(i);
            test_list.push_back(tmp); // 0, 1, 2, 3, 4
        }
        msg1.set_listTest(test_list);//20 bytes
        // Create a vector of int32_t
        std::vector<int32_t> test_basic_type_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            test_basic_type_list.push_back(i); // 0, 1, 2, 3, 4
        }
        msg1.set_basicTypeListTest(test_basic_type_list); //20 bytes
        // Create a vector of uuid
        std::vector<boost::uuids::uuid> test_uuid_list;
        for (uint32_t i = 0; i < test_list_size; i++) {
            boost::uuids::uuid uuid_temp =
                {0x00+i,0x00+i,0x01+i,0x01+i,0x02+i,0x02+i,0x03+i,0x03+i,
                 0x04+i,0x04+i,0x05+i,0x05+i,0x06+i,0x06+i,0x07+i,0x07+i};
            test_uuid_list.push_back(uuid_temp);
        }
        msg1.set_uuidListTest(test_uuid_list); //80 bytes
        // Create a map of <int32_t, string>
        std::map<int32_t, std::string> test_basic_type_map;
        for (uint32_t i = 0; i < test_list_size; i++) {
            // (0, "0"), (1, "1"), (2, "2"), (3, "3"), (4, "4")
            test_basic_type_map.insert(std::pair<int32_t, std::string>
                                       (i, integerToString(i)));
        }
        msg1.set_basicTypeMapTest(test_basic_type_map);//25 bytes
        // Create a map of <int32_t, SandeshListTestElement>
        std::map<int32_t, SandeshListTestElement> test_complex_type_map;
        for (uint32_t i = 0; i < test_list_size; i++) {
            SandeshListTestElement tmp;
            tmp.set_i32Elem(i);
            test_complex_type_map.insert(std::pair<int32_t, SandeshListTestElement>
                                         (i, tmp));
        }
        msg1.set_complexTypeMapTest(test_complex_type_map);//40 bytes
        msg1.set_u16Test(65535);//2 bytes
        msg1.set_u32Test(4294967295u);//4 bytes
        msg1.set_u64Test(18446744073709551615ull);//8 bytes
        msg1.set_ipv4Test(4294967295u);//4 bytes
        boost::uuids::uuid uuid_test =
             {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
              0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};//16 bytes
        msg1.set_uuidTest(uuid_test);
        msg1.set_xmlTest("<abc>");//5 bytes
        msg1.set_xmlTest1("abc");//3 bytes
        msg1.set_xmlTest2("ab]");//3 bytes
        msg1.set_xmlTest3("abc]]");//5 bytes
    }

    virtual void TearDown() {
    }

    bool DequeueEvent(Element element) {
        element.snh->Release();
        return true;
    }

    boost::scoped_ptr<Sandesh::SandeshQueue> sandesh_queue_;
    SandeshStructTest msg1;
    SandeshStructTest msg2;
};

/*
 * Check if GetSize function works for the given sandesh
 */
TEST_F(SandeshSendQueueTest, SizeTest) {
    CreateMessages();
    SandeshResponseTest *sandesh = new SandeshResponseTest();
    sandesh->set_data(msg1);
    EXPECT_EQ(sandesh->GetSize(), 278);
    sandesh->Release();
}

/*
 * Check if sandesh queue size increases upon sending the message
 */
TEST_F(SandeshSendQueueTest, EnqueuTest) {
    CreateMessages();
    SandeshResponseTest *sandesh = new SandeshResponseTest();
    sandesh->set_data(msg1);
    //Intial size of queue is 0
    EXPECT_EQ(sandesh_queue_->Length(), 0);
    Element element_snd, element_rcv;
    element_snd.snh = sandesh;
    sandesh_queue_->Enqueue(element_snd);
    //Size of queue after enqueue
    EXPECT_EQ(sandesh_queue_->Length(), element_snd.GetSize());
    sandesh_queue_->Dequeue(&element_rcv);
    //Size of queue after dequeue
    EXPECT_EQ(sandesh_queue_->Length(), 0);
    sandesh->Release();
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    return success;
}
