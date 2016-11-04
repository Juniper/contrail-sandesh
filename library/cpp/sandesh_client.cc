/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_client.cc
//
// Sandesh Client
//

#include <boost/bind.hpp>
#include <boost/assign.hpp>

#include <base/task_annotations.h>
#include <io/event_manager.h>
#include <io/tcp_session.h>
#include <io/tcp_server.h>

#include <sandesh/sandesh_constants.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_session.h>

#include "sandesh_state_machine.h"

#include <sandesh/protocol/TXMLProtocol.h>
#include <sandesh/sandesh_ctrl_types.h>
#include <sandesh/sandesh_uve_types.h>
#include <sandesh/derived_stats_results_types.h>
#include <sandesh/common/vns_constants.h>
#include "sandesh_client.h"
#include "sandesh_uve.h"

using boost::asio::ip::address;
using namespace boost::asio;
using boost::system::error_code;
using std::string;
using std::map;
using std::make_pair;
using std::vector;

const std::string SandeshClient::kSMTask = "sandesh::SandeshClientSM";
const std::string SandeshClient::kSessionWriterTask = "sandesh::SandeshClientSession";
const std::string SandeshClient::kSessionReaderTask = "sandesh::SandeshClientReader";
bool SandeshClient::task_policy_set_ = false;
const std::vector<Sandesh::QueueWaterMarkInfo> 
    SandeshClient::kSessionWaterMarkInfo = boost::assign::tuple_list_of
                                               (100000, SandeshLevel::SYS_EMERG, true, false)
                                               (50000, SandeshLevel::SYS_ERR, true, false)
                                               (10000, SandeshLevel::SYS_DEBUG, true, false)
                                               (75000, SandeshLevel::SYS_ERR, false, false)
                                               (30000, SandeshLevel::SYS_DEBUG, false, false)
                                               (2500, SandeshLevel::INVALID, false, false);

SandeshClient::SandeshClient(EventManager *evm,
        Endpoint primary, Endpoint secondary, Sandesh::CollectorSubFn csf, bool periodicuve)
    :   TcpServer(evm),
        sm_task_instance_(kSMTaskInstance),
        sm_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSMTask)),
        session_task_instance_(kSessionTaskInstance),
        session_writer_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSessionWriterTask)),
        session_reader_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSessionReaderTask)),
        primary_(primary),
        secondary_(secondary),
        csf_(csf),
        sm_(SandeshClientSM::CreateClientSM(evm, this, sm_task_instance_, sm_task_id_, periodicuve)),
        session_wm_info_(kSessionWaterMarkInfo) {
    SANDESH_LOG(INFO,"primary  " << primary_);
    SANDESH_LOG(INFO,"secondary  " << secondary_);

    // Set task policy for exclusion between state machine and session tasks since
    // session delete happens in state machine task
    if (!task_policy_set_) {
        TaskPolicy sm_task_policy = boost::assign::list_of
                (TaskExclusion(session_writer_task_id_))
                (TaskExclusion(session_reader_task_id_));
        TaskScheduler::GetInstance()->SetPolicy(sm_task_id_, sm_task_policy);
        task_policy_set_ = true;
    }
}

SandeshClient::~SandeshClient() {}

void SandeshClient::CollectorHandler(std::vector<DSResponse> resp) {

    Endpoint primary = Endpoint();
    Endpoint secondary = Endpoint();
    
    if (resp.size()>=1) {
        primary = resp[0].ep;
        SANDESH_LOG(INFO, "DiscUpdate for primary " << primary);
    }
    if (resp.size()>=2) {
        secondary = resp[1].ep;
        SANDESH_LOG(INFO, "DiscUpdate for secondary " << secondary);
    }
    if (primary!=Endpoint()) {
        sm_->SetCandidates(primary, secondary);
    }
}

