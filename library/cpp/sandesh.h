/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh.h
//
// Sandesh means message in Hindi. Sandeshs are used to store
// structured information from various software modules of the
// Virtual Network System (VNS) into the analytics database.
// Sandeshs are also used to exchange information for the purpose of
// debugging the VNS.
// The different types of Sandeshs are:
// 1. Async - Used to send event-triggered messages to the analytics
//    database
// 2. Request and Response - Used for debugging and displaying
//    information from various software modules of the VNS like
//    vnswa and bgpd.
// 3. Trace - Used for triggering internal trace messages
//
// Sandesh module consists of 2 parts:
// 1. The first part is a code generator tool that reads in .sandesh files
//    and generates appropriate C++ code for encoding the Sandeshs using
//    XML.
// 2. The second part is a library that software modules like vnswa and
//    bgpd will link with and use to:
//    a. Send Sandeshs to the analytics database
//    b. Provide operational command implementation
//    c. Enable/disable tracing
//
// For example, vnswa developer wants to generate a structured error message to
// report some error condition. The developer will add an async sandesh to the
// file VNSwitch.sandesh as below:
//
// struct VNSRoute {
//    1: i32                           prefix;
// }
//
// async sandesh VNSwitchError {
//    1: "VNSwitchId = ";
//    2: i32                           vnSwitchId;
//    3: "VNetworkId = ";
//    4: i32                           vnId;
//    5: list<VNSRoute>                vnRoutes;
//    6: VNSRoute                      vnMarkerRoute;
// }
//
// To use Sandesh, the following needs to be added to the module SConscript:
///
// # Generate the source files
// SandeshGenFiles  = env.SandeshGenCpp('VNS.sandesh')
// SandeshGenFiles += env.SandeshGenCpp('VNSwitch.sandesh')
//
// # The above returns VNS_types.h, VNS_types.cpp, VNS_constants.h
// # VNS_constants.cpp, VNSwitch_types.h, VNSwitch_types.cpp,
// # VNSwitch_constants.h, VNSwitch_constants.cpp
//
// # To include the header files above from your module's sources
// env.Append(CPPPATH = env['TOP'])
//
// # Extract the .cpp files to be used as sources
// SandeshGenSrcs = env.ExtractCpp(SandeshGenFiles)
//
// Add SandeshGenSrcs to the module source files
//
// Add libsandesh, and libbase to the module libraries.
//
// The code to send the Sandesh to the analytics database will be:
//
// VNSwitchError::Send(int32_t vnSwitchId, int32_t vnId, std::vector<VNSRoute>  vnRoutes,
//                     VNSRoute vnMarkerRoute)
//
// The developer will have to call Sandesh::InitGenerator from the module's
// initialization before calling the above function.
//

#ifndef __SANDESH_H__
#define __SANDESH_H__

#include <time.h>

#include <map>

#include <boost/asio/ip/tcp.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/tuple/tuple.hpp>
#include <base/contrail_ports.h>
#include <base/logging.h>
#include <base/queue_task.h>
#include <base/string_util.h>
#include <base/time_util.h>
#include <sandesh/sandesh_util.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/protocol/TProtocol.h>
#include <sandesh/transport/TBufferTransports.h>
#include <sandesh/sandesh_trace.h>
#include <sandesh/sandesh_options.h>

// Forward declaration
class EventManager;
class SandeshClient;
class SandeshSession;

// Sandesh Context
class SandeshContext {
    // Abstract base class for users of sandesh library to
    // pass in information to used when handling sandesh
    // request and response
protected:
    SandeshContext() { }
public:
    virtual ~SandeshContext() { }
};

//
// Sandesh
//
class SandeshMessageStatistics;
class SandeshMessageTypeStats;
class SandeshMessageStats;
class SandeshMessageTypeBasicStats;
class SandeshMessageBasicStats;
class SandeshConnection;
class SandeshRequest;


struct SandeshElement;

