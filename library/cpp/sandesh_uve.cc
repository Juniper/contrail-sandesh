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


bool 
SandeshUVETypeMaps::InitDerivedStats(
        const std::map<std::string, ds_conf_elem> &dsmap) {
    bool success = true;
    // For all (per UVE Struct) elements of the DerivedStats config,
    // call the InitDerivedStats functions on that UVE Struct's Cache
    for (map<string, ds_conf_elem>::const_iterator cit = dsmap.begin();
            cit != dsmap.end(); cit++) {
        uve_global_map::iterator it = map_->find(cit->first);
        if (it==map_->end()) {
            success = false;
        } else {
            if (!(it->second.second)->InitDerivedStats(cit->second))
                success = false;
        }
    }
    return success;
}

uint32_t
SandeshUVETypeMaps::Clear(const std::string& proxy, int partition) {
    int count=0;
    for (uve_global_map::iterator it = map_->begin();
            it != map_->end(); it++) {
        uint32_t subcount = it->second.second->ClearUVEs(proxy, partition);
        if (subcount !=0) {
            SANDESH_LOG(INFO, __func__ << " for " << it->first << " partition " <<
                partition << " proxy " << proxy << " cleared " <<
                subcount);
            count += subcount;
        }
    }
    return count;
}

void
SandeshUVETypeMaps::SyncAllMaps(const map<string,uint32_t> & inpMap, bool periodic) {
    static uint64_t periodic_count = 0;
    if (periodic) periodic_count++;
    for (uve_global_map::iterator it = map_->begin();
            it != map_->end(); it++) {
        map<string,uint32_t>::const_iterator iit = inpMap.find(it->first);
        uint32_t cycle = 0;
        if (periodic) {
            uint64_t target_count =
                    (it->second.first * 1000) / SandeshClientSM::kTickInterval;
            if (target_count == 0) continue;
            if ((periodic_count % target_count) != 0) continue;
            cycle = (uint32_t) (periodic_count / target_count);
            assert(inpMap.size() == 0);
        }
        SandeshUVE::SendType stype = (periodic ? SandeshUVE::ST_PERIODIC : SandeshUVE::ST_SYNC); 
        if (iit == inpMap.end()) {
            uint count = it->second.second->SyncUVE("", stype, 0, cycle, "");
            SANDESH_LOG(INFO, __func__ << " for " << it->first << ":" << stype <<
                " period " << it->second.first << " cycle " << cycle <<
                " without seqno , total = " << count);
        } else {
            uint count = it->second.second->SyncUVE("", stype, iit->second, cycle, "");
            SANDESH_LOG(INFO, __func__ << " for " << it->first << ":" << stype <<
                " period " << it->second.first << " cycle " << cycle <<
                " with seqno " << iit->second <<
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
        sti.set_dsconf(it->second.second->GetDSConf());
        sti.set_timeout(it->second.second->GetTimeout());
        stv.push_back(sti);
    }
    SandeshUVETypesResp *sur = new SandeshUVETypesResp();
    sur->set_type_info(stv);
    sur->set_context(context());
    sur->Response();
}
void
SandeshUVEDSConfReq::HandleRequest() const {
    const SandeshUVETypeMaps::uve_global_elem um =
        SandeshUVETypeMaps::TypeMap(get_tname());
    
    map<string,string> dsconf;
    bool success = false;
    if (um.second) {
        dsconf.insert(std::make_pair(get_dsattr(), get_dsconf()));
        success = um.second->InitDerivedStats(dsconf); 
    }
    SandeshUVEDSConfResp *sur = new SandeshUVEDSConfResp();
    sur->set_success(success);
    if (success) sur->set_dsconf(um.second->GetDSConf());
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
            returned += um.second->SyncUVE("", SandeshUVE::ST_INTROSPECT, 0, 0, context());
        }
    }

    SandeshUVECacheResp *sur = new SandeshUVECacheResp();
    sur->set_returned(returned);
    if (um.second) sur->set_period(um.first);
    sur->set_context(context());
    sur->Response();
}