void SandeshClient::ReConfigCollectors(std::vector<std::string> collector_list) {

    Endpoint primary = Endpoint();
    Endpoint secondary = Endpoint();

    std::vector<std::string> ep;
    uint32_t port;
    address addr;
    boost::system::error_code ec;
    if (collector_list.size()>=1) {
        boost::split(ep, collector_list[0], boost::is_any_of(":"));

        addr = address::from_string(ep[0], ec);
        if (ec) {
            SANDESH_LOG(ERROR, "ReConfig for primary failed Error: " << ec);
            return;
        }
        primary.address(addr);
        port = strtoul(ep[1].c_str(), NULL, 0);
        primary.port(port);

        SANDESH_LOG(INFO, "ReConfig for primary " << primary);
    }
    if (collector_list.size()>=2) {
        boost::split(ep, collector_list[1], boost::is_any_of(":"));

        addr = address::from_string(ep[0], ec);
        if (ec) {
            SANDESH_LOG(ERROR, "ReConfig for secondary failed Error: " << ec);
            return;
        }
        secondary.address(addr);
        port = strtoul(ep[1].c_str(), NULL, 0);
        secondary.port(port);

        SANDESH_LOG(INFO, "ReConfig for secondary " << secondary);
    }
    if (primary!=Endpoint()) {
        sm_->SetCandidates(primary, secondary);
    }
}

void SandeshClient::Initiate() {
    sm_->SetAdminState(false);
    if (primary_ != Endpoint())
        sm_->SetCandidates(primary_,secondary_);
    // subscribe for the collector service only if the collector list
    // is not provided by the generator.
    else if (csf_ != 0) {
        SANDESH_LOG(INFO, "Subscribe to Discovery Service for Collector" );
        csf_(g_vns_constants.COLLECTOR_DISCOVERY_SERVICE_NAME, 2,
            boost::bind(&SandeshClient::CollectorHandler, this, _1));
    }
}

void SandeshClient::Shutdown() {
    sm_->SetAdminState(true);
}

bool SandeshClient::SendSandesh(Sandesh *snh) {
    return sm_->SendSandesh(snh);
}

bool SandeshClient::ReceiveCtrlMsg(const std::string &msg,
        const SandeshHeader &header, const std::string &sandesh_name,
        const uint32_t header_offset) {

    Sandesh * sandesh = SandeshSession::DecodeCtrlSandesh(msg, header, sandesh_name, header_offset);

    const SandeshCtrlServerToClient * snh = dynamic_cast<const SandeshCtrlServerToClient *>(sandesh);
    if (!snh) {
        SANDESH_LOG(ERROR, "Received Ctrl Message with wrong type " << sandesh->Name());
        sandesh->Release();
        return false;
    }
    if (!snh->get_success()) {
        SANDESH_LOG(ERROR, "Received Ctrl Message : Connection with server has failed");
        sandesh->Release();
        return false;
    }
    SANDESH_LOG(DEBUG, "Received Ctrl Message with size " << snh->get_type_info().size());

    map<string,uint32_t> sMap;
    const vector<UVETypeInfo> & vu = snh->get_type_info();
    for(uint32_t i = 0; i < vu.size(); i++) {
        sMap.insert(std::make_pair(vu[i].get_type_name(), vu[i].get_seq_num()));
    }
    SandeshUVETypeMaps::SyncAllMaps(sMap);

    sandesh->Release();
    return true;
}


bool SandeshClient::ReceiveMsg(const std::string& msg,
        const SandeshHeader &header, const std::string &sandesh_name,
        const uint32_t header_offset) {

    namespace sandesh_prot = contrail::sandesh::protocol;
    namespace sandesh_trans = contrail::sandesh::transport;

    if (header.get_Hints() & g_sandesh_constants.SANDESH_CONTROL_HINT) {
        bool success = ReceiveCtrlMsg(msg, header, sandesh_name, header_offset);
        if (success) {
            Sandesh::UpdateRxMsgStats(sandesh_name, msg.size());
        } else {
            Sandesh::UpdateRxMsgFailStats(sandesh_name, msg.size(),
                SandeshRxDropReason::ControlMsgFailed);
        }
        return success;
    }

    // Create and process the sandesh
    Sandesh *sandesh = SandeshBaseFactory::CreateInstance(sandesh_name);
    if (sandesh == NULL) {
        SANDESH_LOG(ERROR, __func__ << ": Unknown sandesh: " << sandesh_name);
        Sandesh::UpdateRxMsgFailStats(sandesh_name, msg.size(),
            SandeshRxDropReason::CreateFailed);
        return true;
    }
    boost::shared_ptr<sandesh_trans::TMemoryBuffer> btrans =
            boost::shared_ptr<sandesh_trans::TMemoryBuffer>(
                    new sandesh_trans::TMemoryBuffer((uint8_t *)msg.c_str() + header_offset,
                            msg.size() - header_offset));
    boost::shared_ptr<sandesh_prot::TXMLProtocol> prot =
            boost::shared_ptr<sandesh_prot::TXMLProtocol>(new sandesh_prot::TXMLProtocol(btrans));
    int32_t xfer = sandesh->Read(prot);
    if (xfer < 0) {
        SANDESH_LOG(ERROR, __func__ << ": Decoding " << sandesh_name << " FAILED");
        Sandesh::UpdateRxMsgFailStats(sandesh_name, msg.size(),
            SandeshRxDropReason::DecodingFailed);
        return false;
    }

    Sandesh::UpdateRxMsgStats(sandesh_name, msg.size());
    SandeshRequest *sr = dynamic_cast<SandeshRequest *>(sandesh);
    assert(sr);
    sr->Enqueue(Sandesh::recv_queue());
    return true;
}

