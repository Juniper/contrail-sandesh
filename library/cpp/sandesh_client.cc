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
#include <boost/foreach.hpp>

#include <base/task_annotations.h>
#include <base/time_util.h>
#include <io/event_manager.h>
#include <io/tcp_session.h>
#include <io/tcp_server.h>

#include <sandesh/sandesh_constants.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_trace.h>
#include <sandesh/sandesh_session.h>

#include "sandesh_state_machine.h"

#include <sandesh/protocol/TXMLProtocol.h>
#include <sandesh/sandesh_ctrl_types.h>
#include <sandesh/sandesh_uve_types.h>
#include <sandesh/derived_stats_results_types.h>
#include <sandesh/common/vns_constants.h>
#include "sandesh_client.h"
#include "sandesh_uve.h"
#include "sandesh_util.h"

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
                                               (15*1024*1024, SandeshLevel::SYS_UVE, true, false)
                                               (100*1024, SandeshLevel::SYS_EMERG, true, false)
                                               (50*1024, SandeshLevel::SYS_ERR, true, false)
                                               (10*1024, SandeshLevel::SYS_DEBUG, true, false)
                                               (5*1024*1024, SandeshLevel::SYS_EMERG, false, false)
                                               (75*1024, SandeshLevel::SYS_ERR, false, false)
                                               (30*1024, SandeshLevel::SYS_DEBUG, false, false)
                                               (2*1024, SandeshLevel::INVALID, false, false);

SandeshClient::SandeshClient(EventManager *evm,
        const std::vector<Endpoint> &collectors,
        const SandeshConfig &config,
        bool periodicuve)
    :   SslServer(evm, boost::asio::ssl::context::tlsv1_client,
                  config.sandesh_ssl_enable),
        sm_task_instance_(kSMTaskInstance),
        sm_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSMTask)),
        session_task_instance_(kSessionTaskInstance),
        session_writer_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSessionWriterTask)),
        session_reader_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSessionReaderTask)),
        dscp_value_(0),
        collectors_(collectors),
        sm_(SandeshClientSM::CreateClientSM(evm, this, sm_task_instance_, sm_task_id_, periodicuve)),
        session_wm_info_(kSessionWaterMarkInfo),
        session_close_interval_msec_(0),
        session_close_time_usec_(0) {
    // Set task policy for exclusion between state machine and session tasks since
    // session delete happens in state machine task
    if (!task_policy_set_) {
        TaskPolicy sm_task_policy = boost::assign::list_of
                (TaskExclusion(session_writer_task_id_))
                (TaskExclusion(session_reader_task_id_));
        TaskScheduler::GetInstance()->SetPolicy(sm_task_id_, sm_task_policy);
        task_policy_set_ = true;
    }
    if (config.sandesh_ssl_enable) {
        boost::asio::ssl::context *ctx = context();
        boost::system::error_code ec;
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::no_sslv2, ec);
        if (ec.value() != 0) {
            SANDESH_LOG(ERROR, "Error setting ssl options: " << ec.message());
            exit(EINVAL);
        }
        // CA certificate
        if (!config.ca_cert.empty()) {
            // Verify that the peer certificate is signed by a trusted CA
            ctx->set_verify_mode(boost::asio::ssl::verify_peer |
                                 boost::asio::ssl::verify_fail_if_no_peer_cert,
                                 ec);
            if (ec.value() != 0) {
                SANDESH_LOG(ERROR, "Error setting verification mode: " <<
                            ec.message());
                exit(EINVAL);
            }
            ctx->load_verify_file(config.ca_cert, ec);
            if (ec.value() != 0) {
                SANDESH_LOG(ERROR, "Error loading CA certificate: " <<
                            ec.message());
                exit(EINVAL);
            }
        }
        // Server certificate
        ctx->use_certificate_chain_file(config.certfile, ec);
        if (ec.value() != 0) {
            SANDESH_LOG(ERROR, "Error using server certificate: " <<
                        ec.message());
            exit(EINVAL);
        }
        // Server private key
        ctx->use_private_key_file(config.keyfile,
                                  boost::asio::ssl::context::pem, ec);
        if (ec.value() != 0) {
            SANDESH_LOG(ERROR, "Error using server private key file: " <<
                        ec.message());
            exit(EINVAL);
        }
    }
}

