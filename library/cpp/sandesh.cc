/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh.cc
//
// Sandesh Implementation
//

#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>
#include <base/logging.h>
#include <base/parse_object.h>
#include <base/queue_task.h>
#include <http/http_session.h>
#include <io/tcp_session.h>

#include <sandesh/transport/TBufferTransports.h>
#include <sandesh/protocol/TBinaryProtocol.h>
#include <sandesh/protocol/TProtocol.h>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_trace.h>
#include <sandesh/sandesh_uve_types.h>
#include "sandesh_statistics.h"
#include "sandesh_uve.h"
#include "sandesh_session.h"
#include "sandesh_http.h"
#include "sandesh_client.h"
#include "sandesh_connection.h"
#include "sandesh_state_machine.h"

using boost::asio::ip::tcp;
using boost::asio::ip::address;

using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;

// Statics
Sandesh::SandeshRole::type Sandesh::role_ = SandeshRole::Invalid;
bool Sandesh::enable_local_log_ = false;
bool Sandesh::enable_flow_log_ = false;
int Sandesh::http_port_ = 0;
bool Sandesh::enable_trace_print_ = false;
bool Sandesh::send_queue_enabled_ = true;
bool Sandesh::connect_to_collector_ = false;
SandeshLevel::type Sandesh::sending_level_ = SandeshLevel::INVALID;
SandeshClient *Sandesh::client_ = NULL;
std::auto_ptr<Sandesh::SandeshRxQueue> Sandesh::recv_queue_;
std::string Sandesh::module_;
std::string Sandesh::source_;
std::string Sandesh::node_type_;
std::string Sandesh::instance_id_;
int Sandesh::recv_task_id_ = -1;
SandeshContext* Sandesh::client_context_ = NULL;
Sandesh::SandeshCallback Sandesh::response_callback_ = 0;
SandeshLevel::type Sandesh::logging_level_ = SandeshLevel::INVALID;
SandeshLevel::type Sandesh::logging_ut_level_ =
    getenv("SANDSH_UT_DEBUG") ? SandeshLevel::SYS_DEBUG : SandeshLevel::UT_DEBUG;
std::string Sandesh::logging_category_;
EventManager* Sandesh::event_manager_ = NULL;
SandeshMessageStatistics Sandesh::msg_stats_;
tbb::mutex Sandesh::stats_mutex_;
log4cplus::Logger Sandesh::logger_ =
    log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("SANDESH"));

Sandesh::ModuleContextMap Sandesh::module_context_;
tbb::atomic<uint32_t> Sandesh::sandesh_send_ratelimit_;

const char * Sandesh::SandeshRoleToString(SandeshRole::type role) {
    switch (role) {
    case SandeshRole::Generator:
        return "Generator";
    case SandeshRole::Collector:
        return "Collector";
    case SandeshRole::Test:
        return "Test";
    case SandeshRole::Invalid: 
        return "Invalid";
    default:
        return "Unknown";
    }
}

void Sandesh::InitReceive(int recv_task_inst) {
    assert(recv_task_id_ == -1);
    TaskScheduler *scheduler = TaskScheduler::GetInstance();
    recv_task_id_ = scheduler->GetTaskId("sandesh::RecvQueue");
    recv_queue_.reset(new SandeshRxQueue(recv_task_id_, recv_task_inst,
            &Sandesh::ProcessRecv));
}

void Sandesh::InitClient(EventManager *evm, Endpoint server) {
    connect_to_collector_ = true;
    SANDESH_LOG(INFO, "SANDESH: CONNECT TO COLLECTOR: " <<
        connect_to_collector_);
    // Create and initialize the client
    assert(client_ == NULL);
    client_ = new SandeshClient(evm, server);
    client_->Initiate();
}

static int32_t SandeshHttpCallback(SandeshRequest *rsnh) {
    return rsnh->Enqueue(Sandesh::recv_queue());
}

extern int PullSandeshGenStatsReq;
extern int PullSandeshUVE;
extern int PullSandeshTraceReq;

