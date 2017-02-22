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
#include <tbb/concurrent_hash_map.h>
#include <boost/functional/hash.hpp>

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
    typedef std::map<std::string, std::string> ds_conf_elem;

    static void RegisterType(const std::string &s, SandeshUVEPerTypeMap *tmap,
            int period) {
        assert(GetMap()->insert(std::make_pair(s,
            std::make_pair(period, tmap))).second);
    }
    static const uve_global_elem TypeMap(const std::string &s) {
        uve_global_map::const_iterator it = GetMap()->find(s);
        if (it == GetMap()->end()) 
            return std::make_pair(0, (SandeshUVEPerTypeMap *)(NULL));
        else
            return it->second;
    }
    static void SyncAllMaps(const std::map<std::string,uint32_t> &,
        bool periodic = false);
    static bool InitDerivedStats(
        const std::map<std::string, ds_conf_elem> &);
    static void SyncIntrospect(std::string tname, std::string table, std::string key);

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
    virtual uint32_t SyncUVE(const std::string &table, SandeshUVE::SendType st,
            uint32_t seqno, uint32_t cycle, const std::string &ctx) = 0;
    virtual bool InitDerivedStats(
            const std::map<std::string,std::string> & dsconf) = 0;
    virtual bool SendUVE(const std::string& table, const std::string& name,
            const std::string& ctx) const = 0;
    virtual std::map<std::string, std::string> GetDSConf(void) const = 0;
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

#define SANDESH_UVE_DEF(x,y,z,w) \
    static SandeshUVEPerTypeMapImpl<x, y, z, w> uvemap ## x(#y)

template<typename T, typename U, int P, int TM>
class SandeshUVEPerTypeMapImpl : public SandeshUVEPerTypeMap {
public:

    struct HashCompare { 
        static size_t hash( const std::string& key )                  { return boost::hash_value(key); } 
        static bool   equal( const std::string& key1, const std::string& key2 ) { return ( key1 == key2 ); } 
    }; 

    struct UVEMapEntry {
        UVEMapEntry(std::string table, uint32_t seqnum):
                data(table), seqno(seqnum) {
        }
        U data;
        uint32_t seqno;
    };

    // The key is the table name
    typedef boost::ptr_map<std::string, UVEMapEntry> uve_table_map;

    // The key is the UVE-Key
    typedef tbb::concurrent_hash_map<std::string, uve_table_map, HashCompare > uve_cmap;

    SandeshUVEPerTypeMapImpl(char const * u_name) : uve_name_(u_name),
            dsconf_(T::_DSConf()) {
        SandeshUVETypeMaps::RegisterType(u_name, this, P);
    }

    // This function is called whenever a SandeshUVE is sent from
    // the generator to the collector.
    // It updates the cache.
    bool UpdateUVE(U& data, uint32_t seqnum, uint64_t mono_usec) {
        bool send = false;
        tbb::mutex::scoped_lock lock;
        const std::string &table = data.table_;
        assert(!table.empty());
        const std::string &s = data.get_name();
        if (!mono_usec) mono_usec = ClockMonotonicUsec();
 
        // pickup DS Config
        lock.acquire(uve_mutex_);
        std::map<string,string> dsconf = dsconf_;

        // If we are going to erase, we need a global lock
        // to coordinate with iterators 
        // To prevent deadlock, we always acquire global lock
        // before accessor
        if (!data.get_deleted()) {
            lock.release();
        }
        typename uve_cmap::accessor a;

        // Ensure that the entry exists, and we have an accessor to it
        while (true) {
            if (cmap_.find(a, s)) break;
            else {
                if (cmap_.insert(a, s)) break;
            }
        }

        typename uve_table_map::iterator imapentry = a->second.find(table); 
        if (imapentry == a->second.end()) {
            std::auto_ptr<UVEMapEntry> ume(new UVEMapEntry(data.table_, seqnum));
            T::_InitDerivedStats(ume->data, dsconf);
            send = T::UpdateUVE(data, ume->data, mono_usec);
            imapentry = a->second.insert(table, ume).first;
        } else {
            send = T::UpdateUVE(data, imapentry->second->data, mono_usec);
            imapentry->second->seqno = seqnum;
        }
        if (TM != 0) {
            // If we get an update , mark this UVE so that it is not 
            // deleted during the next round of periodic processing
            imapentry->second->data.set_deleted(false);
        }
        if (data.get_deleted()) {
            a->second.erase(imapentry);
            if (a->second.empty()) cmap_.erase(a);
            lock.release();
        }
        
        return send;
    }

    // This function can be used by the Sandesh Session state machine
    // to get the seq num of the last message sent for this SandeshUVE type
    uint32_t TypeSeq(void) {
        return T::lseqnum();
    }
   
