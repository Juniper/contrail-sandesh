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

#include <base/logging.h>
#include <base/task_annotations.h>
#include <io/event_manager.h>
#include <io/tcp_session.h>
#include <io/tcp_server.h>

#include <sandesh/sandesh_constants.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_session.h>

#include "sandesh_state_machine.h"
#include "../common/sandesh_req_types.h"

#include <sandesh/protocol/TXMLProtocol.h>
#include <sandesh/sandesh_ctrl_types.h>
#include <sandesh/common/vns_constants.h>
#include "sandesh_client.h"
#include "sandesh_uve.h"

using namespace boost::asio;
using boost::system::error_code;
using std::string;
using std::map;
using std::vector;

using boost::system::error_code;

const std::string SandeshClient::kSMTask = "sandesh::SandeshClientSM";
const std::string SandeshClient::kSessionTask = "sandesh::SandeshClientSession";
bool SandeshClient::task_policy_set_ = false;

SandeshClient::SandeshClient(EventManager *evm,
        Endpoint primary, Endpoint secondary, Sandesh::CollectorSubFn csf)
    :   TcpServer(evm),
        sm_task_instance_(kSMTaskInstance),
        sm_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSMTask)),
        session_task_instance_(kSessionTaskInstance),
        session_task_id_(TaskScheduler::GetInstance()->GetTaskId(kSessionTask)),
        primary_(primary),
        secondary_(secondary),
        csf_(csf),
        sm_(SandeshClientSM::CreateClientSM(evm, this, sm_task_instance_, sm_task_id_)) {
    LOG(INFO,"primary  " << primary_);
    LOG(INFO,"secondary  " << secondary_);

    // Set task policy for exclusion between state machine and session task since
    // session delete happens in state machine task
    if (!task_policy_set_) {
        TaskPolicy sm_task_policy = boost::assign::list_of
                (TaskExclusion(session_task_id_));
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
        LOG(INFO, "DiscUpdate for primary " << primary);
    }
    if (resp.size()>=2) {
        secondary = resp[1].ep;
        LOG(INFO, "DiscUpdate for secondary " << secondary);
    }
    if (primary!=Endpoint()) {
        sm_->SetCandidates(primary, secondary);
    }
}

void SandeshClient::Initiate() {
    sm_->SetAdminState(false);
    if (primary_ != Endpoint())
        sm_->SetCandidates(primary_,secondary_);
    if (csf_!=0){
        LOG(INFO, "Subscribe to Discovery Service for Collector" );
        csf_(g_vns_constants.ModuleNames.find(Module::COLLECTOR)->second, 2, 
            boost::bind(&SandeshClient::CollectorHandler, this, _1));
    }
}

void SandeshClient::Shutdown() {
    sm_->SetAdminState(true);
}


bool SandeshClient::ReceiveCtrlMsg(const std::string &msg,
        const SandeshHeader &header, const std::string &sandesh_name,
        const uint32_t header_offset) {

    Sandesh * sandesh = SandeshSession::DecodeCtrlSandesh(msg, header, sandesh_name, header_offset);

    const SandeshCtrlServerToClient * snh = dynamic_cast<const SandeshCtrlServerToClient *>(sandesh);
    if (!snh) {
        LOG(DEBUG, "Received Ctrl Message with wrong type " << sandesh->Name());
        sandesh->Release();
        return false;
    }
    if (!snh->get_success()) {
        LOG(DEBUG, "Received Ctrl Message : Connection with server has failed");
        sandesh->Release();
        return false;
    }
    LOG(DEBUG, "Received Ctrl Message with size " << snh->get_type_info().size());

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

    if (header_offset == 0) {
        return false;
    }

    Sandesh::UpdateSandeshStats(sandesh_name, msg.size(), false);

    if (header.get_Hints() & g_sandesh_constants.SANDESH_CONTROL_HINT) {
        return ReceiveCtrlMsg(msg, header, sandesh_name, header_offset);
    }

    // Create and process the sandesh
    Sandesh *sandesh = SandeshBaseFactory::CreateInstance(sandesh_name);
    if (sandesh == NULL) {
        LOG(ERROR, __func__ << ": Unknown sandesh: " << sandesh_name);
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
        LOG(ERROR, __func__ << ": Decoding " << sandesh_name << " FAILED");
        return false;
    }

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
        SandeshSession::ReceiveMsgCb rmcb,
        TcpServer::Endpoint ep) {
    TcpSession *session = TcpServer::CreateSession();
    Socket *socket = session->socket();

    error_code ec;
    socket->open(ip::tcp::v4(), ec);
    if (ec) {
        LOG(ERROR, __func__ << " Open FAILED: " << ec.message());
        DeleteSession(session);
        return NULL;
    }
    socket->non_blocking(true, ec);
    if (ec) {
        LOG(ERROR, __func__ << " Unable to set the socket as non-blocking: " << ec.message());
        DeleteSession(session);
        return NULL;
    }
    SandeshSession *sandesh_session =
            static_cast<SandeshSession *>(session);
    sandesh_session->SetReceiveMsgCb(rmcb);
    sandesh_session->SetConnection(NULL);
    sandesh_session->set_observer(eocb);
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
    LOG(DEBUG, "Sending Ctrl Message for " << Sandesh::module() << " count " << count);

    SandeshCtrlClientToServer::Request(Sandesh::source(), Sandesh::module(),
            count, stv, getpid(), Sandesh::http_port(), "ctrl");

}

static bool client_start = false;
static uint64_t client_start_time;

void SandeshClient::SendUVE(int count,
        const string & stateName, const string & server,
        Endpoint primary, Endpoint secondary) {
    ModuleClientState mcs;
    mcs.set_name(Sandesh::source() + ":" + Sandesh::module());
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
    mcs.set_client_info(sci);
    SandeshModuleClientTrace::Send(mcs);
}

TcpSession *SandeshClient::AllocSession(Socket *socket) {
    return new SandeshSession(this, socket, session_task_instance_, session_task_id_);
}
