//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/include/util.hpp>
#include <hpx/runtime/actions/continuation.hpp>

#include <cstdint>
#include <mutex>

#include "example.hpp"

///////////////////////////////////////////////////////////////////////////////
typedef hpx::components::component<
    ::performance_counters::example::server::example_counter
> example_counter_type;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC(
    example_counter_type, example_counter, "base_performance_counter");

///////////////////////////////////////////////////////////////////////////////
namespace performance_counters { namespace example { namespace server
{
    ///////////////////////////////////////////////////////////////////////////
    example_counter::example_counter(hpx::performance_counters::counter_info const& info)
      : hpx::performance_counters::base_performance_counter<example_counter>(info),
        current_value_(0),
        evaluated_at_(0),
        timer_(hpx::util::bind(&example_counter::evaluate, this),
            1000000, "example example performance counter")
    {
    }

    bool example_counter::start()
    {
        return timer_.start();
    }

    bool example_counter::stop()
    {
        return timer_.stop();
    }

    hpx::performance_counters::counter_value
        example_counter::get_counter_value(bool reset)
    {
        std::int64_t const scaling = 100000;

        hpx::performance_counters::counter_value value;

        // gather the current value
        {
            std::lock_guard<mutex_type> mtx(mtx_);
            value.value_ = std::int64_t(current_value_ * scaling);
            if (reset)
                current_value_ = 0;
            value.time_ = evaluated_at_;
        }

        value.scaling_ = scaling;
        value.scale_inverse_ = true;
        value.status_ = hpx::performance_counters::status_new_data;
        value.count_ = ++invocation_count_;

        return value;
    }

    void example_counter::finalize()
    {
        timer_.stop();
        hpx::performance_counters::base_performance_counter<example_counter>::finalize();
    }

    bool example_counter::evaluate()
    {
        std::lock_guard<mutex_type> mtx(mtx_);
        evaluated_at_ = static_cast<std::int64_t>(hpx::get_system_uptime());
        current_value_ = std::sin(evaluated_at_ / 1e10);
        return true;
    }
}}}

