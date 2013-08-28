/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_show_trace_test.cc
//
// Sandesh Show Trace Test
//

#include <base/logging.h>
#include <base/queue_task.h>
#include <io/event_manager.h>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include "sandesh_trace.h"
#include "sandesh_trace_test_types.h"

using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;

using boost::asio::ip::address;
using boost::asio::ip::tcp;

class TraceTestTask : public Task {
    public:
        TraceTestTask(std::vector<SandeshTraceBufferPtr> &trace_bufs,
                      int task_id, int task_instance, 
                      int min_val, int max_val, int max_iter = 100) :
            Task(task_id, task_instance),
            trace_bufs_(trace_bufs), 
            kMinVal(min_val), kMaxVal(max_val), 
            kMaxIter(max_iter), val_(kMinVal),
            iter_(0) {
                assert(kMinVal > 0);
                assert(kMaxVal > 0);
                assert(kMaxVal > kMinVal);
                assert(kMaxIter > 0);
        }
        ~TraceTestTask() {
        }
        bool Run() {
            while (iter_++ != kMaxIter) {
                if (kMaxVal == val_) {
                    val_ = kMinVal;
                }
                for (size_t i = 0; i < trace_bufs_.size(); i++) {
                    SANDESH_TRACE_TEST1_TRACE(trace_bufs_[i], 111, val_);
                    SANDESH_TRACE_TEST2_TRACE(trace_bufs_[i], 222, val_);
                    SANDESH_TRACE_TEST3_TRACE(trace_bufs_[i], 333, val_);
                }
                val_++;
            }
            iter_ = 0;
            return false; 
        }
    private:
        std::vector<SandeshTraceBufferPtr> trace_bufs_;
        const int kMinVal;
        const int kMaxVal;
        const int kMaxIter;
        
        int val_;
        int iter_;
};

int main(int argc, char **argv) {
    LoggingInit();
    EventManager evm;
    TaskScheduler *scheduler = TaskScheduler::GetInstance();
    int task_id = scheduler->GetTaskId("sandesh show trace test");
    Sandesh::InitGeneratorTest("sandesh show trace test", "localhost", &evm, 0,
            NULL);
    Sandesh::SetLoggingParams(false, "", SandeshLevel::UT_DEBUG);

    std::vector<SandeshTraceBufferPtr> trace_bufs;
    SandeshTraceBufferPtr test_buf1(SandeshTraceBufferCreate("test_buf1", 100));
    trace_bufs.push_back(test_buf1);
    SandeshTraceBufferPtr test_buf2(SandeshTraceBufferCreate("test_buf2", 200));
    trace_bufs.push_back(test_buf2);
    SandeshTraceBufferPtr test_buf3(SandeshTraceBufferCreate("test_buf3", 300));
    trace_bufs.push_back(test_buf3);

    TraceTestTask *task1 = new TraceTestTask(trace_bufs, task_id, 1, 1, 1000, 50);
    TraceTestTask *task2 = new TraceTestTask(trace_bufs, task_id, 2, 1000, 2000, 75);
    TraceTestTask *task3 = new TraceTestTask(trace_bufs, task_id, 3, 2000, 3000, 100);
    scheduler->Enqueue(task1);
    scheduler->Enqueue(task2);
    scheduler->Enqueue(task3);

    evm.Run();
    return 0;
}
