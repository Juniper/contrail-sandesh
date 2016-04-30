/*
 * Copyright (c) 2016 Juniper Networks, Inc. All rights reserved.
 */


// derived_stats.cpp
// 
// This file has the implementation of 
// derived stats factory methods
//

#include <sandesh/derived_stats.h>
#include <sandesh/derived_stats_results_types.h>
#include <iostream>
using std::vector;
using std::map;
using std::make_pair;
using std::string;

class DerivedStatsCategoryCounter:
      public DerivedStatsImpl< vector<CategoryResultSubElem>, CategoryResult> {
  public:
    DerivedStatsCategoryCounter(const std::string &annotation):
            DerivedStatsImpl< vector<CategoryResultSubElem>, CategoryResult>() {}
  private:
    map<string, CategoryResultSubElem> agg_counts_;
    vector<CategoryResultSubElem> diff_counts_;

  protected:
    bool ResultImpl(CategoryResult &res) const {
        res.set_results(diff_counts_);
        return !diff_counts_.empty();
    }
    void UpdateImpl(vector<CategoryResultSubElem> raw) {
        diff_counts_.clear();
        for (vector<CategoryResultSubElem>::const_iterator it = raw.begin();
                it != raw.end(); it++) {

            // Is there anything to count?
            if (!it->get_count()) continue;
            
            map<string, CategoryResultSubElem>::iterator mit = agg_counts_.find(it->category);
            if (mit==agg_counts_.end()) {
                // This is a new category
                diff_counts_.push_back(*it);
                agg_counts_.insert(make_pair(it->get_category(), *it)); 
            } else {
                assert(it->get_count() >= mit->second.get_count());
                uint64_t diff = it->get_count() - mit->second.get_count();
                if (diff) {
                    // If the count for this category has changed,
                    // report the diff and update the aggregate
                    CategoryResultSubElem se;
                    se.set_category(it->get_category());
                    se.set_count(diff);
                    diff_counts_.push_back(se);
                    mit->second.set_count(it->get_count());
                }
            }
        }
    }
};

template <typename ElemT>
class DerivedStatsSum: public DerivedStatsImpl<ElemT, SumResultElem> {
  public:
    DerivedStatsSum(const std::string &annotation):
                DerivedStatsImpl<ElemT, SumResultElem>(), value_(0) {}
    ElemT value_;
  protected:
    bool ResultImpl(SumResultElem &res) const {
        res.set_value(value_);
        return true;
    }
    void UpdateImpl(ElemT raw) {
        value_ += raw;
    }
};

template <typename ElemT>
class DerivedStatsNull: public DerivedStatsImpl<ElemT, NullResult> {
  public:
    DerivedStatsNull(const std::string &annotation):
                DerivedStatsImpl<ElemT, NullResult>(), value_(0) {}
    ElemT value_;
  protected:
    bool ResultImpl(NullResult &res) const {
        res.set_value(value_);
        return true;
    }
    void UpdateImpl(ElemT raw) {
        value_ = raw;
    }
};

template<>
boost::shared_ptr<DerivedStatsImpl<int, SumResultElem> >
DerivedStatsImpl<int, SumResultElem>::Create(std::string annotation) {
    return boost::shared_ptr<DerivedStatsImpl<int, SumResultElem> >(
        new DerivedStatsSum<int>(annotation));
}

template<>
boost::shared_ptr<DerivedStatsImpl<unsigned int, SumResultElem> >
DerivedStatsImpl<unsigned int, SumResultElem>::Create(std::string annotation) {
    return boost::shared_ptr<DerivedStatsImpl<unsigned int, SumResultElem> >(
        new DerivedStatsSum<unsigned int>(annotation));
}

template<>
boost::shared_ptr<DerivedStatsImpl<int, NullResult> >
DerivedStatsImpl<int, NullResult>::Create(std::string annotation) {
    return boost::shared_ptr<DerivedStatsImpl<int, NullResult> >(
        new DerivedStatsNull<int>(annotation));
}

template<>
boost::shared_ptr<DerivedStatsImpl<unsigned int, NullResult> >
DerivedStatsImpl<unsigned int, NullResult>::Create(std::string annotation) {
    return boost::shared_ptr<DerivedStatsImpl<unsigned int, NullResult> >(
        new DerivedStatsNull<unsigned int>(annotation));
}

template<>
boost::shared_ptr<DerivedStatsImpl<vector<CategoryResultSubElem>, CategoryResult> >
DerivedStatsImpl<vector<CategoryResultSubElem>, CategoryResult>::Create(std::string annotation) {
    return boost::shared_ptr<DerivedStatsImpl<vector<CategoryResultSubElem>, CategoryResult> >(
        new DerivedStatsCategoryCounter(annotation));
}

