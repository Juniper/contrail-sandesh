/*
 * Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_util.cc
//

#include <boost/tokenizer.hpp>

#include <base/string_util.h>

#include "sandesh_util.h"

using boost::asio::ip::address;

bool MakeEndpoint(TcpServer::Endpoint* ep, const std::string& epstr) {
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(":");

    tokenizer tokens(epstr, sep);
    tokenizer::iterator it = tokens.begin();
    std::string sip(*it);
    ++it;
    std::string sport(*it);
    int port;
    stringToInteger(sport, port);
    boost::system::error_code ec;
    address addr = address::from_string(sip, ec);
    if (ec) {
        return false;
    }
    *ep = TcpServer::Endpoint(addr, port);
    return true;
}
