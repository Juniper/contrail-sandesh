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

// This is the interface to feed in all updates to the raw stat
// on which the derived stat will be based.
// It also has a factory method to create DerivedStats objects 
template <typename ElemT>
class DerivedStatsUpdateIf {
  public:
    virtual void Update(ElemT raw) = 0;

    virtual ~DerivedStatsUpdateIf() {}
};

// This is the interface to retrieve the current value
// of the DerivedStat object.
template <typename ResultT>
class DerivedStatsResultIf {
  public:
    // Get hold of the current results
    // of the DerivedStats object
    virtual ResultT GetResult(void) = 0;
    virtual ~DerivedStatsResultIf() {}
};

// This is the base class for DerivedStats objects.
// It fills this information in the result:
// - how long has this object existed
// - how many samples it has processed
// - attribute and listkey (for non-inline derived stats)
// Other fields in the results are filled in by the specific
// Derived Stats algorithm.
template <typename ElemT, typename ResultT>
class DerivedStatsBase: 
        public DerivedStatsResultIf<ResultT>, 
        public DerivedStatsUpdateIf<ElemT> {
  private:
    const std::string attribute_;
    const std::string listkey_;
    uint64_t samples_;
    uint64_t start_time_; 

    virtual void ResultImpl(ResultT &res) = 0;
    virtual void UpdateImpl(ElemT raw, uint64_t tm) = 0;

  protected:
    // For inline DerivedStats, attribute and listkey
    // will be empty strings
    DerivedStatsBase(const std::string &attribute,
          const std::string &listkey):
        attribute_(attribute), listkey_(listkey),  
        samples_(0), start_time_(UTCTimestampUsec()) {}

  public:
    // Interface for DerivedStats Cache
    ResultT GetResult(void) {
        ResultT res;
        if (!attribute_.empty()) {
            res.set_listkey(listkey_);
            res.set_attribute(attribute_);
        }
        res.set_samples(samples_);
        res.set_uptime((UTCTimestampUsec()-start_time_)/1000);
        ResultImpl(res);
        return res; 
    }

    // Interface for generated code (from DerivedStatsInterface)
    void Update(ElemT raw) {
        samples_++;
        UpdateImpl(raw, UTCTimestampUsec());
    }

    enum DS_period { DSP_60s, DSP_3600s };

    // For non-inline derived stats.
    // The function will register the DerivedStatsResultIf to 
    // the DerivedStatsCache.
    // TODO: Needs implementation
    static boost::shared_ptr<DerivedStatsBase<ElemT, ResultT> > Create(
            DS_period ds_period, 
            std::string annotation,
            const std::string &uve_table, const std::string &uve_key,
            const std::string &attribute, const std::string &listkey) {
        return boost::shared_ptr<DerivedStatsBase<ElemT, ResultT> >();
    }

    // For inline derived stats. Call from generated sandesh code.
    static boost::shared_ptr<DerivedStatsBase<ElemT, ResultT> > Create(
            std::string annotation);
  
};

#endif