bool Sandesh::Initialize(SandeshRole::type role,
                         const std::string &module,
                         const std::string &source,
                         const std::string &node_type,
                         const std::string &instance_id,
                         EventManager *evm,
                         unsigned short http_port,
                         SandeshContext *client_context) {
    PullSandeshGenStatsReq = 1;
    PullSandeshUVE = 1;
    PullSandeshTraceReq = 1;

    if (role_ != SandeshRole::Invalid || role == SandeshRole::Invalid) {
        return true;
    }

    SANDESH_LOG(INFO, "SANDESH: ROLE             : " << SandeshRoleToString(role));
    SANDESH_LOG(INFO, "SANDESH: MODULE           : " << module);
    SANDESH_LOG(INFO, "SANDESH: SOURCE           : " << source);
    SANDESH_LOG(INFO, "SANDESH: NODE TYPE        : " << node_type);
    SANDESH_LOG(INFO, "SANDESH: INSTANCE ID      : " << instance_id);
    SANDESH_LOG(INFO, "SANDESH: HTTP SERVER PORT : " << http_port);

    role_           = role;
    module_         = module;
    source_         = source;
    node_type_      = node_type;
    instance_id_    = instance_id;
    client_context_ = client_context;
    event_manager_  = evm;
    //If Sandesh::sandesh_send_ratelimit_ is not defined by client,
    // assign a default value to it
    if (get_send_rate_limit() == 0) {
        set_send_rate_limit(g_sandesh_constants.DEFAULT_SANDESH_SEND_RATELIMIT);
    }

    InitReceive(Task::kTaskInstanceAny);
    bool success(SandeshHttp::Init(evm, module, http_port,
        &SandeshHttpCallback, &http_port_));
    if (!success) {
        SANDESH_LOG(ERROR, "SANDESH: HTTP INIT FAILED (PORT " <<
            http_port << ")");
        return false;
    }
    RecordPort("http", module_, http_port_);
    return true;
}

void Sandesh::RecordPort(const std::string& name, const std::string& module,
        unsigned short port) {
    int fd;
    std::ostringstream myfifoss;
    myfifoss << "/tmp/" << module << "." << getppid() << "." << name << "_port";
    std::string myfifo = myfifoss.str();
    std::ostringstream hss;
    hss << port << "\n";
    std::string hstr = hss.str();

    fd = open(myfifo.c_str(), O_WRONLY | O_NONBLOCK);
    if (fd != -1) {
        SANDESH_LOG(INFO, "SANDESH: Write " << name << "_port " << port <<
                          "TO : " << myfifo);
        write(fd, hstr.c_str(), hstr.length());
        close(fd);
    } else {
        SANDESH_LOG(INFO, "SANDESH: NOT Writing " << name << "_port " << port <<
                          "TO : " << myfifo);
    } 
}

bool Sandesh::ConnectToCollector(const std::string &collector_ip,
                                 int collector_port) {
    boost::system::error_code ec;
    address collector_addr = address::from_string(collector_ip, ec);
    if (ec) {
        SANDESH_LOG(ERROR, __func__ << ": Invalid collector address: " <<
                collector_ip << " Error: " << ec);
        return false;
    }

    SANDESH_LOG(INFO, "SANDESH: COLLECTOR : " << collector_ip);
    SANDESH_LOG(INFO, "SANDESH: COLLECTOR PORT : " << collector_port);

    tcp::endpoint collector(collector_addr, collector_port);
    InitClient(event_manager_, collector);
    return true;
}

static bool make_endpoint(TcpServer::Endpoint& ep,const std::string& epstr) {

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(":");

    tokenizer tokens(epstr, sep);
    tokenizer::iterator it = tokens.begin();
    std::string sip(*it);
    ++it;
    std::string sport(*it);
    int port;
    stringToInteger(sport, port);
    boost::system::error_code ec;
    address addr = address::from_string(sip, ec);
    if (ec) {
        SANDESH_LOG(ERROR, __func__ << ": Invalid collector address: " <<
                sip << " Error: " << ec);
        return false;
    }
    ep = TcpServer::Endpoint(addr, port);
    return true;
}

