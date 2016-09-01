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
#include <sstream>
#include <sandesh/derived_stats_results_types.h>
#include <sandesh/derived_stats.h>
#include <boost/assign/list_of.hpp>
#include <boost/scoped_ptr.hpp>

using std::vector;
using std::map;
using std::pair;
using std::make_pair;
using std::string;

namespace contrail {
namespace sandesh {

// Interface for all Anomaly Detection algorithms
// That can be plugged into the DSAnomaly DerivedStat
template <typename ElemT>
class DSAnomalyIf {
  public:
    virtual bool FillResult(AnomalyResult& res) const = 0;
    virtual ~DSAnomalyIf() { }
    virtual void Update(const ElemT& raw) = 0;
};

// Implementation of Exponential Weighted Mean
// algorithm for Anomaly Detection
template <typename ElemT>
class DSAnomalyEWM : public DSAnomalyIf<ElemT> {
  public:
    DSAnomalyEWM (const std::string &annotation, 
            std::string &errstr) :
                mean_(0), variance_(0), sigma_(0), stddev_(0), psigma_(0) {
        alpha_ = (double) strtod(annotation.c_str(), NULL);
        if ((alpha_ <= 0) || (alpha_ > 1)) {
            errstr = std::string("Invalid alpha ") + annotation;
            alpha_ = 0;
        }
    }
    double alpha_;
    double mean_;
    double variance_;
    double sigma_;
    double stddev_;
    double psigma_;
    
    virtual bool FillResult(AnomalyResult& res) const {
        assert(alpha_ != 0);
        std::ostringstream meanstr, stddevstr;
        meanstr << mean_;
        stddevstr << stddev_;

        res.set_sigma(sigma_);
        res.set_state(boost::assign::map_list_of(
            std::string("mean"), meanstr.str())(
            std::string("stddev"), stddevstr.str()));
       
        // If sigma was low, and is still low, 
        // we don't need to send anything 
        if ((psigma_ < 0.5) && (psigma_ > -0.5) && 
            (sigma_ < 0.5) && (sigma_ > -0.5))
            return false;
        else
	    return true;
    }

    virtual void Update(const ElemT& raw) {
        assert(alpha_ != 0);
        variance_ = (1-alpha_)*(variance_ + (alpha_*pow(raw-mean_,2)));
        mean_ = ((1-alpha_)*mean_) + (alpha_*raw);
        stddev_ = sqrt(variance_);
        psigma_ = sigma_;
        if (stddev_) sigma_ = (raw - mean_) / stddev_;
        else sigma_ = 0;
    }
};

template <typename ElemT, class AnomalyResT>
class DSAnomaly {
  public:
    DSAnomaly(const std::string &annotation):
            samples_(0) {
        size_t rpos = annotation.find(':');
        algo_ = annotation.substr(0,rpos);
        config_ = annotation.substr(rpos+1, string::npos);
        if (algo_.compare("EWM") == 0) {
            impl_.reset(new DSAnomalyEWM<ElemT>(config_, error_));
            if (!error_.empty()) {
                impl_.reset();
            }
            return;
        }
        // No valid Anomaly Detection algorithm was found
        impl_.reset(NULL);
    }

    DSReturnType FillResult(AnomalyResT &res) const {
        DSReturnType ret = DSR_OK;
        if (impl_) {
            if (!impl_->FillResult(res)) ret = DSR_SKIP;
            // We should have cleared impl_ if there was a parsing error
            // with the DSAnomaly config
            assert(error_.empty());
        }
        
	res.set_samples(samples_);
	res.set_algo(algo_);
	res.set_config(config_);
	if (!error_.empty()) res.set_error(error_);
        return ret;
    }

    void Update(const ElemT& raw, uint64_t mono_usec) {
        samples_++;
        if (impl_) impl_->Update(raw);
    }

    uint64_t samples_;
    std::string algo_;
    std::string config_;
    std::string error_;
    boost::scoped_ptr<DSAnomalyIf<ElemT> > impl_;
};

template <typename ElemT, class EWMResT>
class DSEWM {
  public:
    DSEWM(const std::string &annotation):
                mean_(0), variance_(0),
                sigma_(0), stddev_(0), samples_(0) {
        alpha_ = (double) strtod(annotation.c_str(), NULL);
        if (alpha_ == 0) {
            error_ = std::string("Disabled");
            return;
        }
        if ((alpha_ < 0) || (alpha_ > 1)) {
            error_ = std::string("Invalid alpha ") + annotation;
            alpha_ = 0;
        }
    }

    double alpha_;
    double mean_;
    double variance_;
    double sigma_;
    double stddev_;
    uint64_t samples_;
    std::string error_;

    DSReturnType FillResult(EWMResT &res) const {
	res.set_samples(samples_);
        if (!error_.empty()) {
            res.set_error(error_);
            return DSR_OK;
        }
        res.set_mean(mean_);
        res.set_stddev(stddev_);
        res.set_sigma(sigma_);
        return DSR_OK;
    }
    void Update(const ElemT& raw, uint64_t mono_usec) {
        samples_++;
        if (!error_.empty()) return;
        variance_ = (1-alpha_)*(variance_ + (alpha_*pow(raw-mean_,2)));
        mean_ = ((1-alpha_)*mean_) + (alpha_*raw);
        stddev_ = sqrt(variance_);
        if (stddev_) sigma_ = (raw - mean_) / stddev_;
        else sigma_ = 0;
    }
};

template <typename ElemT, class NullResT>
class DSChange {
  public:
    DSChange(const std::string &annotation) : init_(false) {}

