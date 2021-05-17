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
        time_started(0),
        total_time(0),
        counting(false)
        //timer_(hpx::util::bind(&example_counter::evaluate, this),
          //  1000000, "example example performance counter")
    {
    }

	bool example_counter::start()
	{
		time_started = static_cast<std::int64_t>(hpx::get_system_uptime());
		counting = true;
	    return counting;
	}

    bool example_counter::stop()
    {
    	counting = false;
        return counting;
    }

    hpx::performance_counters::counter_value
        example_counter::get_counter_value(bool reset)
    {
        
        std::int64_t current_time = static_cast<std::int64_t>(hpx::get_system_uptime());
        
        std::int64_t const scaling = 1000000;
        hpx::performance_counters::counter_value value;


        if (reset){
        	total_time = 0;
            time_started = current_time;
        }
        if(counting){
    		total_time += current_time - time_started;
    		time_started = current_time;
        }

        //The current counter value.
        value.value_ = total_time;
        //The local time when data was collected.
        value.time_ = current_time;

        //The scaling of the current counter value. 
        value.scaling_ = scaling;
        //If true, value_ needs to be divided by scaling_, otherwise it has to be multiplied.
        value.scale_inverse_ = true;
        //The status of the counter value.
        value.status_ = hpx::performance_counters::status_new_data;
        //The invocation counter for the data.
        value.count_ = ++invocation_count_;
        	
        return value;
    }

    void example_counter::finalize()
    {
        //timer_.stop();
        hpx::performance_counters::base_performance_counter<example_counter>::finalize();
    }

}}}

