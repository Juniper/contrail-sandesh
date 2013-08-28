/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_connection.cc
//
// Sandesh connection management implementation
//

#include <io/tcp_session.h>
#include <io/tcp_server.h>
#include <base/logging.h>
#include <sandesh/protocol/TXMLProtocol.h>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_ctrl_types.h>
#include <sandesh/common/vns_types.h>
#include <sandesh/common/vns_constants.h>
#include "sandesh_uve.h"
#include "sandesh_session.h"
#include "sandesh_client.h"
#include "sandesh_server.h"
#include "sandesh_connection.h"

using namespace std;
using boost::system::error_code;

SandeshConnection::SandeshConnection(const char *prefix, TcpServer *server, Endpoint endpoint)
    : server_(server),
      endpoint_(endpoint),
      admin_down_(false),
      session_(NULL),
      state_machine_(new SandeshStateMachine(prefix, this)) {
}

SandeshConnection::~SandeshConnection() {
    if (session_ != NULL) {
        state_machine_->set_session(NULL);
        session_ = NULL;
    }
}

void SandeshConnection::set_session(SandeshSession *session) {
    session_ = session;
}

SandeshSession *SandeshConnection::session() const {
    return session_;
}

void SandeshConnection::ReceiveMsg(const std::string &msg, SandeshSession *session) {
    state_machine_->OnSandeshMessage(session, msg);
}


SandeshSession *SandeshConnection::CreateSession() {
    TcpSession *session = server_->CreateSession();
    SandeshSession *sandesh_session =
            static_cast<SandeshSession *>(session);
    sandesh_session->SetReceiveMsgCb(
            boost::bind(&SandeshConnection::ReceiveMsg, this, _1, _2));
    sandesh_session->SetConnection(this);
    return sandesh_session;
}

ssm::SsmState SandeshConnection::GetStateMachineState() const {
    SandeshStateMachine *sm = state_machine();
    assert(sm);
    return sm->get_state();
}

void SandeshConnection::AcceptSession(SandeshSession *session) {
    session->SetReceiveMsgCb(boost::bind(&SandeshConnection::ReceiveMsg, this, _1, _2));
    session->SetConnection(this);
    state_machine_->PassiveOpen(session);
}

bool SandeshConnection::Send(const uint8_t *data, size_t size) {
    if (session_ == NULL) return false;
    // Session send
    return true;
}

SandeshStateMachine *SandeshConnection::state_machine() const {
    return state_machine_.get();
}

void SandeshConnection::SetAdminState(bool down) {
    if (admin_down_ != down) {
        admin_down_ = down;
        state_machine_->SetAdminState(down);
    }
}

SandeshServerConnection::SandeshServerConnection(TcpServer *server, Endpoint endpoint)
    : SandeshConnection("SandeshServer: ", server, endpoint) {
}

SandeshServerConnection::~SandeshServerConnection() {
}

bool SandeshServerConnection::IsClient() const {
    return false;
}

bool SandeshServerConnection::HandleSandeshCtrlMessage(const std::string &msg,
        const SandeshHeader &header, const std::string sandesh_name,
        const uint32_t header_offset) {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    if (!sserver) {
        return false;
    }
    if (header_offset == 0) {
        return false;
    }
    Sandesh *ctrl_snh = SandeshSession::DecodeCtrlSandesh(msg, header, sandesh_name,
                                                          header_offset);
    bool ret = sserver->ReceiveSandeshCtrlMsg(state_machine(), session(), ctrl_snh);
    ctrl_snh->Release();
    return ret;
}

bool SandeshServerConnection::HandleSandeshMessage(const std::string &msg,
        const SandeshHeader &header, const std::string sandesh_name,
        const uint32_t header_offset) {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    if (!sserver) {
        return false;
    }
    if (header_offset == 0) {
        return false;
    }
    sserver->ReceiveSandeshMsg(session(), msg, sandesh_name, header, header_offset);
    return true;
}

bool SandeshServerConnection::HandleMessage(ssm::Message *msg) {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    if (!sserver) {
        return false;
    }
    return sserver->ReceiveMsg(session(), msg);
}

void SandeshServerConnection::HandleDisconnect(SandeshSession * sess) {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    sserver->DisconnectSession(sess);
}
