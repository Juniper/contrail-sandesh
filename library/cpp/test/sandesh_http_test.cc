/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

/*
 * License: Curl
 * Copyright (C) 1998 - 2015, Daniel Stenberg, <daniel@haxx.se>, et al.
 * https://curl.haxx.se/libcurl/c/getinmemory.html
 */

//
// sandesh_http_test.cc
//
// Unit Tests for HTTP Introspect
//

#include "testing/gunit.h"
#include <boost/bind.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

extern "C" {
    #include <curl/curl.h>
    #include <unistd.h>
}

#include "base/logging.h"
#include "io/event_manager.h"
#include "io/test/event_manager_test.h"
#include "http/http_server.h"
#include "sandesh/sandesh_types.h"
#include "sandesh.h"
#include "sandesh_http.h"
#include "test/sandesh_http_test_types.h"
#include "base/task.h"
#include "base/test/task_test_util.h"

using namespace std;
using namespace boost::asio::ip;
using namespace boost::uuids;

int currentTestId;
int currentParam;
string currentTestString1;
string currentTestString2;
address currentTestIpaddr1;
uuid currentTestUuid1;

void
SandeshHttpTestRequest::HandleRequest() const{

    switch(testId) {

    case(1): {
        SandeshHttpTestResp *shtp = new SandeshHttpTestResp();
        shtp->set_testId(testId);
        shtp->set_param(param);
        shtp->set_context(context());

        shtp->Response();
        break;
    }

    case(2): {

        VNSwitchRouteResp *vsrr = new VNSwitchRouteResp();
        vsrr->set_vnId(param);
        
        vector<VNSRoute> lval;
        VNSRoute vsnr;
        vsnr.prefix = 2; vsnr.desc = "two";
        lval.push_back(vsnr);
        vsnr.prefix = 3; vsnr.desc = "three";
        lval.push_back(vsnr);
        vsnr.prefix = 4; vsnr.desc = "four";
        lval.push_back(vsnr);
        vsrr->set_vnRoutes(lval);

        vsnr.prefix = 0; vsnr.desc = "zero";
        vsrr->set_vnMarkerRoute(vsnr);
        vsrr->set_context(context());
        vsrr->Response();
        break;
    }

    case (3):
    case (4): {
        VNSwitchRouteResp *vsrr = new VNSwitchRouteResp();
        vsrr->set_vnId(param);
        
        vector<VNSRoute> lval;
        VNSRoute vsnr;
        vsnr.prefix = 2; vsnr.desc = "two";
        lval.push_back(vsnr);
        vsnr.prefix = 3; vsnr.desc = "three";
        lval.push_back(vsnr);
        vsnr.prefix = 4; vsnr.desc = "four";
        lval.push_back(vsnr);
        vsrr->set_vnRoutes(lval);

        vsnr.prefix = 0; vsnr.desc = "zero";
        vsrr->set_vnMarkerRoute(vsnr);
        vsrr->set_context(context());
        vsrr->set_more(true);
        vsrr->Response();

        SandeshHttpTestResp *shtp = new SandeshHttpTestResp();
        shtp->set_testId(testId);
        shtp->set_param(param);
        shtp->set_context(context());
        shtp->Response();
        break;      
    }
    case (5):
    case (6):
    case (7): {
        ASSERT_STREQ(teststring1.c_str(), currentTestString1.c_str());
        ASSERT_STREQ(teststring2.c_str(), currentTestString2.c_str());
        ASSERT_EQ(testIpaddr1, currentTestIpaddr1);
        ASSERT_EQ(testUuid1, currentTestUuid1);
        SandeshHttpTestResp *shtp = new SandeshHttpTestResp();
        shtp->set_testId(testId);
        shtp->set_param(param);
        shtp->set_teststring1(teststring1);
        shtp->set_teststring2(teststring2);
        shtp->set_context(context());
        shtp->Response();
        break;
    }
    }
    ASSERT_EQ(param, currentParam);
    ASSERT_EQ(testId, currentTestId);
}


