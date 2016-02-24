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


template <class ElemT>
boost::shared_ptr<DerivedStatsUpdateIf<ElemT> >
DerivedStatsUpdateIf<ElemT>::Create(std::string type_name, std::string annotation) {
    if (type_name.compare(" ::NullResult") == 0) {
        return boost::shared_ptr<DerivedStatsUpdateIf<ElemT> >(
            new DerivedStatsNull<ElemT>(annotation, "",""));
    } else if (type_name.compare(" ::EWMResult") == 0){
        //TODO: new DerivedStatsEWM<ElemT>(annotation, "","");
        return boost::shared_ptr<DerivedStatsUpdateIf<ElemT> >();
    } else {
        return boost::shared_ptr<DerivedStatsUpdateIf<ElemT> >();
    }
}

template class DerivedStatsUpdateIf<int>;
template class DerivedStatsUpdateIf<unsigned int>;
template class DerivedStatsUpdateIf<double>;

