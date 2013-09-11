/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_message_test.cc
//
// Sandesh Message Test - Async, Request, Response, Trace
//

#include "testing/gunit.h"

#include <boost/bind.hpp>

#include "base/logging.h"
#include "base/test/task_test_util.h"
#include "io/tcp_session.h"
#include "io/tcp_server.h"
#include "io/event_manager.h"
#include "io/test/event_manager_test.h"

#include <sandesh/protocol/TXMLProtocol.h>
#include <sandesh/transport/TBufferTransports.h>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh_constants.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_session.h>
#include <sandesh/sandesh_http.h>
#include <sandesh/sandesh_ctrl_types.h>
#include "sandesh_client.h"
#include "sandesh_server.h"
#include "sandesh_message_test_types.h"
#include "sandesh_buffer_test_types.h"

using std::string;
using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;
using boost::asio::ip::address;
using boost::asio::ip::tcp;

bool asyncserver_done = false;
bool requestserver_done = false;

static const int32_t test_i32 = 0xdeadbeef;
static const std::string xmldata("<sandesh_types><type1>\"systemlog\"</type1><type2>\'objectlog\'</type2><type3>uve & trace</type3></sandesh_types>");
void SandeshRequestTest1::HandleRequest() const {
    EXPECT_STREQ(this->Name(), "SandeshRequestTest1");
    EXPECT_EQ(this->i32Elem, test_i32);
    EXPECT_EQ(type(), SandeshType::REQUEST);
    EXPECT_EQ(this->xmldata, xmldata);
    requestserver_done = true;
}

namespace {

class SandeshServerTest : public SandeshServer {
public:
    typedef boost::function<bool(SandeshSession *session,
            const string&, const string&,
            const SandeshHeader&, uint32_t)> ReceiveMsgCb;

    SandeshServerTest(EventManager *evm, ReceiveMsgCb cb) :
            SandeshServer(evm),
            cb_(cb) {
    }

    virtual ~SandeshServerTest() {
    }

    virtual bool ReceiveSandeshMsg(SandeshSession *session,
                           const string& cmsg, const string& message_type,
                           const SandeshHeader& header, uint32_t xml_offset) {
        return cb_(session, cmsg, message_type, header, xml_offset);
    }

    virtual bool ReceiveMsg(SandeshSession *session, ssm::Message *msg) {
        return true;
    }

private:
    ReceiveMsgCb cb_;
};

class SandeshAsyncTest : public ::testing::Test {
protected:
    SandeshAsyncTest() {
    }

