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

    ./cmd --hpx:print-counter=/runtime{locality#0/component#0}/count/action-invocation{comp_add_action} --hpx:print-counter-interval=500
*/



using hpx::performance_counters::performance_counter;


int hpx_main(hpx::program_options::variables_map& vm)
{

	//Find all localities
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();

    //Create components (in another locality if there are multiple)
    comp component_name = hpx::new_<server::comp>(localities.back());
    

    //Create counter
    performance_counter implicit_counter(hpx::util::format(
        "/runtime{{locality#{}/total}}/count/action-invocation@comp_add_action",
        hpx::get_locality_id()));



    for (int i = 0; i < 5; i++) {
    	component_name.add(1);
        std::cout << implicit_counter.get_value<double>().get() << std::endl;
    }

    hpx::finalize();
    return 0;
}


int main(int argc, char* argv[]) {


    // Initialize and run HPX.
    return hpx::init(argc, argv);
}