/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_state_machine_test.cc
//

#include <boost/assign.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "base/logging.h"
#include "base/test/task_test_util.h"

#include "io/event_manager.h"
#include "testing/gunit.h"

#include <sandesh/protocol/TXMLProtocol.h>
#include <sandesh/transport/TBufferTransports.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_server.h>
#include <sandesh/sandesh_session.h>
#include "sandesh_connection.h"
#include "sandesh_client.h"
#include "sandesh_state_machine.h"

using namespace std;
using namespace boost::assign;
using namespace boost::posix_time;
using boost::system::error_code;
using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;

typedef boost::asio::ip::tcp::endpoint Endpoint;

class SandeshSessionMock : public SandeshSession {
public:
    enum State {
        SERVER_INIT,
        CLOSE,
    };

    enum Direction {
        ACTIVE,
        PASSIVE,
    };

    SandeshSessionMock(TcpServer *server, State state, Direction direction)
        : SandeshSession(server, NULL, Task::kTaskInstanceAny,
                TaskScheduler::GetInstance()->GetTaskId("sandesh::Test::StateMachine"),
                TaskScheduler::GetInstance()->GetTaskId("io::ReaderTask")),
          state_(state),
          direction_(direction),
          defer_reader_(false) {
    }

    ~SandeshSessionMock() {
        LOG(DEBUG, "Session delete");
    }

    virtual bool Send(const u_int8_t *data, size_t size, size_t *sent) {
        return true;
    }

    void Close() {
        state_ = SandeshSessionMock::CLOSE;
        SandeshSession::Close();
    }

    virtual void SetDeferReader(bool defer_reader) {
        defer_reader_ = defer_reader;
    }

    virtual bool IsReaderDeferred() const {
        return defer_reader_;
    }
    State state() { return state_; }
    Direction direction() { return direction_; }

private:
    State state_;
    Direction direction_;
    bool defer_reader_;
};

class SandeshServerMock : public SandeshServer {
public:
    SandeshServerMock(EventManager *evm) :
        SandeshServer(evm),
        session_(NULL),
        old_session_(NULL) {
    }

    ~SandeshServerMock() {
        LOG(DEBUG, "Server delete");
    }

    virtual TcpSession *CreateSession() {
        if (session_) {
            assert(!old_session_);
            old_session_ = session_;
            session_ = NULL;
        }
        assert(!session_);
        session_ = new SandeshSessionMock(this,
                SandeshSessionMock::SERVER_INIT,
                SandeshSessionMock::ACTIVE);
        return session_;
    }

    virtual void DeleteSession(TcpSession *session) {
        if (session_ == session) {
            session_ = NULL;
        } else if (old_session_ == session) {
            old_session_ = NULL;
        } else {
            assert(false);
        }
        delete static_cast<SandeshSessionMock *>(session);
    }

    virtual bool ReceiveSandeshMsg(SandeshSession *session,
       const SandeshMessage *msg, bool rsc) {
        return true;
    }

    SandeshSessionMock *session() { return session_; }
    SandeshSessionMock *old_session() { return old_session_; }
private:
    SandeshSessionMock *session_;
    SandeshSessionMock *old_session_;
};

const std::string FakeMessageSandeshBegin("<FakeSandesh type=\"sandesh\"><str1 type=\"string\" identifier=\"1\">");
const std::string FakeMessageSandeshEnd("</str1></FakeSandesh>");

static void CreateFakeMessage(uint8_t *data, size_t length) {
    size_t offset = 0;
    SandeshHeader header;
    // Populate the header
    header.set_Namespace("Test");
    header.set_Timestamp(123456);
    header.set_Module("SandeshStateMachineTest");
    header.set_Source("TestMachine");
    header.set_Context("");
    header.set_SequenceNum(0);
    header.set_VersionSig(0);
    header.set_Type(SandeshType::SYSTEM);
    header.set_Hints(0);
    header.set_Level(SandeshLevel::SYS_DEBUG);
    header.set_Category("SSM");
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(512));
    boost::shared_ptr<TXMLProtocol> prot =
            boost::shared_ptr<TXMLProtocol>(
                    new TXMLProtocol(btrans));
    // Write the sandesh header
    int ret = header.write(prot);
    EXPECT_GT(ret, 0);
    EXPECT_GT(length, offset + ret);
    // Get the buffer
    uint8_t *hbuffer;
    uint32_t hlen;
    btrans->getBuffer(&hbuffer, &hlen);
    EXPECT_EQ(hlen, ret);
    memcpy(data + offset, hbuffer, hlen);
    offset += hlen;
    EXPECT_GT(length, offset + FakeMessageSandeshBegin.size());
    memcpy(data + offset, FakeMessageSandeshBegin.c_str(),
            FakeMessageSandeshBegin.size());
    offset += FakeMessageSandeshBegin.size();
    memset(data + offset, '0', length - offset - FakeMessageSandeshEnd.size());
    offset += length - offset - FakeMessageSandeshEnd.size();
    memcpy(data + offset, FakeMessageSandeshEnd.c_str(),
           FakeMessageSandeshEnd.size());
    offset += FakeMessageSandeshEnd.size();
    EXPECT_EQ(offset, length);
}

