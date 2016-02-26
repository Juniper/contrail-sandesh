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
class DerivedStatsNull: public DerivedStatsBase<ElemT, NullResult> {
  public:
    DerivedStatsNull(const std::string &annotation,
            const std::string &attribute, const std::string &listkey) :
                DerivedStatsBase<ElemT, NullResult>(attribute,listkey) {}
  protected:
    void ResultImpl(NullResult &res) {
        res.set_uptime(100);
    }
    void UpdateImpl(ElemT raw, uint64_t tm) {
    }
};

template<>
boost::shared_ptr<DerivedStatsBase<int, NullResult> >
DerivedStatsBase<int, NullResult>::Create(std::string annotation) {
    return boost::shared_ptr<DerivedStatsBase<int, NullResult> >(
        new DerivedStatsNull<int>(annotation, "",""));
}

template<>
boost::shared_ptr<DerivedStatsBase<unsigned int, NullResult> >
DerivedStatsBase<unsigned int, NullResult>::Create(std::string annotation) {
    return boost::shared_ptr<DerivedStatsBase<unsigned int, NullResult> >(
        new DerivedStatsNull<unsigned int>(annotation, "",""));
}