SandeshClient::~SandeshClient() {}

void SandeshClient::ReConfigCollectors(
        const std::vector<std::string>& collector_list) {
    std::vector<Endpoint> collector_endpoints;

    BOOST_FOREACH(const std::string& collector, collector_list) {
        Endpoint ep;
        if (!MakeEndpoint(&ep, collector)) {
            SANDESH_LOG(ERROR, __func__ << ": Invalid collector address: " <<
                        collector);
            return;
        }
        collector_endpoints.push_back(ep);
    }
    sm_->SetCollectors(collector_endpoints);
}

void SandeshClient::Initiate() {
    sm_->SetAdminState(false);
    if (collectors_.size())
        sm_->SetCollectors(collectors_);
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
    TcpSession *session = SslServer::CreateSession();
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
    if (dscp_value_) {
        session->SetDscpSocketOption(dscp_value_);
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

bool SandeshClient::CloseSMSessionInternal() {
    SandeshSession *session(sm_->session());
    if (session) {
        session->EnqueueClose();
        return true;
    }
    return false;
}

bool DoCloseSMSession(uint64_t now_usec, uint64_t last_close_usec,
    uint64_t last_close_interval_usec, int *close_interval_msec) {
    // If this is the first time, we will accept the next close
    // only after the initial close interval time
    if (last_close_interval_usec == 0 || last_close_usec == 0) {
        *close_interval_msec =
            SandeshClient::kInitialSMSessionCloseIntervalMSec;
        return true;
    }
    assert(now_usec >= last_close_usec);
    uint64_t time_since_close_usec(now_usec - last_close_usec);
    // We will ignore close events receive before the last close
    // interval is finished
    if (time_since_close_usec <= last_close_interval_usec) {
        *close_interval_msec = 0;
        return false;
    }
    // We will double the close interval time if we get a close
    // event between last close interval and 2 * last close interval.
    // If the close event is between 2 * last close interval and
    // 4 * last close interval, then the close interval will be
    // same as the current close interval. If the close event is
    // after 4 * last close interval, then we will reset the close
    // interval to the initial close interval
    if (time_since_close_usec > last_close_interval_usec &&
        time_since_close_usec <= 2 * last_close_interval_usec) {
        uint64_t nclose_interval_msec((2 * last_close_interval_usec)/1000);
        *close_interval_msec = std::min(nclose_interval_msec,
            static_cast<uint64_t>(
            SandeshClient::kMaxSMSessionCloseIntervalMSec));
        return true;
    } else if ((2 * last_close_interval_usec <= time_since_close_usec) &&
        (time_since_close_usec <= 4 * last_close_interval_usec)) {
        *close_interval_msec = last_close_interval_usec/1000;
        return true;
    } else {
        *close_interval_msec =
            SandeshClient::kInitialSMSessionCloseIntervalMSec;
        return true;
    }
}

bool SandeshClient::CloseSMSession() {
    uint64_t now_usec(UTCTimestampUsec());
    int close_interval_msec(0);
    bool close(DoCloseSMSession(now_usec, session_close_time_usec_,
        session_close_interval_msec_ * 1000, &close_interval_msec));
    if (close) {
        session_close_time_usec_ = now_usec;
        session_close_interval_msec_ = close_interval_msec;
        return CloseSMSessionInternal();
    }
    return false;
}

static bool client_start = false;
static uint64_t client_start_time;

void SandeshClient::SendUVE(int count,
        const string & stateName, const string & server,
        const Endpoint & server_ip,
        const std::vector<TcpServer::Endpoint> & collector_eps) {
    ModuleClientState mcs;
    mcs.set_name(Sandesh::source() + ":" + Sandesh::node_type() +
        ":" + Sandesh::module() + ":" + Sandesh::instance_id());
    SandeshClientInfo sci;
    if (!client_start) {
        client_start_time = UTCTimestampUsec(); 
        client_start = true;
    }
    sci.set_start_time(client_start_time);
    sci.set_successful_connections(count);
    sci.set_pid(getpid());
    sci.set_http_port(Sandesh::http_port());
    sci.set_status(stateName);
    sci.set_collector_name(server);
    std::ostringstream collector_ip;
    collector_ip << server_ip;
    sci.set_collector_ip(collector_ip.str());
    std::vector<std::string> collectors;
    BOOST_FOREACH(const TcpServer::Endpoint& ep, collector_eps) {
        std::ostringstream collector_ip;
        collector_ip << ep;
        collectors.push_back(collector_ip.str());
    }
    sci.set_collector_list(collectors);
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
    csev.insert(make_pair("dropped_sending_disabled",
             magg_stats.get_messages_sent_dropped_sending_disabled()));
    csev.insert(make_pair("dropped_sending_to_syslog",
             magg_stats.get_messages_sent_dropped_sending_to_syslog()));
    mcs.set_tx_msg_agg(csev);

    map <string,SandeshMessageStats> csevm;
    for (vector<SandeshMessageTypeStats>::const_iterator smit = mtype_stats.begin();
            smit != mtype_stats.end(); smit++) {
        SandeshMessageStats res_sms;
        const SandeshMessageStats& src_sms = smit->get_stats();
        res_sms.set_messages_sent(src_sms.get_messages_sent());
        res_sms.set_messages_sent_dropped_no_queue(
            src_sms.get_messages_sent_dropped_no_queue());
        res_sms.set_messages_sent_dropped_no_client(
            src_sms.get_messages_sent_dropped_no_client());
        res_sms.set_messages_sent_dropped_no_session(
            src_sms.get_messages_sent_dropped_no_session());
        res_sms.set_messages_sent_dropped_queue_level(
            src_sms.get_messages_sent_dropped_queue_level());
        res_sms.set_messages_sent_dropped_client_send_failed(
            src_sms.get_messages_sent_dropped_client_send_failed());
        res_sms.set_messages_sent_dropped_session_not_connected(
            src_sms.get_messages_sent_dropped_session_not_connected());
        res_sms.set_messages_sent_dropped_header_write_failed(
            src_sms.get_messages_sent_dropped_header_write_failed());
        res_sms.set_messages_sent_dropped_write_failed(
            src_sms.get_messages_sent_dropped_write_failed());
        res_sms.set_messages_sent_dropped_wrong_client_sm_state(
            src_sms.get_messages_sent_dropped_wrong_client_sm_state());
        res_sms.set_messages_sent_dropped_validation_failed(
            src_sms.get_messages_sent_dropped_validation_failed());
        res_sms.set_messages_sent_dropped_rate_limited(
            src_sms.get_messages_sent_dropped_rate_limited());
        res_sms.set_messages_sent_dropped_sending_disabled(
            src_sms.get_messages_sent_dropped_sending_disabled());
        res_sms.set_messages_sent_dropped_sending_to_syslog(
            src_sms.get_messages_sent_dropped_sending_to_syslog());
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

SslSession *SandeshClient::AllocSession(SslSocket *socket) {
    return new SandeshSession(this, socket, session_task_instance_, 
                              session_writer_task_id_,
                              session_reader_task_id_);
}

void SandeshClient::SetDscpValue(uint8_t value) {
    if (value == dscp_value_)
        return;

    dscp_value_ = value;
    SandeshSession *sess = session();
    if (sess) {
        sess->SetDscpSocketOption(value);
    }

}
