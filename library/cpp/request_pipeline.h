/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// request_pipeline.h
//

#ifndef __REQUEST_PIPELINE_H__
#define __REQUEST_PIPELINE_H__

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

class SandeshRequest;
class Sandesh;

class RequestPipeline {
public:
    class PipeImpl;
    class StageImpl;

    ~RequestPipeline() {}

    // Clients can build and store a (taskId,InstanceId)'s 
    // intermediate information in an object who's type is 
    // derived from InstanceData
    class InstData {
    public:
        virtual ~InstData() {}
    protected:
        InstData() {}
    };

    // In the callback function, clients can access data from any previous
    // stage.
    typedef boost::ptr_vector<InstData> StageData;

    // Client must provide an allocator for their Data
    typedef boost::function<InstData*(int stage)> DataFactory;

    // Clients must provide this callback function on a per-taskID basis
    struct PipeSpec;
    typedef boost::function<bool(const Sandesh * sr, const PipeSpec& pspec,
            int stage, int instNum, InstData* dataInAndOut)> CallbackFunc;

    // The StageSpec for a Stage of the pipeline contains the taskId to
    // be used, the InstanceIds to be launched in parallel, the client
    // callback and an allocator function for allocating InstData
    // If the allocator function is not provided, NULL will be passed
    // back in the callback function for that stage
    struct StageSpec {
        int taskId_;
        std::vector<int> instances_;
        CallbackFunc cbFn_;
        DataFactory allocFn_;
    };
    // The pipespec is used to pass in the stages, and
    // also has an interface (GetStageData) that the callback can use to
    // access Data from previous stages
    struct PipeSpec {
        PipeSpec(const SandeshRequest * sr);
        std::vector<StageSpec> stages_;
        boost::shared_ptr<const SandeshRequest> snhRequest_;

        const StageData* GetStageData(int stage) const;

        // This function is not intended for client use
        PipeSpec(const PipeSpec& ps, PipeImpl * impl) : 
            stages_(ps.stages_), snhRequest_(ps.snhRequest_), impl_(impl) {}
    private:
        PipeImpl * impl_;
    };

    // Client Construct a Pipeline by passing the specification
    RequestPipeline(const PipeSpec& spec);
    
private:
    class StageWorker;

    PipeImpl * impl_;
};

#endif // __REQUEST_PIPELINE_H__
