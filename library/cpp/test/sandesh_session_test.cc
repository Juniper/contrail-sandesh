/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_session_test.cc
//
// Sandesh Session Test
//

#include "testing/gunit.h"

#include <boost/bind.hpp>

#include <io/event_manager.h>
#include <base/logging.h>
#include "base/test/task_test_util.h"

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_server.h>
#include <sandesh/sandesh_session.h>

using namespace std;

using boost::asio::mutable_buffer;
using boost::asio::buffer_cast;
using boost::asio::ip::address;
using boost::asio::ip::tcp;

enum SendAction {
    NO_SEND = 1,
    PARTIAL_SEND = 2,
    SEND = 3,
};

SendAction send_action = SEND; 

class SandeshSessionTest : public SandeshSession {
public:
    SandeshSessionTest(TcpServer *client, Socket *socket)
        : SandeshSession(client, socket, Task::kTaskInstanceAny,
                TaskScheduler::GetInstance()->GetTaskId("sandesh::SandeshSessionTest")),
          release_count_(0) {
        SetReceiveMsgCb(boost::bind(&SandeshSessionTest::ReceiveMsg, this, _1));
    }

    virtual ~SandeshSessionTest() {
        while (!send_buf_list_.empty()) {
            mutable_buffer buffer = send_buf_list_.back();
            uint8_t *data = buffer_cast<uint8_t *>(buffer);
            delete [] data;
            send_buf_list_.pop_back();
        }
    }

    void Read(Buffer buffer) {
        OnRead(buffer);
    }
    virtual void ReleaseBuffer(Buffer buffer) {
        release_count_++;
    }
    int release_count() const { return release_count_; }

    vector<int>::const_iterator begin() const {
        return sizes.begin();
    }
    vector<int>::const_iterator end() const {
        return sizes.end();
    }

    void SendMessage(uint8_t *data,
                     size_t size, bool more) {
        uint8_t *buffer;
        boost::shared_ptr<TMemoryBuffer> buf(new TMemoryBuffer(size));
        buffer = buf->getWritePtr(size);
        memcpy(buffer, data, size);
        buf->wroteBytes(size);

        if (more) {
            SandeshSession::writer()->SendMsgMore(buf);
        } else {
            SandeshSession::writer()->SendMsgAll(buf);
        }
    }

    int send_count() const { return send_buf_list_.size() ; }
    void send_buf(int index, uint8_t **buf, size_t *len) {
        ASSERT_LE(index, send_buf_list_.size());
        *buf = boost::asio::buffer_cast<uint8_t *>(send_buf_list_[index]);
        *len = buffer_size(send_buf_list_[index]);
    }
    
protected:
    virtual void OnRead(Buffer buffer) {
        reader_->OnRead(buffer);
    }

private:
    void ReceiveMsg(const std::string& msg) {
        // Add sandesh open and close envelope lengths
        size_t size = msg.size() + SandeshWriter::sandesh_open_.size() +
                SandeshWriter::sandesh_close_.size();
        LOG(DEBUG, "ReceiveMsg: " << size << " bytes");
        sizes.push_back(size);
    }

    virtual bool Send(const u_int8_t *data, size_t size, size_t *sent) {
        bool ret = true;

        if (NO_SEND == send_action) {
            *sent = 0;
            return false;
        } else if (PARTIAL_SEND == send_action) {
            *sent = size/2;
            ret = false;
        } else if (SEND == send_action) {
            *sent = size;
        }

        LOG(DEBUG, "Send: [" << size << "] bytes");
        uint8_t *msg = new uint8_t[size];
        memcpy(msg, data, size);
        send_buf_list_.push_back(boost::asio::mutable_buffer((void *)msg, size));
        return ret;
    }

    vector<int> sizes;
    int release_count_;

    vector<mutable_buffer> send_buf_list_;
};

class SandeshTest : public Sandesh {
    virtual void Release() {}
    virtual int32_t Read(
        boost::shared_ptr<contrail::sandesh::protocol::TProtocol> iprot) { 
        return 0;
    }
    virtual int32_t Write(
        boost::shared_ptr<contrail::sandesh::protocol::TProtocol> oprot) const {
        return 0;
    }
    virtual void HandleRequest() const { }
};