    virtual void SetUp() {
        msg_num_ = 0;
        evm_.reset(new EventManager());
        server_ = new SandeshServerTest(evm_.get(),
                boost::bind(&SandeshAsyncTest::ReceiveSandeshMsg, this, _1, _2, _3, _4, _5));
        thread_.reset(new ServerThread(evm_.get()));
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
            const string& msg, const string& sandesh_name,
            const SandeshHeader& header, uint32_t header_offset) {

        if (header.get_Type() == SandeshType::UVE) {
            return true;
        } 

        switch (msg_num_++) {
        case 0:
        {
            EXPECT_STREQ("SandeshAsyncTest-Client", header.get_Module().c_str());
            EXPECT_STREQ("localhost", header.get_Source().c_str());
            EXPECT_EQ(1, header.get_SequenceNum());
            EXPECT_EQ(SystemLogTest::sversionsig(), header.get_VersionSig());
            EXPECT_EQ(SandeshType::SYSTEM, header.get_Type());
            EXPECT_EQ(0, header.get_Hints());
            EXPECT_EQ(SandeshLevel::SYS_INFO, header.get_Level());
            EXPECT_EQ("SystemLogTest", header.get_Category());
            const char *expect = "<SystemLogTest type=\"sandesh\"><str1 type=\"string\" identifier=\"1\">Const static string is</str1><f2 type=\"i32\" identifier=\"2\">100</f2><f3 type=\"string\" identifier=\"3\">sat1string100</f3><file type=\"string\" identifier=\"-32768\"></file><line type=\"i32\" identifier=\"-32767\">0</line></SystemLogTest>";
            EXPECT_STREQ(expect, msg.c_str() + header_offset);
            break;
        }
        case 1:
        {
            EXPECT_STREQ("SandeshAsyncTest-Client", header.get_Module().c_str());
            EXPECT_STREQ("localhost", header.get_Source().c_str());
            EXPECT_EQ(2, header.get_SequenceNum());
            EXPECT_EQ(SystemLogTest::sversionsig(), header.get_VersionSig());
            EXPECT_EQ(SandeshType::SYSTEM, header.get_Type());
            EXPECT_EQ(0, header.get_Hints());
            EXPECT_EQ(SandeshLevel::SYS_DEBUG, header.get_Level());
            EXPECT_EQ("SystemLogTest", header.get_Category());
            const char *expect = "<SystemLogTest type=\"sandesh\"><str1 type=\"string\" identifier=\"1\">Const static string is</str1><f2 type=\"i32\" identifier=\"2\">101</f2><f3 type=\"string\" identifier=\"3\">&lt;&apos;sat1string101&apos;&gt;</f3><file type=\"string\" identifier=\"-32768\"></file><line type=\"i32\" identifier=\"-32767\">0</line></SystemLogTest>";
            EXPECT_STREQ(expect, msg.c_str() + header_offset);
            break;
        }
        case 2:
        {
            EXPECT_STREQ("SandeshAsyncTest-Client", header.get_Module().c_str());
            EXPECT_STREQ("localhost", header.get_Source().c_str());
            EXPECT_EQ(3, header.get_SequenceNum());
            EXPECT_EQ(SystemLogTest::sversionsig(), header.get_VersionSig());
            EXPECT_EQ(SandeshType::SYSTEM, header.get_Type());
            EXPECT_EQ(0, header.get_Hints());
            EXPECT_EQ(SandeshLevel::SYS_CRIT, header.get_Level());
            EXPECT_EQ("SystemLogTest", header.get_Category());
            const char *expect = "<SystemLogTest type=\"sandesh\"><str1 type=\"string\" identifier=\"1\">Const static string is</str1><f2 type=\"i32\" identifier=\"2\">102</f2><f3 type=\"string\" identifier=\"3\">sat1string102&amp;</f3><file type=\"string\" identifier=\"-32768\">Test</file><line type=\"i32\" identifier=\"-32767\">1</line></SystemLogTest>";
            EXPECT_STREQ(expect, msg.c_str() + header_offset);
            break;
        }
        case 3:
        {
            EXPECT_STREQ("SandeshAsyncTest-Client", header.get_Module().c_str());
            EXPECT_STREQ("localhost", header.get_Source().c_str());
            EXPECT_EQ(1, header.get_SequenceNum());
            EXPECT_EQ(ObjectLogTest::sversionsig(), header.get_VersionSig());
            EXPECT_EQ(SandeshType::OBJECT, header.get_Type());
            EXPECT_EQ(0, header.get_Hints());
            EXPECT_EQ(SandeshLevel::SYS_INFO, header.get_Level());
            EXPECT_EQ("ObjectLogTest", header.get_Category());
            const char *expect = "<ObjectLogTest type=\"sandesh\"><f1 type=\"struct\" identifier=\"1\"><SAT2_struct><f1 type=\"string\" identifier=\"1\">sat2string100</f1><f2 type=\"i32\" identifier=\"2\">100</f2></SAT2_struct></f1><f2 type=\"i32\" identifier=\"2\">100</f2><file type=\"string\" identifier=\"-32768\"></file><line type=\"i32\" identifier=\"-32767\">0</line></ObjectLogTest>";
            EXPECT_STREQ(expect, msg.c_str() + header_offset);
            break;
        }
        case 4:
        {
            EXPECT_STREQ("SandeshAsyncTest-Client", header.get_Module().c_str());
            EXPECT_STREQ("localhost", header.get_Source().c_str());
            EXPECT_EQ(1, header.get_SequenceNum());
            EXPECT_EQ(ObjectLogAnnTest::sversionsig(), header.get_VersionSig());
            EXPECT_EQ(SandeshType::OBJECT, header.get_Type());
            EXPECT_EQ(g_sandesh_constants.SANDESH_KEY_HINT, header.get_Hints());
            EXPECT_EQ(SandeshLevel::SYS_DEBUG, header.get_Level());
            EXPECT_EQ("ObjectLogAnnTest", header.get_Category());
            const char *expect = "<ObjectLogAnnTest type=\"sandesh\"><f1 type=\"i32\" identifier=\"1\" format=\"%i\" key=\"ObjectIPTable\">1</f1><file type=\"string\" identifier=\"-32768\"></file><line type=\"i32\" identifier=\"-32767\">0</line></ObjectLogAnnTest>";
            EXPECT_STREQ(expect, msg.c_str() + header_offset);
            break;
        }
        case 5:
        {
            EXPECT_STREQ("SandeshAsyncTest-Client", header.get_Module().c_str());
            EXPECT_STREQ("localhost", header.get_Source().c_str());
            EXPECT_EQ(1, header.get_SequenceNum());
            EXPECT_EQ(ObjectLogInnerAnnTest::sversionsig(), header.get_VersionSig());
            EXPECT_EQ(SandeshType::OBJECT, header.get_Type());
            EXPECT_EQ(g_sandesh_constants.SANDESH_KEY_HINT, header.get_Hints());
            EXPECT_EQ(SandeshLevel::SYS_INFO, header.get_Level());
            EXPECT_EQ("ObjectLogInnerAnnTest", header.get_Category());
            const char *expect = "<ObjectLogInnerAnnTest type=\"sandesh\"><f1 type=\"struct\" identifier=\"1\"><SAT3_struct><f1 type=\"string\" identifier=\"1\" key=\"ObjectVNTable\">&quot;sat3string1&quot;</f1><f2 type=\"i32\" identifier=\"2\" format=\"%x\">1</f2></SAT3_struct></f1><file type=\"string\" identifier=\"-32768\"></file><line type=\"i32\" identifier=\"-32767\">0</line></ObjectLogInnerAnnTest>";
            EXPECT_STREQ(expect, msg.c_str() + header_offset);
            asyncserver_done = true;
            break;
        }

        default:
            EXPECT_EQ(0, 1);
            break;
        }
        return true;
    }

