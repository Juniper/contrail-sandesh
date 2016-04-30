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

namespace contrail {
namespace sandesh {

template<template<class,class> class DSTT, typename ElemT, typename ResultT>
void DerivedStatsMerge(const std::map<std::string, ElemT> & raw,
        std::map<std::string, boost::shared_ptr<DSTT<ElemT,ResultT> > > & dsm,
        std::string annotation, bool periodic) {

    std::map<std::string, bool> srcmap;
    typename std::map<std::string, ElemT>::const_iterator rit;

    for(rit=raw.begin(); rit!= raw.end(); rit++) {
        srcmap[rit->first] = false;
    }
    // For all cache entries that are staying, do an update
    typename std::map<std::string,
            boost::shared_ptr<DSTT<
            ElemT,ResultT> > >::iterator dt = dsm.begin();
    while (dt != dsm.end()) {
        std::map<std::string, bool>::iterator st =
            srcmap.find(dt->first);
        if (st != srcmap.end()) {
            // This cache entry IS in the new vector
            st->second = true;
            dt->second->Update((raw.find(st->first))->second);
            ++dt;
        } else {
            // This cache entry IS NOT in the new vector
            typename std::map<std::string,
                    boost::shared_ptr<DSTT<
                    ElemT,ResultT> > >::iterator et = dt;
            dt++;
            // For peridoic stats, un-updated cache entries
            // will get removed during flush
            if (!periodic) dsm.erase(et);
        }
    }

    // Now add all new entries
    std::map<std::string, bool>::iterator mt;
    for (mt = srcmap.begin(); mt != srcmap.end(); mt++) {
        if (mt->second == false) {
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

  public:
    DerivedStatsIf(std::string annotation):
        annotation_(annotation) {}

    bool IsResult(void) const {
        if ((!ds_) && (!dsm_)) return false;
        else return true;
    }

    // This is the interface to retrieve the current value
    // of the DerivedStat object.
    void FillResult(ResultT &res, bool& isset) const {
        if (!ds_) {
            isset = false;
            return;
        } else isset = ds_->FillResult(res);
    }

    void FillResult(std::map<std::string, ResultT> &mres, bool& isset) const {
        mres.clear(); 
        if (dsm_) {
            for (typename result_map::const_iterator dit = dsm_->begin();
                    dit != dsm_->end(); dit++) {
                ResultT res;
                if (dit->second->FillResult(res)) {
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
        }
        ds_->Update(raw);
    }
    
    void Update(const std::map<std::string, ElemT> & raw) {
        if (!dsm_) {
            dsm_ = boost::make_shared<result_map>();
        }
        DerivedStatsMerge<DSTT,ElemT,ResultT>(raw, *dsm_, annotation_, false);
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

  public:
    DerivedStatsPeriodicIf(std::string annotation):
        annotation_(annotation) {}

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
        ds_->Update(raw);
    }

    void Update(const std::map<std::string, ElemT> & raw) {
        if (!dsm_) {
            dsm_ = boost::make_shared<result_map >();
        }
        DerivedStatsMerge<DSTT,ElemT,SubResultT>(raw, *dsm_, annotation_, true);
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
        if (ds_->FillResult(ds_cache_->previous)) {
            ds_cache_->__isset.previous = true;
        } else ds_cache_.reset();

        // Clear the DerivedStat
        ds_.reset();

        return ds_cache_;
    }

    // This is the interface to retrieve the current value
    // of the DerivedStat object.
    void FillResult(ResultT &res, bool& isset) const {
        isset = false;
        if (ds_cache_) {
            // Fill in previous information
            res.previous = ds_cache_->previous;
            res.__isset.previous = true;
            isset = true;
        }
        if (ds_) {
            // Fill in current information
            if (ds_->FillResult(res.current)) {
                res.__isset.current = true;
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
            if (dit->second->FillResult(res.previous)) {
                res.__isset.previous = true;
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
    void FillResult(std::map<std::string, ResultT> &mres, bool& isset) const {
        mres.clear();
        if (dsm_cache_) {
            // Fill in previous information
            for (typename std::map<std::string,ResultT>::const_iterator dit = dsm_cache_->begin();
                    dit != dsm_cache_->end(); dit++) {
                ResultT res;
                res.previous = dit->second.previous;
                res.__isset.previous = true;
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
                    if (dit->second->FillResult(wit->second.current)) {
                        wit->second.__isset.current = true;
                    }
                } else {
                    ResultT res;
                    if (dit->second->FillResult(res.current)) {
                        res.__isset.current = true;
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

