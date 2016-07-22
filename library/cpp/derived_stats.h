/*
 * Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
 */

//
// derived_stats.h
// 
// This file has the interface for derived stats
// towards the sandesh generated code, and
// the derived stats base class
//

#ifndef __DERIVED_STATS_H__
#define __DERIVED_STATS_H__

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <base/time_util.h>

template <typename T>
class SandeshStructDeleteTrait;

namespace contrail {
namespace sandesh {

template<typename ElemT>
std::map<std::string, ElemT> DerivedStatsAgg(const std::map<std::string, ElemT> & raw,
        std::map<std::string, ElemT> & agg,
        const std::map<std::string, bool> &del) {

    std::map<std::string, ElemT> diff;

    typename std::map<std::string, ElemT>::const_iterator rit;
    typename std::map<std::string, ElemT>::iterator ait;
   
    // Go through all raw elements to generate diff and update agg 
    for (rit=raw.begin(); rit!= raw.end(); rit++) {
        ait=agg.find(rit->first);
        // The aggregate already has this element
        if (ait!=agg.end()) {
            diff[rit->first] = rit->second - ait->second;
            // Remove from agg if this element is marked for deletion
            // (It will still be in the diff)
            if (!del.at(rit->first)) ait->second = ait->second + diff[rit->first];
            else agg.erase(ait);
        } else {
            diff[rit->first] = rit->second;
            if (!del.at(rit->first)) agg[rit->first] = diff[rit->first]; 
        }
    }
    return diff;
}  

 
template<template<class,class> class DSTT, typename ElemT, typename ResultT>
void DerivedStatsMerge(const std::map<std::string, ElemT> & raw,
        std::map<std::string, boost::shared_ptr<DSTT<ElemT,ResultT> > > & dsm,
        std::string annotation, const std::map<std::string, bool> &del) {

    std::map<std::string, bool> srcmap;
    typename std::map<std::string, ElemT>::const_iterator rit;
    std::map<std::string, bool>::const_iterator dlt;

    // If the new map is empty, clear all DS objects
    if (raw.empty()) {
        dsm.clear();
        return;
    }

    for(rit=raw.begin(); rit!= raw.end(); rit++) {
        srcmap[rit->first] = false;
    }

    // Go through all existing DS object
    typename std::map<std::string,
            boost::shared_ptr<DSTT<
            ElemT,ResultT> > >::iterator dt = dsm.begin();
    while (dt != dsm.end()) {
        rit = raw.find(dt->first);
        dlt = del.find(dt->first);
        if (rit != raw.end()) {
            assert(dlt!=del.end());
            // we have information about this element
            srcmap[rit->first] = true;
            if (dlt->second) {
                // raw element is requesting deletion
                typename std::map<std::string,
                        boost::shared_ptr<DSTT<
                        ElemT,ResultT> > >::iterator et = dt;
                dt++;
                dsm.erase(et);
            } else {
                dt->second->Update(rit->second);
                ++dt;
            }
        } else {
            // We have no information about this element
            ++dt;
        }
    }

    // Now add all new entries that are not requesting deletion
    std::map<std::string, bool>::iterator mt;
    for (mt = srcmap.begin(); mt != srcmap.end(); mt++) {
        rit = raw.find(mt->first);
        dlt = del.find(mt->first);
        if ((mt->second == false) && (!dlt->second)) {
            assert(dsm.find(mt->first) == dsm.end());
            dsm[mt->first] = boost::make_shared<DSTT<ElemT,ResultT> >(annotation);
            dsm[mt->first]->Update((raw.find(mt->first))->second);
        }
    }
}

template <template<class,class> class DSTT, typename ElemT, typename ResultT>
class DerivedStatsIf {
  private:
    typedef std::map<std::string, boost::shared_ptr<DSTT<ElemT,ResultT> > > result_map;

    boost::shared_ptr<result_map> dsm_;

    boost::shared_ptr<DSTT<ElemT,ResultT> > ds_;
    
    std::string annotation_;
    
    bool is_agg_;
    ElemT agg_;
    std::map<std::string, ElemT> aggm_;
    ElemT diff_;
    std::map<std::string, ElemT> diffm_;

    
  public:
    DerivedStatsIf(std::string annotation, bool is_agg = false):
        annotation_(annotation), is_agg_(is_agg) {}

    bool IsResult(void) const {
        if ((!ds_) && (!dsm_)) return false;
        else return true;
    }

    // This is the interface to retrieve the current value
    // of the DerivedStat object.
    void FillResult(ResultT &res, bool& isset, bool force=false) const {
        isset = false;
        if (ds_) {
            if (ds_->FillResult(res) || force) {
                isset = true;
            }
        }
    }

    void FillResult(std::map<std::string, ResultT> &mres, bool& isset, bool force=false) const {
        mres.clear(); 
        if (dsm_) {
            for (typename result_map::const_iterator dit = dsm_->begin();
                    dit != dsm_->end(); dit++) {
                ResultT res;
                if (dit->second->FillResult(res) || force) {
                    mres.insert(std::make_pair(dit->first, res));
                }
            }
        }
        isset = !mres.empty();
        return;
    }

    // This is the interface to feed in all updates to the raw stat
    // on which the derived stat will be based.
    void Update(ElemT raw) {
        if (!ds_) {
            ds_ = boost::make_shared<DSTT<ElemT,ResultT> >(annotation_);
            if (is_agg_) {
                diff_ = raw;
                agg_ = raw;
            }
        } else {
            if (is_agg_) {
                diff_ = raw - agg_;
                agg_ = agg_ + diff_;
            }
        }
        if (is_agg_) ds_->Update(diff_);
        else ds_->Update(raw);
    }
    