void
SandeshCtrlServerToClient::HandleRequest() const { }

void
SandeshCtrlClientToServer::HandleRequest() const { }


SandeshSession *SandeshClient::CreateSMSession(
        TcpSession::EventObserver eocb,
        SandeshReceiveMsgCb rmcb,
        TcpServer::Endpoint ep) {
    TcpSession *session = TcpServer::CreateSession();
    Socket *socket = session->socket();

    error_code ec;
    socket->open(ip::tcp::v4(), ec);
    if (ec) {
        SANDESH_LOG(ERROR, __func__ << " Open FAILED: " << ec.message());
        DeleteSession(session);
        return NULL;
    }
    ec = session->SetSocketOptions();
    if (ec) {
        SANDESH_LOG(ERROR, __func__ << " Unable to set socket options: " << ec.message());
        DeleteSession(session);
        return NULL;
    }
    SandeshSession *sandesh_session =
            static_cast<SandeshSession *>(session);
    sandesh_session->SetReceiveMsgCb(rmcb);
    sandesh_session->SetConnection(NULL);
    sandesh_session->set_observer(eocb);
    // Set watermarks
    for (size_t i = 0; i < session_wm_info_.size(); i++) {
        sandesh_session->SetSendQueueWaterMark(session_wm_info_[i]);
    }
    TcpServer::Connect(sandesh_session, ep);

    return sandesh_session;
}

void SandeshClient::InitializeSMSession(int count) {
    std::vector<string> stv;

    SandeshUVETypeMaps::uve_global_map::const_iterator it =
        SandeshUVETypeMaps::Begin();
    for(; it!= SandeshUVETypeMaps::End(); it++) {
        stv.push_back(it->first);
    }
    SANDESH_LOG(DEBUG, "Sending Ctrl Message for " << Sandesh::source() << ":" <<
        Sandesh::module() << ":" << Sandesh::instance_id() << ":" <<
        Sandesh::node_type() << " count " << count);

    SandeshCtrlClientToServer::Request(Sandesh::source(), Sandesh::module(),
            count, stv, getpid(), Sandesh::http_port(),
            Sandesh::node_type(), Sandesh::instance_id(), "ctrl");

}

static bool client_start = false;
static uint64_t client_start_time;

