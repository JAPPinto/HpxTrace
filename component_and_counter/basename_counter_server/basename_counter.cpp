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

#include "basename_counter.hpp"

#include <unistd.h>


///////////////////////////////////////////////////////////////////////////////
typedef hpx::components::component<
    ::performance_counters::examples::server::basename_counter
> basename_counter_type;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC(
    basename_counter_type, basename_counter, "base_performance_counter");

///////////////////////////////////////////////////////////////////////////////
namespace performance_counters { namespace examples { namespace server
{
    ///////////////////////////////////////////////////////////////////////////
    basename_counter::basename_counter(hpx::performance_counters::counter_info const& info, std::string component_basename, int sequence_nr)
      : hpx::performance_counters::base_performance_counter<basename_counter>(info),
        current_value_(0),
        evaluated_at_(0),
        component_basename(component_basename),
        sequence_nr(sequence_nr),
        ready(false),
        timer_(hpx::util::bind(&basename_counter::evaluate, this),
            1000000, "basename_counter performance counter")
    {
    }


    bool basename_counter::start()
    {
        return timer_.start();
    }

    bool basename_counter::stop()
    {
        return timer_.stop();
    }

    hpx::performance_counters::counter_value
        basename_counter::get_counter_value(bool reset)
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

        if(ready)
            return component.get();

        //blocks if the components has not been registered
        hpx::future<hpx::id_type> id = hpx::find_from_basename(component_basename, sequence_nr);

        if (id.wait_for(std::chrono::milliseconds(1000)) ==  hpx::lcos::future_status::ready) {
            ready = true;
            component = comp(id.get());
            return component.get();
        } 
        else {
            return 0;
        }
/*
        //temporary fix, if there are multiple localities, it takes a while for the future to be ready
        sleep(1);
        if(id.is_ready()){
            comp component(id.get());

            return component.get();
        }
        else
          return 0;
*/
    }

    void basename_counter::finalize()
    {
        timer_.stop();
        hpx::performance_counters::base_performance_counter<basename_counter>::finalize();
    }

    bool basename_counter::evaluate()
    {
        std::lock_guard<mutex_type> mtx(mtx_);
        evaluated_at_ = static_cast<std::int64_t>(hpx::get_system_uptime());
        current_value_ = std::sin(evaluated_at_ / 1e10);
        return true;
    }
}}}

