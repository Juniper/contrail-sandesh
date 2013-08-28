/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_client_test.cc
//

#include <boost/assign.hpp>
#include <boost/asio.hpp>
#include <boost/foreach.hpp>

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
#include "sandesh_factory.h"
#include "sandesh_client.h"

using namespace std;
using namespace boost::assign;
using namespace boost::posix_time;
using boost::system::error_code;
using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;

class SandeshSessionMock : public SandeshSession {
public:
    enum State {
        CONNECT,
        CLIENT_INIT,
        CLOSE,
    };

    enum Direction {
        ACTIVE,
        PASSIVE,
    };

    SandeshSessionMock(TcpServer *server, State state, Direction direction)
        : SandeshSession(server, NULL), state_(state), direction_(direction) {
    }

    ~SandeshSessionMock() {
        LOG(DEBUG, "Session delete");
    }

    virtual bool Send(const u_int8_t *data, size_t size, size_t *sent) {
        return true;
    }

    virtual bool Connected(Endpoint remote) {
        state_ = SandeshSessionMock::CLIENT_INIT;
        EventObserver obs = observer();
        if (obs != NULL) {
            obs(this, CONNECT_COMPLETE);
        }
        return true;
    }

    void Close() {
        state_ = SandeshSessionMock::CLOSE;
        SandeshSession::Close();
    }

    State state() { return state_; }
    Direction direction() { return direction_; }

private:
    State state_;
    Direction direction_;
};


class SandeshClientMock : public SandeshClient {
public:
    SandeshClientMock(EventManager *evm, Endpoint dummy) :
        SandeshClient(evm, dummy),
        session_(NULL),
        old_session_(NULL) {
    }

    ~SandeshClientMock() {
        LOG(DEBUG, "Client delete");
    }

    virtual void Connect(TcpSession *session, Endpoint remote) {
    }

    virtual TcpSession *CreateSession() {
        if (session_) {
            assert(!old_session_);
            old_session_ = session_;
            session_ = NULL;
        }
        assert(!session_);
        session_ = new SandeshSessionMock(this,
                SandeshSessionMock::CONNECT,
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
       const string& cmsg, const string& message_type,
       const SandeshHeader& header, uint32_t xml_offset) { 
        return true;
    }

    SandeshSessionMock *session() { return session_; }
    SandeshSessionMock *old_session() { return old_session_; }
private:
    SandeshSessionMock *session_;
    SandeshSessionMock *old_session_;
};

const std::string FakeMessageSandeshBegin("<FakeSandesh type=\"sandesh\">");

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
    memset(data + offset, '0', length - offset);
    offset += length - offset;
    EXPECT_EQ(offset, length);
}


class SandeshClientStateMachineTest : public ::testing::Test {
protected:
    SandeshClientStateMachineTest() :
        client_(new SandeshClientMock((&evm_), dummy_)),
        timer_(TimerManager::CreateTimer(*evm_.io_service(), "Dummy timer")) {
        task_util::WaitForIdle();
        client_->set_idle_hold_time(1);
    }

