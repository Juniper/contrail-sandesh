/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_http.cc
//
// Implementation of SandeshHttp
//
// - Register each .sandesh file for generating webpage with all Requests
// - API for Registering Sandesh Requests so they can be served by HttpServer
// - Init and Uninit of HttpServer
//

#include "io/event_manager.h"
#include "http/http_server.h"
#include <cstring>
#include <cstdlib>
#include <boost/bind.hpp>
#include <boost/tokenizer.hpp>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_http.h>
#include <sandesh/transport/TBufferTransports.h>
#include <sandesh/protocol/TXMLProtocol.h>
#include "sandesh_client.h"
#include <sandesh/sandesh_trace.h>
#include <sandesh/sandesh_trace_types.h>

#include "http/http_request.h"
#include "http/http_server.h"
#include "http/http_session.h"

#include "css_bootstrap_min_css.cpp"
#include "css_DT_bootstrap_css.cpp"
#include "css_images_sort_asc_png.cpp"
#include "css_images_sort_asc_disabled_png.cpp"
#include "css_images_sort_both_png.cpp"
#include "css_images_sort_desc_png.cpp"
#include "css_images_sort_desc_disabled_png.cpp"
#include "css_style_css.cpp"
#include "js_bootstrap_min_js.cpp"
#include "js_DT_bootstrap_js.cpp"
#include "js_jquery_2_0_3_min_js.cpp"
#include "js_jquery_dataTables_min_js.cpp"
#include "js_util_js.cpp"
#include "universal_parse_xsl.cpp"


static int kEncodeBufferSize = 4096;

using namespace contrail::sandesh::protocol;
using namespace std;

using contrail::sandesh::transport::TMemoryBuffer;

SandeshHttp::map_type* SandeshHttp::map_ = NULL;
HttpServer* SandeshHttp::hServ_ = NULL;
string SandeshHttp::index_str_;
SandeshHttp::HtmlInfo *SandeshHttp::index_hti_ = NULL;


#define SESSION_SEND(buf) \
    HttpSession::SendSession(context,reinterpret_cast<const u_int8_t *>(buf), strlen(buf), NULL)

enum HttpXMLState {
    HXMLInvalid,
    HXMLNew,
    HXMLIncomplete,
    HXMLMax
};

