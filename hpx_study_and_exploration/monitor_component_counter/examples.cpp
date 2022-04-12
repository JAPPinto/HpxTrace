//#include <hpx/include/util.hpp>
//#include <hpx/include/performance_counters.hpp>
#include <hpx/runtime_local/startup_function.hpp>
#include <cstdint>

#include "name_counter_server/name_counter.hpp"
//#include "name_counter_server/name_counter.cpp"

#include "comp.hpp"





// Add factory registration functionality, We register the module dynamically
// as no executable links against it.
//HPX_REGISTER_COMPONENT_MODULE_DYNAMIC();
HPX_REGISTER_COMPONENT_MODULE();


//typedef hpx::components::component<
//    ::performance_counters::examples::server::name_counter
//> name_counter_type;
typedef hpx::components::component<
    ::performance_counters::examples::server::name_counter<::server::comp::get_action>
> name_counter_type;



namespace performance_counters { namespace examples
{


    /*The purpose of this function is to invoke the supplied function f for all
      allowed counter instance names supported by the counter type this
      function has been registered with.
	  
	  Translation: if the name contains willcards call counter_creator for each possible variation
	  For now, still the same as the official example
	*/

    bool explicit_name_counter_discoverer(
        //type, version, status, fullname, help text, unit of measure
        hpx::performance_counters::counter_info const& info,
        //f is called for each discovered performance counter instance
        hpx::performance_counters::discover_counter_func const& f, 
        hpx::performance_counters::discover_counters_mode mode, 
        hpx::error_code& ec
        ) {

        //std::cout << "discover" << std::endl;

        hpx::performance_counters::counter_info i = info;



        // compose the counter name templates

        //A counter_path_elements holds the elements of a full name for a counter instance.
        ///objectname{parentinstancename::parentindex/instancename#instanceindex}/countername#parameters
        hpx::performance_counters::counter_path_elements p;

        //Fill the given counter_path_elements instance from the given full name of a counter.
        //counter_status -  Status and error codes used by the functions related to performance counters. 
        hpx::performance_counters::counter_status status = 
            get_counter_path_elements(info.fullname_, p, ec);


        //invalid counter name
        if (!status_is_valid(status)) return false;

        //discover_counters_mode = discover_counters_minimal oir discover_counters_full
        if (mode == hpx::performance_counters::discover_counters_minimal ||
            p.parentinstancename_.empty() || p.instancename_.empty())
        {
            //std::cout << "discover if1" << std::endl;

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

            status = get_counter_name(p, i.fullname_, ec);
            if (!status_is_valid(status) || !f(i, ec) || ec)
                return false;
        }

        else if(p.instancename_ == "instance#*") {

            //std::cout << "discover if2" << std::endl;

            HPX_ASSERT(mode == hpx::performance_counters::discover_counters_full);

            // FIXME: expand for all instances
            p.instancename_ = "instance";
            p.instanceindex_ = 0;
            status = get_counter_name(p, i.fullname_, ec);
            if (!status_is_valid(status) || !f(i, ec) || ec)
                return false;
        }
        else if (!f(i, ec) || ec) {
            //std::cout << "discover if3" << std::endl;

            return false;
        }

        if (&ec != &hpx::throws){
            //std::cout << "discover if4" << std::endl;
            ec = hpx::make_success_code();
        }

        //std::cout << "discover end" << std::endl;


        return true;    // everything is ok
    }

    // Creation function for explicit name_counter performance counter. It's purpose is
    // to create and register a new instance of the given name (or reuse an
    // existing instance).
    template <class T>
    hpx::naming::gid_type explicit_name_counter_creator_name(
        hpx::performance_counters::counter_info const& info, hpx::error_code& ec)
    {


        //Fill the given counter_path_elements instance from the given full name of a counter.
        hpx::performance_counters::counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);


        // make sure parent instance name is set properly
        hpx::performance_counters::counter_info complemented_info = info;
        complement_counter_info(complemented_info, info, ec);


        if (paths.parameters_.empty()){
            paths.parameters_ = "0";
        }

        hpx::naming::gid_type id;
        id = hpx::components::server::construct<T>(
                complemented_info, paths.instancename_, paths.parameters_);

        return id;
    }

  



    //Function that is called each time the implicit counter is read
    std::int64_t name_implicit_f(bool reset){
        std::string name = "component";
        comp component(hpx::agas::resolve_name(name).get());


        if(component.get_id() != hpx::naming::invalid_id) //Check if component exists
            return component.get();
        else
            return 0;
    }

    // This function will be registered as a startup function for HPX below.
    //
    // That means it will be executed in a HPX-thread before hpx_main, but after
    // the runtime has been initialized and started.
    template <class T>
    void register_counter(std::string name)
    {

        using namespace hpx::performance_counters;


        install_counter_type(
            name, //name
            counter_raw,                   //type - shows the last observed value 
            "returns ... (explicit version)", //help text
            // function which will be called to create a new instance of this counter type
            &explicit_name_counter_creator_name<T>, 
            //The function will be called to discover counter instances which can be created.
            &explicit_name_counter_discoverer, 
            HPX_PERFORMANCE_COUNTER_V1, //version
            "" //unit of measure 
            );


    }

    void startup(){
        register_counter<name_counter_type>("/examples/name/explicit");
        //register_counter<name_counter_type>("/examples/name/Axplicit");

    }

    

    bool get_startup(hpx::startup_function_type& startup_func, bool& pre_startup)
    {

        // return our startup-function if performance counters are required
        startup_func = startup;   // function to run during startup
        pre_startup = true;       // run 'startup' as pre-startup function
        return true;
    }


}}

    template <class T>
    void register_component_counter(std::string name){



        typedef hpx::components::component<
            ::performance_counters::examples::server::name_counter<T>
            > name_counter_type;
        
        hpx::register_startup_function([name]() -> void {
            performance_counters::examples::register_counter<name_counter_type>(name);

        });
    }
// Register a startup function which will be called as a HPX-thread during
// runtime startup. We use this function to register our performance counter
// type and performance counter instances.
//
// Note that this macro can be used not more than once in one module.
//HPX_REGISTER_STARTUP_MODULE(::performance_counters::examples::get_startup);


#define REGISTER_COMPONENT_COUNTER_FACTORY(action) \
    HPX_REGISTER_DERIVED_COMPONENT_FACTORY_DYNAMIC( \
    hpx::components::component<::performance_counters::examples::server::name_counter<action>>, \
     name_counter, "base_performance_counter");


