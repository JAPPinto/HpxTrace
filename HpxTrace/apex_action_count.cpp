#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>

#include "comp.hpp"

#include <apex_api.hpp>


using hpx::performance_counters::performance_counter;


std::atomic<std::uint64_t> count(0);


int hpx_main(hpx::program_options::variables_map& vm)
{

	//Find all localities
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
    std::uint64_t n_localities = localities.size();


    std::vector<comp> components;
    std::uint64_t n_components = vm["components"].as<std::uint64_t>();

    for (int i = 0; i < n_components; i++) {
    	components.push_back(hpx::new_<server::comp>(localities[i % n_localities]));
    }

    std::uint64_t n_actions = vm["actions"].as<std::uint64_t>();
    

    for (int i = 0; i < n_components; i++) {
    	for (int j = 0; j < n_actions; j++) {
    		components[i].add(1);
    		count ++;
    	}
    }

    hpx::finalize();
    return 0;
}


int main(int argc, char* argv[]) {

    // the HPX runtime pointer (get_hpx_runtime_ptr()) will be null when we
    // ask for the profile if we don't ask for some output.
    apex::apex_options::use_screen_output(true);

    hpx::program_options::options_description
       desc_commandline("Usage: " HPX_APPLICATION_STRING " [options]");


    desc_commandline.add_options()
        ( "components",
          hpx::program_options::value<std::uint64_t>()->default_value(3),
          "number of component instances")
        ;

    desc_commandline.add_options()
        ( "actions",
          hpx::program_options::value<std::uint64_t>()->default_value(10),
          "number of comp_add_action invoked per component")
        ;


    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;


    // Initialize and run HPX
    int status =  hpx::init(argc, argv, init_args);


    std::cout << "Calls to comp_add_action: " << count << std::endl;
    apex_profile * prof = apex::get_profile("comp_add_action");

    std::cout << "APEX measured calls to comp_add_action: "
        << prof->calls << std::endl;


	for (int i = 0; i < 8; i++) {
		std::cout << "Accumulated PAPI hardware metrics: "
        << prof->papi_metrics[i] << std::endl;
    }


    apex::sample_value("threads{locality#0/total/total}/count/cumulative", 0);

    return 0;




}