namespace {

using namespace std;

struct MemoryStruct {
  char *memory;
  size_t size;
};

struct MemoryStruct chunk;
struct MemoryStruct style;

size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = 
    reinterpret_cast<char *>(realloc(mem->memory, mem->size + realsize + 1));
  if (mem->memory == NULL) {
    /* out of memory! */ 
    LOG(DEBUG, "not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}
 

CURLcode curl_fetch(const char* url, struct MemoryStruct *chk)
{
  CURL *curl_handle;
  
  
  CURLcode ret;

  curl_global_init(CURL_GLOBAL_ALL);
 
  /* init the curl session */ 
  curl_handle = curl_easy_init();
 
  /* specify URL to get */ 
  curl_easy_setopt(curl_handle, CURLOPT_URL, url); 
 
  /* send all data to this function  */ 
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
 
  /* we pass our 'chunk' struct to the callback function */ 
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)chk);
 
  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */ 
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1);
  curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10);
 
  /* get it! */ 
  ret = curl_easy_perform(curl_handle);
 
  /* cleanup curl stuff */ 
  curl_easy_cleanup(curl_handle);
 
  /*
   * Now, our chunk.memory points to a memory block that is chunk.size
   * bytes big and contains the remote file.
   *
   * Do something nice with it!
   *
   * You should be aware of the fact that at this point we might have an
   * allocated data block, and nothing has yet deallocated that data. So when
   * you're done with it, you should free() it as a nice application.
   */ 
 
  LOG(DEBUG, "bytes retrieved " <<  (long)chk->size);
  //LOG(DEBUG, chk->memory);
  
  return ret;
}

class SandeshHttpTest;

static int32_t CallbackFn(SandeshHttpTest* stest, Sandesh * sndh) {
    SandeshHttpTestRequest * snh = static_cast<SandeshHttpTestRequest *>(sndh);
    snh->HandleRequest();
    snh->Release();

    return 0;
}

class SandeshHttpTest : public ::testing::Test {
public:
    SandeshHttpTest() {}

    virtual void SetUp() {
        LOG(DEBUG, "SetUp SandeshHttpTest " << sizeof(ServerThread));
        evm_.reset(new EventManager());
        ServerThread *st = new ServerThread(evm_.get());
        thread_.reset(st);
        int port;
        bool success(SandeshHttp::Init(evm_.get(), "sandesh_http_test", 0,
            boost::bind(&CallbackFn, this, _1), &port));
        ASSERT_TRUE(success);
        host_url_ << "http://localhost:";
        host_url_ << port << "/";
        LOG(DEBUG, "Serving " << host_url_.str());
        thread_->Start();
        task_util::WaitForIdle();
    }

    virtual void TearDown() {
        task_util::WaitForIdle();
        SandeshHttp::Uninit();
        task_util::WaitForIdle();
        LOG(DEBUG, "TearDown SandeshHttpTest");
        evm_->Shutdown();
        task_util::WaitForIdle();
        if (thread_.get() != NULL) {
            thread_->Join();
            thread_.reset();
        } else {
            LOG(DEBUG, "Cannot Release thread");
        }
        evm_.reset();
        host_url_.str("");
    }
    std::auto_ptr<ServerThread> thread_;
    std::auto_ptr<EventManager> evm_;
    ostringstream host_url_;

};

