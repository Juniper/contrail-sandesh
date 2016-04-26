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

    void FillResult(ResultT &res) const {
        res.set_samples(samples_);
        ResultImpl(res);
    }
    void Update(ElemT raw) {
        samples_++;
        UpdateImpl(raw);
    }

  protected:
    DerivedStatsImpl() : samples_(0) {}
  private:
    uint64_t samples_;
    virtual void ResultImpl(ResultT &res) const = 0;
    virtual void UpdateImpl(ElemT raw) = 0;
};

template<typename ElemT, typename ResultT> void DerivedStatsMerge(
        const std::map<std::string, ElemT> & raw,
        std::map<std::string, boost::shared_ptr<DerivedStatsImpl<ElemT,ResultT> > > & dsm,
        std::string annotation) {

    std::map<std::string, bool> srcmap;
    typename std::map<std::string, ElemT>::const_iterator rit;

    for(rit=raw.begin(); rit!= raw.end(); rit++) {
        srcmap[rit->first] = false;
    }
    // For all cache entries that are staying, do an update
    typename std::map<std::string,
            boost::shared_ptr<DerivedStatsImpl<
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
                    boost::shared_ptr<DerivedStatsImpl<
                    ElemT,ResultT> > >::iterator et = dt;
            dt++;
            dsm.erase(et);
        }
    }

    // Now add all new entries
    std::map<std::string, bool>::iterator mt;
    for (mt = srcmap.begin(); mt != srcmap.end(); mt++) {
        if (mt->second == false) {
            dsm[mt->first] = DerivedStatsImpl<ElemT,ResultT>::Create(annotation);
            dsm[mt->first]->Update((raw.find(mt->first))->second);
        }
    }
}

template <typename ElemT, typename ResultT>
class DerivedStatsIf {
  private:
    typedef std::map<std::string, boost::shared_ptr<DerivedStatsImpl<ElemT,ResultT> > > result_map;
    boost::shared_ptr<result_map> dsm_;

    boost::shared_ptr<DerivedStatsImpl<ElemT,ResultT> > ds_;
    
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
        } else isset = true;
        ds_->FillResult(res);
    }

    void FillResult(std::map<std::string, ResultT> &mres, bool& isset) const {
        if (!dsm_) {
            isset = false;
            return;
        } else isset = true;

        mres.clear(); 
        for (typename result_map::const_iterator dit = (*dsm_).begin(); dit != (*dsm_).end(); dit++) {
            ResultT res;
            dit->second->FillResult(res);
            mres.insert(std::make_pair(dit->first, res));
        }
    }

    // This is the interface to feed in all updates to the raw stat
    // on which the derived stat will be based.
    void Update(ElemT raw) {
        if (!ds_) {
            ds_ = DerivedStatsImpl<ElemT,ResultT>::Create(annotation_);
        }
        ds_->Update(raw);
    }
    
    void Update(const std::map<std::string, ElemT> & raw) {
        if (!dsm_) {
            dsm_.reset(new result_map());
        }
        DerivedStatsMerge<ElemT,ResultT>(raw, *dsm_, annotation_);
    }
};

template <typename ElemT, typename SubResultT, typename ResultT>
class DerivedStatsPeriodicIf {
  private:
    typedef std::map<std::string, boost::shared_ptr<DerivedStatsImpl<ElemT,SubResultT> > > result_map;
    boost::shared_ptr<result_map> dsm_;
    boost::shared_ptr< std::map<std::string,ResultT> > dsm_cache_;

    boost::shared_ptr<DerivedStatsImpl<ElemT,SubResultT> > ds_;
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
            ds_ = DerivedStatsImpl<ElemT,SubResultT>::Create(annotation_);
        }
        ds_->Update(raw);
    }

    void Update(const std::map<std::string, ElemT> & raw) {
        if (!dsm_) {
            dsm_.reset(new result_map());
        }
        DerivedStatsMerge<ElemT,SubResultT>(raw, *dsm_, annotation_);
    }

    bool Flush(const ResultT &res) {
        (void)res;
        if (!ds_) {
            // There were no updates to the DerivedStat
            // since the last flush
            ds_cache_.reset();
            return false;
        }
        ds_cache_.reset(new ResultT());
        ds_->FillResult(ds_cache_->previous);
        ds_cache_->__isset.previous = true;

        // Clear the DerivedStat
        ds_.reset();

        return true;
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
            ds_->FillResult(res.current);
            res.__isset.current = true;
            isset = true;
        }
    }

    bool Flush(const std::map<std::string, ResultT> &mres) {
        (void)mres;
        if (!dsm_) {
            // There were no updates to the DerivedStats
            // since the last flush
            dsm_cache_.reset();
            return false;
        }
        dsm_cache_.reset(new std::map<std::string,ResultT>());
        for (typename result_map::const_iterator dit = (*dsm_).begin();
                dit != (*dsm_).end(); dit++) {
            ResultT res;
            dit->second->FillResult(res.previous);
            res.__isset.previous = true;
            (*dsm_cache_).insert(std::make_pair(dit->first, res));
        }

        // Clear the DerivedStats
        dsm_.reset();
        return true;
    }

    // This is the interface to retrieve the current value
    // of the DerivedStat object.
    void FillResult(std::map<std::string, ResultT> &mres, bool& isset) const {
        mres.clear();
        isset = false;
        if (dsm_cache_) {
            // Fill in previous information
            for (typename std::map<std::string,ResultT>::const_iterator dit = (*dsm_cache_).begin();
                    dit != (*dsm_cache_).end(); dit++) {
                ResultT res;
                res.previous = dit->second.previous;
                res.__isset.previous = true;
                mres.insert(std::make_pair(dit->first, res));
            }
            isset = true;
        }
        if (dsm_) {
            for (typename result_map::const_iterator dit = (*dsm_).begin();
                    dit != (*dsm_).end(); dit++) {

                // If this derived stat had a previous value, merge the current into it.
                typename std::map<std::string, ResultT>::iterator wit =
                        mres.find(dit->first);
                if (wit != mres.end()) {
                    dit->second->FillResult(wit->second.current);
                    wit->second.__isset.current = true;
                } else {
                    ResultT res;
                    dit->second->FillResult(res.current);
                    res.__isset.current = true;
                    mres.insert(std::make_pair(dit->first, res));
                }
            }
            isset = true;
        }
    }

};

#endif

