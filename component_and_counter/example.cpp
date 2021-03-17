#include <hpx/include/util.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/runtime_local/startup_function.hpp>

#include <cstdint>

#include "counter_server/example.hpp"

#include "counter_server/example.hpp"
#include "main.cpp"







// Add factory registration functionality, We register the module dynamically
// as no executable links against it.
HPX_REGISTER_COMPONENT_MODULE_DYNAMIC();

typedef hpx::components::component<
    ::performance_counters::example::server::example_counter
> example_counter_type;

namespace performance_counters { namespace example
{


    


    // This function will be invoked whenever the implicit counter is queried.
    std::int64_t immediate_example(bool reset)
    {
        static std::uint64_t started_at =
            hpx::chrono::high_resolution_clock::now();

        std::uint64_t up_time =
            hpx::chrono::high_resolution_clock::now() - started_at;
        return std::int64_t(std::sin(up_time / 1e10) * 100000.);
    }



    // This function will be registered as a startup function for HPX below.
    //
    // That means it will be executed in a HPX-thread before hpx_main, but after
    // the runtime has been initialized and started.
    void startup()
    {
        using namespace hpx::performance_counters;
        using hpx::util::placeholders::_1;
        using hpx::util::placeholders::_2;


        install_counter_type(
            "/example/immediate/implicit", //name
            counter_raw,                   //type - shows the last observed value 
            "returns ... (implicit version, using HPX facilities)", //help text
            // function which will be called to create a new instance of this counter type
            hpx::util::bind(&hpx::performance_counters::locality_raw_counter_creator, _1, &f, _2), 
            //The function will be called to discover counter instances which can be created.
            &hpx::performance_counters::locality_counter_discoverer, 
            HPX_PERFORMANCE_COUNTER_V1, //version
            "" //unit of measure 
            );
    }



    

    bool get_startup(hpx::startup_function_type& startup_func, bool& pre_startup)
    {

        // return our startup-function if performance counters are required
        startup_func = startup;   // function to run during startup
        pre_startup = true;       // run 'startup' as pre-startup function
        return true;
    }

}}

// Register a startup function which will be called as a HPX-thread during
// runtime startup. We use this function to register our performance counter
// type and performance counter instances.
//
// Note that this macro can be used not more than once in one module.
HPX_REGISTER_STARTUP_MODULE_DYNAMIC(::performance_counters::example::get_startup);



