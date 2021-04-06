//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/hpx.hpp>
#include <hpx/include/lcos_local.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/include/util.hpp>

#include <cstdint>

namespace performance_counters { namespace examples { namespace server
{

    class basename_counter
      : public hpx::performance_counters::base_performance_counter<basename_counter>

    {
    public:
        basename_counter() : current_value_(0), evaluated_at_(0) {}
        explicit basename_counter(
            hpx::performance_counters::counter_info const& info,
            std::string component_name,
            int sequence_nr
            );


        /// This function will be called in order to query the current value of
        /// this performance counter
        hpx::performance_counters::counter_value get_counter_value(bool reset);

        /// The functions below will be called to start and stop collecting
        /// counter values from this counter.
        bool start();
        bool stop();

        /// finalize() will be called just before the instance gets destructed
        void finalize();

    protected:
        bool evaluate();

    private:
        typedef hpx::lcos::local::spinlock mutex_type;

        mutable mutex_type mtx_;
        double current_value_;
        std::uint64_t evaluated_at_;
        std::string component_basename;
        int sequence_nr;

        hpx::util::interval_timer timer_;
    };
}}}

