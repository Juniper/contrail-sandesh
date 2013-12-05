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

#include <boost/asio/ip/tcp.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <base/queue_task.h>
#include <base/contrail_ports.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/protocol/TProtocol.h>
#include <sandesh/transport/TBufferTransports.h>
#include <discovery/client/discovery_client.h>

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

class SandeshConnection;
//
// Sandesh
//
class Sandesh;
template<>
struct WorkQueueDelete<Sandesh *> {
    template <typename QueueT>
    void operator()(QueueT &q) {
        for (typename QueueT::iterator iter = q.unsafe_begin();
             iter != q.unsafe_end(); ++iter) {
            (*iter)->Release();
        }
    }
};

class SandeshRequest;
template<>
struct WorkQueueDelete<SandeshRequest *> {
    template <typename QueueT>
    void operator()(QueueT &q) {
        for (typename QueueT::iterator iter = q.unsafe_begin();
             iter != q.unsafe_end(); ++iter) {
            (*iter)->Release();
        }
    }
};

class SandeshStatistics;
class Sandesh {
public:
    typedef WorkQueue<SandeshRequest *> SandeshRxQueue;
    typedef WorkQueue<Sandesh *> SandeshQueue;
    typedef WorkQueue<
            boost::shared_ptr<contrail::sandesh::transport::TMemoryBuffer> >
            SandeshBufferQueue;
    typedef boost::asio::ip::tcp::endpoint Endpoint;
    typedef boost::function<void (Sandesh *)> SandeshCallback;
    typedef enum {
        Invalid,
        Generator,
        Collector,
        Test,
    } SandeshRole;

    typedef boost::function<void (std::string serviceName, uint8_t numbOfInstances,
            DiscoveryServiceClient::ServiceHandler)> CollectorSubFn;
    // Initialization APIs
    static bool InitGenerator(const std::string &module,
            const std::string &source,
            EventManager *evm,
            unsigned short http_port,
            CollectorSubFn csf,
            const std::vector<std::string> collectors,
            SandeshContext *client_context = NULL);
    static void InitCollector(const std::string module,
            const std::string source, EventManager *evm,
            const std::string collector_ip, int collector_port,
            unsigned short http_port,
            SandeshContext *client_context = NULL);
    static void InitGenerator(const std::string module,
            const std::string source, EventManager *evm,
            unsigned short http_port,
            SandeshContext *client_context = NULL);
    static bool ConnectToCollector(const std::string collector_ip,
            int collector_port);
    static void InitGeneratorTest(const std::string module,
            const std::string source, EventManager *evm,
            unsigned short http_port,
            SandeshContext *client_context = NULL);
    static void Uninit();

    // Logging and category APIs
    static void SetLoggingParams(bool enable_local_log, std::string category,
            std::string level, bool enable_trace_print = false);
    static void SetLoggingParams(bool enable_local_log, std::string category,
            SandeshLevel::type level, bool enable_trace_print = false);
    static void SetLoggingLevel(std::string level);
    static void SetLoggingLevel(SandeshLevel::type level);
    static SandeshLevel::type LoggingLevel() { return logging_level_; }
    static bool IsLocalLoggingEnabled() { return enable_local_log_; }
    static void SetLocalLogging(bool enable);
    static bool IsTracePrintEnabled() { return enable_trace_print_; }
    static void SetTracePrint(bool enable);
    static void SetLoggingCategory(std::string category);
    static std::string LoggingCategory() { return logging_category_; }
    static void SendLoggingResponse(std::string context);

    // Send queue processing
    static void SetSendQueue(bool enable);
    static inline bool IsSendQueueEnabled() { 
        return send_queue_enabled_;
    }
    static void SendQueueResponse(std::string context);

    static int32_t ReceiveBinaryMsgOne(u_int8_t *buf, u_int32_t buf_len,
            int *error, SandeshContext *client_context);
    static int32_t ReceiveBinaryMsg(u_int8_t *buf, u_int32_t buf_len,
            int *error, SandeshContext *client_context);
    static bool SendReady(SandeshConnection * sconn = NULL);
    static void UpdateSandeshStats(const std::string& sandesh_name,
                                   uint32_t bytes, bool is_tx, bool dropped);
    static const char *  SandeshRoleToString(SandeshRole role);

    virtual void Release() { delete this; }
    virtual void Log() const = 0;
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

    bool IsLoggingAllowed();

    // Accessors
    static void set_source(std::string source) { source_ = source; }
    static std::string source() { return source_; }
    static void set_module(std::string module) { module_ = module; }
    static std::string module() { return module_; }
    static SandeshRole role() { return role_; }
    static uint32_t http_port() { return http_port_; }
    static SandeshRxQueue* recv_queue() { return recv_queue_.get(); }
    static SandeshContext* client_context() { return client_context_; }
    static void set_client_context(SandeshContext *context) { client_context_ = context; }
    static void set_response_callback(SandeshCallback response_cb) { response_callback_ = response_cb; }
    static SandeshCallback response_callback() { return response_callback_; }
    static SandeshClient* client() { return client_; }

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

    static SandeshStatistics stats_;
    static tbb::mutex stats_mutex_;

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
    virtual ~Sandesh() {}
    virtual bool Dispatch(SandeshConnection * sconn = NULL);
    virtual bool SendEnqueue();

    bool HandleTest();

    static bool IsUnitTest() { return role_ == Invalid || role_ == Test; }

    static SandeshCallback response_callback_;
    static SandeshClient *client_;

private:
    friend class SandeshTracePerfTest;

    static void InitReceive(int recv_task_inst = -1);
    static void InitClient(EventManager *evm, Endpoint server);
    static bool ProcessRecv(SandeshRequest *);
    static void Initialize(SandeshRole role, const std::string module,
            const std::string source, EventManager *evm,
            unsigned short http_port,
            SandeshContext *client_context = NULL);

    bool IsLevelUT();

    static SandeshRole role_;
    static std::string module_;
    static std::string source_;
    static uint32_t http_port_;
    static std::auto_ptr<SandeshRxQueue> recv_queue_;
    static int recv_task_id_;
    static SandeshContext *client_context_;
    static bool enable_local_log_;  // whether to just enable local logging
    static SandeshLevel::type logging_level_; // current logging level
    static std::string logging_category_; // current logging category
    static bool enable_trace_print_; // whether to print traces locally
    static EventManager *event_manager_;
    static bool send_queue_enabled_;

    const uint32_t seqnum_;
    std::string context_;
    time_t timestamp_;
    std::string scope_;
    SandeshType::type type_;
    int32_t hints_;
    SandeshLevel::type level_;
    std::string category_;
    std::string name_;
};

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
        Sandesh(SandeshType::BUFFER, name, seqno) {}
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
protected:
    SandeshUVE(const std::string& name, uint32_t seqno) :
        Sandesh(SandeshType::UVE, name, seqno), more_(false) {}
    bool Dispatch(SandeshConnection * sconn = NULL);
    void set_more(const bool val) { more_=val; }
  
 private:
    bool more_;
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

#endif // __SANDESH_H__