class Sandesh {
public:
    typedef WorkQueue<SandeshRequest *> SandeshRxQueue;
    typedef WorkQueue<SandeshElement> SandeshQueue;
    typedef WorkQueue<
            boost::shared_ptr<contrail::sandesh::transport::TMemoryBuffer> >
            SandeshBufferQueue;
    typedef boost::asio::ip::tcp::endpoint Endpoint;
    typedef boost::function<void (Sandesh *)> SandeshCallback;
    struct SandeshRole {
        enum type {
            Invalid,
            Generator,
            Collector,
            Test,
        };
    };
    typedef boost::tuple<size_t, SandeshLevel::type, bool, bool> QueueWaterMarkInfo;
    typedef std::map<std::string, std::map<std::string,std::string> > DerivedStats;

    // Initialization APIs
    static bool InitGenerator(const std::string &module,
            const std::string &source,
            const std::string &node_type,
            const std::string &instance_id,
            EventManager *evm,
            unsigned short http_port,
            const std::vector<std::string> &collectors,
            SandeshContext *client_context = NULL,
            DerivedStats ds = DerivedStats(),
            const SandeshConfig &config = SandeshConfig());
    static bool InitGenerator(const std::string &module,
            const std::string &source, 
            const std::string &node_type,
            const std::string &instance_id,
            EventManager *evm,
            unsigned short http_port,
            SandeshContext *client_context = NULL,
            DerivedStats ds = DerivedStats(),
            const SandeshConfig &config = SandeshConfig());
    static void RecordPort(const std::string& name, const std::string& module,
            unsigned short port);
    // Collector
    static bool InitCollector(const std::string &module,
            const std::string &source, 
            const std::string &node_type,
            const std::string &instance_id,
            EventManager *evm,
            const std::string &collector_ip, int collector_port,
            unsigned short http_port,
            SandeshContext *client_context = NULL,
            const SandeshConfig &config = SandeshConfig());
    // Test
    static bool InitGeneratorTest(const std::string &module,
            const std::string &source,
            const std::string &node_type,
            const std::string &instance_id, 
            EventManager *evm,
            unsigned short http_port,
            SandeshContext *client_context = NULL,
            const SandeshConfig &config = SandeshConfig());
    static bool ConnectToCollector(const std::string &collector_ip,
            int collector_port, bool periodicuve = false);
    static void ReConfigCollectors(const std::vector<std::string>& collector_list);
    static void Uninit();
    static void SetDscpValue(uint8_t value);

    // Disable flow collection
    static void DisableFlowCollection(bool disable);
    static bool IsFlowCollectionDisabled() { return disable_flow_collection_; }

    // Flags to control sending of sandesh from generators
    static void DisableSendingAllMessages(bool disable);
    static bool IsSendingAllMessagesDisabled();
    static void DisableSendingObjectLogs(bool disable);
    static bool IsSendingObjectLogsDisabled();
    static bool IsSendingSystemLogsDisabled();
    static void DisableSendingFlows(bool disable);
    static bool IsSendingFlowsDisabled();
    static void set_send_rate_limit(int rate_limit);
    static uint32_t get_send_rate_limit();

    // Logging and category APIs
    static void SetLoggingParams(bool enable_local_log, std::string category,
            std::string level, bool enable_trace_print = false,
            bool enable_flow_log = false);
    static void SetLoggingParams(bool enable_local_log, std::string category,
            SandeshLevel::type level, bool enable_trace_print = false,
            bool enable_flow_log = false);
    static void SetLoggingLevel(std::string level);
    static void SetLoggingLevel(SandeshLevel::type level);
    static SandeshLevel::type LoggingLevel() { return logging_level_; }
    static SandeshLevel::type LoggingUtLevel() { return logging_ut_level_; }
    static bool IsLocalLoggingEnabled() { return enable_local_log_; }
    static void SetLocalLogging(bool enable);
    static bool IsFlowLoggingEnabled() { return enable_flow_log_; }
    static void SetFlowLogging(bool enable);
    static bool IsTracePrintEnabled() { return enable_trace_print_; }
    static void SetTracePrint(bool enable);
    static void SetLoggingCategory(std::string category);
    static std::string LoggingCategory() { return logging_category_; }

