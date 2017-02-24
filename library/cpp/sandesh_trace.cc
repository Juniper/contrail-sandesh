/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_trace.cc
//
// Sandesh Trace Request Implementation
//

#include <boost/bind.hpp>
#include <ctime>
#include <base/trace.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_trace_types.h>
#include "sandesh_trace.h"

using std::string;
using std::vector;

int PullSandeshTraceReq = 0;

template<> TraceSandeshType *TraceSandeshType::trace_ = NULL;

class SandeshTraceRequestRunner {
public:
    SandeshTraceRequestRunner(SandeshTraceBufferPtr trace_buf,
            const std::string& req_context,
            const std::string& read_context,
            uint32_t count) : 
        req_buf_(trace_buf),
        req_context_(req_context),
        read_context_(read_context),
        sttr_(new SandeshTraceTextResponse),
        req_count_(count),
        read_count_(0) {
        if (!req_count_) {
            // Fill the read_count_ with the size of the trace buffer
            req_count_ = SandeshTraceBufferSizeGet(req_buf_);
        }
    }

    ~SandeshTraceRequestRunner() { }

    void Run() {
        SandeshTraceBufferRead(req_buf_, read_context_, req_count_,
                boost::bind(&SandeshTraceRequestRunner::SandeshTraceRead, 
                    this, _1, _2));
        if ("Collector" != read_context_) {
            sttr_->set_context(req_context_);
            sttr_->Response();
            SandeshTraceBufferReadDone(req_buf_, read_context_);
        }
    }

private:
    void SandeshTraceRead(SandeshTrace *tsnh, bool more) {
        read_count_++;
        assert(tsnh);
        vector<string>& tracebuf = const_cast<vector<string>&>(sttr_->get_traces());
        
        if ("Collector" != read_context_) {
            tracebuf.push_back(tsnh->ToString());
            return;
        }

        if (!more || (read_count_ == req_count_)) {
            tsnh->SendTrace(req_context_, false);
        } else {
            tsnh->SendTrace(req_context_, true);
        }
    }

    SandeshTraceBufferPtr req_buf_;
    std::string req_context_;
    std::string read_context_;
    SandeshTraceTextResponse *sttr_;
    uint32_t req_count_;
    uint32_t read_count_;
};

void SandeshTraceRequest::HandleRequest() const {
    std::string read_context;

    // TODO:
    // We should get the Sandesh Header to differentiate 
    // trace requests from non-http modules.
    if ((0 == context().find("http%")) || (0 == context().find("https%"))) {
        read_context = "Http";
    } else {
        read_context = "Collector";
    }

    SandeshTraceBufferPtr trace_buf(SandeshTraceBufferGet(get_buf_name()));
    if (!trace_buf) {
        if (read_context != "Collector") {
            SandeshTraceTextResponse *sttr = new SandeshTraceTextResponse;
            sttr->set_context(context());
            sttr->Response();
        }
        return;
    }
    std::auto_ptr<SandeshTraceRequestRunner> trace_req_runner(new
            SandeshTraceRequestRunner(trace_buf, context(),
                read_context, get_count()));
    trace_req_runner->Run();
}

void SandeshTraceSend(const std::string& buf_name, uint32_t trace_count) {
    // TODO:
    // Handle this in a separate task, so that the generator can resume its execution.
    // Need to ensure that we don't run parallel tasks that send trace messages 
    // [in the same trace buffer] to the collector. It is possible that when the 
    // generator invoked this api, there may be another task running that handles 
    // the trace request (for the same trace buffer) from the Collector.
    SandeshTraceBufferPtr trace_buf(SandeshTraceBufferGet(buf_name));
    if (!trace_buf) {
        return;
    }
    std::auto_ptr<SandeshTraceRequestRunner> trace_req_runner(new
            SandeshTraceRequestRunner(trace_buf, "", "Collector", trace_count));
    trace_req_runner->Run();
}

