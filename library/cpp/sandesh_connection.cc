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

#define CONNECTION_LOG(_Level, _Msg)                                           \
    do {                                                                       \
        if (LoggingDisabled()) break;                                          \
        log4cplus::Logger _Xlogger = log4cplus::Logger::getRoot();             \
        if (_Xlogger.isEnabledFor(log4cplus::_Level##_LOG_LEVEL)) {            \
            log4cplus::tostringstream _Xbuf;                                   \
            if (!state_machine()->generator_key().empty()) {                   \
                _Xbuf << state_machine()->generator_key() << " (" <<           \
                    GetTaskInstance() << ") ";                                 \
            }                                                                  \
            if (session()) {                                                   \
                _Xbuf << session()->ToString() << " ";                         \
            }                                                                  \
            _Xbuf << _Msg;                                                     \
            _Xlogger.forcedLog(log4cplus::_Level##_LOG_LEVEL,                  \
                               _Xbuf.str());                                   \
        }                                                                      \
    } while (false)

SandeshConnection::SandeshConnection(const char *prefix, TcpServer *server,
        Endpoint endpoint, int task_instance, int task_id)
    : is_deleted_(false),
      server_(server),
      endpoint_(endpoint),
      admin_down_(false),
      session_(NULL),
      task_instance_(task_instance),
      task_id_(task_id),
      state_machine_(new SandeshStateMachine(prefix, this)) {
}

SandeshConnection::~SandeshConnection() {
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

bool SandeshConnection::SendSandesh(Sandesh *snh) {
    if (!session_) {
        return false;
    }
    ssm::SsmState state = state_machine_->get_state();
    if (state != ssm::SERVER_INIT && state != ssm::ESTABLISHED) {
        return false;
    }
    // XXX No bounded work queue
    session_->send_queue()->Enqueue(snh);
    return true;
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

void SandeshConnection::AcceptSession(SandeshSession *session) {
    session->SetReceiveMsgCb(boost::bind(&SandeshConnection::ReceiveMsg, this, _1, _2));
    session->SetConnection(this);
    state_machine_->PassiveOpen(session);
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

void SandeshConnection::Shutdown() {
    is_deleted_ = true;
    deleter()->Delete();
}

bool SandeshConnection::MayDelete() const {
    // XXX Do we have any dependencies?
    return true;
}

class SandeshServerConnection::DeleteActor : public LifetimeActor {
public:
    DeleteActor(SandeshServer *server, SandeshServerConnection *parent)
        : LifetimeActor(server->lifetime_manager()),
          server_(server), parent_(parent) {
    }
    virtual bool MayDelete() const {
        return parent_->MayDelete();
    }
    virtual void Shutdown() {
        SandeshSession *session = NULL;
        if (parent_->state_machine()) {
            session = parent_->state_machine()->session();
            parent_->state_machine()->clear_session();
        }
        if (session) {
            server_->DeleteSession(session);
        }
        parent_->is_deleted_ = true;
    }
    virtual void Destroy() {
        parent_->Destroy();
    }

private:
    SandeshServer *server_;
    SandeshServerConnection *parent_;
};

SandeshServerConnection::SandeshServerConnection(
        TcpServer *server, Endpoint endpoint, int task_instance, int task_id)
    : SandeshConnection("SandeshServer: ", server, endpoint, task_instance, task_id),
      deleter_(new DeleteActor(dynamic_cast<SandeshServer *>(server), this)),
      server_delete_ref_(this, dynamic_cast<SandeshServer *>(server)->deleter()) {
}

SandeshServerConnection::~SandeshServerConnection() {
}

void SandeshServerConnection::ManagedDelete() {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    if (!sserver) {
        CONNECTION_LOG(ERROR, __func__ << " No Server");
        return;
    }
    sserver->lifetime_manager()->Enqueue(deleter_.get());
}

bool SandeshServerConnection::ProcessSandeshCtrlMessage(const std::string &msg,
        const SandeshHeader &header, const std::string sandesh_name,
        const uint32_t header_offset) {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    if (!sserver) {
        CONNECTION_LOG(ERROR, __func__ << " No Server");
        return false;
    }
    if (header_offset == 0) {
        CONNECTION_LOG(ERROR, __func__ << " No Sandesh Header");
        return false;
    }
    Sandesh *ctrl_snh = SandeshSession::DecodeCtrlSandesh(msg, header, sandesh_name,
                                                          header_offset);
    bool ret = sserver->ReceiveSandeshCtrlMsg(state_machine(), session(), ctrl_snh);
    ctrl_snh->Release();
    return ret;
}

bool SandeshServerConnection::ProcessSandeshMessage(const std::string &msg,
        const SandeshHeader &header, const std::string sandesh_name,
        const uint32_t header_offset) {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    if (!sserver) {
        CONNECTION_LOG(ERROR, __func__ << " No Server");
        return false;
    }
    if (header_offset == 0) {
        CONNECTION_LOG(ERROR, __func__ << " No Sandesh Header");
        return false;
    }
    sserver->ReceiveSandeshMsg(session(), msg, sandesh_name, header, header_offset);
    return true;
}

bool SandeshServerConnection::ProcessMessage(ssm::Message *msg) {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    if (!sserver) {
        CONNECTION_LOG(ERROR, __func__ << " No Server");
        return false;
    }
    return sserver->ReceiveMsg(session(), msg);
}

void SandeshServerConnection::ProcessDisconnect(SandeshSession * sess) {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    if (!sserver) {
        CONNECTION_LOG(ERROR, __func__ << " No Server");
        return;
    }
    sserver->DisconnectSession(sess);
}

LifetimeManager *SandeshServerConnection::lifetime_manager() {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    if (!sserver) {
        CONNECTION_LOG(ERROR, __func__ << " No Server");
        return NULL;
    }
    return sserver->lifetime_manager();
}

void SandeshServerConnection::Destroy() {
    SandeshServer *sserver = dynamic_cast<SandeshServer *>(server());
    if (!sserver) {
        CONNECTION_LOG(ERROR, __func__ << " No Server");
        return;
    }
    sserver->RemoveConnection(this);
    int index = GetTaskInstance();
    if (index != -1) {
        sserver->FreeConnectionIndex(index);
    }
};

LifetimeActor *SandeshServerConnection::deleter() {
    return deleter_.get();
}
