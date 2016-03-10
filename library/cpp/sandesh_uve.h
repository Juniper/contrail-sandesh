/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_uve.h
// 
// This file has the interface for the generator-side UVE 
// cache.
//

#ifndef __SANDESH_UVE_H__
#define __SANDESH_UVE_H__

#include <map>
#include <vector>
#include <boost/ptr_container/ptr_map.hpp>
#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>
#include <tbb/mutex.h>

class SandeshUVEPerTypeMap;

// This class holds a map of all per-SandeshUVE-type caches.
// Each cache registers with this class during static initialization.
//
// The Sandesh Session state machine uses this class to 
// find the per-SandeshUVE-type caches and invoke operations on them.
//
class SandeshUVETypeMaps {
public:
    typedef std::pair<int, SandeshUVEPerTypeMap *> uve_global_elem;
    typedef std::map<std::string, uve_global_elem> uve_global_map;

    static void RegisterType(const std::string &s, SandeshUVEPerTypeMap *tmap,
            int period) {
        assert(GetMap()->insert(std::make_pair(s,
            std::make_pair(period, tmap))).second);
    }
    static const uve_global_elem TypeMap(const std::string &s) {
        uve_global_map::const_iterator it = GetMap()->find(s);
        if (it == GetMap()->end()) 
            return std::make_pair(-1, (SandeshUVEPerTypeMap *)(NULL));
        else
            return it->second;
    }
    static void SyncAllMaps(const std::map<std::string,uint32_t> &);
    static uve_global_map::const_iterator Begin() { return GetMap()->begin(); }
    static uve_global_map::const_iterator End() { return GetMap()->end(); }
private:
    static uve_global_map *map_;

    static uve_global_map * GetMap() {
        if (!map_) {
            map_ = new uve_global_map();
        }
        return map_;
    }
};


// This is the interface of the per-SandeshUVE-type caches.
//
class SandeshUVEPerTypeMap {
public:
    virtual ~SandeshUVEPerTypeMap() { }
    virtual uint32_t TypeSeq() = 0;
    virtual uint32_t SyncUVE(const std::string &table, const uint32_t seqno,
            const std::string &ctx, bool more) const = 0;
    virtual bool SendUVE(const std::string& table, const std::string& name,
            const std::string& ctx, bool more) const = 0;
};


// This is the per-SandeshUVE-type cache.
// 
//
// It is parameterized on the Sandesh type, and the underlying UVE Type.
// 
// Assumptions about Sandesh Type:
// - Has a single element called "data", of the UVE type.
// - Provides static "Send" function.
// - Per-object sequence number is available as "lseqnum()"
//
// Assumptions about UVE Type
// - Can be constructed with 0 arguments (default constructor)
// - Has a boolean field called "deleted"
// - The UVE key is in a field called "name"
//

#define SANDESH_UVE_DEF(x,y,z) \
    static SandeshUVEPerTypeMapImpl<x, y, z> uvemap ## x(#y)

template<typename T, typename U, int P>
class SandeshUVEPerTypeMapImpl : public SandeshUVEPerTypeMap {
public:
    static const bool kLoad = ((P==-1)?true:false);

    struct UVEMapEntry {
        UVEMapEntry(U& _data, uint32_t seqnum):
                data(_data.table_), seqno(seqnum) {
            T::UpdateUVE(_data, data);
        }
        U data;
        uint32_t seqno;
    };
    typedef boost::ptr_map<std::string, UVEMapEntry> uve_type_map;
    typedef std::map<std::string, uve_type_map> uve_map;

    SandeshUVEPerTypeMapImpl(char const * u_name) : uve_name_(u_name) {
        SandeshUVETypeMaps::RegisterType(u_name, this, P);
    }

