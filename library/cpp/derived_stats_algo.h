/*
 * Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
 */


// derived_stats_algo.h
// 
// This file has the implementation of 
// derived stats algorithms
//
#ifndef __DERIVED_STATS_ALGO_H__
#define __DERIVED_STATS_ALGO_H__

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <sandesh/derived_stats_results_types.h>

using std::vector;
using std::map;
using std::make_pair;
using std::string;

namespace contrail {
namespace sandesh {

template<class CatElemT, class CatResT>
class DSCategoryCount {
};

template<>
class DSCategoryCount<CategoryResult,CategoryResult> {
  public: 
    DSCategoryCount(const std::string &annotation) {}

    typedef map<string, CounterResult> CatResT;
    CatResT agg_counts_;
    CatResT diff_counts_;

    bool FillResult(CategoryResult &res) const {
        res.set_counters(diff_counts_);
        return !diff_counts_.empty();
    }
    void Update(const CategoryResult& raw) {
        diff_counts_.clear();
        for (CatResT::const_iterator it =
                raw.get_counters().begin();
                it != raw.get_counters().end(); it++) {

            // Is there anything to count?
            if (!it->second.get_count()) continue;
          
            CatResT::iterator mit =
                    agg_counts_.find(it->first);
 
            if (mit==agg_counts_.end()) {
                // This is a new category
                diff_counts_.insert(make_pair(it->first, it->second));
                agg_counts_.insert(make_pair(it->first, it->second));
            } else {
                assert(it->second.get_count() >= mit->second.get_count());
                uint64_t diff = it->second.get_count() -
                        mit->second.get_count();
                if (diff) {
                    // If the count for this category has changed,
                    // report the diff and update the aggregate
                    mit->second.set_count(diff); 
                    diff_counts_.insert(make_pair(it->first, mit->second));
                    mit->second.set_count(it->second.get_count());
                }
            }
        }
    }
};

template <typename ElemT, class SumResT> 
class DSSum {
  public:
    DSSum(const std::string &annotation): value_(0), samples_(0) {}
    ElemT value_;
    uint64_t samples_;

    bool FillResult(SumResT &res) const {
        res.set_samples(samples_);
        res.set_value(value_);
        return true;
    }
    void Update(const ElemT& raw) {
        samples_++;
        value_ += raw;
    }
};

template <typename ElemT, class EWMResT>
class DSEWM {
  public:
    DSEWM(const std::string &annotation):
                mean_(0), variance_(0), samples_(0)  { 
        alpha_ = (double) strtod(annotation.c_str(), NULL);
        assert(alpha_ > 0);
        assert(alpha_ < 1);
    }
    double alpha_;
    double mean_;
    double variance_;
    uint64_t samples_;

    bool FillResult(EWMResT &res) const {
        res.set_samples(samples_);
        res.set_mean(mean_);
        res.set_stddev(sqrt(variance_));
        return true;
    }
    void Update(const ElemT& raw) {
        samples_++;
        variance_ = (1-alpha_)*(variance_ + (alpha_*pow(raw-mean_,2)));
        mean_ = ((1-alpha_)*mean_) + (alpha_*raw);
    }
};

template <typename ElemT, class NullResT>
class DSNull {
  public:
    DSNull(const std::string &annotation): samples_(0) {}
    ElemT value_;
    uint64_t samples_;

    bool FillResult(NullResT &res) const {
        res.set_samples(samples_);
        res.set_value(value_);
        return true;
    }
    void Update(const ElemT& raw) {
        samples_++;
        value_ = raw;
    }
};

template <typename ElemT, class DiffResT>
class DSDiff {
  public:
    DSDiff(const std::string &annotation) {}
    ElemT agg_;
    ElemT diff_;

    bool FillResult(DiffResT &res) const {
        ElemT empty;
        if (diff_ == empty) return false;
        res = diff_;
        return true;
    }
    void Update(const ElemT& raw) {
        diff_ = raw - agg_;
        agg_ = agg_ + diff_;
    }
};



} // namespace sandesh
} // namespace contrail

#endif // #define __DERIVED_STATS_ALGO_H__
