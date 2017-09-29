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
#include <boost/assign/ptr_map_inserter.hpp>
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
    static uint32_t Clear(const std::string& proxy, int partition);
    static bool InitDerivedStats(
        const std::map<std::string, ds_conf_elem> &);
    static void SyncIntrospect(std::string tname, std::string table, std::string key);

    static uve_global_map::const_iterator Begin() { return GetMap()->begin(); }
    static uve_global_map::const_iterator End() { return GetMap()->end(); }
    static const int kProxyPartitions = 30;
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
    virtual uint32_t TypeSeq() const = 0;
    virtual uint32_t SyncUVE(const std::string &table, SandeshUVE::SendType st,
            uint32_t seqno, uint32_t cycle, const std::string &ctx) = 0;
    virtual bool InitDerivedStats(
            const std::map<std::string,std::string> & dsconf) = 0;
    virtual bool SendUVE(const std::string& table, const std::string& name,
            const std::string& ctx) const = 0;
    virtual std::map<std::string, std::string> GetDSConf(void) const = 0;
    virtual int GetTimeout(void) const = 0;
    virtual uint32_t ClearUVEs(const std::string& proxy, int partition) = 0;
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


template<typename T, typename U, int P, int TM>
class SandeshUVEPerTypeMapImpl {
public:

    struct HashCompare { 
        static size_t hash( const std::string& key )
            { return boost::hash_value(key); } 
        static bool   equal( const std::string& key1, const std::string& key2 )
            { return ( key1 == key2 ); } 
    }; 

    struct UVEMapEntry {
        UVEMapEntry(const std::string &table, uint32_t seqnum,
                    SandeshLevel::type level):
                data(table), seqno(seqnum), level(level) {
        }
        U data;
        uint32_t seqno;
        SandeshLevel::type level;
    };

    // The key is the table name
    typedef boost::ptr_map<std::string, UVEMapEntry> uve_table_map;

    // The key is the UVE-Key
    typedef tbb::concurrent_hash_map<std::string, uve_table_map, HashCompare > uve_cmap;

    SandeshUVEPerTypeMapImpl() : 
            dsconf_(T::_DSConf()) {}

    // This function is called whenever a SandeshUVE is sent from
    // the generator to the collector.
    // It updates the cache.
    bool UpdateUVE(U& data, uint32_t seqnum, uint64_t mono_usec,
                   SandeshLevel::type level) {
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
            std::auto_ptr<UVEMapEntry> ume(new
                    UVEMapEntry(data.table_, seqnum, level));
            T::_InitDerivedStats(ume->data, dsconf);
            send = T::UpdateUVE(data, ume->data, mono_usec, level);
            imapentry = a->second.insert(table, ume).first;
        } else {
            if (TM != 0) {
                // If we get an update , mark this UVE so that it is not 
                // deleted during the next round of periodic processing
                imapentry->second->data.set_deleted(false);
            }
            send = T::UpdateUVE(data, imapentry->second->data, mono_usec,
                                level);
            imapentry->second->seqno = seqnum;
        }
        if (data.get_deleted()) {
            a->second.erase(imapentry);
            if (a->second.empty()) cmap_.erase(a);
            lock.release();
        }
        
