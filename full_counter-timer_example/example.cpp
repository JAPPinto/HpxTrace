#include <hpx/include/util.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/runtime_local/startup_function.hpp>

#include <cstdint>

#include "server/example.hpp"

#include <execinfo.h>

#define MAX_INSTANCES 2


HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<
    ::performance_counters::example::server::example_counter
> example_counter_type;

namespace performance_counters { namespace example
{


    // The purpose of this function is to invoke the supplied function f for all
    // allowed counter instance names supported by the counter type this
    // function has been registered with.

    //Invoked if counter name includes wilcards
    bool explicit_example_counter_discoverer(
        hpx::performance_counters::counter_info const& info,
        //f is called for each discovered performance counter instance
        hpx::performance_counters::discover_counter_func const& f, 
        hpx::performance_counters::discover_counters_mode mode, 
        hpx::error_code& ec
        ) {

        std::cout << "discover" << std::endl;
        std::cout << info.fullname_ << std::endl;


    	// compose the counter name templates

    	//A counter_path_elements holds the elements of a full name for a counter instance.
    	///objectname{parentinstancename::parentindex/instancename#instanceindex}/countername#parameters
        hpx::performance_counters::counter_path_elements p;

        //Fill the given counter_path_elements instance from the given full name of a counter. 
        hpx::performance_counters::counter_status status =
            get_counter_path_elements(info.fullname_, p, ec);

        //invalid counter name
        if (!status_is_valid(status)) return false;



        hpx::performance_counters::counter_info i = info;

        std::cout << p.parentinstancename_ << " " << p.parentinstanceindex_  << std::endl;
        std::cout << p.instancename_ << " " << p.instanceindex_  << std::endl;




        if (mode == hpx::performance_counters::discover_counters_minimal ||
            p.parentinstancename_.empty() || p.instancename_.empty())
        {


            std::cout  << "if minimal"  << std::endl;

            if (p.parentinstancename_.empty())
            {
                p.parentinstancename_ = "locality#*";
                p.parentinstanceindex_ = -1;
            }

            if (p.instancename_.empty())
            {
                p.instancename_ = "instance#*";
                p.instanceindex_ = -1;
            }

            //status = get_counter_name(p, i.fullname_, ec);
            //if (!status_is_valid(status) || !f(i, ec) || ec)
              //  return false;
        }
        else if(p.instancename_ == "instance#*") {

            std::cout  << "if instance#*"  << std::endl;

            HPX_ASSERT(mode == hpx::performance_counters::discover_counters_full);

            for (int n = 0; n < MAX_INSTANCES; n++){
                p.instancename_ = "instance";
                p.instanceindex_ = n;
                status = get_counter_name(p, i.fullname_, ec);
                std::cout << "instance#" << n << std::endl;
                if (!status_is_valid(status) || !f(i, ec) || ec)
                    return false;  
            }
        }



        //discover_counters_mode = discover_counters_minimal oir discover_counters_full
        std::cout << "aqui" << std::endl;

        if (!f(i, ec) || ec) {
        std::cout << "aqui1" << std::endl;
            return false;
        }
        std::cout << "discover end \n\n\n" << std::endl;

        if (&ec != &hpx::throws)
            ec = hpx::make_success_code();

        return true;    // everything is ok

    }

	///////////////////////////////////////////////////////////////////////////
    // Creation function for explicit example performance counter. It's purpose is
    // to create and register a new instance of the given name (or reuse an
    // existing instance).
    hpx::naming::gid_type explicit_example_counter_creator(
        hpx::performance_counters::counter_info const& info, hpx::error_code& ec)
    {
        std::cout << "\ncreator" << std::endl;


        //void *array[10];
        //size_t size;

        // get void*'s for all entries on the stack
       // size = backtrace(array, 10);

        // print out all the frames to stderr
        //backtrace_symbols_fd(array, size, STDERR_FILENO);
        //exit(1);


        //Fill the given counter_path_elements instance from the given full name of a counter.
        hpx::performance_counters::counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        // verify the validity of the counter instance name
        if (ec){
        	std::cout << "if1:" << info.fullname_ << std::endl;
        	//gid - Global identifier for components across the HPX system. 
        	return hpx::naming::invalid_gid;
        }

        //???parentinstancename_
        std::cout << paths.parentinstancename_ << std::endl;

        if (paths.parentinstance_is_basename_) {
        	std::cout << "if2:" << std::endl;

            HPX_THROWS_IF(ec, hpx::bad_parameter,
                "example::explicit_example_counter_creator",
                "invalid counter instance parent name: " +
                    paths.parentinstancename_);
            return hpx::naming::invalid_gid;
        }

        // create individual counter
        //verifies instance#n
        if (paths.instancename_ == "instance" && paths.instanceindex_ != -1) {
        	std::cout << "if3:" << std::endl;

            // make sure parent instance name is set properly
            hpx::performance_counters::counter_info complemented_info = info;
            complement_counter_info(complemented_info, info, ec);
            if (ec) return hpx::naming::invalid_gid;

            // create the counter as requested
            hpx::naming::gid_type id;
            try {
                // create the 'example' performance counter component locally, we
                // only get here if this instance does not exist yet
                id = hpx::components::server::construct<example_counter_type>(
                        complemented_info);
            }
            catch (hpx::exception const& e) {
                if (&ec == &hpx::throws)
                    throw;
                ec = make_error_code(e.get_error(), e.what());
                return hpx::naming::invalid_gid;
            }

            if (&ec != &hpx::throws)
                ec = hpx::make_success_code();
            return id;
        }

        	///example{{locality#{}/instance#{}}}/immediate/explicit
        	std::cout << "4" << std::endl;


        HPX_THROWS_IF(ec, hpx::bad_parameter,
            "example::explicit_example_counter_creator",
            "invalid counter instance name: " + paths.instancename_);
        return hpx::naming::invalid_gid;
    }






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

        std::cout << "AAAAAA" << std::endl;
        install_counter_type(
            "/example/immediate/implicit", //name
            counter_raw,                   //type - shows the last observed value 
            "returns ... (implicit version, using HPX facilities)", //help text
            // function which will be called to create a new instance of this counter type
            //!!!Verificar melhor o que isto faz
            hpx::util::bind(&hpx::performance_counters::locality_raw_counter_creator, _1, &immediate_example, _2), 
            //The function will be called to discover counter instances which can be created.
            &hpx::performance_counters::locality_counter_discoverer, 
            HPX_PERFORMANCE_COUNTER_V1, //version
            "" //unit of measure 
            );


        install_counter_type(
            "/example/immediate/explicit", //name
            counter_raw,                   //type - shows the last observed value 
            "returns ... (explicit version)", //help text
            // function which will be called to create a new instance of this counter type
            &explicit_example_counter_creator, 
            //The function will be called to discover counter instances which can be created.
            &explicit_example_counter_discoverer, 
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
HPX_REGISTER_STARTUP_MODULE(::performance_counters::example::get_startup);



