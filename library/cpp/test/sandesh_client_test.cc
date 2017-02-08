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

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_session.h>
#include <sandesh/sandesh_client.h>
#include <sandesh/sandesh_statistics.h>
#include "sandesh_client_sm_priv.h"
#include "sandesh_test_common.h"

using namespace std;
using namespace boost::assign;
using namespace boost::posix_time;
using boost::system::error_code;
using boost::asio::ip::address;
using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;

typedef boost::asio::ip::tcp::endpoint Endpoint;

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

    SandeshSessionMock(SslServer *server, State state, Direction direction)
        : SandeshSession(server, NULL, Task::kTaskInstanceAny,
                TaskScheduler::GetInstance()->GetTaskId("sandesh::Test::ClientSM"),
                TaskScheduler::GetInstance()->GetTaskId("io::ReaderTask")),
          state_(state),
          direction_(direction) {
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

    virtual bool Connected(Endpoint remote) {
        state_ = SandeshSessionMock::CLIENT_INIT;
        EventObserver obs = observer();
        if (obs != NULL) {
            obs(this, CONNECT_COMPLETE);
        }
        return true;
    }

    State state() { return state_; }
    Direction direction() { return direction_; }

private:
    State state_;
    Direction direction_;
};

class SandeshClientMock : public SandeshClient {
public:
    SandeshClientMock(EventManager *evm, std::vector<Endpoint> dummy) :
        SandeshClient(evm, dummy, SandeshConfig()),
        session_(NULL),
        old_session_(NULL) {
    }

    ~SandeshClientMock() {
        LOG(DEBUG, "Client delete");
    }

    virtual void Connect(TcpSession *session, Endpoint remote) {
    }