void SandeshClient::SendUVE(int count,
        const string & stateName, const string & server,
        Endpoint primary, Endpoint secondary) {
    ModuleClientState mcs;
    mcs.set_name(Sandesh::source() + ":" + Sandesh::node_type() +
        ":" + Sandesh::module() + ":" + Sandesh::instance_id());
    SandeshClientInfo sci;
    if (!client_start) {
        client_start_time = UTCTimestampUsec(); 
        client_start = true;
    }
    std::ostringstream pri,sec;
    pri << primary;
    sec << secondary;

    sci.set_start_time(client_start_time);
    sci.set_successful_connections(count);
    sci.set_pid(getpid());
    sci.set_http_port(Sandesh::http_port());
    sci.set_status(stateName);
    sci.set_collector_name(server);
    sci.set_primary(pri.str());
    sci.set_secondary(sec.str());
    // Sandesh client socket statistics
    SocketIOStats rx_stats;
    GetRxSocketStats(rx_stats);
    sci.set_rx_socket_stats(rx_stats);
    SocketIOStats tx_stats;
    GetTxSocketStats(tx_stats);
    sci.set_tx_socket_stats(tx_stats);

    mcs.set_client_info(sci);

    std::vector<SandeshMessageTypeStats> mtype_stats;
    SandeshMessageStats magg_stats;
    Sandesh::GetMsgStats(&mtype_stats, &magg_stats);

    map<string,uint64_t> csev;
    csev.insert(make_pair("sent", magg_stats.get_messages_sent()));
    csev.insert(make_pair("dropped_no_queue",
             magg_stats.get_messages_sent_dropped_no_queue()));
    csev.insert(make_pair("dropped_no_client",
             magg_stats.get_messages_sent_dropped_no_client()));
    csev.insert(make_pair("dropped_no_session",
             magg_stats.get_messages_sent_dropped_no_session()));
    csev.insert(make_pair("dropped_queue_level",
             magg_stats.get_messages_sent_dropped_queue_level()));
    csev.insert(make_pair("dropped_client_send_failed",
             magg_stats.get_messages_sent_dropped_client_send_failed()));
    csev.insert(make_pair("dropped_session_not_connected",
             magg_stats.get_messages_sent_dropped_session_not_connected()));
    csev.insert(make_pair("dropped_header_write_failed",
             magg_stats.get_messages_sent_dropped_header_write_failed()));
    csev.insert(make_pair("dropped_write_failed",
             magg_stats.get_messages_sent_dropped_write_failed()));
    csev.insert(make_pair("dropped_wrong_client_sm_state",
             magg_stats.get_messages_sent_dropped_wrong_client_sm_state()));
    csev.insert(make_pair("dropped_validation_failed",
             magg_stats.get_messages_sent_dropped_validation_failed()));
    csev.insert(make_pair("dropped_rate_limited",
             magg_stats.get_messages_sent_dropped_rate_limited()));
    mcs.set_tx_msg_agg(csev);

    map <string,SandeshMessageStats> csevm;
    for (vector<SandeshMessageTypeStats>::const_iterator smit = mtype_stats.begin();
            smit != mtype_stats.end(); smit++) {
        SandeshMessageStats res_sms;
        const SandeshMessageStats& src_sms = smit->get_stats();
        res_sms.set_messages_sent(src_sms.get_messages_sent());
        res_sms.set_messages_sent_dropped_no_queue(src_sms.get_messages_sent_dropped_no_queue());
        res_sms.set_messages_sent_dropped_no_client(src_sms.get_messages_sent_dropped_no_client());
        res_sms.set_messages_sent_dropped_no_session(src_sms.get_messages_sent_dropped_no_session());
        res_sms.set_messages_sent_dropped_queue_level(src_sms.get_messages_sent_dropped_queue_level());
        res_sms.set_messages_sent_dropped_client_send_failed(src_sms.get_messages_sent_dropped_client_send_failed());
        res_sms.set_messages_sent_dropped_session_not_connected(src_sms.get_messages_sent_dropped_session_not_connected());
        res_sms.set_messages_sent_dropped_header_write_failed(src_sms.get_messages_sent_dropped_header_write_failed());
        res_sms.set_messages_sent_dropped_write_failed(src_sms.get_messages_sent_dropped_write_failed());
        res_sms.set_messages_sent_dropped_wrong_client_sm_state(src_sms.get_messages_sent_dropped_wrong_client_sm_state());
        res_sms.set_messages_sent_dropped_validation_failed(src_sms.get_messages_sent_dropped_validation_failed());
        res_sms.set_messages_sent_dropped_rate_limited(src_sms.get_messages_sent_dropped_rate_limited());
      
        csevm.insert(make_pair(smit->get_message_type(), res_sms));
    }
    mcs.set_msg_type_agg(csevm);

    SandeshModuleClientTrace::Send(mcs);
}

void SandeshClient::SetSessionWaterMarkInfo(
    Sandesh::QueueWaterMarkInfo &scwm) {
    SandeshSession *session = sm_->session();
    if (session) {
        session->SetSendQueueWaterMark(scwm);
    }
    session_wm_info_.push_back(scwm);
}

void SandeshClient::ResetSessionWaterMarkInfo() {
    SandeshSession *session = sm_->session();
    if (session) {
        session->ResetSendQueueWaterMark();
    }
    session_wm_info_.clear();
}    

void SandeshClient::GetSessionWaterMarkInfo(
    std::vector<Sandesh::QueueWaterMarkInfo> &scwm_info) const {
    scwm_info = session_wm_info_;
}

TcpSession *SandeshClient::AllocSession(Socket *socket) {
    return new SandeshSession(this, socket, session_task_instance_, 
                              session_writer_task_id_,
                              session_reader_task_id_);
}
