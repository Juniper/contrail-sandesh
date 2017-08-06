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
#include <io/ssl_server.h>
#include <io/ssl_session.h>
#include <base/queue_task.h>
#include <base/timer.h>
#include <sandesh/sandesh_session.h>
#include "sandesh_client_sm.h"


class SandeshClient;
class Sandesh;
class SandeshUVE;
class SandeshHeader;

bool DoCloseSMSession(uint64_t now_usec, uint64_t last_close_usec,
    uint64_t last_close_interval_usec, int *close_interval_msec);

class SandeshClient : public SslServer, public SandeshClientSM::Mgr {
public:
    static const int kInitialSMSessionCloseIntervalMSec = 10 * 1000;
    static const int kMaxSMSessionCloseIntervalMSec = 60 * 1000;
    
    SandeshClient(EventManager *evm, const std::vector<Endpoint> &collectors,
             const SandeshConfig &config,
             bool periodicuve = false);

    virtual ~SandeshClient();

    void Initiate();
    void Shutdown();

    virtual SandeshSession *CreateSMSession(
            SslSession::EventObserver eocb,
            SandeshReceiveMsgCb rmcb,
            TcpServer::Endpoint ep);

    void InitializeSMSession(int connects);
    void DeleteSMSession(SandeshSession * session) {
        DeleteSession(session);
    } 
    bool CloseSMSession();
    bool ReceiveMsg(const std::string& msg,
        const SandeshHeader &header, const std::string &sandesh_name,
        const uint32_t header_offset);
    void SendUVE(int count,
        const std::string & stateName, const std::string & server,
        const Endpoint & server_ip, const std::vector<Endpoint> & collector_eps);

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

    void SetDscpValue(uint8_t value);

    void SetSessionWaterMarkInfo(Sandesh::QueueWaterMarkInfo &scwm);
    void ResetSessionWaterMarkInfo();
    void GetSessionWaterMarkInfo(
        std::vector<Sandesh::QueueWaterMarkInfo> &scwm_info) const;
    void ReConfigCollectors(const std::vector<std::string>&);

    int session_close_interval_msec() const {
        return session_close_interval_msec_;
    }
    uint64_t session_close_time_usec() const {
        return session_close_time_usec_;
    }

    friend class CollectorInfoRequest;
protected:
    virtual SslSession *AllocSession(SslSocket *socket);
    bool CloseSMSessionInternal();

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
    uint8_t dscp_value_;
    std::vector<Endpoint> collectors_;
    boost::scoped_ptr<SandeshClientSM> sm_;
    std::vector<Sandesh::QueueWaterMarkInfo> session_wm_info_;
    static bool task_policy_set_;
    int session_close_interval_msec_;
    uint64_t session_close_time_usec_;

    bool ReceiveCtrlMsg(const std::string &msg,
        const SandeshHeader &header, const std::string &sandesh_name,
        const uint32_t header_offset);

    DISALLOW_COPY_AND_ASSIGN(SandeshClient);
};

#endif // __SANDESH_CLIENT_H__