class SandeshServerTest : public SandeshServer {
public:
    SandeshServerTest(EventManager *evm):
            SandeshServer(evm) {
    }

    virtual ~SandeshServerTest() {
    }

    virtual TcpSession *AllocSession(Socket *socket) {
        SandeshSessionTest *session = new SandeshSessionTest(this, socket);
        return session;
    }

    virtual bool ReceiveSandeshMsg(SandeshSession *session,
                           const string& cmsg, const string& message_type,
                           const SandeshHeader& header, uint32_t xml_offset, bool rsc) {
        return true;
    }

    virtual bool ReceiveMsg(SandeshSession *session, ssm::Message *msg) {
        return true;
    }
};

class SandeshReaderUnitTest : public ::testing::Test {
protected:
    SandeshReaderUnitTest() :
        server_(new SandeshServerTest(&evm_)) {
        session_ = dynamic_cast<SandeshSessionTest *>(server_->CreateSession());
    }

    virtual void SetUp() {
        server_->Initialize(0);
    }

    virtual void TearDown() {
        session_->set_observer(NULL);
        session_->Close();
        server_->DeleteSession(session_);
        server_->Shutdown();
        task_util::WaitForIdle();
        TcpServerManager::DeleteServer(server_);
        task_util::WaitForIdle();
    }

    SandeshServerTest *server_;
    SandeshSessionTest *session_;
    EventManager evm_;
};

class SandeshSendMsgUnitTest : public ::testing::Test {
protected:
    SandeshSendMsgUnitTest() :
        server_(new SandeshServerTest(&evm_)) {
        session_ = dynamic_cast<SandeshSessionTest *>(server_->CreateSession());
    }

    virtual void SetUp() {
        server_->Initialize(0);
    }

    virtual void TearDown() {
        session_->set_observer(NULL);
        session_->Close();
        server_->DeleteSession(session_);
        server_->Shutdown();
        task_util::WaitForIdle();
        TcpServerManager::DeleteServer(server_);
        task_util::WaitForIdle();
    }

    virtual size_t send_buf_offset() {
        return session_->writer()->send_buf_offset();
    }

    SandeshServerTest *server_;
    SandeshSessionTest *session_;
    EventManager evm_;
};

const std::string FakeMessageBegin = "<sandesh length=\"";
const std::string FakeMessageEnd = "</sandesh>";

static void CreateFakeMessage(uint8_t *data, size_t length) {
    std::stringstream ss;
    size_t offset = 0;
    char prev = ss.fill('0');
    ss.width(10);
    ss << length;
    ss.fill(prev);
    memcpy(data + offset, FakeMessageBegin.c_str(), FakeMessageBegin.size());
    offset += FakeMessageBegin.size();
    memcpy(data + offset, ss.str().c_str(), ss.str().size());
    offset += ss.str().size();
    memcpy(data + offset, "\">", 2);
    offset += 2;
    memset(data + offset, '0', length - offset - FakeMessageEnd.size());
    offset += length - offset - FakeMessageEnd.size();
    memcpy(data + offset, FakeMessageEnd.c_str(), FakeMessageEnd.size());
    offset += FakeMessageEnd.size();
    EXPECT_EQ(offset, length);
}

#define ARRAYLEN(_Array)    sizeof(_Array) / sizeof(_Array[0])

TEST_F(SandeshReaderUnitTest, StreamRead) {
    uint8_t stream[4096];
    int sizes[] = { 100, 400, 80, 110, 70, 80, 90, 100 };
    uint8_t *data = stream;
    for (size_t i = 0; i < ARRAYLEN(sizes); i++) {
        CreateFakeMessage(data, sizes[i]);
        data += sizes[i];
    }
    vector<mutable_buffer> buf_list;
    int segments[] = {
        100 + 60,       // complete msg + start (with header)
        200,            // mid part
        140 + 80 + 10,  // end + start full msg + start (no header)
        7,              // still no header
        50,             // header but no end
        43,             // end of message.
        71,             // complete msg + '<' of next msg
        79,             // header + msg
        90,
        100,
    };
    data = stream;
    for (size_t i = 0; i < ARRAYLEN(segments); i++) {
        buf_list.push_back(mutable_buffer(data, segments[i]));
        data += segments[i];
    }
    for (size_t i = 0; i < buf_list.size(); i++) {
        session_->Read(buf_list[i]);
    }

    int i = 0;
    for (vector<int>::const_iterator iter = session_->begin();
         iter != session_->end(); ++iter) {
        EXPECT_EQ(sizes[i], *iter);
        i++;
    }
    EXPECT_EQ(ARRAYLEN(sizes), i);
    EXPECT_EQ(buf_list.size(), session_->release_count());
}

