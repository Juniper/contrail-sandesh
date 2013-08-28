/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_uve.cc
//
// file to handle requests for Sending Sandesh UVEs
//

#include "sandesh_uve.h"
#include "../common/sandesh_uve_types.h"
#include "sandesh_session.h"
#include "sandesh_http.h"

using std::string;
using std::map; 

SandeshUVETypeMaps::uve_global_map* SandeshUVETypeMaps::map_ = NULL;
int PullSandeshUVE = 0;

void
SandeshUVETypeMaps::SyncAllMaps(const map<string,uint32_t> & inpMap) {
    for (uve_global_map::iterator it = map_->begin();
            it != map_->end(); it++) {
        map<string,uint32_t>::const_iterator iit = inpMap.find(it->first);
        if (iit == inpMap.end()) {
            uint count = it->second->SyncUVE(0, "", false);
            LOG(INFO, __func__ << " for " << it->first << " without seqno , total = " << count);
        } else {
            uint count = it->second->SyncUVE(iit->second, "", false);
            LOG(INFO, __func__ << " for " << it->first << " with seqno " << iit->second << 
                ", total = " << count);
        }
    }
}

void
SandeshUVETypesReq::HandleRequest() const {
    std::vector<SandeshUVETypeInfo> stv;

    SandeshUVETypeMaps::uve_global_map::const_iterator it =
        SandeshUVETypeMaps::Begin();
    for(; it!= SandeshUVETypeMaps::End(); it++) {
        SandeshUVETypeInfo sti;
        sti.set_type_name(it->first);
        sti.set_seq_num(it->second->TypeSeq());
        stv.push_back(sti);
    }
    SandeshUVETypesResp *sur = new SandeshUVETypesResp();
    sur->set_type_info(stv);
    sur->set_context(context());
    sur->Response();
}

void
SandeshUVECacheReq::HandleRequest() const {
    uint32_t returned = 0;

    const SandeshUVEPerTypeMap * tm = SandeshUVETypeMaps::TypeMap(get_tname());
    if (tm)
        returned += tm->SyncUVE(0, context(), true);

    SandeshUVECacheResp *sur = new SandeshUVECacheResp();
    sur->set_returned(returned);
    sur->set_context(context());
    sur->Response();
}
