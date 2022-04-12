#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>

#include "comp.hpp"


/*

     Example to use (base)name_counters with the command line

    ./cmd --hpx:print-counter=/examples{locality#0/total}/name/implicit --hpx:print-counter-interval=500

    ./cmd --hpx:print-counter=/examples{locality#0/component#0}/name/explicit --hpx:print-counter-interval=500

    ./cmd --hpx:print-counter=/examples{locality#0/component#0}/basename/explicit --hpx:print-counter=/examples{locality#0/component#1}/basename/explicit  --hpx:print-counter-interval=500

*/



using hpx::performance_counters::performance_counter;


int hpx_main(hpx::program_options::variables_map& vm)
{

	//Find all localities
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();

    //Create components (in another locality if there are multiple)
    comp component_name = hpx::new_<server::comp>(localities.back());
    comp component_basename_0 = hpx::new_<server::comp>(localities.back());
    comp component_basename_1 = hpx::new_<server::comp>(localities.back());



    //Register component_name name in agas
    std::string name = "component";
    hpx::agas::register_name(name, component_name.get_id());

    //Register component_basename basename and sequence number in agas
    hpx::register_with_basename(name, component_basename_0.get_id(), 0);
    hpx::register_with_basename(name, component_basename_1.get_id(), 1);

    

    for (int i = 0; i < 5; i++) {
    	component_name.add(1);
        component_basename_0.add(1);
        component_basename_1.add(-1);
		sleep(1);
    }

    hpx::finalize();
    return 0;
}


int main(int argc, char* argv[]) {


    // Initialize and run HPX.
    return hpx::init(argc, argv);
}