typedef struct SendMsgInfo_ {
    size_t    msg_size;
    uint8_t   *buf;
    bool      action;
} SendMsgInfo;

typedef struct SendMsgExpectedAtEachIt_ {
    int       send_cnt;
    size_t    buf_offset;
} SendMsgExpectedAtEachIt;

typedef struct SendMsgExpectedAtEnd_ {
    size_t   send_buf_len;
    uint8_t  *send_buf;
} SendMsgExpectedAtEnd;

TEST_F(SandeshSendMsgUnitTest, SendMsgMore) {
    uint32_t max_size = SandeshWriter::kDefaultSendSize;

    SendMsgInfo msg_info[] = { {max_size, NULL, true },
                               {max_size/2, NULL, true },
                               {max_size/4, NULL, true },
                               {(max_size/4)+1,   NULL, true },
                               {(max_size)-1, NULL, true },
                               {(2*max_size)+1, NULL, true },
    };

    SendMsgExpectedAtEachIt exp_at_each_it[] = { {1, 0},
                                                 {1, max_size/2},
                                                 {1, 3*(max_size/4)},
                                                 {3, 0},
                                                 {3, (max_size)-1},
                                                 {5, 0},
    };

    EXPECT_EQ(ARRAYLEN(msg_info), ARRAYLEN(exp_at_each_it));

    // Create Fake sandesh message
    for (size_t i = 0; i < ARRAYLEN(msg_info); i++) {
        msg_info[i].buf = new uint8_t[msg_info[i].msg_size];
        CreateFakeMessage(msg_info[i].buf, msg_info[i].msg_size);
    }

    int send_cnt = exp_at_each_it[ARRAYLEN(msg_info) - 1].send_cnt;
    SendMsgExpectedAtEnd exp_at_end[send_cnt];
    
    // Fill send_buf and send_buf_len expected @ the end of the test
    exp_at_end[0].send_buf = new uint8_t[max_size];
    memcpy(exp_at_end[0].send_buf, msg_info[0].buf, msg_info[0].msg_size);
    exp_at_end[0].send_buf_len = max_size;

    exp_at_end[1].send_buf = new uint8_t[3*(max_size/4)];
    // copy msg_info[1].buf => max_size/2 bytes
    memcpy(exp_at_end[1].send_buf, msg_info[1].buf, msg_info[1].msg_size); 
    // copy msg_info[2].buf => max_size/4 bytes
    memcpy(exp_at_end[1].send_buf + msg_info[1].msg_size, msg_info[2].buf, 
           msg_info[2].msg_size);
    exp_at_end[1].send_buf_len = 3*(max_size/4);

    exp_at_end[2].send_buf = new uint8_t[(max_size/4)+1];
    // copy max_size/4 + 1 bytes in msg_info[3].buf => total -> max_size/4 + 1 bytes
    memcpy(exp_at_end[2].send_buf, msg_info[3].buf, (max_size/4)+1);
    exp_at_end[2].send_buf_len = (max_size/4)+1;

    exp_at_end[3].send_buf = new uint8_t[max_size-1];
    // copy msg_info[4].buf => (max_size-1) bytes
    memcpy(exp_at_end[3].send_buf, msg_info[4].buf, msg_info[4].msg_size);
    exp_at_end[3].send_buf_len = max_size-1;

    exp_at_end[4].send_buf = new uint8_t[(2*max_size)+1];
    // copy msg_info[5].buf => (2*max_size)+1 bytes
    memcpy(exp_at_end[4].send_buf, msg_info[5].buf, msg_info[5].msg_size);
    exp_at_end[4].send_buf_len = (2*max_size)+1;

    for (size_t i = 0; i < ARRAYLEN(msg_info); i++) {
        session_->SendMessage(msg_info[i].buf,
                             msg_info[i].msg_size, true);
        delete [] msg_info[i].buf;
        EXPECT_EQ(exp_at_each_it[i].send_cnt, session_->send_count());
        EXPECT_EQ(exp_at_each_it[i].buf_offset, send_buf_offset());
    }

    for (int i = 0; i < session_->send_count(); i++) {
        uint8_t *send_buf = NULL;
        size_t buf_len;
        session_->send_buf(i, &send_buf, &buf_len);
        EXPECT_EQ(exp_at_end[i].send_buf_len, buf_len);
        EXPECT_EQ(0, memcmp(exp_at_end[i].send_buf, send_buf, buf_len));
        delete [] exp_at_end[i].send_buf;
    }
}