bool Sandesh::InitClient(EventManager *evm, 
                         const std::vector<std::string> &collectors,
                         CollectorSubFn csf) {
    connect_to_collector_ = true;
    SANDESH_LOG(INFO, "SANDESH: CONNECT TO COLLECTOR: " <<
        connect_to_collector_);

    Endpoint primary = Endpoint();
    Endpoint secondary = Endpoint();

    if (collectors.size()!=0) {
        if (!make_endpoint(primary, collectors[0])) {
            return false;
        }
        if (collectors.size()>1) {
            if (!make_endpoint(secondary, collectors[1])) {
                return false;
            } 
        }
    }
    if (collectors.size()>2) {
        SANDESH_LOG(ERROR, __func__ << " Too many collectors");
        return false;
    }

    client_ = new SandeshClient(evm,
            primary, secondary, csf);
    client_->Initiate();
    return true;
}

bool Sandesh::InitGenerator(const std::string &module,
                            const std::string &source, 
                            const std::string &node_type,
                            const std::string &instance_id,
                            EventManager *evm,
                            unsigned short http_port,
                            SandeshContext *client_context) {
    return Initialize(SandeshRole::Generator, module, source, node_type,
                      instance_id, evm, http_port, client_context);
}

bool Sandesh::InitGenerator(const std::string &module,
                            const std::string &source,
                            const std::string &node_type,
                            const std::string &instance_id,
                            EventManager *evm, 
                            unsigned short http_port,
                            CollectorSubFn csf,
                            const std::vector<std::string> &collectors,
                            SandeshContext *client_context) {
    bool success(Initialize(SandeshRole::Generator, module, source, node_type,
                            instance_id, evm, http_port, client_context));
    if (!success) {
        return false;
    }
    return InitClient(evm, collectors, csf);
}

// Collector
bool Sandesh::InitCollector(const std::string &module,
                            const std::string &source,
                            const std::string &node_type,
                            const std::string &instance_id,
                            EventManager *evm, 
                            const std::string &collector_ip, int collector_port,
                            unsigned short http_port,
                            SandeshContext *client_context) {
    bool success(Initialize(SandeshRole::Collector, module, source, node_type,
                            instance_id, evm, http_port, client_context));
    if (!success) {
        return false;
    }
    return ConnectToCollector(collector_ip, collector_port);
}

bool Sandesh::InitGeneratorTest(const std::string &module,
                                const std::string &source,
                                const std::string &node_type,
                                const std::string &instance_id,
                                EventManager *evm,
                                unsigned short http_port, 
                                SandeshContext *client_context) {
    return Initialize(SandeshRole::Test, module, source, node_type,
                      instance_id, evm, http_port, client_context);
}

static void WaitForIdle() {
    static const int kTimeout = 15;
    TaskScheduler *scheduler = TaskScheduler::GetInstance();

    for (int i = 0; i < (kTimeout * 1000); i++) {
        if (scheduler->IsEmpty()) {
            break;
        }
        usleep(1000);
    }    
}

void Sandesh::Uninit() {

    // Wait until all pending http session based tasks are cleaned up.
    long count = 60000;
    while (count--) {
        if (HttpSession::GetPendingTaskCount() == 0) break;
        usleep(1000);
    }
    SandeshHttp::Uninit();
    role_ = SandeshRole::Invalid;
    if (recv_queue_.get() != NULL) {
        recv_queue_->Shutdown();
        recv_queue_.reset(NULL);
        assert(recv_task_id_ != -1);
        recv_task_id_ = -1;
    } else {
        assert(recv_task_id_ == -1);
    }
    if (client_ != NULL) {
        client_->Shutdown();
        while (client()->IsSession()) usleep(100);
        WaitForIdle(); 
        client_->ClearSessions();
        TcpServerManager::DeleteServer(client_);
        client_ = NULL;
    }
}

void Sandesh::SetLoggingParams(bool enable_local_log, std::string category,
        std::string level, bool enable_trace_print, bool enable_flow_log) {
    SetLocalLogging(enable_local_log);
    SetLoggingCategory(category);
    SetLoggingLevel(level);
    SetTracePrint(enable_trace_print);
    SetFlowLogging(enable_flow_log);
}

void Sandesh::SetLoggingParams(bool enable_local_log, std::string category,
        SandeshLevel::type level, bool enable_trace_print,
        bool enable_flow_log) {
    SetLocalLogging(enable_local_log);
    SetLoggingCategory(category);
    SetLoggingLevel(level);
    SetTracePrint(enable_trace_print);
    SetFlowLogging(enable_flow_log);
}

