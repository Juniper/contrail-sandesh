/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_client.h
//
// Sandesh Analytics Database Client
//

#ifndef __SANDESH_CLIENT_H__
#define __SANDESH_CLIENT_H__

#include <boost/asio.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/tuple/tuple.hpp>

#include <tbb/mutex.h>
#include <tbb/atomic.h>

#include <io/tcp_server.h>
#include <io/tcp_session.h>
#include <base/queue_task.h>
#include <base/timer.h>
#include <sandesh/sandesh_session.h>
#include "sandesh_client_sm.h"


class SandeshClient;
class Sandesh;
class SandeshUVE;
class SandeshHeader;


class SandeshClient : public TcpServer, public SandeshClientSM::Mgr {
public:
    
    SandeshClient(EventManager *evm, Endpoint primary,
             Endpoint secondary = Endpoint(),
             Sandesh::CollectorSubFn csf = 0,
             bool periodicuve = false);

    virtual ~SandeshClient();

    void Initiate();
    void Shutdown();

    virtual SandeshSession *CreateSMSession(
            TcpSession::EventObserver eocb,
            SandeshReceiveMsgCb rmcb,
            TcpServer::Endpoint ep);

    void InitializeSMSession(int connects);
    void DeleteSMSession(SandeshSession * session) {
        DeleteSession(session);
    } 
    bool ReceiveMsg(const std::string& msg,
        const SandeshHeader &header, const std::string &sandesh_name,
        const uint32_t header_offset);
    void SendUVE(int count,
        const std::string & stateName, const std::string & server,
        Endpoint primary, Endpoint secondary);

    bool SendSandesh(Sandesh *snh);

    bool SendSandeshUVE(Sandesh *snh_uve) {
        return sm_->SendSandeshUVE(snh_uve);
    }
    
    SandeshClientSM::State state() {
        return sm_->state();
    }

    bool IsSession() {
        if (sm_->session()) return true;
        else return false;
    }

    SandeshSession * session() const {
        return sm_->session();
    }

    SandeshClientSM* state_machine() const {
        return sm_.get();
    }

    void SetSessionWaterMarkInfo(Sandesh::QueueWaterMarkInfo &scwm);
    void ResetSessionWaterMarkInfo();
    void GetSessionWaterMarkInfo(
        std::vector<Sandesh::QueueWaterMarkInfo> &scwm_info) const;

    friend class CollectorInfoRequest;
protected:
    virtual TcpSession *AllocSession(Socket *socket);

private:
    static const int kSMTaskInstance = 0;
    static const std::string kSMTask;
    static const int kSessionTaskInstance = Task::kTaskInstanceAny;
    static const std::string kSessionWriterTask;
    static const std::string kSessionReaderTask;
    static const std::vector<Sandesh::QueueWaterMarkInfo> kSessionWaterMarkInfo;

    int sm_task_instance_;
    int sm_task_id_;
    int session_task_instance_;
    int session_writer_task_id_;
    int session_reader_task_id_;
    Endpoint primary_, secondary_;
    Sandesh::CollectorSubFn csf_;
    boost::scoped_ptr<SandeshClientSM> sm_;
    std::vector<Sandesh::QueueWaterMarkInfo> session_wm_info_;
    static bool task_policy_set_;

    void CollectorHandler(std::vector<DSResponse> resp);
    bool ReceiveCtrlMsg(const std::string &msg,
        const SandeshHeader &header, const std::string &sandesh_name,
        const uint32_t header_offset);

    DISALLOW_COPY_AND_ASSIGN(SandeshClient);
};

#endif // __SANDESH_CLIENT_H__


