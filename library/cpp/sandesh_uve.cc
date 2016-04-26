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
#include "sandesh_client_sm.h"

using std::string;
using std::map; 

SandeshUVETypeMaps::uve_global_map* SandeshUVETypeMaps::map_ = NULL;
int PullSandeshUVE = 0;

void
SandeshUVETypeMaps::SyncAllMaps(const map<string,uint32_t> & inpMap, bool periodic) {
    static uint64_t periodic_count = 0;
    if (periodic) periodic_count++;
    for (uve_global_map::iterator it = map_->begin();
            it != map_->end(); it++) {
        map<string,uint32_t>::const_iterator iit = inpMap.find(it->first);
        if (periodic) {
            uint64_t target_count =
                    (it->second.first * 1000) / SandeshClientSM::kTickInterval;
            if (target_count == 0) continue;
            if ((periodic_count % target_count) != 0) continue;
            assert(inpMap.size() == 0);
        }
        SandeshUVE::SendType stype = (periodic ? SandeshUVE::ST_PERIODIC : SandeshUVE::ST_SYNC); 
        if (iit == inpMap.end()) {
            uint count = it->second.second->SyncUVE("", stype, 0, "");
            SANDESH_LOG(INFO, __func__ << " for " << it->first << ":" << stype <<
                " period " << it->second.first << " without seqno , total = " << count);
        } else {
            uint count = it->second.second->SyncUVE("", stype, iit->second, "");
            SANDESH_LOG(INFO, __func__ << " for " << it->first << ":" << stype <<
                " period " << it->second.first << " with seqno " << iit->second <<
                ", total = " << count);
        }
    }
}

void
SandeshUVETypeMaps::SyncIntrospect(
       std::string tname, std::string table, std::string key) {
    const SandeshUVETypeMaps::uve_global_elem um = TypeMap(tname);
    um.second->SendUVE(table, key, "");
}

void
SandeshUVETypesReq::HandleRequest() const {
    std::vector<SandeshUVETypeInfo> stv;

    SandeshUVETypeMaps::uve_global_map::const_iterator it =
        SandeshUVETypeMaps::Begin();
    for(; it!= SandeshUVETypeMaps::End(); it++) {
        SandeshUVETypeInfo sti;
        sti.set_type_name(it->first);
        sti.set_seq_num(it->second.second->TypeSeq());
        sti.set_period(it->second.first);
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

    const SandeshUVETypeMaps::uve_global_elem um = SandeshUVETypeMaps::TypeMap(get_tname());

    if (um.second) {
        if (__isset.key) {
            returned = um.second->SendUVE("", get_key(), context());
        } else {
            returned += um.second->SyncUVE("", SandeshUVE::ST_INTROSPECT, 0, context());
        }
    }

    SandeshUVECacheResp *sur = new SandeshUVECacheResp();
    sur->set_returned(returned);
    sur->set_period(um.first);
    sur->set_context(context());
    sur->Response();
}
