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
    typedef std::map<std::string, SandeshUVEPerTypeMap *> uve_global_map;

    static void RegisterType(const std::string &s, SandeshUVEPerTypeMap *tmap) {
        assert(GetMap()->insert(std::make_pair(s, tmap)).second);
    }
    static const SandeshUVEPerTypeMap * TypeMap(const std::string &s) {
        uve_global_map::const_iterator it = GetMap()->find(s);
        if (it == GetMap()->end()) 
            return NULL;
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
    virtual uint32_t SyncUVE(const uint32_t seqno,
            const std::string &ctx, bool more) const = 0;
    virtual bool SendUVE(const std::string& name,
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

#define SANDESH_UVE_DEF(x,y) \
    static SandeshUVEPerTypeMapImpl<x, y> uvemap ## x(#y)

template<typename T, typename U>
class SandeshUVEPerTypeMapImpl : public SandeshUVEPerTypeMap {
public:
    struct UVEMapEntry {
        UVEMapEntry(const U & d, uint32_t s): data(d), seqno(s) {}
        U data;
        uint32_t seqno;
    };
    typedef boost::ptr_map<std::string, UVEMapEntry> uve_type_map;

    SandeshUVEPerTypeMapImpl(char const * u_name) : uve_name_(u_name) {
        SandeshUVETypeMaps::RegisterType(u_name, this);
    }

    // This function is called whenever a SandeshUVE is sent from
    // the generator to the collector.
    // It updates the cache.
    void UpdateUVE(T *tsnh) {
        tbb::mutex::scoped_lock lock(uve_mutex_);

        const std::string &s = tsnh->get_data().get_name();
        typename uve_type_map::iterator mapentry = map_.find(s);

        if (mapentry == map_.end()) {
            if (!tsnh->get_data().get_deleted())
                if (false)
                    SANDESH_LOG(DEBUG, __func__ << " Adding to " << uve_name_ <<" cache: " << s);
                std::auto_ptr<UVEMapEntry> ume(new UVEMapEntry(tsnh->get_data(), tsnh->seqnum()));
                map_.insert(s, ume);
        } else {
            if (mapentry->second->data.get_deleted()) {
                map_.erase(mapentry);
                std::auto_ptr<UVEMapEntry> ume(new UVEMapEntry(tsnh->get_data(), tsnh->seqnum()));
                map_.insert(s, ume);
            } else {
                tsnh->UpdateUVE(mapentry->second->data);
                mapentry->second->seqno = tsnh->seqnum();
            }
        }
    }

    // This function can be used by the Sandesh Session state machine
    // to get the seq num of the last message sent for this SandeshUVE type
    uint32_t TypeSeq(void) {
        tbb::mutex::scoped_lock lock(uve_mutex_);
        return T::lseqnum();
    }

    uint32_t SyncUVE(const uint32_t seqno,
            const std::string &ctx, bool more) const {
        tbb::mutex::scoped_lock lock(uve_mutex_);
        uint32_t count = 0;

        // Send all entries that are newer than the given sequence number
        for (typename uve_type_map::const_iterator uit = map_.begin();
                uit!=map_.end(); uit++) {
            if ((seqno < uit->second->seqno) || (seqno == 0)) {
                if (!more) {
                    SANDESH_LOG(DEBUG, __func__ << " Syncing " <<
                        " val " << uit->second->data.log() <<
                        " seq " << uit->second->seqno);
                }
                T::Send(uit->second->data, true, uit->second->seqno, ctx, more);
                count++;
            }
        }
        return count;
    }

    bool SendUVE(const std::string& name, const std::string& ctx,
                 bool more) const {
        tbb::mutex::scoped_lock lock(uve_mutex_);
        typename uve_type_map::const_iterator uve_entry = map_.find(name);
        if (uve_entry != map_.end()) {
            T::Send(uve_entry->second->data, true, uve_entry->second->seqno,
                    ctx, more);
            return true;
        }
        return false;
    }

private:

    uve_type_map map_;
    const std::string uve_name_;
    mutable tbb::mutex uve_mutex_;
};

#endif
