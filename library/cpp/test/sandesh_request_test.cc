//
// Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
//

//
// sandesh_request_test.cc
//

#include "testing/gunit.h"

#include <boost/assign/list_of.hpp>
#include <boost/tuple/tuple.hpp>

#include <base/test/task_test_util.h>
#include <io/test/event_manager_test.h>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_client.h>

#include "sandesh_test_common.h"

using namespace std;
using namespace contrail::sandesh::test;

namespace {

class SandeshRequestTest : public ::testing::Test {
 protected:
    virtual void SetUp() {
        evm_.reset(new EventManager());
        server_ = new SandeshServerTest(evm_.get(),
            boost::bind(&SandeshRequestTest::ReceiveSandeshMsg, this, _1, _2));
        thread_.reset(new ServerThread(evm_.get()));
        server_->Initialize(0);
        thread_->Start();       // Must be called after initialization
        int port = server_->GetPort();
        ASSERT_LT(0, port);
        // Connect to the server
        Sandesh::InitGenerator("SandeshRequestTest-Client", "localhost",
                               "Test", "Test", evm_.get(), 0);
        Sandesh::ConnectToCollector("127.0.0.1", port);
        EXPECT_TRUE(Sandesh::IsConnectToCollectorEnabled());
        EXPECT_TRUE(Sandesh::client() != NULL);
        TASK_UTIL_EXPECT_TRUE(Sandesh::client()->state() ==
            SandeshClientSM::ESTABLISHED);
    }

    virtual void TearDown() {
        task_util::WaitForIdle();
        Sandesh::Uninit();
        task_util::WaitForIdle();
        TASK_UTIL_EXPECT_FALSE(server_->HasSessions());
        task_util::WaitForIdle();
        server_->Shutdown();
        task_util::WaitForIdle();
        TcpServerManager::DeleteServer(server_);
        task_util::WaitForIdle();
        evm_->Shutdown();
        if (thread_.get() != NULL) {
            thread_->Join();
        }
        task_util::WaitForIdle();
    }

    bool ReceiveSandeshMsg(SandeshSession *session,
                           const SandeshMessage *msg) {
        return true;
    }

    static SandeshLoggingParamsSet* PrepareLoggingParamsSetReq(bool enable,
        std::string category, std::string log_level, bool enable_trace_print,
        bool enable_flow_log, int called_from_line) {
        SandeshLoggingParamsSet *req(new SandeshLoggingParamsSet);
        req->set_enable(enable);
        req->set_category(category);
        req->set_log_level(log_level);
        req->set_trace_print(enable_trace_print);
        req->set_enable_flow_log(enable_flow_log);
        Sandesh::set_response_callback(
            boost::bind(ValidateLoggingParamsResponse, _1, enable, category,
            log_level, enable_trace_print, enable_flow_log, called_from_line));
        return req;
    }

    static void ValidateLoggingParamsResponse(Sandesh *response,
        bool enable_local_log, std::string category, std::string level,
        bool enable_trace_print, bool enable_flow_log, int called_from_line) {
        cout << "From line number: " << called_from_line << endl;
        cout << "*****************************************************" << endl;
        SandeshLoggingParams *lresponse(
            dynamic_cast<SandeshLoggingParams *>(response));
        EXPECT_TRUE(lresponse != NULL);
        EXPECT_EQ(enable_local_log, lresponse->get_enable());
        EXPECT_EQ(category, lresponse->get_category());
        EXPECT_EQ(level, lresponse->get_log_level());
        EXPECT_EQ(enable_trace_print, lresponse->get_trace_print());
        EXPECT_EQ(enable_flow_log, lresponse->get_enable_flow_log());
        validate_done_ = true;
        cout << "*****************************************************" << endl;
    }

    static void ValidateSendQueueResponse(Sandesh *response, bool enable,
        int called_from_line) {
        cout << "From line number: " << called_from_line << endl;
        cout << "*****************************************************" << endl;
        SandeshSendQueueResponse *sresponse(
            dynamic_cast<SandeshSendQueueResponse *>(response));
        EXPECT_TRUE(sresponse != NULL);
        EXPECT_EQ(enable, sresponse->get_enable());
        validate_done_ = true;
        cout << "*****************************************************" << endl;
    }

    static SandeshSendingParamsSet* PopulateSendingParamsSetReq(
        const SandeshSendingParams &e_ssp,
        const std::vector<SandeshSendingParams> &v_e_ssp,
        int called_from_line, int sending_params_count) {
        SandeshSendingParamsSet *req(new SandeshSendingParamsSet);
        req->set_high(e_ssp.get_high());
        req->set_queue_count(e_ssp.get_queue_count());
        req->set_sending_level(e_ssp.get_sending_level());
        Sandesh::set_response_callback(
            boost::bind(ValidateSendingParamsResponse, _1, v_e_ssp,
            called_from_line, sending_params_count));
        return req;
    }

