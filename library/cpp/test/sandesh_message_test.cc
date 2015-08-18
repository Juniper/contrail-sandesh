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
#include <boost/assign/list_of.hpp>

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
#include <sandesh/sandesh_uve.h>
#include <sandesh/sandesh_ctrl_types.h>
#include <sandesh/sandesh_uve_types.h>
#include <sandesh/sandesh_message_builder.h>
#include <sandesh/sandesh_statistics.h>
#include <sandesh/sandesh_client.h>
#include "sandesh_message_test_types.h"
#include "sandesh_buffer_test_types.h"
#include "sandesh_test_common.h"

using std::string;
using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;
using boost::asio::ip::address;
using boost::asio::ip::tcp;
using namespace contrail::sandesh::test;

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

class SandeshSendRatelimitTest : public ::testing::Test {
protected:
    SandeshSendRatelimitTest() {
    }

    virtual void SetUp() {
        msg_num_ = 0 ;
        asyncserver_done = false;
        evm_.reset(new EventManager());
        server_ = new SandeshServerTest(evm_.get(),
                boost::bind(&SandeshSendRatelimitTest::ReceiveSandeshMsg, this, _1, _2));
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
            const SandeshMessage *msg) {
        const SandeshHeader &header(msg->GetHeader());
        const SandeshXMLMessage *xmsg =
            dynamic_cast<const SandeshXMLMessage *>(msg);
        EXPECT_TRUE(xmsg != NULL);
        std::string message(xmsg->ExtractMessage());

        if (header.get_Type() == SandeshType::UVE) {
            return true;
        }
        msg_num_++;
        if (msg_num_ > 13) {
            asyncserver_done = true;
        }
        return true;
    }

    uint32_t msg_num_;
    SandeshServerTest *server_;
    std::auto_ptr<ServerThread> thread_;
    std::auto_ptr<EventManager> evm_;
};

class SandeshAsyncTest : public ::testing::Test {
protected:
    SandeshAsyncTest() {
    }

