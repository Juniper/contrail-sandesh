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
using std::vector;
using std::map;
using std::make_pair;
using std::string;

namespace contrail {
namespace sandesh {

template<class CatSubElemVT, class CatResT>
class DSCategoryCount {
  public: 
    DSCategoryCount(const std::string &annotation): samples_(0) {}

    map<string, CatSubElemVT> agg_counts_;
    CatSubElemVT diff_counts_;
    uint64_t samples_;

    bool FillResult(CatResT &res) const {
        res.set_samples(samples_);
        res.set_results(diff_counts_);
        return !diff_counts_.empty();
    }
    void Update(const CatSubElemVT& raw) {
        samples_++;
        diff_counts_.clear();
        for (typename CatSubElemVT::const_iterator it = raw.begin();
                it != raw.end(); it++) {

            // Is there anything to count?
            if (!it->get_count()) continue;
            
            typename map<string, CatSubElemVT>::iterator mit = agg_counts_.find(it->category);
            if (mit==agg_counts_.end()) {
                // This is a new category
                diff_counts_.push_back(*it);
                CatSubElemVT csev;
                csev.push_back(*it);
                agg_counts_.insert(make_pair(it->get_category(), csev)); 
            } else {
                assert(it->get_count() >= mit->second[0].get_count());
                uint64_t diff = it->get_count() - mit->second[0].get_count();
                if (diff) {
                    // If the count for this category has changed,
                    // report the diff and update the aggregate
                    mit->second[0].set_count(it->get_count());
                    diff_counts_.push_back(mit->second[0]);
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

} // namespace sandesh
} // namespace contrail

#endif // #define __DERIVED_STATS_ALGO_H__
