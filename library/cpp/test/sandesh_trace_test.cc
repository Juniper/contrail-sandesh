/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_trace_test.cc
//
// Sandesh Trace Test
//

#include "testing/gunit.h"

#include <boost/bind.hpp>

#include <io/event_manager.h>
#include <base/logging.h>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include "sandesh_trace.h"
#include "sandesh_trace_test_types.h"

using boost::asio::ip::address;
using boost::asio::ip::tcp;

class SandeshTraceTest : public ::testing::Test {
public:
    void TraceRead(SandeshTrace *sandesh) {
        EXPECT_EQ(sandesh->type(), SandeshType::TRACE);
        int32_t magicNo;
        SandeshTraceTest1 *strace1 = dynamic_cast<SandeshTraceTest1 *>(sandesh);
        if (!strace1) {
            SandeshTraceTest2 *strace2 = 
                dynamic_cast<SandeshTraceTest2 *>(sandesh);
            if (!strace2) {
                SandeshTraceTest3 *strace3 = 
                    dynamic_cast<SandeshTraceTest3 *>(sandesh);
                ASSERT_TRUE(strace3 != NULL);
                magicNo = strace3->get_magicNo();
            } else {
                magicNo = strace2->get_magicNo();
            }
        } else {
            magicNo = strace1->get_magicNo();
        }
        trace_list_.push_back(magicNo);
    }

protected:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

    std::vector<int> trace_list_; 
    EventManager evm_;
};

