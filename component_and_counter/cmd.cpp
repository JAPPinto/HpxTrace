#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>

#include "comp.hpp"


/*
    ./cmd --hpx:print-counter=/example{locality#0/total}/immediate/implicit --hpx:print-counter-interval=500

    ./cmd --hpx:print-counter=/example{locality#0/instance#0}/immediate/explicit --hpx:print-counter-interval=500
 
*/



using hpx::performance_counters::performance_counter;


int hpx_main(hpx::program_options::variables_map& vm)
{

	
    //Create component
    comp component = hpx::new_<server::comp>(hpx::find_here());

    //Register component name in agas
    std::string name = "component";
    hpx::agas::register_name(name, component.get_id());

    for (int i = 0; i < 5; i++) {
    	component.add(1);
		sleep(1);
    }

    hpx::finalize();
    return 0;
}


int main(int argc, char* argv[]) {


    // Initialize and run HPX.
    return hpx::init(argc, argv);
}