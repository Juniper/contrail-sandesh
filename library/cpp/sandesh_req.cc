/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_req.cc
//
// File to handle all sandesh requests..
//

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_uve_types.h>

#include "sandesh_statistics.h"
#include "sandesh_client.h"
#include "sandesh_connection.h"

using boost::asio::ip::address;

int PullSandeshGenStatsReq = 0;

void SandeshMessageStatsReq::HandleRequest() const {
    SandeshMessageStatsResp *resp(new SandeshMessageStatsResp);

    tbb::mutex::scoped_lock lock(stats_mutex_);
    SandeshStatistics &stats = Sandesh::stats_;

    // XXX - Check if introspect supports maps
    typedef boost::ptr_map<std::string, SandeshMessageTypeStats> SandeshMessageTypeStatsMap;
    std::vector<SandeshMessageTypeStats> mtype_stats;
    for (SandeshMessageTypeStatsMap::iterator mt_it = stats.type_stats.begin();
            mt_it != stats.type_stats.end();
            mt_it++) {
        mtype_stats.push_back(*mt_it->second);
    }
    SandeshGeneratorStats sandesh_stats;
    sandesh_stats.set_type_stats(mtype_stats);
    sandesh_stats.set_aggregate_stats(stats.agg_stats);

    resp->set_stats(sandesh_stats);
    resp->set_context(context());
    resp->Response();
}

void Sandesh::SendLoggingResponse(std::string context) {
    SandeshLoggingParams *slogger(new SandeshLoggingParams());
    slogger->set_enable(IsLocalLoggingEnabled());
    std::string category = LoggingCategory();
    if (category.empty()) {
        category = "*";
    }
    slogger->set_category(category);
    slogger->set_level(LevelToString(LoggingLevel()));
    slogger->set_trace_print(IsTracePrintEnabled());
    slogger->set_context(context);
    slogger->Response();
}
void SandeshLoggingParamsSet::HandleRequest() const {
    // Set the logging parameters
    if (__isset.level) {
        Sandesh::SetLoggingLevel(get_level());
    }
    if (__isset.category) {
        std::string category = get_category();
        if (category == "*") {
            category = "";
        }
        Sandesh::SetLoggingCategory(category);
    }
    if (__isset.enable) {
        Sandesh::SetLocalLogging(get_enable());
    }
    if (__isset.trace_print) {
        Sandesh::SetTracePrint(get_trace_print());
    }
    // Send response
    Sandesh::SendLoggingResponse(context());
}

void SandeshLoggingParamsStatus::HandleRequest() const {
    // Send response
    Sandesh::SendLoggingResponse(context());
}

void CollectorInfoRequest::HandleRequest() const {
    CollectorInfoResponse *resp (new CollectorInfoResponse());
    SandeshClient *client = Sandesh::client();
    if (client) {
        resp->set_ip(client->sm_->server().address().to_string());
        resp->set_port(client->sm_->server().port());
        resp->set_status(client->sm_->StateName());
    }
    resp->set_context(context());
    resp->Response();
}