TEST_F(SandeshTraceTest, test1) {
    SandeshTraceBufferPtr trace_buf1(SandeshTraceBufferCreate("trace_buf1", 5));
    // Trace object w/o enabling trace
    {
        SandeshTraceDisable();
        SANDESH_TRACE_TEST1_TRACE(trace_buf1, 3, 1);
        SANDESH_TRACE_TEST2_TRACE(trace_buf1, 4, 2);
        SandeshTraceBufferRead(trace_buf1, "test", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(0, trace_list_.size());
    }

    // Enable trace 
    {
        SandeshTraceEnable();
        SANDESH_TRACE_TEST1_TRACE(trace_buf1, 5, 1);
        SANDESH_TRACE_TEST2_TRACE(trace_buf1, 6, 2);
        SandeshTraceBufferRead(trace_buf1, "test", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        SandeshTraceBufferReadDone(trace_buf1, "test");
        EXPECT_EQ(2, trace_list_.size());
        int expected[] = {5, 6};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
    }

    // Disable trace
    {
        trace_list_.clear();
        SandeshTraceDisable();
        SANDESH_TRACE_TEST1_TRACE(trace_buf1, 7, 1);
        SANDESH_TRACE_TEST2_TRACE(trace_buf1, 8, 2);
        SandeshTraceBufferRead(trace_buf1, "test", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        SandeshTraceBufferReadDone(trace_buf1, "test");
        EXPECT_EQ(2, trace_list_.size());
        int expected[] = {5, 6};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
    }

    // Enable trace (after disable)
    SandeshTraceBufferPtr trace_buf2;
    {
        trace_list_.clear();
        SandeshTraceEnable();
        SANDESH_TRACE_TEST1_TRACE(trace_buf1, 10, 1);
        SandeshTraceBufferRead(trace_buf1, "test", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        SandeshTraceBufferReadDone(trace_buf1, "test");
        EXPECT_EQ(3, trace_list_.size());
        int expected[] = {5, 6, 10};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
    }
   
    // Create trace buffer(trace_enable = true)
    {
        trace_list_.clear();
        trace_buf2 = SandeshTraceBufferCreate("trace_buf2", 3, true);
        SANDESH_TRACE_TEST1_TRACE(trace_buf2, 11, 1);
        SANDESH_TRACE_TEST2_TRACE(trace_buf2, 12, 2);
        SANDESH_TRACE_TEST3_TRACE(trace_buf2, 13, 3);
        SandeshTraceBufferRead(trace_buf2, "test", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        SandeshTraceBufferReadDone(trace_buf2, "test");
        EXPECT_EQ(3, trace_list_.size());
        int expected[] = {11, 12, 13};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
    }
    
    // Overwrite trace buffer
    {
        trace_list_.clear();
        SANDESH_TRACE_TEST1_TRACE(trace_buf2, 14, 1);
        SANDESH_TRACE_TEST2_TRACE(trace_buf2, 15, 2);
        SandeshTraceBufferRead(trace_buf2, "test", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        SandeshTraceBufferReadDone(trace_buf2, "test");
        EXPECT_EQ(3, trace_list_.size());
        int expected[] = {13, 14, 15};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
    }

    // Delete trace buffer
    {
        trace_list_.clear();
        trace_buf2.reset();
        trace_buf2 = SandeshTraceBufferGet("trace_buf2");
        EXPECT_EQ(NULL, trace_buf2.get());
    }

    // Create trace buffer (trace_enable = false)
    {
        trace_list_.clear();
        trace_buf2 = SandeshTraceBufferCreate("trace_buf2", 3, false);
        SANDESH_TRACE_TEST1_TRACE(trace_buf2, 16, 1);
        SANDESH_TRACE_TEST2_TRACE(trace_buf2, 17, 2);
        SandeshTraceBufferRead(trace_buf2, "test", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(0, trace_list_.size());
    }

    // Enable trace buffer
    {
        trace_list_.clear();
        SandeshTraceBufferEnable(trace_buf2);
        SANDESH_TRACE_TEST1_TRACE(trace_buf2, 18, 1);
        SANDESH_TRACE_TEST2_TRACE(trace_buf2, 19, 2);
        SandeshTraceBufferRead(trace_buf2, "test", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        SandeshTraceBufferReadDone(trace_buf2, "test");
        EXPECT_EQ(2, trace_list_.size());
        int expected[] = {18, 19};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
    }
    
    // Disable trace buffer
    {
        trace_list_.clear();
        SandeshTraceBufferDisable(trace_buf2);
        SANDESH_TRACE_TEST1_TRACE(trace_buf2, 20, 1);
        SANDESH_TRACE_TEST2_TRACE(trace_buf2, 21, 2);
        SandeshTraceBufferRead(trace_buf2, "test", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        SandeshTraceBufferReadDone(trace_buf2, "test");
        EXPECT_EQ(2, trace_list_.size());
        int expected[] = {18, 19};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
    }

    // After reading the entire content of trace_buf_, 
    // dont delete the read context. Subsequent call to TraceBufferRead()
    // should not read any trace message.
    SandeshTraceBufferPtr trace_buf3;
    {
        trace_list_.clear();
        trace_buf3 = SandeshTraceBufferCreate("trace_buf3", 3);
        SANDESH_TRACE_TEST1_TRACE(trace_buf3, 22, 1);
        SANDESH_TRACE_TEST2_TRACE(trace_buf3, 23, 2);
        SANDESH_TRACE_TEST3_TRACE(trace_buf3, 24, 3);
        SandeshTraceBufferRead(trace_buf3, "context1", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(3, trace_list_.size());
        int expected[] = {22, 23, 24};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
        trace_list_.clear();
        SandeshTraceBufferRead(trace_buf3, "context1", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(0, trace_list_.size());
    }

    // We have not deleted the read context. Add more trace messages
    // and make sure we don't read old trace messages.
    {
        trace_list_.clear();
        SANDESH_TRACE_TEST1_TRACE(trace_buf3, 25, 1);
        SANDESH_TRACE_TEST2_TRACE(trace_buf3, 26, 2);
        SANDESH_TRACE_TEST3_TRACE(trace_buf3, 27, 3);
        // Read only 2 messages
        SandeshTraceBufferRead(trace_buf3, "context1", 2,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(2, trace_list_.size());
        int expected[] = {25, 26};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
        
        trace_list_.clear();
        // Now read the last message
        SandeshTraceBufferRead(trace_buf3, "context1", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(1, trace_list_.size());
        int expected1[] = {27};
        it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected1[i], *it);
        }

        trace_list_.clear();
        // We have read all the messages 
        SandeshTraceBufferRead(trace_buf3, "context1", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(0, trace_list_.size());
    }

    // Multiple read contexts
    // Read the trace messages with new read context (read_index_ @ start)
    {
        trace_list_.clear();
        SandeshTraceBufferRead(trace_buf3, "context2", 15,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(3, trace_list_.size());
        int expected[] = {25, 26, 27};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
    }

    // Multiple read contexts
    // Read the trace messages with new read context (read_index_ @ middle)
    // Read the trace messages with old read context
    {
        trace_list_.clear();
        SANDESH_TRACE_TEST1_TRACE(trace_buf3, 28, 1);
        SandeshTraceBufferRead(trace_buf3, "context1", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(1, trace_list_.size());
        int expected[] = {28};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
        
        trace_list_.clear();
        // Read trace messages with new trace context
        SandeshTraceBufferRead(trace_buf3, "context3", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(3, trace_list_.size());
        int expected1[] = {26, 27, 28};
        it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected1[i], *it);
        }
        
        trace_list_.clear();
        // Read trace messages with old trace context
        SandeshTraceBufferRead(trace_buf3, "context1", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(0, trace_list_.size());
        
        trace_list_.clear();
        // Read trace messages with old trace context
        SandeshTraceBufferRead(trace_buf3, "context3", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(0, trace_list_.size());
    }

    // Multiple read contexts
    // Read the trace messages with new read context (read_index_ @ end)
    // Read the trace messages with old read context
    {
        trace_list_.clear();
        // Add a trace message
        SANDESH_TRACE_TEST2_TRACE(trace_buf3, 29, 2);
        SandeshTraceBufferRead(trace_buf3, "context1", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(1, trace_list_.size());
        int expected[] = {29};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }
        
        trace_list_.clear();
        SandeshTraceBufferRead(trace_buf3, "context1", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(0, trace_list_.size());

        SandeshTraceBufferRead(trace_buf3, "context4", 2,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(2, trace_list_.size());
        int expected1[] = {27, 28};
        it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected1[i], *it);
        }
        
        trace_list_.clear();
        SandeshTraceBufferRead(trace_buf3, "context4", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(1, trace_list_.size());
        int expected2[] = {29};
        it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected2[i], *it);
        }
    }

    // Delete read context
    {
        trace_list_.clear();
        SandeshTraceBufferReadDone(trace_buf3, "context1");
        SandeshTraceBufferRead(trace_buf3, "context1", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(3, trace_list_.size());
        int expected[] = {27, 28, 29};
        std::vector<int>::iterator it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected[i], *it);
        }

        trace_list_.clear();
        SANDESH_TRACE_TEST2_TRACE(trace_buf3, 30, 3);
        SandeshTraceBufferRead(trace_buf3, "context1", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(1, trace_list_.size());
        int expected1[] = {30};
        it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected1[i], *it);
        }
        
        trace_list_.clear();
        SandeshTraceBufferRead(trace_buf3, "context4", 0,
                boost::bind(&SandeshTraceTest::TraceRead, this, _1));
        EXPECT_EQ(1, trace_list_.size());
        int expected2[] = {30};
        it = trace_list_.begin();
        for (size_t i = 0; i < trace_list_.size(); i++, it++) {
            EXPECT_EQ(expected2[i], *it);
        }
    }
    trace_list_.clear();
    SandeshTraceDisable();
}

class SandeshTracePerfTest : public ::testing::Test {
protected:
    virtual void SetUp() {
        // Disable printing of trace
        Sandesh::role_ = Sandesh::Generator;
    }

    virtual void TearDown() {
        Sandesh::role_ = Sandesh::Invalid;
    }
};

TEST_F(SandeshTracePerfTest, DISABLED_1MillionWrite) {
    SandeshTraceEnable();
    SandeshTraceBufferPtr perf_trace_buf(SandeshTraceBufferCreate("Perf", 1000));
    for (int i = 0; i < 1000000; i++) {
        SANDESH_TRACE_TEST1_TRACE(perf_trace_buf, 1, 1);
    }
    SandeshTraceDisable();
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return success;
}
