/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_req.cc
//
// File to handle all sandesh requests..
//

#include "sandesh/sandesh_types.h"
#include "sandesh.h"
#include "sandesh_client.h"
#include "sandesh_connection.h"
#include <sandesh/sandesh_uve_types.h>

using boost::asio::ip::address;

int PullSandeshGenStatsReq = 0;

void SandeshGenStatsReq::HandleRequest() const {
    SandeshGenStatsResp *resp(new SandeshGenStatsResp);

    tbb::mutex::scoped_lock lock(stats_mutex_);
    SandeshGenStatsCollection &stats = Sandesh::stats_;

    typedef boost::ptr_map<std::string, SandeshGenStatsElemCollection> SandeshGenStatsMap;

    std::vector<SandeshGenStatsElem> stats_list;
    for (SandeshGenStatsMap::iterator mt_it = stats.msgtype_stats_map_.begin();
            mt_it != stats.msgtype_stats_map_.end();
            mt_it++) {
        SandeshGenStatsElem stats_elem;
        stats_elem.set_message_type(mt_it->first);
        stats_elem.set_messages_sent((mt_it->second)->messages_sent);
        stats_elem.set_bytes_sent((mt_it->second)->bytes_sent);
        stats_elem.set_messages_received(
                                (mt_it->second)->messages_received);
        stats_elem.set_bytes_received((mt_it->second)->bytes_received);
        stats_list.push_back(stats_elem);
    }
    SandeshGenStats stats_resp;
    stats_resp.set_stats_list(stats_list);
    stats_resp.set_hostname(Sandesh::source());
    stats_resp.set_total_sandesh_sent(stats.total_sandesh_sent);
    stats_resp.set_total_bytes_sent(stats.total_bytes_sent);
    stats_resp.set_total_sandesh_received(stats.total_sandesh_received);
    stats_resp.set_total_bytes_received(stats.total_bytes_received);

    resp->set_stats(stats_resp);

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