    void Update(const std::map<std::string, ElemT> & raw, const std::map<std::string, bool> &del) {
        if (!dsm_) {
            dsm_ = boost::make_shared<result_map>();
        }
        if (is_agg_) {
            diffm_ = DerivedStatsAgg<ElemT>(raw, aggm_, del);
            DerivedStatsMerge<DSTT,ElemT,ResultT>(diffm_, *dsm_, annotation_, del);
        } else {
            DerivedStatsMerge<DSTT,ElemT,ResultT>(raw, *dsm_, annotation_, del);
        }
    }
};

template <template<class,class> class DSTT, typename ElemT,
typename SubResultT, typename ResultT>
class DerivedStatsPeriodicIf {
  private:
    typedef std::map<std::string, boost::shared_ptr<DSTT<ElemT,SubResultT> > > result_map;
    boost::shared_ptr<result_map> dsm_;
    boost::shared_ptr< std::map<std::string,ResultT> > dsm_cache_;

    boost::shared_ptr<DSTT<ElemT,SubResultT> > ds_;
    boost::shared_ptr<ResultT> ds_cache_;
    
    std::string annotation_;

    bool is_agg_;
    bool init_;
    ElemT agg_;
    std::map<std::string, ElemT> aggm_;
    ElemT diff_;
    std::map<std::string, ElemT> diffm_;

  public:
    DerivedStatsPeriodicIf(std::string annotation, bool is_agg=false):
        annotation_(annotation), is_agg_(is_agg), init_(false) {}

    bool IsResult(void) const {
        if (ds_ || ds_cache_) return true;
        if (dsm_ || dsm_cache_) return true;
        return false;
    }

    // This is the interface to feed in all updates to the raw stat
    // on which the derived stat will be based.
    void Update(ElemT raw) {
        if (!ds_) {
            ds_ = boost::make_shared<DSTT<ElemT,SubResultT> >(annotation_);
        }
        if (is_agg_) {
            if (!init_) {
                diff_ = raw;
                agg_ = raw;
            } else {
                diff_ = raw - agg_;
                agg_ = agg_ + diff_;
            }
            ds_->Update(diff_);
        } else ds_->Update(raw);
        init_ = true;
    }

    void Update(const std::map<std::string, ElemT> & raw, const std::map<std::string, bool> &del) {
        if (!dsm_) {
            dsm_ = boost::make_shared<result_map>();
        }
        if (is_agg_) {
            diffm_ = DerivedStatsAgg<ElemT>(raw, aggm_, del);
            DerivedStatsMerge<DSTT,ElemT,SubResultT>(diffm_, *dsm_, annotation_, del);
        } else DerivedStatsMerge<DSTT,ElemT,SubResultT>(raw, *dsm_, annotation_, del);
        init_ = true;
    }

    bool Flush(const ResultT &res) {
        (void)res;
        if (!ds_) {
            // There were no updates to the DerivedStat
            // since the last flush
            ds_cache_.reset();
            return ds_cache_;
        }
        ds_cache_ = boost::make_shared<ResultT>();
        if (ds_->FillResult(ds_cache_->value)) {
            ds_cache_->__isset.value = true;
        } else ds_cache_.reset();

        // Clear the DerivedStat
        ds_.reset();

        return ds_cache_;
    }

    // This is the interface to retrieve the current value
    // of the DerivedStat object.
    void FillResult(ResultT &res, bool& isset, bool force=false) const {
        isset = false;
        if (ds_cache_) {
            // Fill in previous information
            res.value = ds_cache_->value;
            res.__isset.value = true;
            isset = true;
        }
        if (ds_) {
            // Fill in current information
            if (ds_->FillResult(res.staging) || force) {
                res.__isset.staging = true;
                isset = true;
            }
        }
    }

    bool Flush(const std::map<std::string, ResultT> &mres) {
        (void)mres;
        if (!dsm_) {
            // There were no updates to the DerivedStats
            // since the last flush
            dsm_cache_.reset();
            return dsm_cache_;
        }
        dsm_cache_ = boost::make_shared<std::map<std::string,ResultT> >();
        for (typename result_map::const_iterator dit = dsm_->begin();
                dit != dsm_->end(); dit++) {
            ResultT res;
            if (dit->second->FillResult(res.value)) {
                res.__isset.value = true;
                dsm_cache_->insert(std::make_pair(dit->first, res));
            }
        }
        if (dsm_cache_->empty()) dsm_cache_.reset();

        // Clear the DerivedStats
        dsm_.reset();
        return dsm_cache_;
    }

    // This is the interface to retrieve the current value
    // of the DerivedStat object.
    void FillResult(std::map<std::string, ResultT> &mres, bool& isset, bool force=false) const {
        mres.clear();
        if (dsm_cache_) {
            // Fill in previous information
            for (typename std::map<std::string,ResultT>::const_iterator dit = dsm_cache_->begin();
                    dit != dsm_cache_->end(); dit++) {
                ResultT res;
                res.value = dit->second.value;
                res.__isset.value = true;
                mres.insert(std::make_pair(dit->first, res));
            }
        }
        if (dsm_) {
            for (typename result_map::const_iterator dit = (*dsm_).begin();
                    dit != (*dsm_).end(); dit++) {

                // If this derived stat had a previous value, merge the current into it.
                typename std::map<std::string, ResultT>::iterator wit =
                        mres.find(dit->first);
                if (wit != mres.end()) {
                    if (dit->second->FillResult(wit->second.staging) || force) {
                        wit->second.__isset.staging = true;
                    }
                } else {
                    ResultT res;
                    if (dit->second->FillResult(res.staging) || force) {
                        res.__isset.staging = true;
                        mres.insert(std::make_pair(dit->first, res));
                    }
                }
            }
        }
        isset = !mres.empty();
    }

};

} // namespace sandesh
} // namespace contrail
#endif