void Sandesh::SetLoggingLevel(std::string level) {
    SandeshLevel::type nlevel = StringToLevel(level);
    SetLoggingLevel(nlevel);
}

log4cplus::LogLevel SandeshLevelTolog4Level(
    SandeshLevel::type slevel) {
    switch (slevel) {
      case SandeshLevel::SYS_EMERG:
      case SandeshLevel::SYS_ALERT:
      case SandeshLevel::SYS_CRIT:
          return log4cplus::FATAL_LOG_LEVEL;
      case SandeshLevel::SYS_ERR:
          return log4cplus::ERROR_LOG_LEVEL;
      case SandeshLevel::SYS_WARN:
      case SandeshLevel::SYS_NOTICE:
          return log4cplus::WARN_LOG_LEVEL;
      case SandeshLevel::SYS_INFO:
          return log4cplus::INFO_LOG_LEVEL;
      case SandeshLevel::SYS_DEBUG:
          return log4cplus::DEBUG_LOG_LEVEL;
      default:
      case SandeshLevel::INVALID:
          return log4cplus::ALL_LOG_LEVEL;
    }
}

void Sandesh::SetLoggingLevel(SandeshLevel::type level) {
    if (logging_level_ != level) {
        log4cplus::LogLevel log4_old_level(
            SandeshLevelTolog4Level(logging_level_));
        log4cplus::LogLevel log4_new_level(
            SandeshLevelTolog4Level(level));
        const log4cplus::LogLevelManager &log4level_manager(
            log4cplus::getLogLevelManager());
        SANDESH_LOG(INFO, "SANDESH: Logging: LEVEL: " << "[ " <<
                LevelToString(logging_level_) << " ] -> [ " <<
                LevelToString(level) << " ] log4level: [ " <<
                log4level_manager.toString(log4_old_level) <<
                " ] -> [ " <<
                log4level_manager.toString(log4_new_level) <<
                " ]");
        logging_level_ = level;
        logger_.setLogLevel(log4_new_level);
    }
}

void Sandesh::SetLoggingCategory(std::string category) {
    if (logging_category_ != category) {
        SANDESH_LOG(INFO, "SANDESH: Logging: CATEGORY: " <<
                (logging_category_.empty() ? "*" : logging_category_) << " -> " <<
                (category.empty() ? "*" : category));
        logging_category_ = category;
    }
}

void Sandesh::SetLocalLogging(bool enable_local_log) {
    if (enable_local_log_ != enable_local_log) {
        SANDESH_LOG(INFO, "SANDESH: Logging: " <<
                (enable_local_log_ ? "ENABLED" : "DISABLED") << " -> " <<
                (enable_local_log ? "ENABLED" : "DISABLED"));
        enable_local_log_ = enable_local_log;
    }
}

void Sandesh::SetTracePrint(bool enable_trace_print) {
    if (enable_trace_print_ != enable_trace_print) {
        SANDESH_LOG(INFO, "SANDESH: Trace: PRINT: " <<
                (enable_trace_print_ ? "ENABLED" : "DISABLED") << " -> " <<
                (enable_trace_print ? "ENABLED" : "DISABLED"));
        enable_trace_print_ = enable_trace_print;
    }
}

void Sandesh::SetSendingLevel(size_t count, SandeshLevel::type level) {
    if (sending_level_ != level) {
        SANDESH_LOG(INFO, "SANDESH: Sending: LEVEL: " << "[ " <<
            LevelToString(sending_level_) << " ] -> [ " <<
            LevelToString(level) << " ] : " << count);
        sending_level_ = level;
    }
}

void Sandesh::SetFlowLogging(bool enable_flow_log) {
    if (enable_flow_log_ != enable_flow_log) {
        SANDESH_LOG(INFO, "SANDESH: Flow Logging: " <<
            (enable_flow_log_ ? "ENABLED" : "DISABLED") << " -> " <<
            (enable_flow_log ? "ENABLED" : "DISABLED"));
        enable_flow_log_ = enable_flow_log;
    }
}

bool Sandesh::Enqueue(SandeshQueue *queue) {
    if (!queue) {
        if (IsLoggingDroppedAllowed()) {
            SANDESH_LOG(ERROR, __func__ << ": SandeshQueue NULL : Dropping Message: "
                << ToString());
        }
        UpdateTxMsgFailStats(name_, 0, SandeshTxDropReason::NoQueue);
        Release();
        return false;
    }
    if (!queue->Enqueue(this)) {
        // XXX Change when WorkQueue implements bounded queues
        return true;
    }
    return true;
}

