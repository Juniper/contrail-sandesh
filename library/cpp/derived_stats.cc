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

template <typename ElemT>
class DerivedStatsSum: public DerivedStatsImpl<ElemT, SumResultElem> {
  public:
    DerivedStatsSum(const std::string &annotation):
                DerivedStatsImpl<ElemT, SumResultElem>(), value_(0) {}
    ElemT value_;
  protected:
    void ResultImpl(SumResultElem &res) const {
        res.set_value(value_);
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
    void ResultImpl(NullResult &res) const {
        res.set_value(value_);
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