    //GetSize method to report the size
    virtual size_t GetSize() const = 0;

    // Send queue processing
    static void SetSendQueue(bool enable);
    static inline bool IsSendQueueEnabled() { 
        return send_queue_enabled_;
    }
    static inline bool IsConnectToCollectorEnabled() {
        return connect_to_collector_;
    }
    static void SetSendingLevel(size_t count, SandeshLevel::type level);
    static SandeshLevel::type SendingLevel() { return sending_level_; }

    static int32_t ReceiveBinaryMsgOne(u_int8_t *buf, u_int32_t buf_len,
            int *error, SandeshContext *client_context);
    static int32_t ReceiveBinaryMsg(u_int8_t *buf, u_int32_t buf_len,
            int *error, SandeshContext *client_context);
    static bool SendReady(SandeshConnection * sconn = NULL);
    static void UpdateRxMsgStats(const std::string &msg_name, uint64_t bytes);
    static void UpdateRxMsgFailStats(const std::string &msg_name,
        uint64_t bytes, SandeshRxDropReason::type dreason);
    static void UpdateTxMsgStats(const std::string &msg_name, uint64_t bytes);
    static void UpdateTxMsgFailStats(const std::string &msg_name,
        uint64_t bytes, SandeshTxDropReason::type dreason);
    static void GetMsgStats(
        std::vector<SandeshMessageTypeStats> *mtype_stats,
        SandeshMessageStats *magg_stats);
    static void GetMsgStats(
        boost::ptr_map<std::string, SandeshMessageTypeStats> *mtype_stats,
        SandeshMessageStats *magg_stats);
    static const char *  SandeshRoleToString(SandeshRole::type role);

    virtual void Release() { delete this; }
    virtual void Log() const = 0;
    virtual void ForcedLog() const = 0;
    virtual std::string ToString() const = 0;
    virtual std::string ModuleName() const = 0;
    virtual int32_t Read(
             boost::shared_ptr<contrail::sandesh::protocol::TProtocol> iprot) = 0;
    virtual int32_t Write(
             boost::shared_ptr<contrail::sandesh::protocol::TProtocol> oprot) const = 0;
    virtual const uint32_t seqnum() { return seqnum_; }
    virtual const int32_t versionsig() const = 0;
    virtual const char *Name() const { return name_.c_str(); }
    bool Enqueue(SandeshQueue* queue);
    virtual int32_t WriteBinary(u_int8_t *buf, u_int32_t buf_len, int *error);
    virtual int32_t ReadBinary(u_int8_t *buf, u_int32_t buf_len, int *error);
    virtual int32_t WriteBinaryToFile(const std::string& path, int *error);
    virtual int32_t ReadBinaryFromFile(const std::string& path, int *error);

    bool IsLoggingAllowed() const;
    static bool IsLoggingDroppedAllowed(SandeshType::type);

    // Accessors
    static void set_source(std::string &source) { source_ = source; }
    static std::string source() { return source_; }
    static void set_module(std::string &module) { module_ = module; }
    static std::string module() { return module_; }
    static void set_instance_id(std::string &instance_id) { instance_id_ = instance_id; }
    static std::string instance_id() { return instance_id_; }
    static void set_node_type(std::string &node_type) { node_type_ = node_type; }
    static std::string node_type() { return node_type_; }
    static SandeshRole::type role() { return role_; }
    static int http_port() { return http_port_; }
    static SandeshRxQueue* recv_queue() { return recv_queue_.get(); }
    static SandeshContext* client_context() { return client_context_; }
    static void set_client_context(SandeshContext *context) { client_context_ = context; }
    static SandeshContext *module_context(const std::string &module_name);
    static void set_module_context(const std::string &module_name,
                                   SandeshContext *context);
    static void set_response_callback(SandeshCallback response_cb) { response_callback_ = response_cb; }
    static SandeshCallback response_callback() { return response_callback_; }
    static SandeshClient* client() { return client_; }
    static SandeshConfig& config() { return config_; }

