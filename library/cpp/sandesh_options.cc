//
// Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
//

#include <cassert>
#include "base/util.h"
#include <sandesh/sandesh_options.h>
//#include <base/options_util.h>
//#include <dns/cmn/dns_options.h>

namespace opt = boost::program_options;
//using namespace options::util;
using namespace boost::asio::ip;

namespace sandesh {
namespace options {

void AddOptions(opt::options_description *sandesh_options,
    SandeshConfig *sandesh_config) {
    // Command line and config file options.
    sandesh_options->add_options()
        ("SANDESH.tcp_keepalive_enable",
         opt::bool_switch(&sandesh_config->tcp_keepalive_enable)->default_value(true),
         "Enable Keepalive for tcp socket")
        ("SANDESH.tcp_keepalive_idle_time",
         opt::value<int>(&sandesh_config->tcp_keepalive_idle_time),
         "Keepalive idle time for tcp socket")
        ("SANDESH.tcp_keepalive_probes",
         opt::value<int>(&sandesh_config->tcp_keepalive_probes),
         "Keepalive probes for tcp socket")
        ("SANDESH.tcp_keepalive_interval",
         opt::value<int>(&sandesh_config->tcp_keepalive_interval),
         "Keepalive interval for tcp socket")
        ;
}

void ProcessOptions(const opt::variables_map &var_map,
    SandeshConfig *sandesh_config) {
/*    GetOptValue<bool>(var_map, sandesh_config->tcp_keepalive_enable,
                       "SANDESH.tcp_keepalive_enable");
    GetOptValue<int>(var_map, sandesh_config->tcp_keepalive_idle_time,
                     "SANDESH.tcp_keepalive_idle_time");
    GetOptValue<int>(var_map, sandesh_config->tcp_keepalive_probes,
                     "SANDESH.tcp_keepalive_probes");
    GetOptValue<int>(var_map, sandesh_config->tcp_keepalive_interval,
                     "SANDESH.tcp_keepalive_interval");
*/}

}  // namespace options
}  // namespace sandesh
