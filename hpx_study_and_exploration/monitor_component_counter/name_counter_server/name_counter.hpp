#ifndef __NAME_COUNTER__
#define __NAME_COUNTER__
#pragma once

#include <hpx/hpx.hpp>
#include <hpx/include/lcos_local.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/include/util.hpp>
#include "../comp.hpp"
#include "../comp_server/comp.hpp"


#include <cstdint>



#define A 0
#define B 1
#define C 2


namespace performance_counters { namespace examples { namespace server
{

    template <class T> class name_counter
      : public hpx::performance_counters::base_performance_counter<name_counter<T>>

    {
    public:
        name_counter() : current_value_(0), evaluated_at_(0) {}
        explicit name_counter(
            hpx::performance_counters::counter_info const& info,
            std::string component_name,
            std::string parameters
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

        std::string component_name;
        int component_status;
        hpx::naming::id_type component_id;
        int default_value;
        
        hpx::util::interval_timer timer_;
    };

   ///////////////////////////////////////////////////////////////////////////
    template <class T>
    name_counter<T>::name_counter(
         hpx::performance_counters::counter_info const& info,
         std::string component_name,
         std::string parameters
    )
    : hpx::performance_counters::base_performance_counter<name_counter>(info),
        current_value_(0),
        evaluated_at_(0),
        component_name(component_name),
        component_status(A),
        default_value(std::stoi(parameters)),
        timer_(hpx::util::bind(&name_counter::evaluate, this),
            1000000, "name_counter performance counter")
    {
    }

    template <class T>
    bool name_counter<T>::start()
    {
        return timer_.start();
    }
    template <class T>
    bool name_counter<T>::stop()
    {
        return timer_.stop();
    }

    template <class T>
    hpx::performance_counters::counter_value
        name_counter<T>::get_counter_value(bool reset)
    {
        std::int64_t const scaling = 1;

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
        value.count_ = 1;//++invocation_count_;

        //Important part


       /* switch(component_status) {
          case A:
            component_id = hpx::agas::resolve_name(component_name).get();
            if(component_id != hpx::naming::invalid_id){
                value.value_ = T()(component_id);
                component_status = B;
            }
            else{
                value.value_ = 0;
            }
            break;
          case B:
            if(component_id != hpx::naming::invalid_id){
                value.value_ = T()(component_id);
            }
            else{
                value.value_ = 0;
                component_status = C;
            }
            break;
           case C:
            value.value_ = 0;
            break;
        } */


        if(component_id == hpx::naming::invalid_id){
            component_id = hpx::agas::resolve_name(component_name).get();
            value.value_ = default_value;
        }

        if(component_id != hpx::naming::invalid_id){
            value.value_ = T()(component_id);
        }

        return value;

    }

    template <class T>
    void name_counter<T>::finalize()
    {
        timer_.stop();
        hpx::performance_counters::base_performance_counter<name_counter>::finalize();
    }

    template <class T>
    bool name_counter<T>::evaluate()
    {
        std::lock_guard<mutex_type> mtx(mtx_);
        evaluated_at_ = static_cast<std::int64_t>(hpx::get_system_uptime());
        current_value_ = std::sin(evaluated_at_ / 1e10);
        return true;
    }



}}}

//#include "name_counter.cpp"

///////////////////////////////////////////////////////////////////////////////

#endif