    time_t timestamp() const { return timestamp_; }
    void set_context(std::string context) { context_ = context; }
    std::string context() const { return context_; }
    void set_scope(std::string scope) { scope_ = scope; }
    std::string scope() const { return scope_; }
    SandeshType::type type() const { return type_; }
    void set_hints(int32_t hints) { hints_ = hints; }
    int32_t hints() const { return hints_; }
    void set_level(SandeshLevel::type level) { level_ = level; }
    SandeshLevel::type level() const { return level_; }
    void set_category(std::string category) { category_ = category; }
    std::string category() const { return category_; }
    static const char* LevelToString(SandeshLevel::type level);
    static SandeshLevel::type StringToLevel(std::string level);
    static log4cplus::Logger& logger() { return logger_; }

protected:
    void set_timestamp(time_t timestamp) { timestamp_ = timestamp; }
    void set_type(SandeshType::type type) { type_ = type; }
    void set_name(const char *name) { name_ = name; }

    Sandesh(SandeshType::type type, const std::string& name, uint32_t seqno) :
        seqnum_(seqno),
        context_(""),
        timestamp_(UTCTimestampUsec()),
        scope_(""),
        type_(type),
        hints_(0),
        level_(SandeshLevel::INVALID),
        category_(""),
        name_(name) {
    }
    Sandesh(SandeshType::type type, const std::string& name, uint32_t seqno, bool no_time_stamp) :
        seqnum_(seqno),
        context_(""),
        scope_(""),
        type_(type),
        hints_(0),
        level_(SandeshLevel::INVALID),
        category_(""),
        name_(name) {
        timestamp_ = no_time_stamp ? 0 : UTCTimestampUsec();
    }
    virtual ~Sandesh() {}
    virtual bool Dispatch(SandeshConnection * sconn = NULL);
    virtual bool SendEnqueue();

    static bool HandleTest(SandeshLevel::type level, const
        std::string& category);

    static bool IsUnitTest() {
        return role_ == SandeshRole::Invalid || role_ == SandeshRole::Test;
    }
    static SandeshCallback response_callback_;
    static SandeshClient *client_;
    static bool IsLevelUT(SandeshLevel::type level);
    static bool IsLevelCategoryLoggingAllowed(SandeshType::type type,
                                              SandeshLevel::type level,
                                              const std::string& category);

private:
    friend class SandeshTracePerfTest;

    typedef std::map<std::string, SandeshContext *> ModuleContextMap;

    static void InitReceive(int recv_task_inst = -1);
    static void InitClient(EventManager *evm, Endpoint server,
                           const SandeshConfig &config, bool periodicuve);
    static bool InitClient(EventManager *evm,
                           const std::vector<std::string> &collectors,
                           const SandeshConfig &config);
    static bool ProcessRecv(SandeshRequest *);
    static bool Initialize(SandeshRole::type role, const std::string &module,
            const std::string &source, 
            const std::string &node_type,
            const std::string &instance_id,
            EventManager *evm,
            unsigned short http_port,
            SandeshContext *client_context = NULL,
            const SandeshConfig &config = SandeshConfig());

    static SandeshRole::type role_;
    static std::string module_;
    static std::string source_;
    static std::string node_type_;
    static std::string instance_id_;
    static int http_port_;
    static std::auto_ptr<SandeshRxQueue> recv_queue_;
    static int recv_task_id_;
    static SandeshContext *client_context_;
    static ModuleContextMap module_context_;
    static bool enable_local_log_;  // whether to just enable local logging
    static bool enable_flow_log_;  // whether to enable flow sandesh message logging
    static SandeshLevel::type logging_level_; // current logging level
    static SandeshLevel::type logging_ut_level_; // ut_debug logging level
    static std::string logging_category_; // current logging category
    static bool enable_trace_print_; // whether to print traces locally
    static bool connect_to_collector_; // whether to connect to collector
    static EventManager *event_manager_;
    static bool send_queue_enabled_;
    static SandeshLevel::type sending_level_;
    static SandeshMessageStatistics msg_stats_;
    static tbb::mutex stats_mutex_;
    static log4cplus::Logger logger_;
    static bool disable_flow_collection_; // disable flow collection
    static SandeshConfig config_;
    static bool disable_sending_all_;
    static bool disable_sending_object_logs_;
    static bool disable_sending_flows_;