void SandeshTraceBufferListRequest::HandleRequest() const {
    std::vector<std::string> trace_buf_list;
    SandeshTraceBufferListGet(trace_buf_list);
    std::vector<SandeshTraceBufInfo> trace_buf_info_list;
    for (std::vector<std::string>::const_iterator it = trace_buf_list.begin(); 
         it != trace_buf_list.end(); ++it) {
        SandeshTraceBufInfo trace_buf_info;
        trace_buf_info.set_trace_buf_name(*it);
        trace_buf_info_list.push_back(trace_buf_info);
    }
    SandeshTraceBufferListResponse *resp = new SandeshTraceBufferListResponse;
    resp->set_trace_buffer_list(trace_buf_info_list);
    resp->set_context(context());
    resp->set_more(false);
    resp->Response();
}

void SandeshTraceEnableDisableReq::HandleRequest() const {
    std::string status;
    if (__isset.enable) {
       if (get_enable()) {
           SandeshTraceEnable();
           status = "Sandesh Trace Enabled";
       } else {
           SandeshTraceDisable();
           status = "Sandesh Trace Disabled";
       }
    } else {
        // If the user has not specified whether to enable or disable
        // trace, then toggle the trace status.
        if (IsSandeshTraceEnabled()) {
            SandeshTraceDisable();
            status = "Sandesh Trace Disabled";
        } else {
            SandeshTraceEnable();
            status = "Sandesh Trace Enabled";
        }
    }
    SandeshTraceEnableDisableRes *resp = new
        SandeshTraceEnableDisableRes;
    resp->set_enable_disable_status(status);
    resp->set_context(context());
    resp->set_more(false);
    resp->Response();
}

void SandeshTraceBufStatusReq::HandleRequest() const {
    std::vector<std::string> trace_buf_list;
    SandeshTraceBufferListGet(trace_buf_list);
    std::vector<SandeshTraceBufStatusInfo> trace_buf_status_list;
    for (std::vector<std::string>::const_iterator it = trace_buf_list.begin(); 
         it != trace_buf_list.end(); ++it) {
        SandeshTraceBufStatusInfo trace_buf_status;
        trace_buf_status.set_trace_buf_name(*it);
        SandeshTraceBufferPtr tbp = SandeshTraceBufferGet(*it);
        assert(tbp);
        if (IsSandeshTraceBufferEnabled(tbp)) {
            trace_buf_status.set_enable_disable("Enabled");
        } else {
            trace_buf_status.set_enable_disable("Disabled");
        }
        trace_buf_status_list.push_back(trace_buf_status);
    }
    SandeshTraceBufStatusRes *resp = new SandeshTraceBufStatusRes;
    resp->set_trace_buf_status_list(trace_buf_status_list);
    resp->set_context(context());
    resp->set_more(false);
    resp->Response();
}

void SandeshTraceBufferEnableDisableReq::HandleRequest() const {
    SandeshTraceBufferEnableDisableRes *resp = new 
        SandeshTraceBufferEnableDisableRes;
    std::string status;
    SandeshTraceBufferPtr trace_buf(
        SandeshTraceBufferGet(get_trace_buf_name()));
    if (trace_buf) {
        if (__isset.enable) {
            if (get_enable()) {
                SandeshTraceBufferEnable(trace_buf);
                status = "Trace buffer Enabled";
            } else {
                SandeshTraceBufferDisable(trace_buf);
                status = "Trace buffer Disabled";
            }
        } else {
            // If the user has not specified whether to enable or disable
            // trace, then toggle the trace status.
            if (IsSandeshTraceBufferEnabled(trace_buf)) {
                SandeshTraceBufferDisable(trace_buf);
                status = "Trace buffer Disabled";
            } else {
                SandeshTraceBufferEnable(trace_buf);
                status = "Trace buffer Enabled";
            }
        }
    } else {
        status = "Invalid Trace buffer";
    }
    resp->set_enable_disable_status(status);
    resp->set_context(context());
    resp->set_more(false);
    resp->Response();
}