bool Sandesh::ProcessRecv(SandeshRequest *rsnh) {
    rsnh->HandleRequest();
    rsnh->Release();
    return true;
}

void SandeshRequest::Release() { self_.reset(); }

int32_t Sandesh::WriteBinary(u_int8_t *buf, u_int32_t buf_len,
        int *error) {
    int32_t xfer;
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(buf, buf_len));
    btrans->setWriteBuffer(buf, buf_len);
    boost::shared_ptr<TBinaryProtocol> prot =
            boost::shared_ptr<TBinaryProtocol>(new TBinaryProtocol(btrans));
    xfer = Write(prot);
    if (xfer < 0) {
        SANDESH_LOG(DEBUG, __func__ << "Write sandesh to " << buf_len <<
                " bytes FAILED" << std::endl);
        *error = EINVAL;
        return xfer;
    }
    return xfer;
}

int32_t Sandesh::ReadBinary(u_int8_t *buf, u_int32_t buf_len,
        int *error) {
    int32_t xfer = 0;
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(buf, buf_len));
    boost::shared_ptr<TBinaryProtocol> prot =
            boost::shared_ptr<TBinaryProtocol>(new TBinaryProtocol(btrans));
    xfer = Read(prot);
    if (xfer < 0) {
        SANDESH_LOG(DEBUG, __func__ << "Read sandesh from " << buf_len <<
                " bytes FAILED" << std::endl);
        *error = EINVAL;
        return xfer;
    }
    return xfer;
}

int32_t Sandesh::ReceiveBinaryMsgOne(u_int8_t *buf, u_int32_t buf_len,
        int *error, SandeshContext *client_context) {
    int32_t xfer;
    std::string sandesh_name;
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(buf, buf_len));
    boost::shared_ptr<TBinaryProtocol> prot =
            boost::shared_ptr<TBinaryProtocol>(new TBinaryProtocol(btrans));
    // Extract sandesh name
    xfer = prot->readSandeshBegin(sandesh_name);
    if (xfer < 0) {
        SANDESH_LOG(DEBUG, __func__ << "Read sandesh begin from " << buf_len <<
                " bytes FAILED" << std::endl);
        *error = EINVAL;
        return xfer;
    }
    // Create and process the sandesh
    Sandesh *sandesh = SandeshBaseFactory::CreateInstance(sandesh_name);
    if (sandesh == NULL) {
        SANDESH_LOG(DEBUG, __func__ << " Unknown sandesh:" <<
                sandesh_name << std::endl);
        *error = EINVAL;
        return -1;
    }
    // Reinitialize buffer and protocol
    btrans = boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(buf, buf_len));
    prot = boost::shared_ptr<TBinaryProtocol>(new TBinaryProtocol(btrans));
    xfer = sandesh->Read(prot);
    if (xfer < 0) {
        SANDESH_LOG(DEBUG, __func__ << " Decoding " << sandesh_name << " FAILED" <<
                std::endl);
        *error = EINVAL;
        return xfer;
    }
    SandeshBuffer * bsnh = dynamic_cast<SandeshBuffer *>(sandesh);
    if (bsnh) bsnh->Process(client_context);
    sandesh->Release();
    return xfer;
}

int32_t Sandesh::ReceiveBinaryMsg(u_int8_t *buf, u_int32_t buf_len,
        int *error, SandeshContext *client_context) {
    u_int32_t xfer = 0;
    int ret;
    while (xfer < buf_len) {
        ret = ReceiveBinaryMsgOne(buf + xfer, buf_len - xfer, error,
                                  client_context);
        if (ret < 0) {
            SANDESH_LOG(DEBUG, __func__ << "Read sandesh from " << buf_len <<
                    " bytes at offset " << xfer << " FAILED (" <<
                    error << ")");
            return ret;
        }
        xfer += ret;
    }
    return xfer;
}

bool Sandesh::HandleTest() {
    // Handle unit test scenario
    if (IsUnitTest() || IsLevelUT(level_)) {
        if (IsLevelCategoryLoggingAllowed(level_, category_)) {
            ForcedLog();
        }
        Release();
        return true;
    }
    return false;
}