    virtual void SetUp() {
        msg_num_ = 0;
        evm_.reset(new EventManager());
        server_ = new SandeshServerTest(evm_.get(),
                boost::bind(&SandeshAsyncTest::ReceiveSandeshMsg, this, _1, _2));
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
            const SandeshMessage *msg) {
        const SandeshHeader &header(msg->GetHeader());
        const SandeshXMLMessage *xmsg = 
            dynamic_cast<const SandeshXMLMessage *>(msg);
        EXPECT_TRUE(xmsg != NULL);
        std::string message(xmsg->ExtractMessage());
    
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
            const char *expect = "<SystemLogTest type=\"sandesh\"><str1 type=\"string\" identifier=\"1\">Const static string is</str1><f2 type=\"i32\" identifier=\"2\">100</f2><f3 type=\"string\" identifier=\"3\">sat1string100</f3><file type=\"string\" identifier=\"-32768\">Test</file><line type=\"i32\" identifier=\"-32767\">0</line></SystemLogTest>";
            EXPECT_STREQ(expect, message.c_str());
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
            const char *expect = "<SystemLogTest type=\"sandesh\"><str1 type=\"string\" identifier=\"1\">Const static string is</str1><f2 type=\"i32\" identifier=\"2\">101</f2><f3 type=\"string\" identifier=\"3\">&lt;&apos;sat1string101&apos;&gt;</f3><file type=\"string\" identifier=\"-32768\">Test</file><line type=\"i32\" identifier=\"-32767\">0</line></SystemLogTest>";
            EXPECT_STREQ(expect, message.c_str());
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
            EXPECT_STREQ(expect, message.c_str());
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
            const char *expect = "<ObjectLogTest type=\"sandesh\"><f1 type=\"struct\" identifier=\"1\"><SAT2_struct><f1 type=\"string\" identifier=\"1\">sat2string100</f1><f2 type=\"i32\" identifier=\"2\">100</f2></SAT2_struct></f1><f2 type=\"i32\" identifier=\"2\">100</f2><file type=\"string\" identifier=\"-32768\">Test</file><line type=\"i32\" identifier=\"-32767\">0</line></ObjectLogTest>";
            EXPECT_STREQ(expect, message.c_str());
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
            const char *expect = "<ObjectLogAnnTest type=\"sandesh\"><f1 type=\"i32\" identifier=\"1\" format=\"%i\" key=\"ObjectIPTable\">1</f1><file type=\"string\" identifier=\"-32768\">Test</file><line type=\"i32\" identifier=\"-32767\">0</line></ObjectLogAnnTest>";
            EXPECT_STREQ(expect, message.c_str());
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
            const char *expect = "<ObjectLogInnerAnnTest type=\"sandesh\"><f1 type=\"struct\" identifier=\"1\"><SAT3_struct><f1 type=\"string\" identifier=\"1\" key=\"ObjectVNTable\">\"sat3string1\"</f1><f2 type=\"i32\" identifier=\"2\" format=\"%x\">1</f2></SAT3_struct></f1><file type=\"string\" identifier=\"-32768\">Test</file><line type=\"i32\" identifier=\"-32767\">0</line></ObjectLogInnerAnnTest>";
            EXPECT_STREQ(expect, message.c_str());
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
    Sandesh::InitGenerator("SandeshAsyncTest-Client", "localhost", 
                           "Test", "Test", evm_.get(),
                           0);
    EXPECT_FALSE(Sandesh::IsConnectToCollectorEnabled());
    Sandesh::ConnectToCollector("127.0.0.1", port);
    EXPECT_TRUE(Sandesh::IsConnectToCollectorEnabled());
    EXPECT_TRUE(Sandesh::client() != NULL);
    Sandesh::SetLoggingParams(true, "", "UT_DEBUG");

    TASK_UTIL_EXPECT_TRUE(Sandesh::client()->state() == SandeshClientSM::ESTABLISHED);

    // Set the logging parameters
    Sandesh::SetLoggingParams(true, "SystemLogTest", SandeshLevel::SYS_INFO);
    // Send the Asyncs (systemlog)
    SystemLogTest::Send("SystemLogTest", SandeshLevel::SYS_INFO, "Test", 0, 100, "sat1string100"); // case 0
    SystemLogTest::Send("SystemLogTest", SandeshLevel::SYS_DEBUG, "Test", 0, 101, "<'sat1string101'>"); // case 1
    SystemLogTest::Send("SystemLogTest", SandeshLevel::SYS_CRIT, "Test", 1, 102, "sat1string102&"); // case 2

    // Set the logging category
    Sandesh::SetLoggingCategory("ObjectLogTest");
    // Send the Asyncs (objectlog)
    SAT2_struct s1;
    s1.f1 = "sat2string100";
    s1.f2 = 100;
    ObjectLogTest::Send("ObjectLogTest", SandeshLevel::SYS_INFO, "Test", 0, s1, 100); // case 3

    // Set logging category and level
    Sandesh::SetLoggingCategory("");
    Sandesh::SetLoggingLevel(SandeshLevel::SYS_DEBUG);
    ObjectLogAnnTest::Send("ObjectLogAnnTest", SandeshLevel::SYS_DEBUG, "Test", 0, 1); // case 4

    SAT3_struct s3;
    s3.f1 = "\"sat3string1\"";
    s3.f2 = 1;
    ObjectLogInnerAnnTest::Send("ObjectLogInnerAnnTest", SandeshLevel::SYS_INFO, "Test", 0, s3); // case 5

    // Wait server is done receiving msgs
    TASK_UTIL_EXPECT_TRUE(asyncserver_done);

    {
        boost::ptr_map<std::string, SandeshMessageTypeStats> type_stats;
        SandeshMessageStats agg_stats;
        Sandesh::GetMsgStats(&type_stats, &agg_stats);
        boost::ptr_map<std::string, SandeshMessageTypeStats>::iterator it;
        it = type_stats.find("SystemLogTest");
        EXPECT_EQ(3, it->second->stats.messages_sent);
        it = type_stats.find("ObjectLogTest");
        EXPECT_EQ(1, it->second->stats.messages_sent);
        it = type_stats.find("ObjectLogAnnTest");
        EXPECT_EQ(1, it->second->stats.messages_sent);
        it = type_stats.find("ObjectLogInnerAnnTest");
        EXPECT_EQ(1, it->second->stats.messages_sent);
    }
}

TEST_F(SandeshSendRatelimitTest, Buffer) {
    server_->Initialize(0);
    thread_->Start();       // Must be called after initialization
    int port = server_->GetPort();
    ASSERT_LT(0, port);
    // Connect to the server
    Sandesh::set_send_rate_limit(10);
    Sandesh::InitGenerator("SandeshSendRatelimitTest-Client", "localhost",
                           "Test", "Test", evm_.get(),0);

    Sandesh::ConnectToCollector("127.0.0.1", port);
    EXPECT_TRUE(Sandesh::IsConnectToCollectorEnabled());
    EXPECT_TRUE(Sandesh::client() != NULL);
    TASK_UTIL_EXPECT_TRUE(Sandesh::client()->state() == SandeshClientSM::ESTABLISHED);
    Sandesh::SetLoggingParams(true, "SystemLogTest", SandeshLevel::SYS_INFO);
    boost::ptr_map<std::string, SandeshMessageTypeStats> type_stats;
    SandeshMessageStats agg_stats;
    Sandesh::GetMsgStats(&type_stats, &agg_stats);
    boost::ptr_map<std::string, SandeshMessageTypeStats>::iterator it;
    it = type_stats.find("SystemLogTest");
    //Initialize dropped rate to 0
    it->second->stats.messages_sent_dropped_rate_limited = 0;
    /*
     * First 10 out of the 15 msgs will be recived.messages 15-20 should albe received
     */
    for (int cnt = 0; cnt < 20; cnt++) {
        SystemLogTest::Send("SystemLogTest", SandeshLevel::SYS_INFO, "Test", cnt, 100, "sat1string100");
        if (cnt == 15) {
            sleep(1);
        }
    }
    //Allow all messages to be recieved
    sleep(1);
    Sandesh::GetMsgStats(&type_stats, &agg_stats);
    it = type_stats.find("SystemLogTest");
    TASK_UTIL_EXPECT_TRUE(msg_num_ == 20 -
        it->second->stats.messages_sent_dropped_rate_limited);
}

class SandeshUVEAlarmTest : public ::testing::Test {
protected:
    SandeshUVEAlarmTest() {
    }