TEST_F(SandeshHttpTest, Index) {
    chunk.memory = reinterpret_cast<char *>(malloc(1));  
    chunk.size = 0;
    const string url = host_url_.str() + "index.html";
    ASSERT_EQ(CURLE_OK, curl_fetch(url.c_str(), &chunk)) ;
    string s1(chunk.memory);
    ASSERT_NE(string::npos, s1.find("sandesh_http_test.xml"));
    if (chunk.memory)
      free(chunk.memory);
}
TEST_F(SandeshHttpTest, Module) {
    chunk.memory = reinterpret_cast<char *>(malloc(1));  
    chunk.size = 0;
    const string url = host_url_.str() + "sandesh_http_test.xml";
    ASSERT_EQ(CURLE_OK, curl_fetch(url.c_str(), &chunk)) ;
    string s1(chunk.memory);
    ASSERT_NE(string::npos,
      s1.find("<SandeshHttpTestRequest type=\"sandesh\">"));
    if (chunk.memory)
      free(chunk.memory);
}
TEST_F(SandeshHttpTest, Request) {
    chunk.memory = reinterpret_cast<char *>(malloc(1));  
    chunk.size = 0;

    currentTestId = 1; currentParam = 11;
    const string url = host_url_.str() + \
      "Snh_SandeshHttpTestRequest?testId=1&param=11";
    ASSERT_EQ(CURLE_OK, curl_fetch(url.c_str(), &chunk)) ;
    if (chunk.memory)
      free(chunk.memory);
}

TEST_F(SandeshHttpTest, Response) {
    chunk.memory = reinterpret_cast<char *>(malloc(1));  
    chunk.size = 0;

    currentTestId = 2; currentParam = 22;
    const string url = host_url_.str() + \
      "Snh_SandeshHttpTestRequest?testId=2&param=22";
    ASSERT_EQ(CURLE_OK, curl_fetch(url.c_str(), &chunk)) ;
    if (chunk.memory)
      free(chunk.memory);
}

TEST_F(SandeshHttpTest, MultiResponse) {
    chunk.memory = reinterpret_cast<char *>(malloc(1));  
    chunk.size = 0;

    currentTestId = 3; currentParam = 33;
    const string url = host_url_.str() + \
      "Snh_SandeshHttpTestRequest?testId=3&param=33";

    ASSERT_EQ(CURLE_OK, curl_fetch(url.c_str(), &chunk));

    if (chunk.memory)
      free(chunk.memory);
}

TEST_F(SandeshHttpTest, XSLT) {
    chunk.memory = reinterpret_cast<char *>(malloc(1));  
    chunk.size = 0;

    style.memory = reinterpret_cast<char *>(malloc(1));  
    style.size = 0;

    const string style_url = host_url_.str() + \
        "universal_parse.xsl";

    ASSERT_EQ(CURLE_OK, curl_fetch(style_url.c_str(), &style));
    ofstream sfile;
    ostringstream sfilename;
    sfilename << "/tmp/universal_parse.xsl." << getpid();
    sfile.open(sfilename.str().c_str());
    sfile << style.memory;
    sfile.close();

    currentTestId = 4; currentParam = 44;
    const string url = host_url_.str() + \
      "Snh_SandeshHttpTestRequest?testId=4&param=44";

    ASSERT_EQ(CURLE_OK, curl_fetch(url.c_str(), &chunk));
    ofstream xfile;
    ostringstream xfilename;
    xfilename << "/tmp/SandeshHttpTestRequest.xml." << getpid();
    xfile.open(xfilename.str().c_str());
    xfile << chunk.memory;
    xfile.close();


    xsltStylesheetPtr cur =
        xsltParseStylesheetFile((const xmlChar *)sfilename.str().c_str());
    ASSERT_NE((long)NULL, (long) cur);

    xmlDocPtr doc = xmlParseFile(xfilename.str().c_str());
    ASSERT_NE((long)NULL, (long) doc);

    xsltTransformContextPtr ctxt = xsltNewTransformContext(cur, doc);
    ASSERT_NE((long)NULL, (long) ctxt);

    xmlDocPtr res = 
        xsltApplyStylesheetUser(cur, doc, NULL, NULL, NULL, ctxt);
    ASSERT_NE(long(NULL), (long) res);
    ASSERT_EQ(XSLT_STATE_OK, ctxt->state);

    xsltFreeTransformContext(ctxt);
    xsltFreeStylesheet(cur);
    xmlFreeDoc(doc);
    xmlFreeDoc(res);

    remove(sfilename.str().c_str());
    remove(xfilename.str().c_str());

    if (chunk.memory)
      free(chunk.memory);
    if (style.memory)
      free(style.memory);
}