TEST_F(SandeshSendMsgUnitTest, SendMsgAll) {
    uint32_t max_size = SandeshWriter::kDefaultSendSize;
    
    SendMsgInfo msg_info[] = { {max_size, NULL, true},
                               {max_size/2, NULL, false}, // store this msg in send_buf_
                               {max_size/2, NULL, true},  // send send_buf_ + this msg (max_size)
                               {max_size/4, NULL, false}, // store this msg in send_buf_
                               {max_size/4, NULL, true},  // send send_buf_ + this msg (max_Size/2)
                               {max_size/2, NULL, false}, // store this msg in send_buf_
                               {3*(max_size/4), NULL, true}   // send msg in send_buf_ followed by this msg
    };
   
    SendMsgExpectedAtEachIt exp_at_each_it[] = { {1, 0},
                                                 {1, max_size/2},
                                                 {2, 0},
                                                 {2, max_size/4},
                                                 {3, 0},
                                                 {3, max_size/2},
                                                 {5, 0}
    };

    EXPECT_EQ(ARRAYLEN(msg_info), ARRAYLEN(exp_at_each_it));
    
    // Create Fake sandesh message
    for (size_t i = 0; i < ARRAYLEN(msg_info); i++) {
        ASSERT_LE(msg_info[i].msg_size, max_size);
        msg_info[i].buf = new uint8_t[msg_info[i].msg_size];
        CreateFakeMessage(msg_info[i].buf, msg_info[i].msg_size);
    }
    
    int send_cnt = exp_at_each_it[ARRAYLEN(msg_info) - 1].send_cnt;
    SendMsgExpectedAtEnd exp_at_end[send_cnt];

    exp_at_end[0].send_buf= new uint8_t[max_size];
    memcpy(exp_at_end[0].send_buf, msg_info[0].buf, msg_info[0].msg_size);
    exp_at_end[0].send_buf_len = max_size;

    exp_at_end[1].send_buf = new uint8_t[max_size];
    // copy msg_info[1].buf => max_size/2 bytes
    memcpy(exp_at_end[1].send_buf, msg_info[1].buf, msg_info[1].msg_size);
    // copy msg_info[2].buf => max_size/2 bytes
    memcpy(exp_at_end[1].send_buf + msg_info[1].msg_size, 
           msg_info[2].buf, msg_info[2].msg_size);
    exp_at_end[1].send_buf_len = max_size;

    exp_at_end[2].send_buf = new uint8_t[max_size];
    // copy msg_info[3].buf => max_size/4 bytes
    memcpy(exp_at_end[2].send_buf, msg_info[3].buf, msg_info[3].msg_size);
    // copy msg_info[4].buf => max_size/4 bytes
    memcpy(exp_at_end[2].send_buf + msg_info[3].msg_size, 
           msg_info[4].buf, msg_info[4].msg_size);
    exp_at_end[2].send_buf_len = max_size/2;

    exp_at_end[3].send_buf = new uint8_t[max_size];
    // copy msg_info[5].buf=> max_size/2 bytes
    memcpy(exp_at_end[3].send_buf, msg_info[5].buf, msg_info[5].msg_size);
    exp_at_end[3].send_buf_len = max_size/2;

    exp_at_end[4].send_buf = new uint8_t[max_size];
    memcpy(exp_at_end[4].send_buf, msg_info[6].buf, msg_info[6].msg_size);
    exp_at_end[4].send_buf_len = 3*(max_size/4);
    
    for (size_t i = 0; i < ARRAYLEN(msg_info); i++) {
        if (true == msg_info[i].action) {
            session_->SendMessage(msg_info[i].buf,
                                 msg_info[i].msg_size, false);
        } else {
            session_->SendMessage(msg_info[i].buf,
                                 msg_info[i].msg_size, true);
        }
        delete [] msg_info[i].buf;
        EXPECT_EQ(exp_at_each_it[i].send_cnt, session_->send_count());
        EXPECT_EQ(exp_at_each_it[i].buf_offset, send_buf_offset());
    }

    for (int i = 0; i < session_->send_count(); i++) {
        uint8_t *send_buf = NULL;
        size_t buf_len;
        session_->send_buf(i, &send_buf, &buf_len);
        EXPECT_EQ(exp_at_end[i].send_buf_len, buf_len);
        EXPECT_EQ(0, memcmp(exp_at_end[i].send_buf, send_buf, buf_len));
        delete [] exp_at_end[i].send_buf;
    }
}

