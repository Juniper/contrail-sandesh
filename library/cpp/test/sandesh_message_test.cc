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
#include <pugixml/pugixml.hpp>
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
using std::map;
using std::make_pair;
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

        case 6:
        {
            EXPECT_STREQ("SandeshAsyncTest-Client", header.get_Module().c_str());
            EXPECT_STREQ("localhost", header.get_Source().c_str());
            EXPECT_EQ(1, header.get_SequenceNum());
            EXPECT_EQ(ObjectLogOptionalTest::sversionsig(), header.get_VersionSig());
            EXPECT_EQ(SandeshType::OBJECT, header.get_Type());
            EXPECT_EQ(0, header.get_Hints());
            EXPECT_EQ(SandeshLevel::SYS_INFO, header.get_Level());
            EXPECT_EQ("", header.get_Category());
            const char *expect = "<ObjectLogOptionalTest type=\"sandesh\"><f1 type=\"i32\" identifier=\"1\">200</f1><f3 type=\"i32\" identifier=\"2\">100</f3><file type=\"string\" identifier=\"-32768\">tools/sandesh/library/cpp/test/sandesh_message_test.cc</file><line type=\"i32\" identifier=\"-32767\">319</line></ObjectLogOptionalTest>";
            EXPECT_STREQ(expect, message.c_str());
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

    // Check for optional field
    ObjectLogOptionalTest *smh = OBJECT_LOG_OPTIONAL_TEST_CREATE();
    smh->set_f1(200);
    smh->set_f3(100);
    OBJECT_LOG_OPTIONAL_TEST_SEND_SANDESH(smh); // case 6

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
        it = type_stats.find("ObjectLogOptionalTest");
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

    // If negativa values are assigned the old value should be retained
    Sandesh::set_send_rate_limit(10);
    Sandesh::set_send_rate_limit(-10);
    EXPECT_TRUE(Sandesh::get_send_rate_limit() == 10);
    Sandesh::set_send_rate_limit(0);
    EXPECT_TRUE(Sandesh::get_send_rate_limit() == 10);
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
            boost::assign::list_of(
                "SandeshUVETest")("SandeshAlarmTest")("SandeshPeriodicTest");

        
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

        pugi::xml_node mnode = xmsg->GetMessageNode();
        pugi::xml_node dnode = mnode.first_child();
        EXPECT_STREQ(dnode.name(), "data");
        dnode = dnode.first_child();
        std::map<string,string> mm;
        for (pugi::xml_node node = dnode.first_child(); node;
                node = node.next_sibling()) {
            std::ostringstream ostr;
            node.print(ostr, "", pugi::format_raw | pugi::format_no_escapes);
            mm.insert(std::make_pair(node.name(), ostr.str()));
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
                EXPECT_STREQ("<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>", mm["name"].c_str());
                break;
            }
            case 1:
            {
                EXPECT_EQ(2, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                EXPECT_STREQ(mm["z"].c_str(),
"<z type=\"map\" identifier=\"3\" metric=\"agg\"><map key=\"string\" value=\"struct\" size=\"2\"><element>idx1</element><TestAggStruct><count type=\"i32\" identifier=\"1\">55</count></TestAggStruct><element>idx2</element><TestAggStruct><count type=\"i32\" identifier=\"1\">20</count></TestAggStruct></map></z>");
                EXPECT_STREQ(mm["diff_z"].c_str(),
"<diff_z type=\"map\" identifier=\"4\" mstats=\"z:DSNone:\"><map key=\"string\" value=\"struct\" size=\"2\"><element>idx1</element><TestAggStruct><count type=\"i32\" identifier=\"1\">55</count></TestAggStruct><element>idx2</element><TestAggStruct><count type=\"i32\" identifier=\"1\">20</count></TestAggStruct></map></diff_z>");
                EXPECT_STREQ(mm["nullm_zc"].c_str(),
"<nullm_zc type=\"map\" identifier=\"12\" mstats=\"z.count:DSNull:\"><map key=\"string\" value=\"struct\" size=\"2\"><element>idx1</element><NullResult><samples type=\"u64\" identifier=\"3\">1</samples><value type=\"i32\" identifier=\"5\">55</value></NullResult><element>idx2</element><NullResult><samples type=\"u64\" identifier=\"3\">1</samples><value type=\"i32\" identifier=\"5\">20</value></NullResult></map></nullm_zc>");
                EXPECT_STREQ(mm["null_fz"].c_str(),
"<null_fz type=\"i32\" identifier=\"11\" stats=\"fz.count:DSChange:\">20</null_fz>");
                EXPECT_STREQ(mm["null_hz"].c_str(),
"<null_hz type=\"i32\" identifier=\"17\" stats=\"gz.inner.value:DSNone:\">14</null_hz>");
                EXPECT_STREQ(mm["tsm"].c_str(),
"<tsm type=\"map\" identifier=\"8\"><map key=\"string\" value=\"i32\" size=\"1\"><element>i2</element><element>20</element></map></tsm>");
                EXPECT_STREQ(mm["null_tsm"].c_str(),
"<null_tsm type=\"map\" identifier=\"9\" mstats=\"tsm:DSNull:\"><map key=\"string\" value=\"struct\" size=\"1\"><element>i2</element><NullResult><samples type=\"u64\" identifier=\"3\">1</samples><value type=\"i32\" identifier=\"5\">20</value></NullResult></map></null_tsm>");
                break;
            }
            case 2:
            {
                EXPECT_EQ(3, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                EXPECT_STREQ(mm["y"].c_str(),
"<y type=\"i32\" identifier=\"7\">1</y>");
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectCollectorInfo\">uve2</name>");
                EXPECT_STREQ(mm["ewm_y"].c_str(),
"<ewm_y type=\"struct\" identifier=\"10\" stats=\"y:DSAnomaly:EWM:0.2\"><AnomalyResult><samples type=\"u64\" identifier=\"1\">1</samples><algo type=\"string\" identifier=\"2\">EWM</algo><config type=\"string\" identifier=\"3\">0.2</config><state type=\"map\" identifier=\"5\"><map key=\"string\" value=\"string\" size=\"2\"><element>mean</element><element>0.2</element><element>stddev</element><element>0.4</element></map></state><sigma type=\"double\" identifier=\"6\">2</sigma></AnomalyResult></ewm_y>");
                EXPECT_STREQ(mm["ewmd_y"].c_str(),
"<ewmd_y type=\"struct\" identifier=\"13\" stats=\"y:DSAnomaly:EWM:0.2\"><AnomalyResult><samples type=\"u64\" identifier=\"1\">1</samples><algo type=\"string\" identifier=\"2\">EWM</algo><config type=\"string\" identifier=\"3\">0.1</config><state type=\"map\" identifier=\"5\"><map key=\"string\" value=\"string\" size=\"2\"><element>mean</element><element>0.1</element><element>stddev</element><element>0.3</element></map></state><sigma type=\"double\" identifier=\"6\">3</sigma></AnomalyResult></ewmd_y>");
                EXPECT_STREQ(mm["ewmn_y"].c_str(),
"<ewmn_y type=\"struct\" identifier=\"14\" stats=\"y:DSAnomaly:EWM:0.2\"><AnomalyResult><samples type=\"u64\" identifier=\"1\">1</samples><algo type=\"string\" identifier=\"2\">Null</algo><config type=\"string\" identifier=\"3\">Null</config></AnomalyResult></ewmn_y>");
                EXPECT_STREQ(mm["ewmi_y"].c_str(),
"<ewmi_y type=\"struct\" identifier=\"15\" stats=\"y:DSAnomaly:EWM:0.2\"><AnomalyResult><samples type=\"u64\" identifier=\"1\">1</samples><algo type=\"string\" identifier=\"2\">EWM</algo><config type=\"string\" identifier=\"3\">0</config><error type=\"string\" identifier=\"4\">Invalid alpha 0</error></AnomalyResult></ewmi_y>");
                break;
            }
            case 3:
            {
                EXPECT_EQ(4, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                EXPECT_STREQ(mm["y"].c_str(),
"<y type=\"i32\" identifier=\"7\">11</y>");
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve2</name>");
                EXPECT_STREQ(mm["ewm_y"].c_str(),
"<ewm_y type=\"struct\" identifier=\"10\" stats=\"y:DSAnomaly:EWM:0.2\"><AnomalyResult><samples type=\"u64\" identifier=\"1\">1</samples><algo type=\"string\" identifier=\"2\">EWM</algo><config type=\"string\" identifier=\"3\">0.2</config><state type=\"map\" identifier=\"5\"><map key=\"string\" value=\"string\" size=\"2\"><element>mean</element><element>2.2</element><element>stddev</element><element>4.4</element></map></state><sigma type=\"double\" identifier=\"6\">2</sigma></AnomalyResult></ewm_y>");
                break;
            }
            case 4:
            {
                EXPECT_EQ(5, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshUVETest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshUVEData><name type=\"string\" identifier=\"1\" key=\"ObjectCollectorInfo\">uve2</name><deleted type=\"bool\" identifier=\"2\">true</deleted></SandeshUVEData></data></SandeshUVETest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 5:
            {
                EXPECT_EQ(6, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshUVETest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshUVEData><name type=\"string\" identifier=\"1\" key=\"ObjectCollectorInfo\">uve2</name></SandeshUVEData></data></SandeshUVETest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 6:
            {
                EXPECT_EQ(1, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm1</name><description type=\"string\" identifier=\"3\">alarm1 generated</description></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 7:
            {
                EXPECT_EQ(2, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm1</name><acknowledged type=\"bool\" identifier=\"4\">true</acknowledged></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 8:
            {
                EXPECT_EQ(3, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectCollectorInfo\">alarm2</name><description type=\"string\" identifier=\"3\">alarm2 generated</description></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 9:
            {
                EXPECT_EQ(4, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm2</name><description type=\"string\" identifier=\"3\">alarm2 generated</description></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 10:
            {
                EXPECT_EQ(5, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm2</name><deleted type=\"bool\" identifier=\"2\">true</deleted></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 11:
            {
                EXPECT_EQ(6, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm2</name></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 12:
            {
                EXPECT_EQ(1, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshPeriodicTest::sversionsig(), header.get_VersionSig());
                if (mm.find("x")!=mm.end()) EXPECT_TRUE(false);
                if (mm.find("null_x")!=mm.end()) EXPECT_TRUE(false);
                if (mm.find("null_tsm")!=mm.end()) EXPECT_TRUE(false);
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                EXPECT_STREQ(mm["tsm"].c_str(),
"<tsm type=\"map\" identifier=\"5\"><map key=\"string\" value=\"i32\" size=\"2\"><element>j2</element><element>17</element><element>j3</element><element>27</element></map></tsm>");
                EXPECT_STREQ(mm["jx"].c_str(),
"<jx type=\"i32\" identifier=\"8\">75</jx>");
                EXPECT_STREQ(mm["avh_jx"].c_str(),
"");
                EXPECT_STREQ(mm["avg_jx"].c_str(),
"");
                break;
            }
            case 14:
            {
                EXPECT_EQ(3, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectCollectorInfo\">alarm2</name><description type=\"string\" identifier=\"3\">alarm2 generated</description></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 13:
            {
                EXPECT_EQ(2, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm1</name><description type=\"string\" identifier=\"3\">alarm1 generated</description><acknowledged type=\"bool\" identifier=\"4\">true</acknowledged></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 15:
            {
                EXPECT_EQ(6, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::ALARM, header.get_Type());
                EXPECT_EQ(SandeshAlarmTest::sversionsig(), header.get_VersionSig());
                const char* expected_xml = "<SandeshAlarmTest type=\"sandesh\"><data type=\"struct\" identifier=\"1\"><SandeshAlarmData><name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">alarm2</name></SandeshAlarmData></data></SandeshAlarmTest>";
                EXPECT_STREQ(expected_xml, message.c_str());
                break;
            }
            case 16:
            {
                EXPECT_EQ(2, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshPeriodicTest::sversionsig(), header.get_VersionSig());
                if (mm.find("x")!=mm.end()) EXPECT_TRUE(false);
                if (mm.find("null_x")!=mm.end()) EXPECT_TRUE(false);
                if (mm.find("null_tsm")!=mm.end()) EXPECT_TRUE(false);
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                EXPECT_STREQ(mm["tsm"].c_str(),
"<tsm type=\"map\" identifier=\"5\"><map key=\"string\" value=\"i32\" size=\"2\"><element>j2</element><element>17</element><element>j3</element><element>27</element></map></tsm>");
                EXPECT_STREQ(mm["avi_jx"].c_str(),
"<avi_jx type=\"i32\" identifier=\"11\" stats=\"0-jx:DSNone:3\">75</avi_jx>");
                EXPECT_STREQ(mm["avg_jx"].c_str(),
"");
                EXPECT_STREQ(mm["avh_jx"].c_str(),
"");
                break;
            }
            case 18:
            {
                EXPECT_EQ(6, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                if (mm.find("x")!=mm.end()) EXPECT_TRUE(false);
                if (mm.find("null_x")!=mm.end()) EXPECT_TRUE(false);
                if (mm.find("null_tsm")!=mm.end()) EXPECT_TRUE(false);
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectCollectorInfo\">uve2</name>");
                break;
            }
            case 17:
            {
                EXPECT_EQ(2, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                EXPECT_STREQ(mm["z"].c_str(),
"<z type=\"map\" identifier=\"3\" metric=\"agg\"><map key=\"string\" value=\"struct\" size=\"2\"><element>idx1</element><TestAggStruct><count type=\"i32\" identifier=\"1\">55</count></TestAggStruct><element>idx2</element><TestAggStruct><count type=\"i32\" identifier=\"1\">20</count></TestAggStruct></map></z>");
                EXPECT_STREQ(mm["diff_z"].c_str(),
"<diff_z type=\"map\" identifier=\"4\" mstats=\"z:DSNone:\"><map key=\"string\" value=\"struct\" size=\"2\"><element>idx1</element><TestAggStruct><count type=\"i32\" identifier=\"1\">55</count></TestAggStruct><element>idx2</element><TestAggStruct><count type=\"i32\" identifier=\"1\">20</count></TestAggStruct></map></diff_z>");
                EXPECT_STREQ(mm["nullm_zc"].c_str(),
"<nullm_zc type=\"map\" identifier=\"12\" mstats=\"z.count:DSNull:\"><map key=\"string\" value=\"struct\" size=\"2\"><element>idx1</element><NullResult><samples type=\"u64\" identifier=\"3\">1</samples><value type=\"i32\" identifier=\"5\">55</value></NullResult><element>idx2</element><NullResult><samples type=\"u64\" identifier=\"3\">1</samples><value type=\"i32\" identifier=\"5\">20</value></NullResult></map></nullm_zc>");
                EXPECT_STREQ(mm["null_fz"].c_str(),
"<null_fz type=\"i32\" identifier=\"11\" stats=\"fz.count:DSChange:\">20</null_fz>");
                EXPECT_STREQ(mm["tsm"].c_str(),
"<tsm type=\"map\" identifier=\"8\"><map key=\"string\" value=\"i32\" size=\"1\"><element>i2</element><element>20</element></map></tsm>");
                EXPECT_STREQ(mm["null_tsm"].c_str(),
"<null_tsm type=\"map\" identifier=\"9\" mstats=\"tsm:DSNull:\"><map key=\"string\" value=\"struct\" size=\"1\"><element>i2</element><NullResult><samples type=\"u64\" identifier=\"3\">1</samples><value type=\"i32\" identifier=\"5\">20</value></NullResult></map></null_tsm>");
                EXPECT_STREQ(mm["nh_tsm"].c_str(),
"");
                break;
            }
            case 19:
            {
                EXPECT_EQ(4, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                if (mm.find("x")!=mm.end()) EXPECT_TRUE(false);
                if (mm.find("null_x")!=mm.end()) EXPECT_TRUE(false);
                if (mm.find("null_tsm")!=mm.end()) EXPECT_TRUE(false);
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve2</name>");
                EXPECT_STREQ(mm["y"].c_str(),
"<y type=\"i32\" identifier=\"7\">11</y>");
                EXPECT_STREQ(mm["ewmd_y"].c_str(),
"<ewmd_y type=\"struct\" identifier=\"13\" stats=\"y:DSAnomaly:EWM:0.2\"><AnomalyResult><samples type=\"u64\" identifier=\"1\">1</samples><algo type=\"string\" identifier=\"2\">EWM</algo><config type=\"string\" identifier=\"3\">0.1</config><state type=\"map\" identifier=\"5\"><map key=\"string\" value=\"string\" size=\"2\"><element>mean</element><element>1.1</element><element>stddev</element><element>3.3</element></map></state><sigma type=\"double\" identifier=\"6\">3</sigma></AnomalyResult></ewmd_y>");
                EXPECT_STREQ(mm["ewmn_y"].c_str(),
"<ewmn_y type=\"struct\" identifier=\"14\" stats=\"y:DSAnomaly:EWM:0.2\"><AnomalyResult><samples type=\"u64\" identifier=\"1\">1</samples><algo type=\"string\" identifier=\"2\">Null</algo><config type=\"string\" identifier=\"3\">Null</config></AnomalyResult></ewmn_y>");
                EXPECT_STREQ(mm["ewmi_y"].c_str(),
"<ewmi_y type=\"struct\" identifier=\"15\" stats=\"y:DSAnomaly:EWM:0.2\"><AnomalyResult><samples type=\"u64\" identifier=\"1\">1</samples><algo type=\"string\" identifier=\"2\">EWM</algo><config type=\"string\" identifier=\"3\">0</config><error type=\"string\" identifier=\"4\">Invalid alpha 0</error></AnomalyResult></ewmi_y>");
                break;
            }
            case 20:
            {
                EXPECT_EQ(7, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                EXPECT_STREQ(mm["z"].c_str(),
"<z type=\"map\" identifier=\"3\" metric=\"agg\"><map key=\"string\" value=\"struct\" size=\"2\"><element>idx1</element><TestAggStruct><count type=\"i32\" identifier=\"1\">111</count></TestAggStruct><element>idx2</element><TestAggStruct><count type=\"i32\" identifier=\"1\">111</count><deleted type=\"bool\" identifier=\"2\">true</deleted></TestAggStruct></map></z>");
                EXPECT_STREQ(mm["diff_z"].c_str(),
"<diff_z type=\"map\" identifier=\"4\" mstats=\"z:DSNone:\"><map key=\"string\" value=\"struct\" size=\"1\"><element>idx1</element><TestAggStruct><count type=\"i32\" identifier=\"1\">56</count></TestAggStruct></map></diff_z>");
                EXPECT_STREQ(mm["nullm_zc"].c_str(),
"<nullm_zc type=\"map\" identifier=\"12\" mstats=\"z.count:DSNull:\"><map key=\"string\" value=\"struct\" size=\"1\"><element>idx1</element><NullResult><samples type=\"u64\" identifier=\"3\">2</samples><value type=\"i32\" identifier=\"5\">56</value></NullResult></map></nullm_zc>");
                EXPECT_STREQ(mm["null_fz"].c_str(),
"");
                EXPECT_STREQ(mm["fz"].c_str(),
"<fz type=\"struct\" identifier=\"5\"><TestAggStruct><count type=\"i32\" identifier=\"1\">20</count></TestAggStruct></fz>");
                EXPECT_STREQ(mm["null_gz"].c_str(),
"");
                EXPECT_STREQ(mm["gz"].c_str(),
"<gz type=\"struct\" identifier=\"6\" metric=\"diff\"><TestAggStruct><count type=\"i32\" identifier=\"1\">0</count><inner type=\"struct\" identifier=\"3\"><int_P_><value type=\"i32\" identifier=\"2\">27</value></int_P_></inner></TestAggStruct></gz>");
                EXPECT_STREQ(mm["null_hz"].c_str(),
"<null_hz type=\"i32\" identifier=\"17\" stats=\"gz.inner.value:DSNone:\">27</null_hz>");
                EXPECT_STREQ(mm["tsm"].c_str(),
"<tsm type=\"map\" identifier=\"8\"><map key=\"string\" value=\"i32\" size=\"2\"><element>i2</element><element>21</element><element>i3</element><element>31</element></map></tsm>");
                EXPECT_STREQ(mm["null_tsm"].c_str(),
"<null_tsm type=\"map\" identifier=\"9\" mstats=\"tsm:DSNull:\"><map key=\"string\" value=\"struct\" size=\"2\"><element>i2</element><NullResult><samples type=\"u64\" identifier=\"3\">2</samples><value type=\"i32\" identifier=\"5\">21</value></NullResult><element>i3</element><NullResult><samples type=\"u64\" identifier=\"3\">1</samples><value type=\"i32\" identifier=\"5\">31</value></NullResult></map></null_tsm>");
                break;
            }
            case 21:
            {
                EXPECT_EQ(8, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                EXPECT_STREQ(mm["tsm"].c_str(),
"<tsm type=\"map\" identifier=\"8\"><map key=\"string\" value=\"i32\" size=\"1\"><element>i2</element><element>22</element></map></tsm>");
                EXPECT_STREQ(mm["null_tsm"].c_str(),
"<null_tsm type=\"map\" identifier=\"9\" mstats=\"tsm:DSNull:\"><map key=\"string\" value=\"struct\" size=\"2\"><element>i2</element><NullResult><samples type=\"u64\" identifier=\"3\">3</samples><value type=\"i32\" identifier=\"5\">22</value></NullResult><element>i3</element><NullResult><samples type=\"u64\" identifier=\"3\">1</samples><value type=\"i32\" identifier=\"5\">31</value></NullResult></map></null_tsm>");
                EXPECT_STREQ(mm["null_gz"].c_str(),
"<null_gz type=\"i32\" identifier=\"16\" stats=\"gz.count:DSNon0\">20</null_gz>");
                EXPECT_STREQ(mm["null_fz"].c_str(),
"<null_fz type=\"i32\" identifier=\"11\" stats=\"fz.count:DSChange:\">0</null_fz>");
                break;
            }
            case 22:
            {
                EXPECT_EQ(9, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                EXPECT_STREQ(mm["deleted"].c_str(),
"<deleted type=\"bool\" identifier=\"2\">true</deleted>");
                break;
            }
            case 23:
            {
                EXPECT_EQ(10, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                if (mm.find("diff_z")!=mm.end()) EXPECT_TRUE(false);
                EXPECT_STREQ(mm["tsm"].c_str(),
"<tsm type=\"map\" identifier=\"8\"><map key=\"string\" value=\"i32\" size=\"1\"><element>i2</element><element>20</element></map></tsm>");
                EXPECT_STREQ(mm["null_tsm"].c_str(),
"<null_tsm type=\"map\" identifier=\"9\" mstats=\"tsm:DSNull:\"><map key=\"string\" value=\"struct\" size=\"1\"><element>i2</element><NullResult><samples type=\"u64\" identifier=\"3\">1</samples><value type=\"i32\" identifier=\"5\">20</value></NullResult></map></null_tsm>");
                EXPECT_STREQ(mm["nh_tsm"].c_str(),
"");
                break;
            }
            case 24:
            {
                EXPECT_EQ(2, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshPeriodicTest::sversionsig(), header.get_VersionSig());
                if (mm.find("x")!=mm.end()) EXPECT_TRUE(false);
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                EXPECT_STREQ(mm["null_x"].c_str(),
"<null_x type=\"struct\" identifier=\"4\" stats=\"x:DSSum:\"><int_P_><value type=\"i32\" identifier=\"2\">197</value></int_P_></null_x>");
                EXPECT_STREQ(mm["sum_tsm"].c_str(),
"<sum_tsm type=\"map\" identifier=\"6\" mstats=\"tsm:DSSum:\"><map key=\"string\" value=\"struct\" size=\"2\"><element>j2</element><int_P_><value type=\"i32\" identifier=\"2\">17</value></int_P_><element>j3</element><int_P_><value type=\"i32\" identifier=\"2\">27</value></int_P_></map></sum_tsm>");
                EXPECT_STREQ(mm["avg_x"].c_str(),
"");
                EXPECT_STREQ(mm["avh_jx"].c_str(),
"");
                EXPECT_STREQ(mm["avg_jx"].c_str(),
"");
                EXPECT_STREQ(mm["jx"].c_str(),
"");
                EXPECT_STREQ(mm["avi_jx"].c_str(),
"");
                break;
            }
            case 26:
            {
                EXPECT_EQ(6, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshPeriodicTest::sversionsig(), header.get_VersionSig());
                if (mm.find("x")!=mm.end()) EXPECT_TRUE(false);
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                EXPECT_STREQ(mm["null_x"].c_str(),
"<null_x type=\"struct\" identifier=\"4\" stats=\"x:DSSum:\"><int_P_><value type=\"i32\" identifier=\"2\">293</value></int_P_></null_x>");
                EXPECT_STREQ(mm["sum_tsm"].c_str(),
"");
                EXPECT_STREQ(mm["avg_x"].c_str(),
"<avg_x type=\"struct\" identifier=\"7\" stats=\"x:DSAvg:3\"><int_P_><value type=\"i32\" identifier=\"2\">94</value></int_P_></avg_x>");
                EXPECT_STREQ(mm["avg_jx"].c_str(),
"<avg_jx type=\"struct\" identifier=\"10\" stats=\"2-jx:DSNone:3\"><int_P_><value type=\"i32\" identifier=\"2\">75</value></int_P_></avg_jx>");
                EXPECT_STREQ(mm["avh_jx"].c_str(),
"");

                break;
            }
            case 25:
            {
                EXPECT_EQ(6, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshPeriodicTest::sversionsig(), header.get_VersionSig());
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                EXPECT_STREQ(mm["avg_x"].c_str(),
"<avg_x type=\"struct\" identifier=\"7\" stats=\"x:DSAvg:3\"><int_P_><staging type=\"i32\" identifier=\"1\">94</staging></int_P_></avg_x>");
                EXPECT_STREQ(mm["x"].c_str(),
"<x type=\"i32\" identifier=\"3\" hidden=\"yes\">97</x>");
                EXPECT_STREQ(mm["tsm"].c_str(),
"<tsm type=\"map\" identifier=\"5\"><map key=\"string\" value=\"i32\" size=\"2\"><element>j2</element><element>17</element><element>j3</element><element>27</element></map></tsm>");
                EXPECT_STREQ(mm["sum_tsm"].c_str(),
"<sum_tsm type=\"map\" identifier=\"6\" mstats=\"tsm:DSSum:\"><map key=\"string\" value=\"struct\" size=\"2\"><element>j2</element><int_P_><value type=\"i32\" identifier=\"2\">17</value></int_P_><element>j3</element><int_P_><value type=\"i32\" identifier=\"2\">27</value></int_P_></map></sum_tsm>");
                EXPECT_STREQ(mm["avh_jx"].c_str(),
"<avh_jx type=\"struct\" identifier=\"9\" hidden=\"yes\" stats=\"jx:DSAvg\"><int_P_><value type=\"i32\" identifier=\"2\">75</value></int_P_></avh_jx>");
                EXPECT_STREQ(mm["avi_jx"].c_str(),
"<avi_jx type=\"i32\" identifier=\"11\" stats=\"0-jx:DSNone:3\">75</avi_jx>");
                EXPECT_STREQ(mm["avg_jx"].c_str(),
"<avg_jx type=\"struct\" identifier=\"10\" stats=\"2-jx:DSNone:3\"><int_P_><staging type=\"i32\" identifier=\"1\">75</staging></int_P_></avg_jx>");
                break;
            }
            case 27:
            {
                EXPECT_EQ(10, header.get_SequenceNum());
                EXPECT_EQ(SandeshType::UVE, header.get_Type());
                EXPECT_EQ(SandeshUVETest::sversionsig(), header.get_VersionSig());
                EXPECT_STREQ(mm["name"].c_str(),
"<name type=\"string\" identifier=\"1\" key=\"ObjectGeneratorInfo\">uve1</name>");
                if (mm.find("diff_z")!=mm.end()) EXPECT_TRUE(false);
                EXPECT_STREQ(mm["tsm"].c_str(),
"<tsm type=\"map\" identifier=\"8\"><map key=\"string\" value=\"i32\" size=\"1\"><element>i2</element><element>20</element></map></tsm>");
                EXPECT_STREQ(mm["null_tsm"].c_str(),
"<null_tsm type=\"map\" identifier=\"9\" mstats=\"tsm:DSNull:\"><map key=\"string\" value=\"struct\" size=\"1\"><element>i2</element><NullResult><samples type=\"u64\" identifier=\"3\">1</samples><value type=\"i32\" identifier=\"5\">20</value></NullResult></map></null_tsm>");
                EXPECT_STREQ(mm["nh_tsm"].c_str(),
"<nh_tsm type=\"map\" identifier=\"18\" hidden=\"yes\" mstats=\"tsm:DSNone:\"><map key=\"string\" value=\"i32\" size=\"1\"><element>i2</element><element>20</element></map></nh_tsm>");
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

    map<string,string> dselem;
    dselem.insert(make_pair(string("ewmd_y"), string("EWM:0.1")));
    dselem.insert(make_pair(string("ewmn_y"), string("Null")));
    dselem.insert(make_pair(string("ewmi_y"), string("EWM:0")));
    map<string, map<string,string> > dsmap;
    dsmap.insert(make_pair(string("SandeshUVEData"),dselem));
    Sandesh::InitGenerator("SandeshUVEAlarmTest-Client", "localhost", "Test",
                           "0", evm_.get(), 0, NULL, dsmap);
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
    std::map<std::string,TestAggStruct> mtas2;
    TestAggStruct tas2;
    tas2.set_count(55);
    mtas2.insert(std::make_pair("idx1", tas2));
    tas2.set_count(20);
    mtas2.insert(std::make_pair("idx2", tas2));
    uve_data2.set_z(mtas2);
    uve_data2.set_fz(tas2);

    int_P_ inn;
    inn.set_value(14);
    tas2.set_inner(inn);
    uve_data2.set_gz(tas2);

    std::map<string,int32_t> my;
    my.insert(std::make_pair("i2",20));
    uve_data2.set_tsm(my);

    SandeshUVETest::Send(uve_data2);

    // add another uve (override key @ run time)
    // case 2
    SandeshUVEData uve_data3;
    uve_data3.set_name("uve2");

    uve_data3.set_y(1);
    SandeshUVETest::Send(uve_data3, "ObjectCollectorInfo");

    // add uve with existing <name>, but different key value
    // case 3
    SandeshUVEData uve_data4;
    uve_data4.set_name("uve2");
    uve_data4.set_y(11);
    SandeshUVETest::Send(uve_data4);

    // delete uve
    // case 4
    SandeshUVEData uve_data5;
    uve_data5.set_name("uve2");
    uve_data5.set_deleted(true);
    SandeshUVETest::Send(uve_data5, "ObjectCollectorInfo");

    // add deleted uve
    // case 5
    SandeshUVEData uve_data6;
    uve_data6.set_name("uve2");
    SandeshUVETest::Send(uve_data6, "ObjectCollectorInfo");

    // add alarm
    // case 6
    SandeshAlarmData alarm_data1;
    alarm_data1.set_name("alarm1");
    alarm_data1.set_description("alarm1 generated");
    SandeshAlarmTest::Send(alarm_data1);

    // update alarm
    // case 7
    SandeshAlarmData alarm_data2;
    alarm_data2.set_name("alarm1");
    alarm_data2.set_acknowledged(true);
    SandeshAlarmTest::Send(alarm_data2);

    // add another alarm (override key @ run time)
    // case 8
    SandeshAlarmData alarm_data3;
    alarm_data3.set_name("alarm2");
    alarm_data3.set_description("alarm2 generated");
    SandeshAlarmTest::Send(alarm_data3, "ObjectCollectorInfo");

    // add alarm with already existing <name>, but with different key
    // case 9
    SandeshAlarmData alarm_data4;
    alarm_data4.set_name("alarm2");
    alarm_data4.set_description("alarm2 generated");
    SandeshAlarmTest::Send(alarm_data4);

    // delete alarm
    // case 10
    SandeshAlarmData alarm_data5;
    alarm_data5.set_name("alarm2");
    alarm_data5.set_deleted(true);
    SandeshAlarmTest::Send(alarm_data5);

    // add deleted alarm
    // case 11
    SandeshAlarmData alarm_data6;
    alarm_data6.set_name("alarm2");
    SandeshAlarmTest::Send(alarm_data6);

    // PeriodicCache
    {
        // case 12
        SandeshPeriodicData uve_data2;
        uve_data2.set_name("uve1");
        uve_data2.set_x(99);

        std::map<string,int32_t> my;
        my.insert(std::make_pair("j2",17));
        my.insert(std::make_pair("j3",27));
        uve_data2.set_tsm(my);
    
        uve_data2.set_jx(75);

        SandeshPeriodicTest::Send(uve_data2, "ObjectGeneratorInfo", 2000000);

    }    
    {
        SandeshPeriodicData uve_data10;
        uve_data10.set_name("uve1");
        uve_data10.set_x(98);

        SandeshPeriodicTest::Send(uve_data10, "ObjectGeneratorInfo", 3000000);
    }

    // verify SyncAllMaps() sends all UVEs/Alarms from the cache
    // case 13, 14, 15, 16, 17 ,18 and 19
    std::map<std::string, uint32_t> uve_map;
    SandeshUVETypeMaps::SyncAllMaps(uve_map);

    // update uve for derived Stats
    // case 20
    {
	SandeshUVEData uve_data10;
	uve_data10.set_name("uve1");
	std::map<std::string,TestAggStruct> mtas10;
	TestAggStruct tas10;
	tas10.set_count(111);
	mtas10.insert(std::make_pair("idx1",tas10));
	tas10.set_deleted(true);
	mtas10.insert(std::make_pair("idx2",tas10));
	uve_data10.set_z(mtas10);

	TestAggStruct tas11;
	tas11.set_count(20);
	uve_data10.set_fz(tas11);
        TestAggStruct tas12;
        tas12.set_count(0);

        int_P_ inn;
        inn.set_value(27);
        tas12.set_inner(inn);

        uve_data10.set_gz(tas12);

	std::map<string,int32_t> my2;
	my2.insert(std::make_pair("i2",21));
	my2.insert(std::make_pair("i3",31));
	uve_data10.set_tsm(my2);

	SandeshUVETest::Send(uve_data10);
    }

    // modity stats case for derived Stats
    // case 21
    {
	SandeshUVEData uve_data20;
	uve_data20.set_name("uve1");

	std::map<string,int32_t> my3;
	my3.insert(std::make_pair("i2",22));
	uve_data20.set_tsm(my3);

	TestAggStruct tas11;
	tas11.set_count(0);
	uve_data20.set_fz(tas11);
        TestAggStruct tas12;
        tas12.set_count(20);
        uve_data20.set_gz(tas12);

	SandeshUVETest::Send(uve_data20);
    }

    // delete UVE
    // case 22
    {
	SandeshUVEData uve_data30;
	uve_data30.set_name("uve1");
	uve_data30.set_deleted(true);

	SandeshUVETest::Send(uve_data30);
    }

    // recreate UVE
    // case 23
    {
        SandeshUVEData uve_data2;
        uve_data2.set_name("uve1");

        std::map<string,int32_t> my3;
        my3.insert(std::make_pair("i2",20));
        uve_data2.set_tsm(my3);

        SandeshUVETest::Send(uve_data2);
    }

    // Periodic Stats
    // case 24
    SandeshUVETypeMaps::SyncAllMaps(uve_map, true);
    SandeshUVETypeMaps::SyncAllMaps(uve_map, true);


    {
        SandeshPeriodicData uve_data2;
        uve_data2.set_name("uve1");
        uve_data2.set_x(11);
        SandeshPeriodicTest::Send(uve_data2, "ObjectGeneratorInfo", 4000000);

    }    
    {
        SandeshPeriodicData uve_data2;
        uve_data2.set_name("uve1");
        uve_data2.set_x(95);
        SandeshPeriodicTest::Send(uve_data2, "ObjectGeneratorInfo", 5000000);

    }    
    {
        SandeshPeriodicData uve_data2;
        uve_data2.set_name("uve1");
        uve_data2.set_x(90);
        SandeshPeriodicTest::Send(uve_data2, "ObjectGeneratorInfo", 6000000);

    }    
    {
        SandeshPeriodicData uve_data2;
        uve_data2.set_name("uve1");
        uve_data2.set_x(97);
        SandeshPeriodicTest::Send(uve_data2, "ObjectGeneratorInfo", 7000000);

    }    

    // Introspect case for derived stats
    // case 25
    SandeshUVETypeMaps::SyncIntrospect(
       "SandeshPeriodicData", "ObjectGeneratorInfo", "uve1");

    // More Periodic Stats
    // case 26
    SandeshUVETypeMaps::SyncAllMaps(uve_map, true);
    SandeshUVETypeMaps::SyncAllMaps(uve_map, true);

    // Introspect case for derived stats
    // case 27
    SandeshUVETypeMaps::SyncIntrospect(
       "SandeshUVEData", "ObjectGeneratorInfo", "uve1");

    TASK_UTIL_EXPECT_TRUE(msg_num_ == 28);
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