    uint32_t msg_num_;
    SandeshServerTest *server_;
    std::auto_ptr<ServerThread> thread_;
    std::auto_ptr<EventManager> evm_;
};

TEST_F(SandeshAsyncTest, Async) {
    server_->Initialize(0);
    thread_->Start();       // Must be called after initialization
    int port = server_->GetPort();
    ASSERT_LT(0, port);
    // Connect to the server
    Sandesh::InitGenerator("SandeshAsyncTest-Client", "localhost", evm_.get(),
                           0);
    Sandesh::ConnectToCollector("127.0.0.1", port);
    Sandesh::SetLoggingParams(true, "", "UT_DEBUG");

    TASK_UTIL_EXPECT_TRUE(Sandesh::client()->state() == SandeshClientSM::ESTABLISHED);

    // Set the logging parameters
    Sandesh::SetLoggingParams(true, "SystemLogTest", SandeshLevel::SYS_INFO);
    // Send the Asyncs (systemlog)
    SystemLogTest::Send("SystemLogTest", SandeshLevel::SYS_INFO, "", 0, 100, "sat1string100"); // case 0
    SystemLogTest::Send("SystemLogTest", SandeshLevel::SYS_DEBUG, "", 0, 101, "<'sat1string101'>"); // case 1
    SystemLogTest::Send("SystemLogTest", SandeshLevel::SYS_CRIT, "Test", 1, 102, "sat1string102&"); // case 2

    // Set the logging category
    Sandesh::SetLoggingCategory("ObjectLogTest");
    // Send the Asyncs (objectlog)
    SAT2_struct s1;
    s1.f1 = "sat2string100";
    s1.f2 = 100;
    ObjectLogTest::Send("ObjectLogTest", SandeshLevel::SYS_INFO, "", 0, s1, 100); // case 3

    // Set logging category and level
    Sandesh::SetLoggingCategory("");
    Sandesh::SetLoggingLevel(SandeshLevel::SYS_DEBUG);
    ObjectLogAnnTest::Send("ObjectLogAnnTest", SandeshLevel::SYS_DEBUG, "", 0, 1); // case 4

    SAT3_struct s3;
    s3.f1 = "\"sat3string1\"";
    s3.f2 = 1;
    ObjectLogInnerAnnTest::Send("ObjectLogInnerAnnTest", SandeshLevel::SYS_INFO, "", 0, s3); // case 5

    // Wait server is done receiving msgs
    TASK_UTIL_EXPECT_TRUE(asyncserver_done);

    {
        tbb::mutex::scoped_lock lock(Sandesh::stats_mutex_);

        boost::ptr_map<std::string, SandeshGenStatsElemCollection>::iterator it;
        it = Sandesh::stats_.msgtype_stats_map_.find("SystemLogTest");
        EXPECT_EQ(3, it->second->messages_sent);
        it = Sandesh::stats_.msgtype_stats_map_.find("ObjectLogTest");
        EXPECT_EQ(1, it->second->messages_sent);
        it = Sandesh::stats_.msgtype_stats_map_.find("ObjectLogAnnTest");
        EXPECT_EQ(1, it->second->messages_sent);
        it = Sandesh::stats_.msgtype_stats_map_.find("ObjectLogInnerAnnTest");
        EXPECT_EQ(1, it->second->messages_sent);
    }
}

class SandeshBaseFactoryTest : public ::testing::Test {
protected:
    SandeshBaseFactoryTest() {
    }
};

class SandeshBaseFactoryTestContext : public SandeshContext {
public:
    std::string sandesh_name_;