    virtual void SetUp() {
        msg_num_ = 0;
        evm_.reset(new EventManager());
        server_ = new SandeshServerTest(evm_.get(),
            boost::bind(&SandeshUVEAlarmTest::ReceiveSandeshMsg, this, _1, _2));
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

    bool ReceiveSandeshMsg(SandeshSession* session,
                           const SandeshMessage* msg) {
        std::vector<std::string> message_types =
            boost::assign::list_of("SandeshUVETest")("SandeshAlarmTest");
        const SandeshHeader &header(msg->GetHeader());
        const SandeshXMLMessage *xmsg =
            dynamic_cast<const SandeshXMLMessage *>(msg);
        EXPECT_TRUE(xmsg != NULL);
        std::string message(xmsg->ExtractMessage());

        // Ignore UVEs sent by the sandesh library
        if (std::find(message_types.begin(), message_types.end(),
                      msg->GetMessageType()) == message_types.end()) {
            return true;
        }

        EXPECT_STREQ("SandeshUVEAlarmTest-Client", header.get_Module().c_str());
        EXPECT_STREQ("localhost", header.get_Source().c_str());
        EXPECT_NE(0, header.get_Hints() & g_sandesh_constants.SANDESH_KEY_HINT);

        switch(msg_num_++) {
            case 0:
            {
                EXPECT_EQ(1, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshUVETest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshUVEData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name></SandeshUVEData></data></SandeshUVETest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 1:
            {
                EXPECT_EQ(2, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshUVETest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshUVEData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name><x type=\"i32\" identifier=\"3\">55</x></SandeshUVEData></data></SandeshUVETest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 2:
            {
                EXPECT_EQ(3, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshUVETest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshUVEData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve2</name><x type=\"i32\" identifier=\"3\">1</x></SandeshUVEData></data></SandeshUVETest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 3:
            {
                EXPECT_EQ(4, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshUVETest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshUVEData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve2</name><deleted type=\"bool\" identifier=\"2\">true</deleted></SandeshUVEData></data></SandeshUVETest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 4:
            {
                EXPECT_EQ(5, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshUVETest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshUVEData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve2</name></SandeshUVEData></data></SandeshUVETest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 5:
            {
                EXPECT_EQ(1, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm1</name><description type=\"string\" identifier=\"3\">alarm1 generated</description></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 6:
            {
                EXPECT_EQ(2, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm1</name><acknowledged type=\"bool\" identifier=\"4\">true</acknowledged></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 7:
            {
                EXPECT_EQ(3, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm2</name><description type=\"string\" identifier=\"3\">alarm2 generated</description></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 8:
            {
                EXPECT_EQ(4, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm2</name><deleted type=\"bool\" identifier=\"2\">true</deleted></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 9:
            {
                EXPECT_EQ(5, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm2</name></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 10:
            {
                EXPECT_EQ(2, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm1</name><description type=\"string\" identifier=\"3\">alarm1 generated</description><acknowledged type=\"bool\" identifier=\"4\">true</acknowledged></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 11:
            {
                EXPECT_EQ(5, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm2</name></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 12:
            {
                EXPECT_EQ(2, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshUVETest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshUVEData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name><x type=\"i32\" identifier=\"3\">55</x></SandeshUVEData></data></SandeshUVETest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 13:
            {
                EXPECT_EQ(5, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshUVETest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshUVEData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve2</name></SandeshUVEData></data></SandeshUVETest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            default:
                EXPECT_FALSE(true);
        }
        return true;
    }

    uint32_t msg_num_;
    std::auto_ptr<ServerThread> thread_;
    std::auto_ptr<EventManager> evm_;
    SandeshServerTest *server_;
};

TEST_F(SandeshUVEAlarmTest, UVEAlarm) {
    server_->Initialize(0);
    thread_->Start();
    int port = server_->GetPort();
    ASSERT_LT(0, port);

    Sandesh::InitGenerator("SandeshUVEAlarmTest-Client", "localhost", "Test",
                           "0", evm_.get(), 0, NULL);
    Sandesh::ConnectToCollector("127.0.0.1", port);
    TASK_UTIL_EXPECT_TRUE(Sandesh::client()->state() == SandeshClientSM::ESTABLISHED);

    // add uve
    // case 0
    SandeshUVEData uve_data1;
    uve_data1.set_name("uve1");
    SandeshUVETest::Send(uve_data1);

    // update uve
    // case 1
    SandeshUVEData uve_data2;
    uve_data2.set_name("uve1");
    uve_data2.set_x(55);
    SandeshUVETest::Send(uve_data2);

    // case 2
    SandeshUVEData uve_data3;
    uve_data3.set_name("uve2");
    uve_data3.set_x(1);
    SandeshUVETest::Send(uve_data3);

    // delete uve
    // case 3
    SandeshUVEData uve_data4;
    uve_data4.set_name("uve2");
    uve_data4.set_deleted(true);
    SandeshUVETest::Send(uve_data4);

    // add deleted uve
    // case 4
    SandeshUVEData uve_data5;
    uve_data5.set_name("uve2");
    SandeshUVETest::Send(uve_data5);

    // add alarm
    // case 5
    SandeshAlarmData alarm_data1;
    alarm_data1.set_name("alarm1");
    alarm_data1.set_description("alarm1 generated");
    SandeshAlarmTest::Send(alarm_data1);

    // update alarm
    // case 6
    SandeshAlarmData alarm_data2;
    alarm_data2.set_name("alarm1");
    alarm_data2.set_acknowledged(true);
    SandeshAlarmTest::Send(alarm_data2);

    // add another alarm
    // case 7
    SandeshAlarmData alarm_data3;
    alarm_data3.set_name("alarm2");
    alarm_data3.set_description("alarm2 generated");
    SandeshAlarmTest::Send(alarm_data3);

    // delete alarm
    // case 8
    SandeshAlarmData alarm_data4;
    alarm_data4.set_name("alarm2");
    alarm_data4.set_deleted(true);
    SandeshAlarmTest::Send(alarm_data4);

    // add deleted alarm
    // case 9
    SandeshAlarmData alarm_data5;
    alarm_data5.set_name("alarm2");
    SandeshAlarmTest::Send(alarm_data5);

    // verify SyncAllMaps() sends all UVEs/Alarms from the cache
    // case 10, 11, 12 and 13
    std::map<std::string, uint32_t> uve_map;
    SandeshUVETypeMaps::SyncAllMaps(uve_map);

    TASK_UTIL_EXPECT_TRUE(msg_num_ == 14);
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
                boost::bind(&SandeshRequestTest::ReceiveSandeshMsg, this, _1, _2));
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
           const SandeshMessage *msg) {
        const SandeshHeader &header(msg->GetHeader());
        const SandeshXMLMessage *xmsg(
            dynamic_cast<const SandeshXMLMessage *>(msg));
        EXPECT_TRUE(xmsg != NULL);
        const std::string xml_msg(xmsg->ExtractMessage());
        const std::string message_type(xmsg->GetMessageType()); 
        // Call the client ReceiveMsg
        return Sandesh::client()->ReceiveMsg(xml_msg, header, message_type, 0);
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
            "Test", "Test", evm_.get(), 0, NULL);
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

class SandeshHeaderTest : public ::testing::Test {
protected:
    SandeshHeaderTest() :
        buffer_(NULL),
        offset_(0),
        wbuffer_(new TMemoryBuffer(1024)),
        protocol_(new TXMLProtocol(wbuffer_)),
        builder_(SandeshMessageBuilder::GetInstance(
            SandeshMessageBuilder::XML)) {
        // Populate the header
        header_.set_Namespace("");
        header_.set_Timestamp(UTCTimestampUsec());
        header_.set_Module("Test");
        header_.set_Source("Test-Source");
        header_.set_Context("");
        header_.set_SequenceNum(1);
        header_.set_VersionSig(123456789);
        header_.set_Type(SandeshType::FLOW);
        header_.set_Hints(0);
        header_.set_Level(SandeshLevel::SYS_INFO);
        header_.set_Category("");
        header_.set_NodeType("Compute");
        header_.set_InstanceId("0");
        // Write the sandesh header
        int ret = header_.write(protocol_);
        EXPECT_GE(ret, 0);
        // Get the buffer and populate the string
        wbuffer_->getBuffer(&buffer_, &offset_);
        EXPECT_TRUE(buffer_ != NULL);
        EXPECT_EQ(offset_, ret);
    }

    SandeshHeader header_;
    uint8_t *buffer_;
    uint32_t offset_;
    boost::shared_ptr<TMemoryBuffer> wbuffer_;
    boost::shared_ptr<TXMLProtocol> protocol_;
    SandeshMessageBuilder *builder_;
};

TEST_F(SandeshHeaderTest, DISABLED_Autogen) {
    for (int cnt = 0; cnt < 100000; cnt++) {
        boost::shared_ptr<TMemoryBuffer> rbuffer(
            new TMemoryBuffer(buffer_, offset_));
        boost::shared_ptr<TXMLProtocol> rprotocol(
            new TXMLProtocol(rbuffer));
        SandeshHeader lheader;
        // Read the sandesh header
        int ret = lheader.read(rprotocol);
        ASSERT_GE(ret, 0);
        EXPECT_EQ(header_, lheader);
    }
}

TEST_F(SandeshHeaderTest, DISABLED_PugiXML) {
    for (int cnt = 0; cnt < 100000; cnt++) { 
        const SandeshMessage *msg = builder_->Create(buffer_, offset_);
        const SandeshHeader &lheader(msg->GetHeader());
        EXPECT_EQ(header_, lheader);
        delete msg;
    }
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