    // This function is called whenever a SandeshUVE is sent from
    // the generator to the collector.
    // It updates the cache.
    void UpdateUVE(U& data, uint32_t seqnum) {
        tbb::mutex::scoped_lock lock(uve_mutex_);

        const std::string &table = data.table_;
        assert(!table.empty());
        const std::string &s = data.get_name();
        typename uve_map::iterator omapentry = map_.find(table);
        if (omapentry == map_.end()) {
            omapentry = map_.insert(std::make_pair(table, uve_type_map())).first;
        }
        typename uve_type_map::iterator imapentry =
            omapentry->second.find(s);
        if (imapentry == omapentry->second.end()) {
            std::auto_ptr<UVEMapEntry> ume(new UVEMapEntry(data, seqnum));
            omapentry->second.insert(s, ume);
        } else {
            if (imapentry->second->data.get_deleted()) {
                omapentry->second.erase(imapentry);
                std::auto_ptr<UVEMapEntry> ume(new UVEMapEntry(data, seqnum));
                omapentry->second.insert(s, ume);
            } else {
                T::UpdateUVE(data, imapentry->second->data);
                imapentry->second->seqno = seqnum;
            }
        }
    }

    // This function can be used by the Sandesh Session state machine
    // to get the seq num of the last message sent for this SandeshUVE type
    uint32_t TypeSeq(void) {
        tbb::mutex::scoped_lock lock(uve_mutex_);
        return T::lseqnum();
    }

    uint32_t SyncUVE(const std::string &table, const uint32_t seqno,
            const std::string &ctx, bool more) const {
        tbb::mutex::scoped_lock lock(uve_mutex_);
        uint32_t count = 0;

        for (typename uve_map::const_iterator oit = map_.begin();
             oit != map_.end(); oit++) {
            if (!table.empty() && oit->first != table)
                continue;
            // Send all entries that are newer than the given sequence number
            for (typename uve_type_map::const_iterator uit =
                 oit->second.begin(); uit != oit->second.end(); uit++) {
                if ((seqno < uit->second->seqno) || (seqno == 0)) {
                    if (!more) {
                        SANDESH_LOG(DEBUG, __func__ << " Syncing " <<
                                    " val " << uit->second->data.log() <<
                                    " seq " << uit->second->seqno);
                    }
                    T::Send(uit->second->data, "", true, uit->second->seqno,
                            ctx, more);
                    count++;
                }
            }
        }
        return count;
    }

    bool SendUVE(const std::string& table, const std::string& name,
                 const std::string& ctx, bool more) const {
        tbb::mutex::scoped_lock lock(uve_mutex_);

        for (typename uve_map::const_iterator oit = map_.begin();
             oit != map_.end(); oit++) {
            if (!table.empty() && oit->first != table)
                continue;
            typename uve_type_map::const_iterator uve_entry =
                oit->second.find(name);
            if (uve_entry != oit->second.end()) {
                T::Send(uve_entry->second->data, "", true, uve_entry->second->seqno,
                        ctx, more);
                return true;
            }
        }
        return false;
    }

private:

    uve_map map_;
    const std::string uve_name_;
    mutable tbb::mutex uve_mutex_;
};

template <typename ChildT>
void
SandeshUVECacheListMerge(std::vector<ChildT>& src,
        std::vector<ChildT>& dest, bool _load) {
    std::map<std::string, std::pair<size_t, bool> > srcmap;
    for (size_t idx=0; idx!=src.size(); idx++) {
        srcmap[src[idx].__listkey()] = std::make_pair(idx, false);
    }

    // For all cache entries that are staying, do an update
    typename std::vector<ChildT>::iterator dt = dest.begin();
    while (dt != dest.end()) {
        std::map<std::string, std::pair<size_t, bool> >::iterator st =
            srcmap.find(dt->__listkey());
        if (st != srcmap.end()) {
            // This cache entry IS in the new vector
            st->second.second = true;
            dt->__update(src[st->second.first], _load);
            ++dt;
        } else {
            // This cache entry IS NOT in the new vector
            dt = dest.erase(dt);
        }
    }

    // Now add all new entries
    std::map<std::string, std::pair<size_t, bool> >::iterator mt;
    for (mt = srcmap.begin(); mt != srcmap.end(); mt++) {
        if (mt->second.second == false) {
            typename std::vector<ChildT>::iterator nt =
                dest.insert(dest.end(), src[mt->second.first]);
            nt->__update(src[mt->second.first], _load);
        }
    }
}

#endif