bool Sandesh::SendEnqueue() {
    if (!client_) {
        if (IsLoggingDroppedAllowed()) {
            if (IsConnectToCollectorEnabled()) {
                SANDESH_LOG(ERROR, "SANDESH: No client: " << ToString());
            } else {
                Log();
            }
        }
        UpdateTxMsgFailStats(name_, 0, SandeshTxDropReason::NoClient);
        Release();
        return false;        
    } 
    if (!client_->SendSandesh(this)) {
        if (IsLoggingDroppedAllowed()) {
            SANDESH_LOG(ERROR, "SANDESH: Send FAILED: " << ToString());
        }
        UpdateTxMsgFailStats(name_, 0,
            SandeshTxDropReason::ClientSendFailed);
        Release();
        return false;
    }
    return true;
}

bool Sandesh::Dispatch(SandeshConnection * sconn) {
    // Handle unit test
    if (HandleTest()) {
        return true;
    }
    // Sandesh client does not have a connection
    if (sconn) {
        return sconn->SendSandesh(this);
    } else {
        return SendEnqueue();
    }
}

bool SandeshResponse::Dispatch(SandeshConnection * sconn) {
    assert(sconn == NULL);
    if (context().find("http%") == 0) {
        SandeshHttp::Response(this, context());
        return true;
    }
    if (response_callback_) {
        response_callback_(this);
    }
    return Sandesh::Dispatch(sconn);
}

bool SandeshTrace::Dispatch(SandeshConnection * sconn) {
    assert(sconn == NULL);
    if (0 == context().find("http%")) {
        SandeshHttp::Response(this, context());
        return true;
    }
    return Sandesh::Dispatch();
}

bool SandeshUVE::Dispatch(SandeshConnection * sconn) {
    assert(sconn == NULL);
    if (0 == context().find("http%")) {
        SandeshHttp::Response(this, context());
        return true;
    }
    // Handle unit test
    if (HandleTest()) {
        return true;
    }
    if (client_) {
        if (!client_->SendSandeshUVE(this)) {
            SANDESH_LOG(ERROR, "SandeshUVE : Send FAILED: " << ToString());
            UpdateTxMsgFailStats(Name(), 0,
                SandeshTxDropReason::ClientSendFailed);
            Release();
            return false;
        }
        return true;
    }
    if (IsConnectToCollectorEnabled()) {
        SANDESH_LOG(ERROR, "SANDESH: No Client: " << ToString());
    } else {
        Log();
    }
    UpdateTxMsgFailStats(Name(), 0, SandeshTxDropReason::NoClient);
    Release();
    return false;
}

bool SandeshRequest::Enqueue(SandeshRxQueue *queue) {
    if (!queue) {
        SANDESH_LOG(ERROR, "SandeshRequest: No RxQueue: " << ToString());
        UpdateRxMsgFailStats(Name(), 0, SandeshRxDropReason::NoQueue);
        Release();
        return false;
    }
    if (!queue->Enqueue(this)) {
        // XXX Change when WorkQueue implements bounded queues
        return true;
    }
    return true;
}

bool Sandesh::IsLevelUT(SandeshLevel::type level) {
    return level >= SandeshLevel::UT_START &&
            level <= SandeshLevel::UT_END;
}

bool Sandesh::IsLevelCategoryLoggingAllowed(SandeshLevel::type level,
                                                   const std::string& category) {
    bool level_allowed = logging_level_ >= level;
    bool category_allowed = !logging_category_.empty() ?
            logging_category_ == category : true;
    return level_allowed && category_allowed;
}

bool Sandesh::IsLoggingAllowed() const {
    if (type_ == SandeshType::FLOW) {
        return enable_flow_log_;
    } else {
        return IsLocalLoggingEnabled() &&
                IsLevelCategoryLoggingAllowed(level_, category_);
    }
}

bool Sandesh::IsLoggingDroppedAllowed() const {
    if (type_ == SandeshType::FLOW) {
        return enable_flow_log_;
    } else {
        return true;
    }
}

