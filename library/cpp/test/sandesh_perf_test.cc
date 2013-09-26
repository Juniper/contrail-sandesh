/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_perf_test.cc
//
// Sandesh Performance Test
//

#include "testing/gunit.h"

#include <boost/bind.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/range/iterator_range.hpp>

#include <base/logging.h>
#include <base/util.h>
#include <base/queue_task.h>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh_constants.h>
#include <sandesh/sandesh.h>

#include "sandesh_perf_test_types.h"

// Verfiy the performance of WorkQueue enqueue and dequeue for the following cases:
// 1. Allocating new Sandesh
// 2. Allocating new structure
// 3. Structure on the stack
// 4. Using boost::pool to allocate new structure
class SandeshPerfTestEnqueue : public ::testing::Test {
public:
    typedef struct TestStructPool {
        void Release() { BoostTestStructPool::free(this); }
    } TestStructPool;

protected:
    typedef struct TestStructPoolTag {
        char data[512];
    } TestStructPoolTag;

    typedef struct TestStruct {
        char data[512];
    } TestStruct;

    typedef boost::singleton_pool <TestStructPoolTag, sizeof(TestStructPool)> BoostTestStructPool;
    
    virtual void SetUp() {
        sandesh_queue_.reset(new Sandesh::SandeshQueue(
            TaskScheduler::GetInstance()->GetTaskId("sandesh::Test::PerfTest::sandesh_queue"),
            Task::kTaskInstanceAny,
            boost::bind(&SandeshPerfTestEnqueue::DequeueSandesh, this, _1),
            boost::bind(&SandeshPerfTestEnqueue::StartDequeueSandesh, this)));
        pstruct_queue_.reset(new WorkQueue<TestStruct *>(
            TaskScheduler::GetInstance()->GetTaskId("sandesh::Test::PerfTest::pstruct_queue_"),
            Task::kTaskInstanceAny,
            boost::bind(&SandeshPerfTestEnqueue::DequeuePTestStruct, this, _1),
            boost::bind(&SandeshPerfTestEnqueue::StartDequeuePTestStruct, this)));
        struct_queue_.reset(new WorkQueue<TestStruct>(
            TaskScheduler::GetInstance()->GetTaskId("sandesh::Test::PerfTest::struct_queue_"),
            Task::kTaskInstanceAny,
            boost::bind(&SandeshPerfTestEnqueue::DequeueTestStruct, this, _1),
            boost::bind(&SandeshPerfTestEnqueue::StartDequeueTestStruct, this)));
        pstructpool_queue_.reset(new WorkQueue<TestStructPool *>(
            TaskScheduler::GetInstance()->GetTaskId("sandesh::Test::PerfTest::pstructpool_queue_"),
            Task::kTaskInstanceAny,
            boost::bind(&SandeshPerfTestEnqueue::DequeuePTestStructPool, this, _1),
            boost::bind(&SandeshPerfTestEnqueue::StartDequeuePTestStructPool, this)));
    }

    virtual void TearDown() {
    }

    bool DequeueSandesh(Sandesh *entry) {
        entry->Release();
        return true;
    }

    bool DequeuePTestStruct(TestStruct *entry) {
        delete entry;
        return true;
    }

    bool DequeuePTestStructPool(TestStructPool *entry) {
        BoostTestStructPool::free(entry);
        return true;
    }

    bool DequeueTestStruct(TestStruct entry) {
        return true;
    }

    bool StartDequeueSandesh() {
        return false;
    }

    bool StartDequeuePTestStruct() {
        return false;
    }

    bool StartDequeuePTestStructPool() {
        return false;
    }

    bool StartDequeueTestStruct() {
        return false;
    }

    boost::scoped_ptr<Sandesh::SandeshQueue> sandesh_queue_;
    boost::scoped_ptr<WorkQueue<TestStruct> > struct_queue_;
    boost::scoped_ptr<WorkQueue<TestStruct *> > pstruct_queue_;
    boost::scoped_ptr<WorkQueue<TestStructPool *> > pstructpool_queue_;
};

template <>
struct WorkQueueDelete<SandeshPerfTestEnqueue::TestStructPool *> {
    template <typename QueueT>
    void operator()(QueueT &q) {
        for (typename QueueT::iterator iter = q.unsafe_begin();
             iter != q.unsafe_end(); ++iter) {
             (*iter)->Release();
        }
    }
};

TEST_F(SandeshPerfTestEnqueue, DISABLED_SandeshEnqueue) {
    for (int i = 0; i < 1000000; i++) {
        PerfTestSandesh *sandesh = new PerfTestSandesh();
        sandesh_queue_->Enqueue(sandesh);
    }
    sandesh_queue_->Shutdown();   
}

TEST_F(SandeshPerfTestEnqueue, DISABLED_StructEnqueue) {
    for (int i = 0; i < 1000000; i++) {
        TestStruct tstruct;
        struct_queue_->Enqueue(tstruct);
    }
    struct_queue_->Shutdown();   
}

TEST_F(SandeshPerfTestEnqueue, DISABLED_PStructEnqueue) {
    for (int i = 0; i < 1000000; i++) {
        TestStruct *tstruct = new TestStruct;
        pstruct_queue_->Enqueue(tstruct);
    }
    pstruct_queue_->Shutdown();   
}

TEST_F(SandeshPerfTestEnqueue, DISABLED_PStructPoolEnqueue) {
    for (int i = 0; i < 1000000; i++) {
        TestStructPool *tstruct = (TestStructPool *)BoostTestStructPool::malloc();
        pstructpool_queue_->Enqueue(tstruct);
    }
    pstructpool_queue_->Shutdown();   
}

// string append outperforms both snprintf and string addition
class SandeshPerfTestString : public ::testing::Test {
protected:
    SandeshPerfTestString() :
        xml_("str1 type=\"string\" identifier=\"1\""),
        cmp_("nomatch") {
    }

    std::string xml_;
    std::string cmp_;
};

TEST_F(SandeshPerfTestString, DISABLED_StringAppend) {
    for (int i = 0; i < 1000000; i++) {
        std::string numbers;
        numbers.reserve(1024);
        numbers += "name";
        numbers += " ";
        numbers += "value";
        numbers += " ";
        numbers += integerToString(i);
        numbers += " ";
    }
}

TEST_F(SandeshPerfTestString, DISABLED_StringAdd) {
    std::string name("name");
    std::string value("value");
    for (int i = 0; i < 1000000; i++) {
        std::string numbers(name + " " + value + " " + integerToString(i) + " ");
    }
}

TEST_F(SandeshPerfTestString, DISABLED_CString) {
    for (int i = 0; i < 1000000; i++) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "name value %s ", 
                         integerToString(i).c_str());
    }
}

// boost::tokenizer outperforms the rest
TEST_F(SandeshPerfTestString, DISABLED_PerfTestTokenizer) {
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

TEST_F(SandeshPerfTestString, DISABLED_PerfTestSplit) {
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

TEST_F(SandeshPerfTestString, DISABLED_PerfTestSplitIter) {
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

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return success;
}
