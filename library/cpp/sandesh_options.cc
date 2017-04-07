//
// Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
//

#include <base/options_util.h>
#include <sandesh/sandesh_options.h>

namespace opt = boost::program_options;
using namespace options::util;

namespace sandesh {
namespace options {

void AddOptions(opt::options_description *sandesh_options,
    SandeshConfig *sandesh_config) {
    // Command line and config file options.
    sandesh_options->add_options()
        ("SANDESH.sandesh_keyfile", opt::value<std::string>()->default_value(
         "/etc/contrail/ssl/private/server-privkey.pem"),
         "Sandesh SSL private key")
        ("SANDESH.sandesh_certfile", opt::value<std::string>()->default_value(
         "/etc/contrail/ssl/certs/server.pem"),
         "Sandesh SSL certificate")
        ("SANDESH.sandesh_ca_cert", opt::value<std::string>()->default_value(
         "/etc/contrail/ssl/certs/ca-cert.pem"),
         "Sandesh CA SSL certificate")
        ("SANDESH.sandesh_ssl_enable",
         opt::bool_switch(&sandesh_config->sandesh_ssl_enable),
         "Enable SSL for sandesh connection")
        ("SANDESH.introspect_ssl_enable",
         opt::bool_switch(&sandesh_config->introspect_ssl_enable),
         "Enable SSL for introspect connection")
        ("SANDESH.enable_system_and_object_logs",
         opt::bool_switch(&sandesh_config->enable_system_and_object_logs),
         "Enable sending of system and object logs to collector")
        ;
}

void ProcessOptions(const opt::variables_map &var_map,
    SandeshConfig *sandesh_config) {
    GetOptValue<std::string>(var_map, sandesh_config->keyfile,
                        "SANDESH.sandesh_keyfile");
    GetOptValue<std::string>(var_map, sandesh_config->certfile,
                        "SANDESH.sandesh_certfile");
    GetOptValue<std::string>(var_map, sandesh_config->ca_cert,
                        "SANDESH.sandesh_ca_cert");
    GetOptValue<bool>(var_map, sandesh_config->sandesh_ssl_enable,
                      "SANDESH.sandesh_ssl_enable");
    GetOptValue<bool>(var_map, sandesh_config->introspect_ssl_enable,
                      "SANDESH.introspect_ssl_enable");
    GetOptValue<bool>(var_map, sandesh_config->enable_system_and_object_logs,
                      "SANDESH.enable_system_and_object_logs");
}

}  // namespace options
}  // namespace sandesh
