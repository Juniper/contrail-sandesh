//
// Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
//

#ifndef LIBRARY_CPP_SANDESH_OPTIONS_H_
#define LIBRARY_CPP_SANDESH_OPTIONS_H_

#include <boost/program_options.hpp>

struct SandeshConfig {
    SandeshConfig() :
        keyfile(),
        certfile(),
        ca_cert(),
        sandesh_ssl_enable(false),
        introspect_ssl_enable(false),
        enable_system_and_object_logs(true) {
    }
    ~SandeshConfig() {
    }

    std::string keyfile;
    std::string certfile;
    std::string ca_cert;
    bool sandesh_ssl_enable;
    bool introspect_ssl_enable;
    bool enable_system_and_object_logs;
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
