#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>

#include "comp.hpp"





using hpx::performance_counters::performance_counter;


int hpx_main(hpx::program_options::variables_map& vm)
{

	//Find all localities
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();

    //Create components (in another locality if there are multiple)
    comp component = hpx::new_<server::comp>(localities.back());


    for (int i = 0; i < 100000; i++) {
    	component.add(1);
        i++;
    }

    hpx::finalize();
    return 0;
}


int main(int argc, char* argv[]) {


    // Initialize and run HPX.
    return hpx::init(argc, argv);
}