    static void ValidateSendingParamsResponse(Sandesh *response,
        const std::vector<SandeshSendingParams> &v_ssp, int called_from_line,
        int sending_params_count) {
        cout << "From line number: " << called_from_line << endl;
        cout << "Sending params count: " << sending_params_count << endl;
        cout << "*****************************************************" << endl;
        SandeshSendingParamsResponse *sspr(
            dynamic_cast<SandeshSendingParamsResponse *>(response));
        EXPECT_TRUE(sspr != NULL);
        EXPECT_THAT(v_ssp, ::testing::ContainerEq(sspr->get_info()));
        validate_done_ = true;
        cout << "*****************************************************" << endl;
    }

    static void ValidateCollectorInfoResponse(Sandesh *response,
        const std::string &ip, int port, const std::string &status,
        int called_from_line) {
        cout << "From line number: " << called_from_line << endl;
        cout << "*****************************************************" << endl;
        CollectorInfoResponse *resp(
            dynamic_cast<CollectorInfoResponse *>(response));
        EXPECT_TRUE(resp != NULL);
        EXPECT_EQ(ip, resp->get_ip());
        EXPECT_EQ(port, resp->get_port());
        EXPECT_EQ(status, resp->get_status());
        validate_done_ = true;
        cout << "*****************************************************" << endl;
    }

    static void ValidateMessageStatsResponse(Sandesh *response,
        const SandeshGeneratorStats &e_stats, const SandeshQueueStats &e_qstats,
        int called_from_line) {
        cout << "From line number: " << called_from_line << endl;
        cout << "*****************************************************" << endl;
        SandeshMessageStatsResp *resp(
            dynamic_cast<SandeshMessageStatsResp *>(response));
        EXPECT_TRUE(resp != NULL);
        const SandeshGeneratorStats &a_stats(resp->get_stats());
        const std::vector<SandeshMessageTypeStats> &e_mtype_stats(
            e_stats.get_type_stats());
        const SandeshMessageStats &e_magg_stats(e_stats.get_aggregate_stats());
        EXPECT_EQ(e_magg_stats, a_stats.get_aggregate_stats());
        EXPECT_THAT(e_mtype_stats,
            ::testing::ContainerEq(a_stats.get_type_stats()));
        EXPECT_EQ(e_stats, a_stats);
        const SandeshQueueStats &a_qstats(resp->get_send_queue_stats());
        EXPECT_EQ(e_qstats, a_qstats);
        validate_done_ = true;
        cout << "*****************************************************" << endl;
    }

    static bool validate_done_;