TEST_F(SandeshSendMsgUnitTest, AsyncSend) {
    SendMsgInfo msg_info[] = { {1234}, 
                               {3456},
                               {90},
                               {3456}
    };

    for (size_t i = 0; i < ARRAYLEN(msg_info); i++) {
        ASSERT_LE(msg_info[i].msg_size, 4096);
        msg_info[i].buf = new uint8_t[msg_info[i].msg_size];
        CreateFakeMessage(msg_info[i].buf, msg_info[i].msg_size);
    }

    boost::system::error_code ec;
    int cnt = 0;
    send_action = NO_SEND;
    session_->SendMessage(msg_info[cnt].buf,
                         msg_info[cnt].msg_size, false);
    EXPECT_EQ(0, session_->send_count());
    EXPECT_EQ(0, send_buf_offset());
    EXPECT_EQ(1, session_->writer()->WaitMsgQSize());
    EXPECT_EQ(0, session_->writer()->SendReady());

    cnt++; // 1
    session_->SendMessage(msg_info[cnt].buf,
                         msg_info[cnt].msg_size, false);
    EXPECT_EQ(0, session_->send_count());
    EXPECT_EQ(0, send_buf_offset());
    EXPECT_EQ(2, session_->writer()->WaitMsgQSize());
    EXPECT_EQ(0, session_->writer()->SendReady());

    send_action = PARTIAL_SEND;
    session_->WriteReady(ec);
    EXPECT_EQ(1, session_->send_count());
    EXPECT_EQ(1, session_->writer()->WaitMsgQSize());
    EXPECT_EQ(0, session_->writer()->SendReady());

    session_->WriteReady(ec);
    EXPECT_EQ(2, session_->send_count());
    EXPECT_EQ(0, session_->writer()->WaitMsgQSize());
    EXPECT_EQ(0, session_->writer()->SendReady());

    session_->WriteReady(ec);
    EXPECT_EQ(2, session_->send_count());
    EXPECT_EQ(0, session_->writer()->WaitMsgQSize());
    EXPECT_EQ(1, session_->writer()->SendReady());

    cnt++; // 2
    session_->SendMessage(msg_info[cnt].buf,
                         msg_info[cnt].msg_size, false);
    EXPECT_EQ(3, session_->send_count());
    EXPECT_EQ(0, send_buf_offset());
    EXPECT_EQ(0, session_->writer()->WaitMsgQSize());
    EXPECT_EQ(0, session_->writer()->SendReady());

    cnt++; // 3
    session_->SendMessage(msg_info[cnt].buf,
                         msg_info[cnt].msg_size, false);
    EXPECT_EQ(3, session_->send_count());
    EXPECT_EQ(0, send_buf_offset());
    EXPECT_EQ(1, session_->writer()->WaitMsgQSize());
    EXPECT_EQ(0, session_->writer()->SendReady());

    send_action = SEND;
    session_->WriteReady(ec);
    EXPECT_EQ(4, session_->send_count());
    EXPECT_EQ(0, send_buf_offset());
    EXPECT_EQ(0, session_->writer()->WaitMsgQSize());
    EXPECT_EQ(1, session_->writer()->SendReady());

    for (int i = 0; i < session_->send_count(); i++) {
        uint8_t *send_buf = NULL;
        size_t buf_len;
        session_->send_buf(i, &send_buf, &buf_len);
        EXPECT_EQ(msg_info[i].msg_size, buf_len);
        EXPECT_EQ(0, memcmp(msg_info[i].buf, send_buf, buf_len));
        delete [] msg_info[i].buf;
    }
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return success;
}