        return send;
    }

    // Clear all UVEs in this cache
    // This is used ONLY with proxy groups
    uint32_t ClearUVEs(void) {

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
                SANDESH_LOG(INFO, __func__ << " Clearing " << uit->first << 
                    " val " << uit->second->data.log() << " proxy " <<
                    SandeshStructProxyTrait<U>::get(uit->second->data) << 
                    " seq " << uit->second->seqno);
                uit->second->data.set_deleted(true);
                T::Send(uit->second->data, uit->second->level,
                        SandeshUVE::ST_SYNC, uit->second->seqno, 0, "");
                count++;
                ++uit;
            }
            ++git;
        }
        cmap_.clear();
        return count;
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
                            " val " << uit->second->data.log() << " proxy " <<
                            SandeshStructProxyTrait<U>::get(uit->second->data) << 
                            " seq " << uit->second->seqno);
                    }
                    T::Send(uit->second->data, uit->second->level, st,
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
                T::Send(uve_entry->second->data, uve_entry->second->level,
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
    std::map<std::string, std::string> dsconf_;
    mutable tbb::mutex uve_mutex_;
};

#define SANDESH_UVE_DEF(x,y,z,w) \
    static SandeshUVEPerTypeMapGroup<x, y, z, w> uvemap ## x(#y)

template<typename T, typename U, int P, int TM>
class SandeshUVEPerTypeMapGroup: public SandeshUVEPerTypeMap {
public:
    SandeshUVEPerTypeMapGroup(char const * u_name) {
        SandeshUVETypeMaps::RegisterType(u_name, this, P);
    }
    
    typedef SandeshUVEPerTypeMapImpl<T,U,P,TM> uve_emap;

    // One UVE Type Map per partition
    typedef boost::ptr_map<int, uve_emap> uve_pmap;

    // One set of per-partition UVE Type Maps for each proxy group
    typedef boost::ptr_map<string, uve_pmap> uve_gmap;

    // Get the partition UVE Type maps for the given proxy group
    uve_pmap * GetGMap(const std::string& proxy) {
        tbb::mutex::scoped_lock lock(gmutex_);
        if (group_map_.find(proxy) == group_map_.end()) {
            uve_pmap* up = new uve_pmap;
            std::string kstring(proxy);
            for (size_t idx=0; idx<SandeshUVETypeMaps::kProxyPartitions; idx++) {
                boost::assign::ptr_map_insert(*up)(idx);
            }
            group_map_.insert(kstring ,up);
        }
        return &(group_map_.at(proxy));
    }
   
    // Get the partition UVE Type maps for all proxy groups 
    std::vector<uve_pmap *> GetGMaps(void) {
        tbb::mutex::scoped_lock lock(gmutex_);
        std::vector<uve_pmap *> pv;
        for (typename uve_gmap::iterator ugi = group_map_.begin();
                ugi != group_map_.end(); ugi++) {
            pv.push_back(ugi->second);
        }
        return pv;
    }

    // This function can be used by the Sandesh Session state machine
    // to get the seq num of the last message sent for this SandeshUVE type
    uint32_t TypeSeq(void) const {
        return T::lseqnum();
    }

    int GetTimeout(void) const {
        return TM;
    }

    // Sync UVEs for both the native UVE Map and the proxy groups
    uint32_t SyncUVE(const std::string &table,
            SandeshUVE::SendType st,
            uint32_t seqno, uint32_t cycle,
            const std::string &ctx) {
        uint32_t count=0;
        count += native_map_.SyncUVE(table, st, seqno, cycle, ctx);
        std::vector<uve_pmap *> pv = GetGMaps();
        
        for (size_t jdx=0; jdx<pv.size(); jdx++) {
            for (size_t idx=0; idx<SandeshUVETypeMaps::kProxyPartitions; idx++) {
                count += pv[jdx]->at(idx).SyncUVE(table, st, seqno, cycle, ctx);
            }
        }
        return count; 
    }

    // DerivedStats for proxy groups cannot be re-configured
    // at InitGenerator time or from Introspect
    // The Proxy group configuration should be changed instead
    bool InitDerivedStats(
            const std::map<std::string,std::string> & dsconf) {
        return native_map_.InitDerivedStats(dsconf);
    }

    // Send the given UVE, for the native UVE Type map, as
    // well as proxy groups
    bool SendUVE(const std::string& table, const std::string& name,
            const std::string& ctx) const {
        bool sent = false;
        if (native_map_.SendUVE(table, name, ctx)) sent = true;
        std::vector<uve_pmap *> pv =
                const_cast<SandeshUVEPerTypeMapGroup<T,U,P,TM> * >(this)->GetGMaps();
        for (size_t jdx=0; jdx<pv.size(); jdx++) {
            for (size_t idx=0; idx<SandeshUVETypeMaps::kProxyPartitions; idx++) {
                if (pv[jdx]->at(idx).SendUVE(table, name, ctx)) sent = true;
            }
        }
        return sent;
    }

    std::map<std::string, std::string> GetDSConf(void) const {
        return native_map_.GetDSConf();
    }

    bool UpdateUVE(U& data, uint32_t seqnum,
            uint64_t mono_usec, int partition, SandeshLevel::type level) {
        if (partition == -1) {
            return native_map_.UpdateUVE(data, seqnum, mono_usec, level);
        } else {
            std::string proxy = SandeshStructProxyTrait<U>::get(data);
            assert(partition < SandeshUVETypeMaps::kProxyPartitions);
            uve_pmap * pp = GetGMap(proxy);
            return pp->at(partition).UpdateUVE(data, seqnum, mono_usec, level);
        }
    }

    // Delete all UVEs for the given partition for the given proxy group
    uint32_t ClearUVEs(const std::string& proxy, int partition) {
        tbb::mutex::scoped_lock lock(gmutex_);
        typename uve_gmap::iterator gi = group_map_.find(proxy);
        if (gi != group_map_.end()) {
            assert(partition < SandeshUVETypeMaps::kProxyPartitions);
            return gi->second->at(partition).ClearUVEs();
        }
        return 0;
    }

private:
    mutable tbb::mutex gmutex_;
    uve_gmap group_map_;
    SandeshUVEPerTypeMapImpl<T, U, P, TM> native_map_;
};

#endif