const char * Sandesh::LevelToString(SandeshLevel::type level) {
    std::map<int, const char*>::const_iterator it = _SandeshLevel_VALUES_TO_NAMES.find(level);
    if (it != _SandeshLevel_VALUES_TO_NAMES.end()) {
        return it->second;
    } else {
        return "UNKNOWN";
    }
}

SandeshLevel::type Sandesh::StringToLevel(std::string level) {
    std::map<int, const char*>::const_iterator it = _SandeshLevel_VALUES_TO_NAMES.begin();
    while (it != _SandeshLevel_VALUES_TO_NAMES.end()) {
        if (strncmp(level.c_str(), it->second, strlen(it->second)) == 0) {
            return static_cast<SandeshLevel::type>(it->first);
        }
        it++;
    }
    return SandeshLevel::INVALID;
}

void Sandesh::UpdateRxMsgStats(const std::string &msg_name,
                               uint64_t bytes) {
    tbb::mutex::scoped_lock lock(stats_mutex_);
    msg_stats_.UpdateRecv(msg_name, bytes);
}

void Sandesh::UpdateRxMsgFailStats(const std::string &msg_name,
    uint64_t bytes, SandeshRxDropReason::type dreason) {
    tbb::mutex::scoped_lock lock(stats_mutex_);
    msg_stats_.UpdateRecvFailed(msg_name, bytes, dreason);
}

void Sandesh::UpdateTxMsgStats(const std::string &msg_name,
                               uint64_t bytes) {
    tbb::mutex::scoped_lock lock(stats_mutex_);
    msg_stats_.UpdateSend(msg_name, bytes);
}

void Sandesh::UpdateTxMsgFailStats(const std::string &msg_name,
    uint64_t bytes, SandeshTxDropReason::type dreason) {
    tbb::mutex::scoped_lock lock(stats_mutex_);
    msg_stats_.UpdateSendFailed(msg_name, bytes, dreason);
}

void Sandesh::GetMsgStats(
    std::vector<SandeshMessageTypeStats> *mtype_stats,
    SandeshMessageStats *magg_stats) {
    tbb::mutex::scoped_lock lock(stats_mutex_);
    msg_stats_.Get(mtype_stats, magg_stats);
}

void Sandesh::GetMsgStats(
    boost::ptr_map<std::string, SandeshMessageTypeStats> *mtype_stats,
    SandeshMessageStats *magg_stats) {
    tbb::mutex::scoped_lock lock(stats_mutex_);
    msg_stats_.Get(mtype_stats, magg_stats);
}

void Sandesh::SetSendQueue(bool enable) {
    if (send_queue_enabled_ != enable) {
        SANDESH_LOG(INFO, "SANDESH: CLIENT: SEND QUEUE: " <<
            (send_queue_enabled_ ? "ENABLED" : "DISABLED") << " -> " <<
            (enable ? "ENABLED" : "DISABLED"));
        send_queue_enabled_ = enable;
        if (enable) {
            if (client_ && client_->IsSession()) {
                client_->session()->send_queue()->MayBeStartRunner();
            }
        } 
    }
}

bool DoDropSandeshMessage(const SandeshHeader &header,
    const SandeshLevel::type drop_level) {
    // Only systemlog, objectlog, and flow have level
    SandeshType::type stype(header.get_Type());
    if (stype == SandeshType::SYSTEM ||
        stype == SandeshType::OBJECT ||
        stype == SandeshType::FLOW) {
        // Is level above drop level?
        SandeshLevel::type slevel(
            static_cast<SandeshLevel::type>(header.get_Level()));
        if (slevel >= drop_level) {
            return true;
        }
    }
    return false;
}

SandeshContext *Sandesh::module_context(const std::string &module_name) {
    ModuleContextMap::const_iterator loc = module_context_.find(module_name);
    if (loc != module_context_.end()) {
        return loc->second;
    }
    return NULL;
}

void Sandesh::set_module_context(const std::string &module_name,
                                 SandeshContext *context) {
    std::pair<ModuleContextMap::iterator, bool> result =
            module_context_.insert(std::make_pair(module_name, context));
    if (!result.second) {
        result.first->second = context;
    }
}

bool SandeshSystem::HandleTest(SandeshLevel::type level,
                                      const std::string& category) {
    // Handle unit test scenario
    if (IsUnitTest() || IsLevelUT(level)) {
        return true;
    }
    return false;
}