    const uint32_t seqnum_;
    std::string context_;
    time_t timestamp_;
    std::string scope_;
    SandeshType::type type_;
    int32_t hints_;
    SandeshLevel::type level_;
    std::string category_;
    std::string name_;
    static tbb::atomic<uint32_t> sandesh_send_ratelimit_;
};

struct SandeshElement {
    Sandesh *snh_;
    //Explicit constructor creating only if Sandesh is passed as arg
    explicit SandeshElement(Sandesh *snh):snh_(snh),size_(snh->GetSize()) {
    }
    SandeshElement():size_(0) { }
    size_t GetSize() const {
        return size_;
    }
    private:
        size_t size_;
};

template<>
size_t Sandesh::SandeshQueue::AtomicIncrementQueueCount(
    SandeshElement *element);

template<>
size_t Sandesh::SandeshQueue::AtomicDecrementQueueCount(
    SandeshElement *element);

#define SANDESH_LOG(_Level, _Msg)                                \
    do {                                                         \
        if (LoggingDisabled()) break;                            \
        LOG4CPLUS_##_Level(Sandesh::logger(), _Msg);             \
    } while (0)

class SandeshRequest : public Sandesh,
                       public boost::enable_shared_from_this<SandeshRequest> {
public:
    virtual void HandleRequest() const = 0;
    virtual bool RequestFromHttp(const std::string& ctx, 
        const std::string& snh_query) = 0; 
    void Release();
    boost::shared_ptr<const SandeshRequest> SharedPtr() const { return shared_from_this(); }
    bool Enqueue(SandeshRxQueue* queue);
protected:
    SandeshRequest(const std::string& name, uint32_t seqno) :
        Sandesh(SandeshType::REQUEST, name, seqno), self_(this) {}

    friend void boost::checked_delete<SandeshRequest>(SandeshRequest * x);
    boost::shared_ptr<SandeshRequest> self_;
};

class SandeshResponse : public Sandesh {
public:
    virtual const bool get_more() const = 0;
    virtual void set_more(const bool val) = 0;
    virtual void Response() {}
protected:
    SandeshResponse(const std::string& name, uint32_t seqno) :
        Sandesh(SandeshType::RESPONSE, name, seqno) {}
    bool Dispatch(SandeshConnection * sconn = NULL);
};

class SandeshBuffer : public Sandesh {
public:
    virtual void Process(SandeshContext *context) = 0;
    SandeshBuffer& operator=(const SandeshBuffer& r) {
        set_context(r.context());
        set_timestamp(r.timestamp());
        set_scope(r.scope());
        set_hints(r.hints()); 
        set_type(r.type());
        set_level(r.level());
        set_category(r.category());
        set_name(r.Name());
        return(*this);
    }
protected:
    SandeshBuffer(const std::string& name, uint32_t seqno) :
        Sandesh(SandeshType::BUFFER, name, seqno, true) {}
};

class SandeshTrace : public Sandesh {
public:
    const uint32_t seqnum() { return xseqnum_; }
    virtual void SendTrace(const std::string& context, bool more) = 0;
    const bool get_more() const { return more_; }
    virtual ~SandeshTrace() {}
protected:
    SandeshTrace(const std::string& name, uint32_t seqno) :
        Sandesh(SandeshType::TRACE, name, 0), xseqnum_(0), more_(true) {}
    bool Dispatch(SandeshConnection * sconn = NULL);
    void set_seqnum(const uint32_t seqnum) { xseqnum_= seqnum; }
    void set_more(bool more) { more_ = more; }
private:
    uint32_t xseqnum_;
    bool more_;
};

class SandeshSystem : public Sandesh {
protected:
    SandeshSystem(const std::string& name, uint32_t seqno) :
        Sandesh(SandeshType::SYSTEM, name, seqno) {}
};