    virtual SandeshSession *CreateSMSession(
            TcpSession::EventObserver eocb,
            SandeshReceiveMsgCb rmcb,
            TcpServer::Endpoint ep) {
        SandeshSession *session(dynamic_cast<SandeshSession *>(
            CreateSession()));
        session->SetReceiveMsgCb(rmcb);
        session->set_observer(eocb);
        return session;
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

class SandeshClientStateMachineTest : public ::testing::Test {
protected:
    SandeshClientStateMachineTest() :
        client_(new SandeshClientMock((&evm_), std::vector<Endpoint>())),
        timer_(TimerManager::CreateTimer(*evm_.io_service(), "Dummy timer")),
        sm_(dynamic_cast<SandeshClientSMImpl *>(client_->state_machine())) {
        task_util::WaitForIdle();
        sm_->set_idle_hold_time(1);
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

    std::vector<Endpoint> GetCollectorEndpoints(
                            const std::vector<string>& collectors) {
        boost::system::error_code ec;
        std::vector<Endpoint> collector_endpoints;
        BOOST_FOREACH(const std::string &collector, collectors) {
            Endpoint ep(address::from_string(collector, ec), 8086);
            collector_endpoints.push_back(ep);
        }
        return collector_endpoints;
    }

    void RunToState(SandeshClientSM::State state) {
        timer_->Start(15000,
                     boost::bind(&SandeshClientStateMachineTest::DummyTimerHandler, this));
        task_util::WaitForIdle();
        for (int count = 0; count < 100 && sm_->get_state() != state; count++) {
            evm_.RunOnce();
            task_util::WaitForIdle();
        }
        timer_->Cancel();
        VerifyState(state);
    }

    void GetToState(SandeshClientSM::State state) {
        switch (state) {
        case SandeshClientSM::IDLE: {
            EvAdminDown();
            VerifyState(state);
            break;
        }
        case SandeshClientSM::DISCONNECT: {
            GetToState(SandeshClientSM::IDLE);
            EvAdminUpNoHoldTime();
            RunToState(state);
            break;
        }
        case SandeshClientSM::CONNECT: {
            GetToState(SandeshClientSM::IDLE);
            EvAdminUp();
            RunToState(state);
            break;
        }
        case SandeshClientSM::CLIENT_INIT: {
            GetToState(SandeshClientSM::CONNECT);
            Endpoint endpoint;
            client_->session()->Connected(endpoint);
            VerifyState(SandeshClientSM::CLIENT_INIT);
            break;
        }
        case SandeshClientSM::ESTABLISHED: {
            GetToState(SandeshClientSM::CLIENT_INIT);
            EvSandeshCtrlMessageRecv();
            RunToState(SandeshClientSM::ESTABLISHED);
            break;
        }
        default: {
            ASSERT_TRUE(false);
            break;
        }
        }
    }

    void VerifyState(SandeshClientSM::State state) {
        task_util::WaitForIdle();
        TaskScheduler::GetInstance()->Stop();

        LOG(DEBUG, "VerifyState " << state);
        EXPECT_EQ(state, sm_->get_state());

        switch (state) {
        case SandeshClientSM::IDLE:
            EXPECT_FALSE(ConnectTimerRunning());
            EXPECT_TRUE(client_->session() == NULL);
            break;
        case SandeshClientSM::DISCONNECT:
            EXPECT_FALSE(ConnectTimerRunning());
            EXPECT_TRUE(client_->session() == NULL);
            break;
        case SandeshClientSM::CONNECT:
            EXPECT_TRUE(ConnectTimerRunning());
            EXPECT_FALSE(IdleHoldTimerRunning());
            EXPECT_TRUE(client_->session() != NULL);
            break;
        case SandeshClientSM::CLIENT_INIT:
            EXPECT_TRUE(ConnectTimerRunning());
            EXPECT_FALSE(IdleHoldTimerRunning());
            EXPECT_TRUE(client_->session() != NULL);
            break;
        case SandeshClientSM::ESTABLISHED:
            EXPECT_FALSE(ConnectTimerRunning());
            EXPECT_FALSE(IdleHoldTimerRunning());
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

    void EvAdminUp() {
        sm_->SetAdminState(false);
        sm_->set_idle_hold_time(1);
    }
    void EvAdminUpNoHoldTime() {
        sm_->SetAdminState(false);
        sm_->set_idle_hold_time(0);
    }
    void EvAdminDown() {
        sm_->SetAdminState(true);
        sm_->set_idle_hold_time(1);
    }
    void EvConnectTimerExpired() {
        sm_->FireConnectTimer();
    }
    void EvTcpConnected() {
        sm_->OnSessionEvent(client_->session(),
            TcpSession::CONNECT_COMPLETE);
    }
    void EvTcpConnectFail() {
        sm_->OnSessionEvent(client_->session(),
            TcpSession::CONNECT_FAILED);
    }
    void EvTcpClose(SandeshSessionMock *session = NULL) {
        if (!session) session = client_->session();
        sm_->OnSessionEvent(session, TcpSession::CLOSE);
    }
    void EvSandeshMessageRecv(SandeshSessionMock *session = NULL) {
        session = GetSession(session);
        uint8_t msg[1024];
        contrail::sandesh::test::CreateFakeMessage(msg, sizeof(msg));
        string xml((const char *)msg, sizeof(msg));
        sm_->OnMessage(session, xml);
    }
    void EvSandeshCtrlMessageRecv(SandeshSessionMock *session = NULL) {
        session = GetSession(session);
        string xml;
        contrail::sandesh::test::CreateFakeCtrlMessage(xml);
        sm_->OnMessage(session, xml);
    }
    void EvCollectorUpdateTrue() {
        if (collector_list_ != collector_list1_) {
            collector_list_ = collector_list1_;
        } else {
            collector_list_ = collector_list2_;
        }
        sm_->SetCollectors(GetCollectorEndpoints(collector_list_));
    }
    void EvCollectorUpdateFalse() {
        if (collector_list_.size() == 0) {
            collector_list_ = collector_list1_;
        }
        sm_->SetCollectors(GetCollectorEndpoints(collector_list_));
    }

    bool IdleHoldTimerRunning() { return sm_->IdleHoldTimerRunning(); }
    bool ConnectTimerRunning() { return sm_->ConnectTimerRunning(); }

    EventManager evm_;
    SandeshClientMock *client_;
    SandeshClientSMImpl *sm_;
    Timer *timer_;
    std::vector<string> collector_list_;

    static const std::vector<string> collector_list1_;
    static const std::vector<string> collector_list2_;
};

const std::vector<string> SandeshClientStateMachineTest::collector_list1_ =
    boost::assign::list_of("1.1.1.1")("2.2.2.2");
const std::vector<string> SandeshClientStateMachineTest::collector_list2_ =
    boost::assign::list_of("3.3.3.3");

typedef boost::function<void(void)> EvGen;
struct EvGenComp {
    bool operator()(const EvGen &lhs, const EvGen &rhs) { return &lhs < &rhs; }
};


TEST_F(SandeshClientStateMachineTest, Matrix) {
    boost::asio::io_service::work work(*evm_.io_service());
    typedef std::map<EvGen, SandeshClientSM::State, EvGenComp> Transitions;

#define CLIENT_SSM_TRANSITION(F, E) \
    ((EvGen) boost::bind(&SandeshClientStateMachineTest_Matrix_Test::F, this), E)
#define CLIENT_SSM_TRANSITION2(F, E) \
    ((EvGen) boost::bind(&SandeshClientStateMachineTest_Matrix_Test::F, this, \
            (SandeshSessionMock *) NULL), E)

    Transitions idle = map_list_of
            CLIENT_SSM_TRANSITION(EvAdminUp, SandeshClientSM::CONNECT)
            CLIENT_SSM_TRANSITION(EvAdminUpNoHoldTime, SandeshClientSM::DISCONNECT)
            CLIENT_SSM_TRANSITION(EvCollectorUpdateTrue, SandeshClientSM::IDLE) // collector_list1_
            CLIENT_SSM_TRANSITION(EvCollectorUpdateFalse, SandeshClientSM::IDLE); // collector_list1_

    Transitions disconnect = map_list_of
            CLIENT_SSM_TRANSITION(EvCollectorUpdateTrue, SandeshClientSM::CONNECT) // collector_list2_
            CLIENT_SSM_TRANSITION(EvCollectorUpdateFalse, SandeshClientSM::CONNECT); // collector_list2_

    Transitions connect = map_list_of
            CLIENT_SSM_TRANSITION(EvAdminDown, SandeshClientSM::IDLE)
            CLIENT_SSM_TRANSITION(EvConnectTimerExpired, SandeshClientSM::IDLE)
            CLIENT_SSM_TRANSITION(EvTcpConnected, SandeshClientSM::CLIENT_INIT)
            CLIENT_SSM_TRANSITION(EvTcpConnectFail, SandeshClientSM::IDLE)
            CLIENT_SSM_TRANSITION2(EvTcpClose, SandeshClientSM::IDLE)
            CLIENT_SSM_TRANSITION(EvCollectorUpdateTrue, SandeshClientSM::IDLE) // collector_list1_
            CLIENT_SSM_TRANSITION(EvCollectorUpdateFalse, SandeshClientSM::CONNECT); // collector_list1_

    Transitions client_init = map_list_of
            CLIENT_SSM_TRANSITION2(EvTcpClose, SandeshClientSM::IDLE)
            CLIENT_SSM_TRANSITION2(EvSandeshMessageRecv, SandeshClientSM::CLIENT_INIT)
            CLIENT_SSM_TRANSITION2(EvSandeshCtrlMessageRecv, SandeshClientSM::ESTABLISHED)
            CLIENT_SSM_TRANSITION(EvCollectorUpdateTrue, SandeshClientSM::IDLE) // collector_list2_
            CLIENT_SSM_TRANSITION(EvCollectorUpdateFalse, SandeshClientSM::CLIENT_INIT); // collector_list2_

    Transitions established = map_list_of
            CLIENT_SSM_TRANSITION2(EvTcpClose, SandeshClientSM::IDLE)
            CLIENT_SSM_TRANSITION2(EvSandeshMessageRecv, SandeshClientSM::ESTABLISHED)
            CLIENT_SSM_TRANSITION(EvCollectorUpdateTrue, SandeshClientSM::CONNECT) // collector_list1_
            CLIENT_SSM_TRANSITION(EvCollectorUpdateFalse, SandeshClientSM::ESTABLISHED) // collector_list1_
            CLIENT_SSM_TRANSITION2(EvTcpClose, SandeshClientSM::CONNECT);

    Transitions matrix[] =
        { idle, disconnect, connect, client_init, established };

    for (int k = SandeshClientSM::IDLE; k <= SandeshClientSM::ESTABLISHED; k++) {
        SandeshClientSM::State i = static_cast<SandeshClientSM::State>(k);
        int count = 0;
        for (Transitions::iterator j = matrix[i].begin();
                j != matrix[i].end(); j++) {
            LOG(DEBUG, "Starting test " << count++ << " in state " << sm_->StateName(i)
                    << " expecting " << sm_->StateName(j->second)<<
                    " *********************");
            GetToState(i);
            j->first();
            RunToState(j->second);
        }
    }
}

class SandeshClientStateMachineConnectTest : public SandeshClientStateMachineTest {
    virtual void SetUp() {
        GetToState(SandeshClientSM::CONNECT);
    }

    virtual void TearDown() {
    }
};

// Old State: Connect
// Event:     EvTcpConnected
// New State: ClientInit
// Timers:    Connect timer must be running again.
// Messages:  None.
TEST_F(SandeshClientStateMachineConnectTest, TcpConnected) {
    EvTcpConnected();
    VerifyState(SandeshClientSM::CLIENT_INIT);
    EXPECT_TRUE(ConnectTimerRunning());
    EXPECT_TRUE(client_->session() != NULL);
}

// Old State: Connect
// Event:     EvConnectTimerExpired
// New State: Idle.
// Timers:    Idle hold timer must be running. Connect timer should not be running.
// Messages:  None.
TEST_F(SandeshClientStateMachineConnectTest, ConnectTimerExpired) {
    EvConnectTimerExpired();
    VerifyState(SandeshClientSM::IDLE);
    EXPECT_TRUE(!ConnectTimerRunning());
    EXPECT_TRUE(IdleHoldTimerRunning());
    EXPECT_TRUE(client_->session() == NULL);
}

// Old State: Connect
// Event:     EvTcpConnectFail
// New State: Idle.
// Timers:    Idle hold timer must be running. Connect timer should not be running.
// Messages:  None.
TEST_F(SandeshClientStateMachineConnectTest, TcpConnectFail) {
    EvTcpConnectFail();
    VerifyState(SandeshClientSM::IDLE);
    EXPECT_TRUE(!ConnectTimerRunning());
    EXPECT_TRUE(IdleHoldTimerRunning());
    EXPECT_TRUE(client_->session() == NULL);
}

// Old State: Connect
// Event:     EvTcpConnected + EvConnectTimerExpired
// New State: Idle
// Messages:  None.
// Other:     The Connect timer has expired before it can be cancelled while
//            processing EvTcpConnected.
TEST_F(SandeshClientStateMachineConnectTest, TcpConnectedThenConnectTimerExpired) {
    TaskScheduler::GetInstance()->Stop();
    EvTcpConnected();
    EvConnectTimerExpired();
    TaskScheduler::GetInstance()->Start();
    VerifyState(SandeshClientSM::IDLE);
    EXPECT_TRUE(client_->session() == NULL);
    EXPECT_TRUE(!ConnectTimerRunning());
    EXPECT_TRUE(IdleHoldTimerRunning());
}

// Old State: Connect
// Event:     EvConnectTimerExpired + EvTcpConnected
// New State: Idle
// Messages:  None.
// Other:     The Connect timer expired before we see EvTcpConnected, thus
//            the session would already have been deleted when we process
//            EvTcpConnected.
TEST_F(SandeshClientStateMachineConnectTest, ConnectTimerExpiredThenTcpConnected) {
    TaskScheduler::GetInstance()->Stop();
    EvConnectTimerExpired();
    EvTcpConnected();
    TaskScheduler::GetInstance()->Start();
    VerifyState(SandeshClientSM::IDLE);
    EXPECT_TRUE(client_->session() == NULL);
    EXPECT_TRUE(!ConnectTimerRunning());
    EXPECT_TRUE(IdleHoldTimerRunning());
}

class SandeshClientStateMachineEstablishedTest : public SandeshClientStateMachineTest {
    virtual void SetUp() {
        GetToState(SandeshClientSM::CLIENT_INIT);
        VerifyDirection(SandeshSessionMock::ACTIVE);
    }

    virtual void TearDown() {
    }
};

// Old State: Established
// Event:     EvTcpClose
// New State: Idle
TEST_F(SandeshClientStateMachineEstablishedTest, TcpClose) {
    EvTcpClose();
    VerifyState(SandeshClientSM::IDLE);
    EXPECT_TRUE(sm_->session() == NULL);
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return result;
}