    SandeshBaseFactoryTestContext() :
        sandesh_name_("") {
    }
};

TEST_F(SandeshBaseFactoryTest, Basic) {
    std::string sandesh_name("SandeshRequestTest1");
    Sandesh *sandesh = SandeshBaseFactory::CreateInstance(sandesh_name);
    std::string sandesh_fake_name("SandeshFakeName");
    Sandesh *fake_sandesh = SandeshBaseFactory::CreateInstance(sandesh_fake_name);
    EXPECT_NE(sandesh, fake_sandesh);
    sandesh->Release();
    // Test for buffer sandesh
    std::string sandesh_buffer_name("BufferTest");
    Sandesh *buffer_sandesh = SandeshBaseFactory::CreateInstance(sandesh_buffer_name);
    EXPECT_NE(buffer_sandesh, fake_sandesh);
    buffer_sandesh->Release();
}

class BufferUpdateTest_Derived: public BufferUpdateTest {
    virtual void Process(SandeshContext *context) {
        SandeshBaseFactoryTestContext *client_context =
                dynamic_cast<SandeshBaseFactoryTestContext *>(context);
        client_context->sandesh_name_ = "BufferUpdateTest_Derived";
    }
};

TEST_F(SandeshBaseFactoryTest, Update) {
    // Create instance and process to confirm that base class object is created
    std::string sandesh_buffer_name("BufferUpdateTest");
    SandeshBuffer *buffer_sandesh =
        dynamic_cast<SandeshBuffer *>(SandeshBaseFactory::CreateInstance(sandesh_buffer_name));
    SandeshBaseFactoryTestContext context;
    buffer_sandesh->Process(&context);
    EXPECT_EQ(context.sandesh_name_, sandesh_buffer_name);
    buffer_sandesh->Release();
    // Create the update map
    SandeshBaseFactory::map_type update_map;
    update_map["BufferUpdateTest"] = &createT<BufferUpdateTest_Derived>;
    // Update
    SandeshBaseFactory::Update(update_map);
    // Create instance and process to confirm that derived class object is created
    buffer_sandesh =
        dynamic_cast<SandeshBuffer *>(SandeshBaseFactory::CreateInstance(sandesh_buffer_name));
    buffer_sandesh->Process(&context);
    EXPECT_EQ(context.sandesh_name_, "BufferUpdateTest_Derived");
    buffer_sandesh->Release();
}

class SandeshRequestTest : public ::testing::Test {
protected:
    SandeshRequestTest() {
    }