    ~SandeshClientStateMachineTest() {
        task_util::WaitForIdle();
        client_->Shutdown();
        task_util::WaitForIdle();
        TcpServerManager::DeleteServer(client_);
        client_ = NULL;
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

    void RunToState(scm::SsmState state) {
        timer_->Start(15000,
                     boost::bind(&SandeshClientStateMachineTest::DummyTimerHandler, this));
        task_util::WaitForIdle();
        for (int count = 0; count < 100 && client_->get_state() != state; count++) {
            evm_.RunOnce();
            task_util::WaitForIdle();
        }
        timer_->Cancel();
        VerifyState(state);
    }

    void GetToState(scm::SsmState state) {
        switch (state) {
        case scm::IDLE: {
            EvAdminDown();
            VerifyState(state);
            break;
        }
        case scm::CONNECT: {
            GetToState(scm::IDLE);
            EvAdminUp();
            RunToState(state);
            break;
        }
        case scm::CLIENT_INIT: {
            GetToState(scm::CONNECT);
            Endpoint endpoint;
            client_->session()->Connected(endpoint);
            VerifyState(scm::CLIENT_INIT);
            break;
        }
        default: {
            ASSERT_TRUE(false);
            break;
        }
        }
    }

    void VerifyState(scm::SsmState state) {
        task_util::WaitForIdle();
        TaskScheduler::GetInstance()->Stop();

        LOG(DEBUG, "VerifyState " << state);
        EXPECT_EQ(state, client_->get_state());

        switch (state) {
        case scm::IDLE:
            EXPECT_TRUE(!ConnectTimerRunning());
            EXPECT_TRUE(client_->session() == NULL);
            break;
        case scm::CONNECT:
            EXPECT_TRUE(ConnectTimerRunning());
            EXPECT_TRUE(!IdleHoldTimerRunning());
            EXPECT_TRUE(client_->session() != NULL);
            break;
        case scm::CLIENT_INIT:
            EXPECT_TRUE(!ConnectTimerRunning());
            EXPECT_TRUE(!IdleHoldTimerRunning());
            EXPECT_TRUE(client_->session() != NULL);
            break;
        default:
            ASSERT_TRUE(false);
            break;
        }

        TaskScheduler::GetInstance()->Start();
    }

    void VerifyDirection(SandeshSessionMock::Direction direction) {
        SandeshSession *session = client_->session();
        SandeshSessionMock *mock_session = dynamic_cast<SandeshSessionMock *>(session);
        ASSERT_TRUE(mock_session != NULL);
        EXPECT_TRUE(mock_session->direction() == direction);
    }

    SandeshSessionMock *GetSession(SandeshSessionMock *session) {
        if (session) return session;
        session = client_->session();
        return session;
    }

    void EvStart() {
        client_->Initiate();
    }
    void EvStop() {
        client_->Shutdown();
    }
    void EvAdminUp() {
        client_->SetAdminState(false);
        client_->set_idle_hold_time(1);
    }
    void EvAdminDown() {
        client_->SetAdminState(true);
        client_->set_idle_hold_time(1);
    }
    void EvConnectTimerExpired() {
        client_->FireConnectTimer();
    }
    void EvTcpConnected() {
        client_->OnSessionEvent(client_->session(),
            TcpSession::CONNECT_COMPLETE);
    }
    void EvTcpConnectFail() {
        client_->OnSessionEvent(client_->session(),
            TcpSession::CONNECT_FAILED);
    }
    void EvTcpClose(SandeshSessionMock *session = NULL) {
        if (!session) session = client_->session();
        client_->OnSessionEvent(session, TcpSession::CLOSE);
    }
    void EvSandeshMessageRecv(SandeshSessionMock *session = NULL) {
        session = GetSession(session);
        uint8_t msg[1024];
        CreateFakeMessage(msg, sizeof(msg));
        string xml((const char *)msg, sizeof(msg));
        client_->OnMessage(session, xml);
    }

    bool IdleHoldTimerRunning() { return client_->idle_hold_timer_->running(); }
    bool ConnectTimerRunning() { return client_->connect_timer_->running(); }

    EventManager evm_;
    SandeshClientMock *client_;
    Timer *timer_;
    Endpoint dummy_;
};

typedef boost::function<void(void)> EvGen;
struct EvGenComp {
    bool operator()(const EvGen &lhs, const EvGen &rhs) { return &lhs < &rhs; }
};


TEST_F(SandeshClientStateMachineTest, Matrix) {
    boost::asio::io_service::work work(*evm_.io_service());
    typedef std::map<EvGen, scm::SsmState, EvGenComp> Transitions;

#define CLIENT_SSM_TRANSITION(F, E) \
    ((EvGen) boost::bind(&SandeshClientStateMachineTest_Matrix_Test::F, this), E)
#define CLIENT_SSM_TRANSITION2(F, E) \
    ((EvGen) boost::bind(&SandeshClientStateMachineTest_Matrix_Test::F, this, \
            (SandeshSessionMock *) NULL), E)

    Transitions idle = map_list_of
            CLIENT_SSM_TRANSITION(EvAdminUp, scm::CONNECT);

    Transitions none = map_list_of
            CLIENT_SSM_TRANSITION(EvStop, scm::IDLE);

    Transitions connect = map_list_of
            CLIENT_SSM_TRANSITION(EvStop, scm::IDLE)
            CLIENT_SSM_TRANSITION(EvConnectTimerExpired, scm::CONNECT)
            CLIENT_SSM_TRANSITION(EvTcpConnected, scm::CLIENT_INIT)
            CLIENT_SSM_TRANSITION(EvTcpConnectFail, scm::IDLE)
            CLIENT_SSM_TRANSITION2(EvTcpClose, scm::CONNECT);

    Transitions client_init = map_list_of
            CLIENT_SSM_TRANSITION2(EvTcpClose, scm::CONNECT)
            CLIENT_SSM_TRANSITION2(EvSandeshMessageRecv, scm::CLIENT_INIT);

    Transitions matrix[] =
        { idle, connect, none, client_init };

    for (int k = scm::IDLE; k <= scm::CLIENT_INIT; k++) {
        scm::SsmState i = static_cast<scm::SsmState> (k);
        // Ignore  ESTABLISHED in SandeshClientStateMachine
        if (i == scm::ESTABLISHED) {
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


class SandeshClientStateMachineConnectTest : public SandeshClientStateMachineTest {
    virtual void SetUp() {
        GetToState(scm::CONNECT);
    }

    virtual void TearDown() {
    }
};

// Old State: Connect
// Event:     EvTcpConnected
// New State: Established
// Timers:    None should be running.
// Messages:  None.
TEST_F(SandeshClientStateMachineConnectTest, TcpConnected) {
    EvTcpConnected();
    VerifyState(scm::CLIENT_INIT);
    EXPECT_TRUE(!ConnectTimerRunning());
    EXPECT_TRUE(client_->session() != NULL);
}

// Old State: Connect
// Event:     EvConnectTimerExpired
// New State: Active
// Timers:    Connect timer must be running again.
// Messages:  None.
TEST_F(SandeshClientStateMachineConnectTest, ConnectTimerExpired) {
    EvConnectTimerExpired();
    VerifyState(scm::CONNECT);
    EXPECT_TRUE(ConnectTimerRunning());
    EXPECT_TRUE(client_->session() != NULL);
}

// Old State: Connect
// Event:     EvTcpConnectFail
// New State: Idle
// Timers:    Idle hold timer must be running. Connect timer should not be running.
// Messages:  None.
TEST_F(SandeshClientStateMachineConnectTest, TcpConnectFail) {
    EvTcpConnectFail();
    VerifyState(scm::IDLE);
    EXPECT_TRUE(!ConnectTimerRunning());
    EXPECT_TRUE(IdleHoldTimerRunning());
    EXPECT_TRUE(client_->session() == NULL);
}

// Old State: Connect
// Event:     EvTcpConnected + EvConnectTimerExpired
// New State: ClientInit
// Messages:  None.
// Other:     The Connect timer has expired before it can be cancelled while
//            processing EvTcpConnected.
TEST_F(SandeshClientStateMachineConnectTest, TcpConnectedThenConnectTimerExpired) {
    TaskScheduler::GetInstance()->Stop();
    EvTcpConnected();
    EvConnectTimerExpired();
    TaskScheduler::GetInstance()->Start();
    VerifyState(scm::CLIENT_INIT);
    EXPECT_TRUE(client_->session() != NULL);
    EXPECT_TRUE(!ConnectTimerRunning());
    EXPECT_TRUE(!IdleHoldTimerRunning());
}

// Old State: Connect
// Event:     EvConnectTimerExpired + EvTcpConnected
// New State: Connect
// Messages:  None.
// Other:     The Connect timer expired before we see EvTcpConnected, thus
//            the session would already have been deleted when we process
//            EvTcpConnected.
TEST_F(SandeshClientStateMachineConnectTest, ConnectTimerExpiredThenTcpConnected) {
    TaskScheduler::GetInstance()->Stop();
    EvConnectTimerExpired();
    EvTcpConnected();
    TaskScheduler::GetInstance()->Start();
    VerifyState(scm::CONNECT);
    EXPECT_TRUE(client_->session() != NULL);
    EXPECT_TRUE(ConnectTimerRunning());
    EXPECT_TRUE(!IdleHoldTimerRunning());
}

#if 0
class SandeshClientStateMachineEstablishedTest : public SandeshClientStateMachineTest {
    virtual void SetUp() {
        GetToState(scm::CLIENT_INIT);
        VerifyDirection(SandeshSessionMock::ACTIVE);
    }

    virtual void TearDown() {
    }
};

// Old State: Established
// Event:     EvTcpClose
// New State: Idle
TEST_F(SandeshClientStateMachineEstablishedTest, TcpClose) {
    TaskScheduler::GetInstance()->Stop();
    EvTcpClose();
    TcpSession *session = client_->session();
    TaskScheduler::GetInstance()->Start();
    TASK_UTIL_EXPECT_EQ(true, !session->IsEstablished(),
                        "Wait for session to close");
    Endpoint endpoint;
    TASK_UTIL_EXPECT_EQ(false, session->Connected(endpoint),
                        "Wait for session to close");
    VerifyState(scm::ESTABLISHED);
    VerifyDirection(SandeshSessionMock::ACTIVE);
}
#endif

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return result;
}