    bool init_;
    ElemT value_;
    ElemT prev_;

    DSReturnType FillResult(NullResT &res) const {
        if (!init_) return DSR_INVALID;
        res = value_;
	if (prev_ == value_) return DSR_SKIP;
	else return DSR_OK;
    }

    void Update(const ElemT& raw, uint64_t mono_usec) {
        if (init_) prev_ = value_;
        value_ = raw;
        init_ = true;
    }
};

template <typename ElemT, class NullResT>
class DSNon0 {
  public:
    DSNon0(const std::string &annotation) {}
    ElemT value_;

    DSReturnType FillResult(NullResT &res) const {
	res = value_;
	if (value_ == 0) return DSR_SKIP;
	else return DSR_OK;
    }

    void Update(const ElemT& raw, uint64_t mono_usec) {
        value_ = raw;
    }
};

template <typename ElemT, class NullResT>
class DSNone {
  public:
    DSNone(const std::string &annotation) {}
    ElemT value_;

    DSReturnType FillResult(NullResT &res) const {
        res = value_;
        return DSR_OK;
    }
    void Update(const ElemT& raw, uint64_t mono_usec) {
        value_ = raw;
    }
};

template <typename ElemT, class NullResT>
class DSNull {
  public:
    DSNull(const std::string &annotation): samples_(0) {}
    ElemT value_;
    uint64_t samples_;

    DSReturnType FillResult(NullResT &res) const {
        res.set_samples(samples_);
        res.set_value(value_);
        return DSR_OK;
    }
    void Update(const ElemT& raw, uint64_t mono_usec) {
        samples_++;
        value_ = raw;
    }
};

template <typename ElemT, class SumResT> 
class DSSum {
  public:
    DSSum(const std::string &annotation): samples_(0), shifter_(0),
            start_tbin_(0), last_tbin_(0) {

        if (annotation.empty()) {
            range_usecs_ = 0;
        } else {
            range_usecs_ = ((uint64_t) strtoul(annotation.c_str(), NULL, 10)) * 1000000;
            // We want each time bucket to represent between
            // 1/128th and 1/256th of the entire range
            // This ensures that there will never be more than 256 buckets
            while ((1 << (shifter_ + 8)) < range_usecs_) shifter_++;   
        }
    }
    uint64_t samples_;
    SumResT value_;
    map<uint64_t, pair<uint64_t,ElemT> > history_buf_;
    uint64_t range_usecs_;
    uint8_t shifter_;
    uint64_t start_tbin_;
    uint64_t last_tbin_;

    virtual DSReturnType FillResult(SumResT &res) const {
        static SumResT empty_val;
        if (!samples_) {
            res = empty_val;
            return DSR_INVALID;
        }
        if (range_usecs_) {
            // At least one time-range's worth of samples must be seen
            // before we can report a SUM
            uint64_t end_range =
                    ((start_tbin_ + range_usecs_) >> shifter_) << shifter_;
            if (end_range > last_tbin_) {
                res = empty_val;
                return DSR_INVALID;
            } else {
            }
        }
        res = value_;
        return DSR_OK;
    }

    virtual void Purge(uint64_t mono_usec) {

        for (typename map<uint64_t, pair<uint64_t,ElemT> >::iterator it = 
                history_buf_.begin(); it!= history_buf_.end(); ) {
            uint64_t end_range = ((it->first + range_usecs_) >> shifter_) << shifter_;
            if (end_range <= mono_usec) {
                value_ = value_ - it->second.second;
                samples_ = samples_ - it->second.first;
                history_buf_.erase(it++);
            } else {
                ++it;
            }
        } 
    }

    virtual void Update(const ElemT& raw, uint64_t mono_usec) {
        if (!samples_) {
            value_ = raw;
        } else {
            value_ = value_ + raw;
        }
        if (range_usecs_) {
            // if 'range_usecs_' is non-0, we need to aggregate only
            // the last 'range_usecs_' worth of elements
            uint64_t tbin = (mono_usec >> shifter_) << shifter_;
            if (!start_tbin_) start_tbin_ = tbin;
            last_tbin_ = tbin;

            // Subtract old entries, if there are any
            Purge(tbin);

            // Record the new update, so we can subtract when needed
            typename map<uint64_t, pair<uint64_t,ElemT> >::iterator ut =
                    history_buf_.find(tbin);
            if (ut == history_buf_.end()) {
                history_buf_[tbin] = make_pair(1,raw);
            } else {
                ut->second.second = ut->second.second + raw;
                ut->second.first++;
            }
        }
        samples_++;
    }
};

template <typename ElemT, class AvgResT>
class DSAvg : public DSSum<ElemT,AvgResT> {
  public:
    DSAvg(const std::string &annotation): DSSum<ElemT,AvgResT>(annotation) {}

    virtual DSReturnType FillResult(AvgResT &res) const {
        AvgResT sres;
        if (DSSum<ElemT,AvgResT>::FillResult(sres) == DSR_OK) {
            res = sres / DSSum<ElemT,AvgResT>::samples_;
            return DSR_OK;
        } else {
            return DSR_INVALID;
        }
    }
};

} // namespace sandesh
} // namespace contrail

#endif // #define __DERIVED_STATS_ALGO_H__
