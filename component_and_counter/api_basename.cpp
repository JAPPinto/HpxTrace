
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>

#include <hpx/modules/program_options.hpp>

#include "comp.hpp"


using hpx::performance_counters::performance_counter;


int hpx_main(hpx::program_options::variables_map& vm)
{

    //Create component
    comp component = hpx::new_<server::comp>(hpx::find_here());

    //Register component name in agas
    std::string name = "component";
    hpx::register_with_basename(name, component.get_id(), 0);

    //Create counter
    performance_counter counter(hpx::util::format(
        "/examples{{locality#{}/component#0}}/basename/explicit",
        hpx::get_locality_id()));

    //Read counter before component is created -- Doesn't work!!!!!!!!
    //std::cout << "Counter value: " << counter.get_value<double>().get() << std::endl;




    component.add(5);
    std::cout << "Component value: " << component.get() << std::endl;

    std::cout << "Counter value: " << counter.get_value<double>().get() << std::endl;

    component.add(5);
    std::cout << "Component value: " << component.get() << std::endl;
    std::cout << "Counter value: " << counter.get_value<double>().get() << std::endl;


    hpx::finalize();
    return 0;
}

int main(int argc, char* argv[])
{

    // Initialize and run HPX.
    return hpx::init();
}