class SandeshObject : public Sandesh {
protected:
    SandeshObject(const std::string& name, uint32_t seqno) :
        Sandesh(SandeshType::OBJECT, name, seqno) {}
};

class SandeshFlow : public Sandesh {
protected:
    SandeshFlow(const std::string& name, uint32_t seqno) :
        Sandesh(SandeshType::FLOW, name, seqno) {}
};

class SandeshUVE : public Sandesh {
public:
    const bool get_more() const { return more_; }
    typedef enum {
        ST_SYNC = 0,
        ST_INTROSPECT = 1,
        ST_PERIODIC = 2,
        ST_MAX = 3
    } SendType;
    virtual std::string DataLog(void) { return std::string(); }
protected:
    SandeshUVE(const std::string& name, uint32_t seqno,
               SandeshType::type t = SandeshType::UVE) :
        Sandesh(t, name, seqno), more_(false) {}
    bool Dispatch(SandeshConnection * sconn = NULL);
    void set_more(const bool val) { more_=val; }
  
 private:
    bool more_;
};

class SandeshAlarm : public SandeshUVE {
protected:
    SandeshAlarm(const std::string& name, uint32_t seqno) :
        SandeshUVE(name, seqno, SandeshType::ALARM) {}
};

template<>
struct WorkQueueDelete<SandeshElement> {
    template <typename QueueT>
    void operator()(QueueT &q, bool delete_entry) {
        SandeshElement element;
        while (q.try_pop(element)) {
            Sandesh *sandesh(element.snh_);
            sandesh->Release();
        }
    }
};

template<>
struct WorkQueueDelete<SandeshRequest *> {
    template <typename QueueT>
    void operator()(QueueT &q, bool delete_entry) {
        SandeshRequest *rsandesh;
        while (q.try_pop(rsandesh)) {
            rsandesh->Release();
        }
    }
};

template<typename T> Sandesh* createT() { return new T; }

class SandeshBaseFactory {
public:
    static Sandesh* CreateInstance(std::string const& s) {
        map_type::iterator it = GetMap()->find(s);
        if (it == GetMap()->end()) {
            return 0;
        }
        return it->second();
    }

    typedef std::map<std::string, Sandesh*(*)()> map_type;

    static void Update(map_type &map) {
        map_type::const_iterator m_iter = map.begin();
        for (; m_iter != map.end(); m_iter++) {
            map_type::iterator b_iter = GetMap()->find((*m_iter).first);
            if (b_iter != End()) {
                GetMap()->erase(b_iter);
            }
            GetMap()->insert(*m_iter);
        }
    }
    static map_type::const_iterator Begin() { return GetMap()->begin(); }
    static map_type::const_iterator End() { return GetMap()->end(); }

protected:
    static map_type* GetMap() {
        static map_type map_;
        return &map_;
    }
};

template<typename T>
struct SandeshDerivedRegister : public SandeshBaseFactory {
    SandeshDerivedRegister(std::string const& s) :
        name_(s) {
        GetMap()->insert(std::make_pair(s, &createT<T>));
    }
    ~SandeshDerivedRegister() {
        map_type::iterator iter = GetMap()->find(name_);
        GetMap()->erase(iter);
    }
private:
    std::string name_;
};

#define SANDESH_REGISTER_DEC_TYPE(NAME) \
        static SandeshDerivedRegister<NAME> reg

#define SANDESH_REGISTER_DEF_TYPE(NAME) \
        SandeshDerivedRegister<NAME> NAME::reg(#NAME)

bool DoDropSandeshMessage(const SandeshHeader &header,
    SandeshLevel::type drop_level);

log4cplus::LogLevel SandeshLevelTolog4Level(
    SandeshLevel::type slevel);

template <typename T>
struct SandeshStructDeleteTrait {
    static bool get(const T& s) { return false; } 
};
template <typename T>
struct SandeshStructProxyTrait {
    static std::string get(const T& s) { return std::string(""); } 
};
#endif // __SANDESH_H__
