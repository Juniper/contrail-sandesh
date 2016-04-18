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
#include <boost/function.hpp>
#include <base/time_util.h>

// This is the base class for all DerivedStats algorithm implementations
template <typename ElemT, typename ResultT>
class DerivedStatsImpl {
  public:
    static boost::shared_ptr<DerivedStatsImpl<ElemT, ResultT> > Create(
    std::string annotation);

    void FillResult(ResultT &res, bool flush) {
        res.set_samples(samples_);
        ResultImpl(res, flush);
        if (flush) samples_ = 0;
    }
    void Update(ElemT raw) {
        samples_++;
        UpdateImpl(raw);
    }

  protected:
    DerivedStatsImpl() : samples_(0) {}
  private:
    uint64_t samples_;
    virtual void ResultImpl(ResultT &res, bool flush) = 0;
    virtual void UpdateImpl(ElemT raw) = 0;
};

template <typename ElemT, typename ResultT>
class DerivedStatsIf {
  private:
    typedef std::map<std::string, boost::shared_ptr<DerivedStatsImpl<ElemT,ResultT> > > result_map;
    result_map dsm_;
    boost::shared_ptr<DerivedStatsImpl<ElemT,ResultT> > ds_;
    std::string annotation_;

  public:
    DerivedStatsIf(std::string annotation):
        annotation_(annotation),
        ds_(DerivedStatsImpl<ElemT,ResultT>::Create(annotation)) {}

    // This is the interface to retrieve the current value
    // of the DerivedStat object.
    void FillResult(ResultT &res, bool flush = false) {
        ds_->FillResult(res, flush);
    }

    void FillResult(std::map<std::string, ResultT> &mres, bool flush = false) {
        mres.clear(); 
        for (typename result_map::const_iterator dit = dsm_.begin(); dit != dsm_.end(); dit++) {
            ResultT res;
            dit->second->FillResult(res, flush);
            mres.insert(std::make_pair(dit->first, res));
        }
    }

    // This is the interface to feed in all updates to the raw stat
    // on which the derived stat will be based.
    void Update(ElemT raw) {
        ds_->Update(raw);
    }
    
    void Update(const std::map<std::string, ElemT> & raw) {
        std::map<std::string, bool> srcmap;
        typename std::map<std::string, ElemT>::const_iterator rit;
        for(rit=raw.begin(); rit!= raw.end(); rit++) {
            srcmap[rit->first] = false;
        }
        // For all cache entries that are staying, do an update
        typename result_map::iterator dt = dsm_.begin();
        while (dt != dsm_.end()) {
            std::map<std::string, bool>::iterator st =
                srcmap.find(dt->first);
            if (st != srcmap.end()) {
                // This cache entry IS in the new vector
                st->second = true;
                //dt->second->Update(raw[st->first]);
                dt->second->Update((raw.find(st->first))->second);
                ++dt;
            } else {
                // This cache entry IS NOT in the new vector
                typename result_map::iterator et = dt;
                dt++;
                dsm_.erase(et);
            }
        }

        // Now add all new entries
        std::map<std::string, bool>::iterator mt;
        for (mt = srcmap.begin(); mt != srcmap.end(); mt++) {
            if (mt->second == false) {
                dsm_[mt->first] = DerivedStatsImpl<ElemT,ResultT>::Create(annotation_);
                //dsm_[mt->first]->Update(raw[mt->first]);
                dsm_[mt->first]->Update((raw.find(mt->first))->second);
            }
        }
    }
};

#endif

