//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/include/util.hpp>

#include <cstdint>
#include <mutex>

#include "name_counter.hpp"
#include "../comp.hpp"

///////////////////////////////////////////////////////////////////////////////
typedef hpx::components::component<
    ::performance_counters::examples::server::name_counter
> name_counter_type;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC(
    name_counter_type, name_counter, "base_performance_counter");

///////////////////////////////////////////////////////////////////////////////
namespace performance_counters { namespace examples { namespace server
{
    ///////////////////////////////////////////////////////////////////////////
    name_counter::name_counter(hpx::performance_counters::counter_info const& info, std::string component_name)
      : hpx::performance_counters::base_performance_counter<name_counter>(info),
        current_value_(0),
        evaluated_at_(0),
        component_name(component_name),
        timer_(hpx::util::bind(&name_counter::evaluate, this),
            1000000, "name_counter performance counter")
    {
    }


    bool name_counter::start()
    {
        return timer_.start();
    }

    bool name_counter::stop()
    {
        return timer_.stop();
    }

    hpx::performance_counters::counter_value
        name_counter::get_counter_value(bool reset)
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

        //Important part

        comp component(hpx::agas::resolve_name(component_name).get());

        if(component.get_id() != hpx::naming::invalid_id) //Check if component exists
            return component.get();
        else
            return 0;



    }

    void name_counter::finalize()
    {
        timer_.stop();
        hpx::performance_counters::base_performance_counter<name_counter>::finalize();
    }

    bool name_counter::evaluate()
    {
        std::lock_guard<mutex_type> mtx(mtx_);
        evaluated_at_ = static_cast<std::int64_t>(hpx::get_system_uptime());
        current_value_ = std::sin(evaluated_at_ / 1e10);
        return true;
    }
}}}

