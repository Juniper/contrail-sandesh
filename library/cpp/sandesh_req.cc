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
    std::vector<SandeshMessageTypeStats> mtype_stats;
    SandeshMessageStats magg_stats;
    GetSandeshStats(mtype_stats, magg_stats);
    SandeshGeneratorStats sandesh_stats;
    sandesh_stats.set_type_stats(mtype_stats);
    sandesh_stats.set_aggregate_stats(magg_stats);
    SandeshClient *client = Sandesh::client();
    if (client && client->IsSession()) {
        SandeshQueueStats qstats;
        qstats.set_enqueues(client->session()->send_queue()->NumEnqueues());
        qstats.set_count(client->session()->send_queue()->Length());
        sandesh_stats.set_send_queue_stats(qstats);
    }

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

void Sandesh::SendQueueResponse(std::string context) {
    SandeshSendQueueResponse *ssqr(new SandeshSendQueueResponse);
    ssqr->set_enable(IsSendQueueEnabled());
    ssqr->set_context(context);
    ssqr->Response();
}

void SandeshSendQueueSet::HandleRequest() const {
    if (__isset.enable) {
        Sandesh::SetSendQueue(get_enable());
    }
    // Send response
    Sandesh::SendQueueResponse(context());
}

void SandeshSendQueueStatus::HandleRequest() const {
    // Send response
    Sandesh::SendQueueResponse(context());
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

void Sandesh::SendingParamsResponse(std::string context) {
    std::vector<SandeshSession::SessionWaterMarkInfo> scwm_info;
    std::vector<SandeshSendingParams> ssp_info;
    SandeshClient *client(Sandesh::client());
    if (client) {
        client->GetSessionWaterMarkInfo(scwm_info);
        for (size_t i = 0; i < scwm_info.size(); i++) {
            SandeshSession::SessionWaterMarkInfo &scwm(scwm_info[i]);
            SandeshSendingParams ssp;
            ssp.set_queue_count(boost::get<0>(scwm));
            SandeshLevel::type level(boost::get<1>(scwm));
            ssp.set_level(Sandesh::LevelToString(level));
            ssp.set_high(boost::get<2>(scwm));
            ssp_info.push_back(ssp);
        }
    }
    SandeshSendingParamsResponse *sspr(new SandeshSendingParamsResponse);
    sspr->set_context(context);
    sspr->set_info(ssp_info);
    sspr->Response();
}

void SandeshSendingParamsSet::HandleRequest() const {
    if (!(__isset.high && __isset.queue_count &&  __isset.level)) {
        // Send response
        Sandesh::SendingParamsResponse(context());
        return;
    }
    SandeshClient *client(Sandesh::client());
    if (client) {
        size_t qcount(get_queue_count());
        bool high(get_high());
        std::string slevel(get_level());
        SandeshLevel::type level(Sandesh::StringToLevel(slevel));
        SandeshSession::SessionWaterMarkInfo scwm(qcount, level, high);
        client->SetSessionWaterMarkInfo(scwm);
    }
    // Send response
    Sandesh::SendingParamsResponse(context()); 
}

void SandeshSendingParamsReset::HandleRequest() const {
    SandeshClient *client(Sandesh::client());
    if (client) {
        client->ResetSessionWaterMarkInfo();
    }
    // Send response
    Sandesh::SendingParamsResponse(context());
}

void SandeshSendingParamsStatus::HandleRequest() const {
    // Send response
    Sandesh::SendingParamsResponse(context());
}
