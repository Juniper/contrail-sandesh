/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __SANDESH_STATISTICS_H__
#define __SANDESH_STATISTICS_H__

#include <sandesh/sandesh_uve_types.h>

class SandeshStatistics {
public:
    SandeshStatistics() {}

    void Update(const std::string& sandesh_name,
                uint32_t bytes, bool is_tx, bool dropped);

    boost::ptr_map<std::string, SandeshMessageTypeStats> type_stats;
    SandeshMessageStats agg_stats;
};

#endif // __SANDESH_STATISTICS_H__