    virtual ~SandeshRequestTest() {
    }

    virtual void SetUp() {
        evm_.reset(new EventManager());
        server_ = new SandeshServerTest(evm_.get(),
                boost::bind(&SandeshRequestTest::ReceiveSandeshMsg, this, _1, _2, _3, _4, _5));
        thread_.reset(new ServerThread(evm_.get()));
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
           const string& cmsg, const string& message_type,
           const SandeshHeader& header, uint32_t xml_offset) {
        // Call the client ReceiveMsg
        return Sandesh::client()->ReceiveMsg(cmsg, header, message_type, xml_offset);
    }

    std::auto_ptr<ServerThread> thread_;
    std::auto_ptr<EventManager> evm_;
    SandeshServerTest *server_;
};

TEST_F(SandeshRequestTest, Request) {
    server_->Initialize(0);
    thread_->Start();       // Must be called after initialization
    int port = server_->GetPort();
    ASSERT_LT(0, port);
    // Connect to the server
    Sandesh::InitGenerator("SandeshRequestTest-Client", "localhost",
            evm_.get(), 0, NULL);
    Sandesh::ConnectToCollector("127.0.0.1", port);
    TASK_UTIL_EXPECT_TRUE(Sandesh::client()->state() == SandeshClientSM::ESTABLISHED);
    // Send the request
    std::string context;
    SandeshRequestTest1::Request(xmldata, test_i32, context);
    // Wait server is done receiving msgs
    TASK_UTIL_EXPECT_TRUE(requestserver_done);
}

class SandeshBufferTest : public ::testing::Test {
public:
    static BufferTest GetSandesh() { return test; }

protected:
    SandeshBufferTest() {
    }
    static BufferTest test;
};

BufferTest SandeshBufferTest::test;

class SandeshBufferTestContext : public SandeshContext {
public:
    int num_decoded_;

    SandeshBufferTestContext() :
        num_decoded_(0) {
    }
};

TEST_F(SandeshBufferTest, ReadWrite) {
    u_int8_t buf[512];
    u_int32_t buf_len = sizeof(buf);
    int32_t rxfer, wxfer, wxfer1, error = 0;
    int num_encoded = 0;
    std::vector<int8_t> rwinfo;
    SandeshBufferTestContext context;
    // Populate test
    test.set_i32Elem1(1);
    test.set_stringElem1("test");
    for (int i = 0; i < 5; i++) {
        rwinfo.push_back(i);
    }
    test.set_listElem1(rwinfo);
    test.set_i64Elem1(1);
    // Write test to buf
    wxfer = test.WriteBinary(buf, buf_len, &error);
    num_encoded++;
    EXPECT_GT(wxfer, 0);
    EXPECT_EQ(error, 0);
    // Write test to buf again
    wxfer1 = test.WriteBinary(buf + wxfer, buf_len - wxfer, &error);
    num_encoded++;
    EXPECT_GT(wxfer1, 0);
    EXPECT_EQ(error, 0);
    wxfer += wxfer1;
    // Now read it back and compare the read and write lengths
    rxfer = Sandesh::ReceiveBinaryMsg(buf, wxfer, &error, &context);
    EXPECT_EQ(wxfer, rxfer);
    EXPECT_EQ(error, 0);
    EXPECT_EQ(num_encoded, context.num_decoded_);
}
} // namespace

void SandeshRequestEmptyTest::HandleRequest() const {
}

void BufferTest::Process(SandeshContext *context) {
    EXPECT_EQ(*this, SandeshBufferTest::GetSandesh());
    EXPECT_EQ(type(), SandeshType::BUFFER);
    SandeshBufferTestContext *client_context =
            dynamic_cast<SandeshBufferTestContext *>(context);
    client_context->num_decoded_++;
}

void BufferUpdateTest::Process(SandeshContext *context) {
    SandeshBaseFactoryTestContext *client_context =
            dynamic_cast<SandeshBaseFactoryTestContext *>(context);
    client_context->sandesh_name_ = "BufferUpdateTest";
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    bool success = RUN_ALL_TESTS();
    TaskScheduler::GetInstance()->Terminate();
    return success;
}