TEST_F(SandeshHttpTest, ValidateUrlFieldsWithSpecialChar) {
    chunk.memory = reinterpret_cast<char *>(malloc(1));
    chunk.size = 0;

    currentTestId = 5; currentParam = 55;
    currentTestString1 = "&one<&>two&";
    currentTestString2 = "%1&2%>";
    boost::system::error_code ec;
    currentTestIpaddr1 = address::from_string("80.80.80.9", ec);
    CURL * cr = curl_easy_init();
    char *string1 = curl_easy_escape(cr, currentTestString1.c_str(), 0);
    char *string2 = curl_easy_escape(cr, currentTestString2.c_str(), 0);
    const string url = host_url_.str() + \
      "Snh_SandeshHttpTestRequest?testId=5&param=55&teststring1=" + \
       string1 + "&teststring2=" + string2 + "&testIpaddr1=" +
       currentTestIpaddr1.to_string();
    curl_free(string1);
    curl_free(string2);
    curl_easy_cleanup(cr);
    ASSERT_EQ(CURLE_OK, curl_fetch(url.c_str(), &chunk)) ;
    if (chunk.memory)
      free(chunk.memory);
}

TEST_F(SandeshHttpTest, ValidateUrlWithTwoEmptyFields) {
    chunk.memory = reinterpret_cast<char *>(malloc(1));
    chunk.size = 0;

    currentTestId = 6; currentParam = 66;
    currentTestString1 = "";
    currentTestString2 = "one&two";
    boost::system::error_code ec;
    currentTestIpaddr1 = address::from_string("2001:0:3238:df10:63::fefb", ec);
    std::stringstream ss;
    ss << "00010203-0405-0607-0809-0a0b0c0d0e0f";
    ss >> currentTestUuid1;
    CURL * cr = curl_easy_init();
    char *string1 = curl_easy_escape(cr, currentTestString1.c_str(), 0);
    char *string2 = curl_easy_escape(cr, currentTestString2.c_str(), 0);
    const string url = host_url_.str() + \
      "Snh_SandeshHttpTestRequest?testId=6&param=66&teststring1=" + \
       string1 + "&teststring2=" + string2 + "&testIpaddr1=" +
       currentTestIpaddr1.to_string() + "&testUuid1=" +
       boost::uuids::to_string(currentTestUuid1);
    curl_free(string1);
    curl_free(string2);
    curl_easy_cleanup(cr);
    ASSERT_EQ(CURLE_OK, curl_fetch(url.c_str(), &chunk)) ;
    if (chunk.memory)
      free(chunk.memory);
}

TEST_F(SandeshHttpTest, ValidateUrlWithXValue) {
    chunk.memory = reinterpret_cast<char *>(malloc(1));
    chunk.size = 0;

    currentTestId = 7; currentParam = 0;
    currentTestString1 = "";
    currentTestString2 = "";
    currentTestIpaddr1 = address();
    currentTestUuid1 = boost::uuids::nil_uuid();
    CURL * cr = curl_easy_init();
    char *string1 = curl_easy_escape(cr, currentTestString1.c_str(), 0);
    char *string2 = curl_easy_escape(cr, currentTestString2.c_str(), 0);
    const string url = host_url_.str() + \
      "Snh_SandeshHttpTestRequest?x=7";
    curl_free(string1);
    curl_free(string2);
    curl_easy_cleanup(cr);
    ASSERT_EQ(CURLE_OK, curl_fetch(url.c_str(), &chunk)) ;
    if (chunk.memory)
      free(chunk.memory);
}
}

int main(int argc, char **argv) {
    LoggingInit();
    ::testing::InitGoogleTest(&argc, argv);
    LOG(DEBUG, "Start SandeshHttpTest");
    return RUN_ALL_TESTS();
}