    std::auto_ptr<ServerThread> thread_;
    std::auto_ptr<EventManager> evm_;
    SandeshServerTest *server_;
};

bool SandeshRequestTest::validate_done_;

TEST_F(SandeshRequestTest, LoggingParams) {
    // Get logging params
    bool o_enable(Sandesh::IsLocalLoggingEnabled());
    std::string o_category(Sandesh::LoggingCategory());
    if (o_category.empty()) {
        o_category = "*";
    }
    std::string o_log_level(Sandesh::LevelToString(Sandesh::LoggingLevel()));
    bool o_enable_trace_print(Sandesh::IsTracePrintEnabled());
    bool o_enable_flow_log(Sandesh::IsFlowLoggingEnabled());
    // Set
    bool enable(false);
    std::string category("SandeshRequest");
    std::string log_level("SYS_NOTICE");
    bool enable_trace_print(true);
    bool enable_flow_log(true);
    SandeshLoggingParamsSet *req(PrepareLoggingParamsSetReq(enable, category,
        log_level, enable_trace_print, enable_flow_log, __LINE__));
    validate_done_ = false;
    req->HandleRequest();
    req->Release();
    task_util::WaitForIdle();
    TASK_UTIL_EXPECT_EQ(true, validate_done_);
    // Status
    SandeshLoggingParamsStatus *req1(new SandeshLoggingParamsStatus);
    Sandesh::set_response_callback(boost::bind(ValidateLoggingParamsResponse,
        _1, enable, category, log_level, enable_trace_print, enable_flow_log,
        __LINE__));
    validate_done_ = false;
    req1->HandleRequest();
    req1->Release();
    task_util::WaitForIdle();
    TASK_UTIL_EXPECT_EQ(true, validate_done_);
    // Set to original
    SandeshLoggingParamsSet *req2(PrepareLoggingParamsSetReq(o_enable,
        o_category, o_log_level, o_enable_trace_print, o_enable_flow_log,
        __LINE__));
    validate_done_ = false;
    req2->HandleRequest();
    req2->Release();
    task_util::WaitForIdle();
    TASK_UTIL_EXPECT_EQ(true, validate_done_);
}

TEST_F(SandeshRequestTest, SendingParams) {
    // Reset sending params
    SandeshSendingParamsReset *req(new SandeshSendingParamsReset);
    Sandesh::set_response_callback(boost::bind(ValidateSendingParamsResponse,
        _1, std::vector<SandeshSendingParams>(), __LINE__, 0));
    validate_done_ = false;
    req->HandleRequest();
    req->Release();
    task_util::WaitForIdle();
    TASK_UTIL_EXPECT_EQ(true, validate_done_);
    // Set sending params
    typedef boost::tuple<bool, uint32_t, std::string> SendingParams;
    std::vector<SendingParams> v_sp = boost::assign::tuple_list_of
        (true, 10000, "SYS_EMERG")
        (true, 5000, "SYS_ERR")
        (true, 2500, "SYS_DEBUG")
        (false, 8000, "SYS_ERR")
        (false, 3000, "SYS_DEBUG")
        (false, 500, "INVALID");
    std::vector<SandeshSendingParams> v_ssp;
    for (int count = 0; count < v_sp.size(); count++) {
        const SendingParams &sp(v_sp[count]);
        SandeshSendingParams ssp;
        ssp.set_high(boost::get<0>(sp));
        ssp.set_queue_count(boost::get<1>(sp));
        ssp.set_sending_level(boost::get<2>(sp));
        v_ssp.push_back(ssp);
    }
    for (int count = 0; count < v_ssp.size(); count++) {
        SandeshSendingParamsSet *req1(PopulateSendingParamsSetReq(v_ssp[count],
            std::vector<SandeshSendingParams>(
                v_ssp.begin(), v_ssp.begin() + count + 1), __LINE__, count));
        validate_done_ = false;
        req1->HandleRequest();
        req1->Release();
        task_util::WaitForIdle();
        TASK_UTIL_EXPECT_EQ(true, validate_done_);
    }
    // Status
    SandeshSendingParamsStatus *req2(new SandeshSendingParamsStatus);
    Sandesh::set_response_callback(boost::bind(ValidateSendingParamsResponse,
        _1, v_ssp, __LINE__, 0));
    validate_done_ = false;
    req2->HandleRequest();
    req2->Release();
    task_util::WaitForIdle();
    TASK_UTIL_EXPECT_EQ(true, validate_done_);
}

TEST_F(SandeshRequestTest, SendQueue) {
    // Get send queue status
    bool enabled(Sandesh::IsSendQueueEnabled());
    // Set
    SandeshSendQueueSet *req(new SandeshSendQueueSet);
    req->set_enable(!enabled);
    Sandesh::set_response_callback(boost::bind(ValidateSendQueueResponse, _1,
        !enabled, __LINE__));
    validate_done_ = false;
    req->HandleRequest();
    req->Release();
    task_util::WaitForIdle();
    TASK_UTIL_EXPECT_EQ(true, validate_done_);
    // Status
    SandeshSendQueueStatus *req1(new SandeshSendQueueStatus);
    Sandesh::set_response_callback(boost::bind(ValidateSendQueueResponse, _1,
        !enabled, __LINE__));
    validate_done_ = false;
    req1->HandleRequest();
    req1->Release();
    task_util::WaitForIdle();
    TASK_UTIL_EXPECT_EQ(true, validate_done_);
    // Set to original
    SandeshSendQueueSet *req2(new SandeshSendQueueSet);
    req2->set_enable(enabled);
    Sandesh::set_response_callback(boost::bind(ValidateSendQueueResponse, _1,
        enabled, __LINE__));
    validate_done_ = false;
    req2->HandleRequest();
    req2->Release();
    task_util::WaitForIdle();
    TASK_UTIL_EXPECT_EQ(true, validate_done_);
}

TEST_F(SandeshRequestTest, CollectorInfo) {
    CollectorInfoRequest *req(new CollectorInfoRequest);
    Sandesh::set_response_callback(boost::bind(ValidateCollectorInfoResponse,
        _1, "127.0.0.1", server_->GetPort(), "Established", __LINE__));
    validate_done_ = false;
    req->HandleRequest();
    req->Release();
    task_util::WaitForIdle();
    TASK_UTIL_EXPECT_EQ(true, validate_done_);
}

TEST_F(SandeshRequestTest, MessageStats) {
    task_util::WaitForIdle();
    TaskScheduler *scheduler(TaskScheduler::GetInstance());
    scheduler->Stop();
    // Extract stats
    std::vector<SandeshMessageTypeStats> mtype_stats;
    SandeshMessageStats magg_stats;
    Sandesh::GetMsgStats(&mtype_stats, &magg_stats);
    SandeshGeneratorStats sandesh_stats;
    sandesh_stats.set_type_stats(mtype_stats);
    sandesh_stats.set_aggregate_stats(magg_stats);
    SandeshClient *client = Sandesh::client();
    ASSERT_TRUE(client != NULL);
    ASSERT_TRUE(client->session() != NULL);
    ASSERT_TRUE(client->session()->send_queue() != NULL);
    SandeshQueueStats qstats;
    qstats.set_enqueues(client->session()->send_queue()->NumEnqueues());
    qstats.set_count(client->session()->send_queue()->Length());
    qstats.set_max_count(client->session()->send_queue()->max_queue_len());
    // Send request
    SandeshMessageStatsReq *req(new SandeshMessageStatsReq);
    Sandesh::set_response_callback(boost::bind(ValidateMessageStatsResponse,
        _1, sandesh_stats, qstats, __LINE__));
    validate_done_ = false;
    req->HandleRequest();
    req->Release();
    scheduler->Start();
    task_util::WaitForIdle();
    TASK_UTIL_EXPECT_EQ(true, validate_done_);
}

} // end namespace

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    return result;
}