    bool InitDerivedStats(const std::map<std::string,std::string> & dsconf) {

        // Global lock is needed for iterator
        tbb::mutex::scoped_lock lock(uve_mutex_);

        // Copy the existing configuration
        // We will be replacing elements in it.
        std::map<std::string,std::string> dsnew = dsconf_;
        
        bool failure = false;
        for (map<std::string,std::string>::const_iterator n_iter = dsconf.begin();
                n_iter != dsconf.end(); n_iter++) {
            if (dsnew.find(n_iter->first) != dsnew.end()) {
                SANDESH_LOG(INFO, __func__ << " Overide DSConf for " <<
                    n_iter->first << " , " << dsnew[n_iter->first] <<
                    " with " << n_iter->second);
                dsnew[n_iter->first] = n_iter->second;
            } else {
                SANDESH_LOG(INFO, __func__ << " Cannot find DSConf for " <<
                    n_iter->first << " , " << n_iter->second);
                failure = true;
            }
        }
        
        if (failure) return false;

        // Copy the new conf into the old one if there we no errors
        dsconf_ = dsnew;

        (const_cast<SandeshUVEPerTypeMapImpl<T,U,P,TM> *>(this))->cmap_.rehash();
        for (typename uve_cmap::iterator git = cmap_.begin();
                git != cmap_.end(); git++) {
            typename uve_cmap::accessor a;
            if (!cmap_.find(a, git->first)) continue;
            for (typename uve_table_map::iterator uit = a->second.begin();
                    uit != a->second.end(); uit++) {
                SANDESH_LOG(INFO, __func__ << " Reset Derived Stats for " <<
                    git->first);
                T::_InitDerivedStats(uit->second->data, dsconf_);
            }
        }
        return true;
    }

    uint32_t SyncUVE(const std::string &table,
            SandeshUVE::SendType st,
            uint32_t seqno, uint32_t cycle,
            const std::string &ctx) {
        // Global lock is needed for iterator
        tbb::mutex::scoped_lock lock(uve_mutex_);
        uint32_t count = 0;
        (const_cast<SandeshUVEPerTypeMapImpl<T,U,P,TM> *>(this))->cmap_.rehash();
        typename uve_cmap::iterator git = cmap_.begin();
        while (git != cmap_.end()) {
            typename uve_cmap::accessor a;
            if (!cmap_.find(a, git->first)) continue;
            typename uve_table_map::iterator uit = a->second.begin();
            while (uit != a->second.end()) {
                typename uve_table_map::iterator dit = a->second.end();
                if (!table.empty() && uit->first != table) continue;
                if ((seqno < uit->second->seqno) || (seqno == 0)) {
                    if (ctx.empty()) {
                        SANDESH_LOG(INFO, __func__ << " Syncing " << uit->first << 
                                    " val " << uit->second->data.log() <<
                                    " seq " << uit->second->seqno);
                    }
                    T::Send(uit->second->data, st,
                            uit->second->seqno, cycle, ctx);
                    if ((TM != 0) && (st == SandeshUVE::ST_PERIODIC)) {
                        if ((cycle % TM) == 0) {
                            if (uit->second->data.get_deleted()) {
                                // This UVE was marked for deletion during the
                                // last periodic processing round, and there
                                // have been no UVE updates
                                dit = uit;
                            } else {
                                // Mark this UVE to be deleted during the next
                                // periodic processing round, unless it is updated
                                uit->second->data.set_deleted(true);
                            }
                        }
                    }
                    count++;
                }
                ++uit;
                if (dit != a->second.end()) a->second.erase(dit);
            }
            ++git;
            if (a->second.empty()) cmap_.erase(a);
        }
        return count;
    }

    bool SendUVE(const std::string& table, const std::string& name,
                 const std::string& ctx) const {
        bool sent = false;
        typename uve_cmap::const_accessor a;
        if (cmap_.find(a, name)) {
            for (typename uve_table_map::const_iterator uve_entry = a->second.begin();
                    uve_entry != a->second.end(); uve_entry++) {
                if (!table.empty() && uve_entry->first != table) continue;
                sent = true;
                T::Send(uve_entry->second->data,
                    (ctx.empty() ? SandeshUVE::ST_INTROSPECT : SandeshUVE::ST_SYNC),
                    uve_entry->second->seqno, 0, ctx);
            }
        }
        return sent;
    }
    
    std::map<std::string, std::string> GetDSConf(void) const {
        return dsconf_;
    }

private:

    uve_cmap cmap_;
    const std::string uve_name_;
    std::map<std::string, std::string> dsconf_;
    mutable tbb::mutex uve_mutex_;
};
#endif
