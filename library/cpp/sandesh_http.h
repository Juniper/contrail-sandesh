/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_http.h
//
// This class supports feeding Sandesh Requests into a process
// using a HTTP Client (Webbrowser) and getting the response back.
//
// It is the backdoor for getting operational state.
//

#ifndef __SANDESH_HTTP_H__
#define __SANDESH_HTTP_H__

#include "http/http_server.h"
#include "sandesh.h"

class HttpServer;
class EventManager;
class SandeshRequest;
class Sandesh;

class SandeshHttp {
public:
    typedef boost::function<int32_t(SandeshRequest *)> RequestCallbackFn;

    static void Response(Sandesh *snh, std::string context);
    static bool Init(EventManager *evm, const std::string module,
        short port, RequestCallbackFn reqcb, int *hport,
        const SandeshConfig &config = SandeshConfig());
    static void Uninit(void);

    class HtmlInfo {
    public:
        HtmlInfo(const unsigned char * html_str, unsigned int html_len) :
                str_(html_str), len_(html_len) {}
        const unsigned char * html_str() const { return str_; }
        unsigned int html_len() const { return len_; }
    private:
        const unsigned char * str_;
        unsigned int len_;
    };

    static HtmlInfo GetHtmlInfo(std::string const& s) {
        map_type::iterator it = GetMap()->find(s);
        if (it == GetMap()->end()) {
            HtmlInfo hin(0,0);
            return hin;
        }
        return it->second;
    }

    SandeshHttp(std::string const& s, HtmlInfo h_info) {
        GetMap()->insert(std::make_pair(s, h_info));
    }

private:
    typedef std::map<std::string, HtmlInfo> map_type;

    static map_type* GetMap() {
        if (!map_) {
            map_ = new map_type();
        }
        return map_;
    }

    static map_type *map_;
    static HttpServer *hServ_;
    static std::string index_str_;
    static HtmlInfo *index_hti_;

};

#endif // __SANDESH_HTTP_H__