class SandeshServerStateMachineTest : public ::testing::Test {
protected:
    SandeshServerStateMachineTest() :
        server_(new SandeshServerMock(&evm_)),
        timer_(TimerManager::CreateTimer(*evm_.io_service(), "Dummy timer")),
        connection_(new SandeshServerConnection(server_, dummy_, 
            Task::kTaskInstanceAny,
            TaskScheduler::GetInstance()->GetTaskId("sandesh::Test::StateMachine"))) {
        task_util::WaitForIdle();
        sm_ = connection_->state_machine();
        sm_->set_idle_hold_time(1);
        sm_->SetGeneratorKey("Test");
    }

    ~SandeshServerStateMachineTest() {
        task_util::WaitForIdle();
        connection_->Shutdown();
        task_util::WaitForIdle();
        delete connection_;
        server_->Shutdown();
        task_util::WaitForIdle();
        TcpServerManager::DeleteServer(server_);
        server_ = NULL;
        TimerManager::DeleteTimer(timer_);
        task_util::WaitForIdle();
        evm_.Shutdown();
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

    bool DummyTimerHandler() {
        LOG(DEBUG, "Dummy handler called");
        return false;
    }

    void RunToState(ssm::SsmState state) {
        timer_->Start(15000,
                     boost::bind(&SandeshServerStateMachineTest::DummyTimerHandler, this));
        task_util::WaitForIdle();
        for (int count = 0; count < 100 && sm_->get_state() != state; count++) {
            evm_.RunOnce();
            task_util::WaitForIdle();
        }
        timer_->Cancel();
        VerifyState(state);
    }

    void GetToState(ssm::SsmState state) {
        switch (state) {
        case ssm::IDLE: {
            EvAdminDown();
            VerifyState(state);
            break;
        }
        case ssm::ACTIVE: {
            GetToState(ssm::IDLE);
            EvAdminUp();
            RunToState(state);
            break;
        }
        case ssm::SERVER_INIT: {
            GetToState(ssm::ACTIVE);
            EvTcpPassiveOpen();
            VerifyState(ssm::SERVER_INIT);
            break;
        }
        default: {
            ASSERT_TRUE(false);
            break;
        }
        }
    }

    void VerifyState(ssm::SsmState state) {
        task_util::WaitForIdle();
        TaskScheduler::GetInstance()->Stop();

        LOG(DEBUG, "VerifyState " << state);
        EXPECT_EQ(state, sm_->get_state());

        switch (state) {
        case ssm::IDLE:
            EXPECT_TRUE(sm_->session() == NULL);
            EXPECT_TRUE(connection_->session() == NULL);
            break;
        case ssm::ACTIVE:
            EXPECT_TRUE(!IdleHoldTimerRunning());
            EXPECT_TRUE(sm_->session() == NULL);
            EXPECT_TRUE(connection_->session() == NULL);
            break;
        case ssm::SERVER_INIT:
            EXPECT_TRUE(!IdleHoldTimerRunning());
            EXPECT_TRUE(sm_->session() != NULL);
            EXPECT_TRUE(connection_->session() != NULL);
            break;
        default:
            ASSERT_TRUE(false);
            break;
        }

        TaskScheduler::GetInstance()->Start();
    }

    void VerifyDirection(SandeshSessionMock::Direction direction) {
        SandeshSession *session = sm_->connection()->session();
        SandeshSessionMock *mock_session = dynamic_cast<SandeshSessionMock *>(session);
        ASSERT_TRUE(mock_session != NULL);
        EXPECT_TRUE(mock_session->direction() == direction);
    }

    SandeshSessionMock *GetSession(SandeshSessionMock *session) {
        if (session) return session;
        session = server_->session();
        return session;
    }

    void EvStart() {
        sm_->Initialize();
    }
    void EvStop() {
        sm_->Shutdown();
    }
    void EvAdminUp() {
        connection_->SetAdminState(false);
        sm_->set_idle_hold_time(1);
    }
    void EvAdminDown() {
        connection_->SetAdminState(true);
        sm_->set_idle_hold_time(1);
    }
    void EvTcpPassiveOpen() {
        TcpSession *session = server_->CreateSession();
        SandeshSessionMock *mock_session = dynamic_cast<SandeshSessionMock *>(session);
        ASSERT_TRUE(mock_session != NULL);
        connection_->AcceptSession(mock_session);
    }
    void EvTcpClose(SandeshSessionMock *session = NULL) {
        if (!session) session = server_->session();
        sm_->OnSessionEvent(session, TcpSession::CLOSE);
    }
    void EvSandeshMessageRecv(SandeshSessionMock *session = NULL) {
        session = GetSession(session);
        uint8_t msg[1024];
        CreateFakeMessage(msg, sizeof(msg));
        string xml((const char *)msg, sizeof(msg));
        sm_->OnSandeshMessage(session, xml);
    }
    SandeshLevel::type MessageDropLevel() const {
        return sm_->message_drop_level_;
    }

    bool IdleHoldTimerRunning() { return sm_->idle_hold_timer_->running(); }

    EventManager evm_;
    SandeshServerMock *server_;
    SandeshStateMachine *sm_;
    Timer *timer_;
    Endpoint dummy_;
    SandeshConnection *connection_;
};

typedef boost::function<void(void)> EvGen;
struct EvGenComp {
    bool operator()(const EvGen &lhs, const EvGen &rhs) const {
        return &lhs < &rhs;
    }
};

TEST_F(SandeshServerStateMachineTest, Matrix) {
    boost::asio::io_service::work work(*evm_.io_service());
    typedef std::map<EvGen, ssm::SsmState, EvGenComp> Transitions;

#define SERVER_SSM_TRANSITION(F, E) \
    ((EvGen) boost::bind(&SandeshServerStateMachineTest_Matrix_Test::F, this), E)
#define SERVER_SSM_TRANSITION2(F, E) \
    ((EvGen) boost::bind(&SandeshServerStateMachineTest_Matrix_Test::F, this, \
            (SandeshSessionMock *) NULL), E)

    Transitions idle = map_list_of
            SERVER_SSM_TRANSITION(EvTcpPassiveOpen, ssm::IDLE)
            SERVER_SSM_TRANSITION(EvAdminUp, ssm::ACTIVE);

    Transitions active = map_list_of
            SERVER_SSM_TRANSITION(EvStop, ssm::IDLE)
            SERVER_SSM_TRANSITION(EvAdminDown, ssm::IDLE)
            SERVER_SSM_TRANSITION(EvTcpPassiveOpen, ssm::SERVER_INIT)
            SERVER_SSM_TRANSITION2(EvTcpClose, ssm::IDLE);

    Transitions none = map_list_of
            SERVER_SSM_TRANSITION(EvStop, ssm::IDLE);

    Transitions server_init = map_list_of
            SERVER_SSM_TRANSITION2(EvTcpClose, ssm::IDLE)
            SERVER_SSM_TRANSITION2(EvSandeshMessageRecv, ssm::SERVER_INIT)
            SERVER_SSM_TRANSITION2(EvTcpClose, ssm::IDLE);

    Transitions matrix[] =
        { idle, active, none, server_init };

    for (int k = ssm::IDLE; k <= ssm::SERVER_INIT; k++) {
        ssm::SsmState i = static_cast<ssm::SsmState> (k);
        // Ignore CONNECT, CLIENT_INIT and ESTABLISHED in SandeshServerStateMachine
        if (i == ssm::ESTABLISHED) {
            continue;
        }
        int count = 0;
        for (Transitions::iterator j = matrix[i].begin();
                j != matrix[i].end(); j++) {
            LOG(DEBUG, "Starting test " << count++ << " in state " << i
                    << " expecting " << j->second << " *********************");
            GetToState(i);
            j->first();
            RunToState(j->second);
        }
    }
}

TEST_F(SandeshServerStateMachineTest, WaterMark) {
    // Set watermarks
    std::vector<Sandesh::QueueWaterMarkInfo> wm_info =
        boost::assign::tuple_list_of
            (1*1024, SandeshLevel::SYS_EMERG, true, true)
            (512, SandeshLevel::INVALID, false, true);
    for (int i = 0; i < wm_info.size(); i++) {
        sm_->SetQueueWaterMarkInfo(wm_info[i]);
    }
    // Verify initial message drop level and the session reader defer status
    EXPECT_EQ(SandeshLevel::INVALID, MessageDropLevel());
    EXPECT_TRUE(sm_->session() == NULL);
    // Move to ssm::SERVER_INIT
    GetToState(ssm::SERVER_INIT);
    // Stop the task scheduler
    TaskScheduler::GetInstance()->Stop();
    // Enqueue 1 message 1024 byte
    EvSandeshMessageRecv();
    // Verify the message drop level and the session reader defer status
    EXPECT_EQ(SandeshLevel::SYS_EMERG, MessageDropLevel());
    EXPECT_TRUE(sm_->session()->IsReaderDeferred());
    // Start the task scheduler
    TaskScheduler::GetInstance()->Start();
    task_util::WaitForIdle();
    // Verify the message drop level and the session reader defer status
    EXPECT_EQ(SandeshLevel::INVALID, MessageDropLevel());
    EXPECT_FALSE(sm_->session()->IsReaderDeferred());
}

TEST_F(SandeshServerStateMachineTest, DeferDequeue) {
    // Move to ssm::SERVER_INIT
    GetToState(ssm::SERVER_INIT);
    // Verify the state machine queue is empty
    uint64_t sm_queue_count;
    ASSERT_TRUE(sm_->GetQueueCount(sm_queue_count));
    EXPECT_EQ(0, sm_queue_count);
    // Defer the dequeue
    sm_->SetDeferDequeue(true);
    // Enqueue 1 message
    EvSandeshMessageRecv();
    // Verify that state machine queue now has 1 entry
    ASSERT_TRUE(sm_->GetQueueCount(sm_queue_count));
    EXPECT_EQ(1024, sm_queue_count);
    // Verify the state machine state
    EXPECT_EQ(ssm::SERVER_INIT, sm_->get_state());
    // Undefer the dequeue
    sm_->SetDeferDequeue(false);
    task_util::WaitForIdle();
    // Verify the state machine queue is empty
    ASSERT_TRUE(sm_->GetQueueCount(sm_queue_count));
    EXPECT_EQ(0, sm_queue_count);
    // Verify the state machine state
    EXPECT_EQ(ssm::SERVER_INIT, sm_->get_state());
}

class SandeshServerStateMachineIdleTest : public SandeshServerStateMachineTest {
    virtual void SetUp() {
        GetToState(ssm::IDLE);
    }

