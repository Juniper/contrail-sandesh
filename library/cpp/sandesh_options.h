//
// Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
//

#ifndef LIBRARY_CPP_SANDESH_OPTIONS_H_
#define LIBRARY_CPP_SANDESH_OPTIONS_H_

#include <boost/program_options.hpp>

#include <sandesh/sandesh_constants.h>

struct SandeshConfig {
    SandeshConfig() :
        keyfile(),
        certfile(),
        ca_cert(),
        sandesh_ssl_enable(false),
        introspect_ssl_enable(false),
        disable_object_logs(false),
        system_logs_rate_limit(
            g_sandesh_constants.DEFAULT_SANDESH_SEND_RATELIMIT) {
    }
    ~SandeshConfig() {
    }

    std::string keyfile;
    std::string certfile;
    std::string ca_cert;
    bool sandesh_ssl_enable;
    bool introspect_ssl_enable;
    bool disable_object_logs;
    uint32_t system_logs_rate_limit;
};

namespace sandesh {
namespace options {

void AddOptions(boost::program_options::options_description *sandesh_options,
    SandeshConfig *sandesh_config);
void ProcessOptions(const boost::program_options::variables_map &var_map,
    SandeshConfig *sandesh_config);

}  // namespace options
}  // namespace sandesh

#endif  // LIBRARY_CPP_SANDESH_OPTIONS_H_
