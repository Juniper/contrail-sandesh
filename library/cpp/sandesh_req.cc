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
    GetMsgStats(&mtype_stats, &magg_stats);
    SandeshGeneratorStats sandesh_stats;
    sandesh_stats.set_type_stats(mtype_stats);
    sandesh_stats.set_aggregate_stats(magg_stats);
    SandeshClient *client = Sandesh::client();
    if (client && client->IsSession()) {
        SandeshQueueStats qstats;
        qstats.set_enqueues(client->session()->send_queue()->NumEnqueues());
        qstats.set_count(client->session()->send_queue()->Length());
        qstats.set_max_count(client->session()->send_queue()->max_queue_len());
        sandesh_stats.set_send_queue_stats(qstats);
    }

    resp->set_stats(sandesh_stats);
    resp->set_context(context());
    resp->Response();
}

static void SendSandeshLoggingParams(const std::string &context) {
    SandeshLoggingParams *slogger(new SandeshLoggingParams());
    slogger->set_enable(Sandesh::IsLocalLoggingEnabled());
    std::string category = Sandesh::LoggingCategory();
    if (category.empty()) {
        category = "*";
    }
    slogger->set_category(category);
    slogger->set_log_level(Sandesh::LevelToString(Sandesh::LoggingLevel()));
    slogger->set_trace_print(Sandesh::IsTracePrintEnabled());
    slogger->set_enable_flow_log(Sandesh::IsFlowLoggingEnabled());
    slogger->set_context(context);
    slogger->Response();
}

void SandeshLoggingParamsSet::HandleRequest() const {
    // Set the logging parameters
    if (__isset.log_level) {
        Sandesh::SetLoggingLevel(get_log_level());
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
    if (__isset.enable_flow_log) {
        Sandesh::SetFlowLogging(get_enable_flow_log());
    }
    // Send response
    SendSandeshLoggingParams(context());
}

void SandeshLoggingParamsStatus::HandleRequest() const {
    // Send response
    SendSandeshLoggingParams(context());
}

static void SendSandeshSendingParams(const std::string &context) {
    SandeshSendingParams *ssparams(new SandeshSendingParams());
    ssparams->set_system_logs_rate_limit(Sandesh::get_send_rate_limit());
    ssparams->set_disable_object_logs(Sandesh::IsSendingObjectLogsDisabled());
    ssparams->set_disable_all_logs(Sandesh::IsSendingAllMessagesDisabled());
    ssparams->set_context(context);
    ssparams->Response();
}

void SandeshSendingParamsSet::HandleRequest() const {
    if (__isset.system_logs_rate_limit) {
        Sandesh::set_send_rate_limit(system_logs_rate_limit);
    }
    if (__isset.disable_object_logs) {
        Sandesh::DisableSendingObjectLogs(disable_object_logs);
    }
    if (__isset.disable_all_logs) {
        Sandesh::DisableSendingAllMessages(disable_all_logs);
    }
    // Send response
    SendSandeshSendingParams(context());
}

void SandeshSendingParamsStatus::HandleRequest() const {
    // Send response
    SendSandeshSendingParams(context());
}

static void SendSandeshSendQueueResponse(std::string context) {
    SandeshSendQueueResponse *ssqr(new SandeshSendQueueResponse);
    ssqr->set_enable(Sandesh::IsSendQueueEnabled());
    ssqr->set_context(context);
    ssqr->Response();
}

void SandeshSendQueueSet::HandleRequest() const {
    if (__isset.enable) {
        Sandesh::SetSendQueue(get_enable());
    }
    // Send response
    SendSandeshSendQueueResponse(context());
}

void SandeshSendQueueStatus::HandleRequest() const {
    // Send response
    SendSandeshSendQueueResponse(context());
}

void CollectorInfoRequest::HandleRequest() const {
    CollectorInfoResponse *resp (new CollectorInfoResponse());
    SandeshClient *client = Sandesh::client();
    if (client) {
        resp->set_ip(client->sm_->server().address().to_string());
        resp->set_port(client->sm_->server().port());
        resp->set_status(client->sm_->StateName());
        SandeshSession *sess = client->sm_->session();
        if (sess) {
            resp->set_dscp(sess->GetDscpValue());
        }
    }
    resp->set_context(context());
    resp->Response();
}

static void SendSandeshSendQueueParamsResponse(const std::string &context) {
    std::vector<Sandesh::QueueWaterMarkInfo> scwm_info;
    std::vector<SandeshSendQueueParams> ssp_info;
    SandeshClient *client(Sandesh::client());
    if (client) {
        client->GetSessionWaterMarkInfo(scwm_info);
        for (size_t i = 0; i < scwm_info.size(); i++) {
            Sandesh::QueueWaterMarkInfo &scwm(scwm_info[i]);
            SandeshSendQueueParams ssp;
            ssp.set_queue_count(boost::get<0>(scwm));
            SandeshLevel::type level(boost::get<1>(scwm));
            ssp.set_sending_level(Sandesh::LevelToString(level));
            ssp.set_high(boost::get<2>(scwm));
            ssp_info.push_back(ssp);
        }
    }
    SandeshSendQueueParamsResponse *sspr(new SandeshSendQueueParamsResponse);
    sspr->set_context(context);
    sspr->set_info(ssp_info);
    sspr->Response();
}

void SandeshSendQueueParamsSet::HandleRequest() const {
    if (!(__isset.high && __isset.queue_count &&  __isset.sending_level)) {
        // Send response
        SendSandeshSendQueueParamsResponse(context());
        return;
    }
    SandeshClient *client(Sandesh::client());
    if (client) {
        size_t qcount(get_queue_count());
        bool high(get_high());
        std::string slevel(get_sending_level());
        SandeshLevel::type level(Sandesh::StringToLevel(slevel));
        Sandesh::QueueWaterMarkInfo scwm(qcount, level, high);
        client->SetSessionWaterMarkInfo(scwm);
    }
    // Send response
    SendSandeshSendQueueParamsResponse(context());
}

void SandeshSendQueueParamsReset::HandleRequest() const {
    SandeshClient *client(Sandesh::client());
    if (client) {
        client->ResetSessionWaterMarkInfo();
    }
    // Send response
    SendSandeshSendQueueParamsResponse(context());
}

void SandeshSendQueueParamsStatus::HandleRequest() const {
    // Send response
    SendSandeshSendQueueParamsResponse(context());
}
