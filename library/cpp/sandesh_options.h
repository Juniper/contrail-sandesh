//
// Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
//

#ifndef LIBRARY_CPP_SANDESH_OPTIONS_H_
#define LIBRARY_CPP_SANDESH_OPTIONS_H_

#include <boost/program_options.hpp>

#include <sandesh/sandesh_constants.h>

struct SandeshConfig {
    SandeshConfig() :
        tcp_keepalive_enable(true),
        tcp_keepalive_idle_time(7200),
        tcp_keepalive_probes(9),
        tcp_keepalive_interval(75) {
    }
    ~SandeshConfig() {
    }

    bool tcp_keepalive_enable;
    int tcp_keepalive_idle_time;
    int tcp_keepalive_probes;
    int tcp_keepalive_interval;
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