// Helper function for forming HTTP headers and sending a bytestream
// for XML
//
// Arguments:
//   context : handle to Session on which to send bytestream
//   buf : Buffer that contains XML payload
//   len : length of buffer
//   name : Name of Sandesh Module  
//   more : This is true if there is more content coming for this response
//
static void
HttpSendXML(const std::string& context, const u_int8_t * buf, uint32_t len,
            const char * name, bool more) {

    char buffer_str[100];
    char length_str[80];
    static const char xsl_response[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/xml\r\n"
;
    static const char chunk_response[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/xml\r\n"
"Transfer-Encoding: chunked\r\n\r\n"
;
    char resp_name[40];
    int loc = strcspn(reinterpret_cast<const char *>(buf)," ");
    strncpy(resp_name, reinterpret_cast<const char *>(buf + 1), loc - 1);
    resp_name[loc - 1] = 0;

    static tbb::mutex hmutex;
    static int seq = 0;
    tbb::mutex::scoped_lock lock(hmutex);
    std::string client_ctx;
    seq++;
   
    client_ctx = HttpSession::get_client_context(context);
    HttpXMLState state;

    // Calculate current state;
    if (!client_ctx.empty()) {
        state = HXMLIncomplete;
        if (client_ctx.compare(resp_name)) {
            SANDESH_LOG(DEBUG, __func__ << " Response Mismatch . Expected " <<
                    client_ctx << " , Got " << resp_name);
        }
    } else {
        state = HXMLNew;
        client_ctx = resp_name;

        // If the session is gone, we can stop processing.
        if (!HttpSession::set_client_context(context, client_ctx))
            return;
    }
   
    if ((HXMLNew == state) && (!more)) {
        // This is the first and last chunk of this response

        SESSION_SEND(xsl_response);

        size_t total_len = len;
        sprintf(buffer_str,
            "<?xml-stylesheet type=\"text/xsl\" href=\"/universal_parse.xsl\"?>"
            );
        sprintf(length_str,
                "Content-Length: %zu\r\n\r\n", total_len + strlen(buffer_str));
        SESSION_SEND(length_str);
        SESSION_SEND(buffer_str);

        HttpSession::SendSession(context, buf, len, NULL);

    } else if ((HXMLNew == state) && more) {
        // This is the first chunk of this response
        SESSION_SEND(chunk_response);

        // Calculation for Header
        sprintf(buffer_str,
            "<?xml-stylesheet type=\"text/xsl\" href=\"/universal_parse.xsl\"?>"
            );
        int hdr_len = strlen(buffer_str);
        sprintf(buffer_str,
            "<__%s_list type=\"slist\">\r\n", client_ctx.c_str());
        hdr_len += (strlen(buffer_str) - 2);

        sprintf(length_str, "%x\r\n", hdr_len);
        SESSION_SEND(length_str);

        sprintf(buffer_str,
            "<?xml-stylesheet type=\"text/xsl\" href=\"/universal_parse.xsl\"?>"
            );
        SESSION_SEND(buffer_str);

        sprintf(buffer_str,
            "<__%s_list type=\"slist\">\r\n", client_ctx.c_str());
        SESSION_SEND(buffer_str);

       
        // Calculation for Data
        sprintf(length_str, "%x\r\n", len);
        SESSION_SEND(length_str);

        HttpSession::SendSession(context, buf, len, NULL);

        sprintf(buffer_str,"\r\n");
        SESSION_SEND(buffer_str);
        
    } else if ((HXMLIncomplete == state) && !more) {
        // This is the last chunk of this response
        
        // Calculation for Data
        sprintf(length_str, "%x\r\n", len);
        SESSION_SEND(length_str);

        HttpSession::SendSession(context, buf, len, NULL);

        sprintf(buffer_str,"\r\n");
        SESSION_SEND(buffer_str);

        //Calculation for Footer
        sprintf(buffer_str,"</__%s_list>\r\n", client_ctx.c_str());
        sprintf(length_str, "%zx\r\n", strlen(buffer_str) - 2);
        SESSION_SEND(length_str);
        SESSION_SEND(buffer_str);

        sprintf(length_str, "0\r\n\r\n");
        SESSION_SEND(length_str);
    } else if ((HXMLIncomplete == state) && more) {
        // This is a middle chunk of the response
       
        // Calculation for Data
        sprintf(length_str, "%x\r\n", len);
        SESSION_SEND(length_str);

        HttpSession::SendSession(context, buf, len, NULL);

        sprintf(buffer_str,"\r\n");
        SESSION_SEND(buffer_str);
    }
    // Update context for other users
    if (!more) {
        client_ctx = resp_name;
        HttpSession::set_client_context(context,"");
    }
    
}

// Function for HTTP Server to call when HTTP Client Requests a .sandesh module
// or it's stylesheet 
//
// Arguments:
//   hti : Buffer Ptr and length of HTTP payload to send
//   session : HttpSession on which to send
//   request : Contains the URL that was sent by the Client
//
static void
HttpSandeshFileCallback(SandeshHttp::HtmlInfo *hti,
        HttpSession *session,
        const HttpRequest *request) {

    char length_str[80];
    static const char html_response[] =
"HTTP/1.1 200 OK\r\n"
;
    static const char json_response[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: application/json\r\n"
;
    static const char xsl_response[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/xsl\r\n"
;
    static const char xml_response[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/xml\r\n"
;
    static const char css_response[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/css\r\n"
;
    const char * response =
        ((request->UrlPath().find(".js")!=string::npos) ?
         json_response : ((request->UrlPath().find(".xsl")!=string::npos) ?
            xsl_response : ((request->UrlPath().find(".css")!=string::npos) ?
                css_response : ((request->UrlPath().find(".xml")!=string::npos) ?
                    xml_response : html_response))));

    sprintf(length_str,
            "Content-Length: %d\r\n\r\n", hti->html_len());
    session->Send(reinterpret_cast<const u_int8_t *>(response),
            strlen(response), NULL);
    session->Send(reinterpret_cast<const u_int8_t *>(length_str),
            strlen(length_str), NULL);
    session->Send(hti->html_str(), hti->html_len(), NULL);
    delete request;
}

SandeshHttp::RequestCallbackFn httpreqcb;

// Function for HTTP Server to call when HTTP Client sends a Sandesh Request
//
// Arguments:
//   hs : Ptr to the HTTP Server
//   session : HttpSession on which to send
//   request : Contains the URL that was sent by the Client
//             This URL encodes the attributes of the Sandesh Request
//
static void
HttpSandeshRequestCallback(HttpServer *hs,
        HttpSession *session,
        const HttpRequest *request) {

    string snh_name = request->UrlPath().substr(5);
    Sandesh *sandesh = SandeshBaseFactory::CreateInstance(snh_name);
    if (sandesh == NULL) {
        SANDESH_LOG(DEBUG, __func__ << " Unknown sandesh:" <<
            snh_name << std::endl);
        return;
    }
    SandeshRequest *rsnh = dynamic_cast<SandeshRequest *>(sandesh);
    assert(rsnh);
    rsnh->RequestFromHttp(session->get_context(), request->UrlQuery());
    httpreqcb(rsnh);
    delete request;
}

// This function is called by the Sandesh Response handling code
// if the "context" of the Originating Sandesh Request indicates
// that the Request came from the HTTP Server
// We will form a HTTP/XML payload to send the contents of the Sandesh
// response back to the HTTP Client
//
// Arguments:
//   snh : Sandesh Response to send to the HTTP Client (base class)
//   context: Context that was in the Sandesh Request.
//            All HTTP Server-originated contexts start with "http%"
//          
void
SandeshHttp::Response(Sandesh *snh, std::string context) {
    bool more = false;

    SandeshResponse * rsnh = dynamic_cast<SandeshResponse *>(snh);
    if (rsnh) {
        more = rsnh->get_more();
    } else {
        SandeshTrace * tsnh = dynamic_cast<SandeshTrace *>(snh);
        if (tsnh) {
            more = tsnh->get_more();
        } else {
            SandeshUVE * usnh = dynamic_cast<SandeshUVE *>(snh);
            assert(usnh);
            more = usnh->get_more();
        }
    }
    std::stringstream ss;
    uint8_t *buffer;
    uint32_t xfer = 0, offset;

    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer(kEncodeBufferSize));
    boost::shared_ptr<TXMLProtocol> prot =
            boost::shared_ptr<TXMLProtocol>(
                    new TXMLProtocol(btrans));
    // Write the sandesh
    xfer += snh->Write(prot);
    // Get the buffer
    btrans->getBuffer(&buffer, &offset);
    HttpSendXML(context, buffer, offset, snh->ModuleName().c_str(), more);
    buffer[offset] = 0;
    snh->Release();
}

// This function should be called during Sandesh Generator Initialization
// It initializes the HTTP Server and Registers callbacks for sandesh modules
// and sandesh requests.
//
// Arguments:
//   evm : EventManager that should be used by the HTTP Server
//   module : Name of module
//   port : Port number for HTTP Server (e.g. 8080)
//
int
SandeshHttp::Init(EventManager *evm, const string module,
    short port, RequestCallbackFn reqcb) {

    if (hServ_) return hServ_->GetPort();
    ostringstream index_ss;
   
    SandeshTraceBufferPtr httpbuf(SandeshTraceBufferCreate("httpbuf", 100));
    SANDESH_TRACE_TEXT_TRACE(httpbuf, "<Initializing httpbuf");
    SANDESH_TRACE_TEXT_TRACE(httpbuf, "Size 100");

    hServ_ = new HttpServer(evm);
    httpreqcb = reqcb;
    index_ss << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"" << 
        " \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">" << endl;
    index_ss << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl;
    index_ss << "<head>" << 
        "<link href=\"css/style.css\" rel=\"stylesheet\" type=\"text/css\"/>" << 
        "<title>" << module << "</title></head><body>" << endl;
    index_ss << "<h1>Modules for " << module << "</h1>" << endl;
    
    for (map_type::iterator it=map_->begin(); it!=map_->end(); it++) {
        std::string regString = "/" + it->first;
        hServ_->RegisterHandler(regString.c_str(),
                boost::bind(&HttpSandeshFileCallback, &it->second, _1, _2));
        if (it->first.find(".xml")!=string::npos) {
            index_ss << "<a href=\"" << it->first << "\">" <<
                it->first << "</a><br/>" << endl;
        }
    }
    index_ss << "</body></html>" << endl;

    index_str_ = index_ss.str();
    const char * index_str = index_str_.c_str();
    SANDESH_LOG(DEBUG, "HTTP Introspect Init");

    index_hti_ = new HtmlInfo(reinterpret_cast<const unsigned char *>(
                index_str),strlen(index_str));
    hServ_->RegisterHandler("/index.html",
        boost::bind(&HttpSandeshFileCallback, index_hti_, _1, _2));
    hServ_->RegisterHandler("/",
        boost::bind(&HttpSandeshFileCallback, index_hti_, _1, _2));

    SandeshBaseFactory::map_type::const_iterator it= SandeshBaseFactory::Begin();
    for (; it!=SandeshBaseFactory::End(); it++) {
        std::string regString = "/Snh_" + (*it).first;
        hServ_->RegisterHandler(regString.c_str(),
            boost::bind(&HttpSandeshRequestCallback, hServ_, _1, _2));
    }

    hServ_->Initialize(port);
    SANDESH_LOG(DEBUG, "Sandesh Http Server Port " << hServ_->GetPort());

    return hServ_->GetPort();
}

// Function to shut down HTTP Server 
void
SandeshHttp::Uninit(void) {
    if (!hServ_) return;

    index_str_.clear();
    delete index_hti_;

    hServ_->Shutdown();
    hServ_->ClearSessions();
    hServ_->WaitForEmpty();

    //
    // TODO TcpServer delete can only happen after all inflight sessions
    // have been properly deleted. ASIO threads can hold reference to
    // HttpSession objects
    //
    TcpServerManager::DeleteServer(hServ_);
    hServ_ = NULL;
}