    virtual void TearDown() {
    }
};

// Old State: Idle
// Event:     EvTcpPassiveOpen
// New State: Idle
TEST_F(SandeshServerStateMachineIdleTest, TcpPassiveOpen) {
    TaskScheduler::GetInstance()->Stop();
    EvTcpPassiveOpen();
    TaskScheduler::GetInstance()->Start();
    VerifyState(ssm::IDLE);
    EXPECT_TRUE(sm_->session() == NULL);
    VerifyState(ssm::IDLE);

    // Also verify that we can proceed after this event
    GetToState(ssm::ACTIVE);
}

class SandeshServerStateMachineActiveTest : public SandeshServerStateMachineTest {
    virtual void SetUp() {
        GetToState(ssm::ACTIVE);
    }

    virtual void TearDown() {
    }
};

// Old State: Active
// Event:     EvTcpPassiveOpen.
// New State: ServerInit
// Timers:    None should be running.
// Messages:  None.
TEST_F(SandeshServerStateMachineActiveTest, TcpPassiveOpen) {
    EvTcpPassiveOpen();
    RunToState(ssm::SERVER_INIT);
    EXPECT_TRUE(!IdleHoldTimerRunning());
    EXPECT_TRUE(sm_->session() != NULL);
}

// Old State: Active
// Event:     EvTcpClose on the session.
// New State: Active
// Timers:    None should be running.
// Messages:  None.
TEST_F(SandeshServerStateMachineActiveTest, TcpClose) {
    EvTcpPassiveOpen();
    VerifyState(ssm::SERVER_INIT);
    EvTcpClose(server_->session());
    VerifyState(ssm::IDLE);
    EXPECT_TRUE(sm_->session() == NULL);
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return result;
}
