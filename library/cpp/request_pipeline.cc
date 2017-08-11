/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// request_pipeline.cc
//
// Implementation of RequestPipeline
// 
// - The PipeImpl class implements the Pipeline
//   It contains multiple stages inside it.
//
// - The StageImpl class hold a given stage of the Pipeline
//   It contains multiple instances inside it.
//
// - The StageWorker class is a Task which runs the Client's callback functions
//

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif
#include <boost/utility.hpp>
#include <utility>
#include <tbb/atomic.h>
#include "base/logging.h"
#include "base/task.h"
#include <queue>
#include <boost/assign/list_of.hpp>
#include "tbb/mutex.h"

#include "sandesh/sandesh_types.h"
#include "sandesh.h"
#include "request_pipeline.h"

using namespace std;

RequestPipeline::PipeSpec::PipeSpec(const SandeshRequest * sr) {
    snhRequest_ = sr->SharedPtr();
}

class RequestPipeline::PipeImpl {
public:
    PipeImpl(const PipeSpec& spec);
    ~PipeImpl() {}
    
    const StageData* GetStageData(int stage) const;

    bool RunInstance(int instNum);

    void DoneInstance(void);
    
private:
    bool NextStage(void);

    const PipeSpec spec_;
    boost::ptr_vector<StageImpl> stageImpls_;
    int currentStage_;

    static tbb::mutex mutex_;
    static int activePipes_;
    static queue<PipeImpl*> pendPipes_;
};

class RequestPipeline::StageImpl {
public:
    StageImpl(const StageSpec& spec, int stage);
    ~StageImpl() {}

    // Decrement the number of outstanding instances.
    // Returns "true" when there are no instances left.
    bool DecrInst() {
        int prev = remainingInst_.fetch_and_decrement();
        return (prev == 1 ? true : false);
    }

    StageData data_;
    tbb::atomic<int> remainingInst_;
};

class RequestPipeline::StageWorker : public Task {
public:
    StageWorker(PipeImpl& pImpl, int taskId, int instId, int instNum) :
        Task(taskId, instId) , pImpl_(pImpl), instNum_(instNum) {}

    virtual bool Run() {
       
        if (!pImpl_.RunInstance(instNum_)) return false;

        pImpl_.DoneInstance();

        return true;
    }
    std::string Description() const { return "RequestPipeline::StageWorker"; }
private:
    PipeImpl& pImpl_;
    const int instNum_;
};

tbb::mutex  RequestPipeline::PipeImpl::mutex_;
int RequestPipeline::PipeImpl::activePipes_ = 0;
queue<RequestPipeline::PipeImpl*> RequestPipeline::PipeImpl::pendPipes_;

// This is the interface for the client callback function to look into
// the Client Data generated during earlier stages of the pipeline.
//
// Returns the StageData for a previous stage of the pipeline
const RequestPipeline::StageData*
RequestPipeline::PipeSpec::GetStageData(int stage) const {
    if (impl_)
        return impl_->GetStageData(stage);
    else
        return NULL;
}

// Constructor of PipeImpl.
// If there are no active Pipelines, start it's first stage.
// If there are active Pipelines, queue this up for later
RequestPipeline::PipeImpl::PipeImpl(const PipeSpec &spec) :
        spec_(spec, this), currentStage_(-1) {

    tbb::mutex::scoped_lock lock(mutex_);
    if (activePipes_ < 1) {
        activePipes_++;
        NextStage();
    }
    else {
        pendPipes_.push(this);
    }
}

// This function moves the Pipeline to it's next stage.
// If we are the last stage,
//  - delete the pipeline 
//  - release the Sandesh
//  - if any Pipelines are queue up, start one pipeline from the queue
//
//  Return true if we moved to the next stage of the given pipeline.
//         false if reached the last stage.
bool
RequestPipeline::PipeImpl::NextStage(void) {
    currentStage_++;
    if (currentStage_ == static_cast<int>(spec_.stages_.size())) {
        delete this;
        {
            tbb::mutex::scoped_lock lock(mutex_);
            if (!pendPipes_.empty()) {
                PipeImpl * pipe = pendPipes_.front();
                pendPipes_.pop();
                pipe->NextStage();
            } else {
                activePipes_--;
            }
        }
        return false;
    }
    else {
        stageImpls_.push_back(new StageImpl(
            spec_.stages_[currentStage_],currentStage_));
        
        const StageSpec& stageSpec = spec_.stages_[currentStage_];
        int insts = stageSpec.instances_.size();
        for (int i=0; i < insts; i++) {
            StageWorker * sw = new StageWorker(*this, stageSpec.taskId_,
                                               stageSpec.instances_[i], i);
            TaskScheduler *scheduler = TaskScheduler::GetInstance();
            scheduler->Enqueue(sw);
        } 
        return true;
    }
}

// The StageWorker calls this function when an Instance completes.
// If all instances of the current stage have completed execution,
// we can move to the next stage of the pipeline.
void
RequestPipeline::PipeImpl::DoneInstance(void) {
    if (stageImpls_[currentStage_].DecrInst()) {
        NextStage();
    }
}

// The StageWorker calls this function to drive execution
// of the client callback function.
//
// Returns true if this instance's execution is complete
//         false if the callback needs to be scheduled again.
bool
RequestPipeline::PipeImpl::RunInstance(int instNum) {
    const StageSpec &ss = spec_.stages_[currentStage_];
    return ss.cbFn_(spec_.snhRequest_.get(), spec_,
                currentStage_, instNum, (ss.allocFn_.empty() ?
                    NULL : &(stageImpls_[currentStage_].data_[instNum])));
}

// This function allows the client callback function to look into
// the Client Data generated during earlier stages of the pipeline.
//
// Returns the StageData for a previous stage of the pipeline
const RequestPipeline::StageData*
RequestPipeline::PipeImpl::GetStageData(int stage) const {
    if (stage >= currentStage_)
        return NULL;
    else
        return &(stageImpls_[stage].data_); 
}

// Contructor for StageImpl
// Creates the StageWorker for each Instance of this Stage
// Also Creates the Client Data for each Instance.
RequestPipeline::StageImpl::StageImpl(const StageSpec& spec, int stage) {
    remainingInst_ = spec.instances_.size(); 
    for (int i=0; i<remainingInst_; i++) {
        if (!spec.allocFn_.empty()) data_.push_back(spec.allocFn_(stage));
    }
}

RequestPipeline::RequestPipeline(const PipeSpec& spec) {
    impl_ = new PipeImpl(spec);
}

