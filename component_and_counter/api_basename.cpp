
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>

#include <hpx/modules/program_options.hpp>

#include "comp.hpp"

/*

    Example that uses basename_counter in the code

    With basename_counter the correspondent component is identified through its registered basename and sequence number
        basename#sequenc_number
    If the component does not yet exist, get_value() will block
*/


using hpx::performance_counters::performance_counter;


int hpx_main(hpx::program_options::variables_map& vm)
{

    //Create component
    comp component0 = hpx::new_<server::comp>(hpx::find_here());
    comp component1 = hpx::new_<server::comp>(hpx::find_here());


    //Register components name in agas
    std::string name = "component";
    hpx::register_with_basename(name, component0.get_id(), 0);
    hpx::register_with_basename(name, component1.get_id(), 1);


    //Create counters
    performance_counter counter0(hpx::util::format(
        "/examples{{locality#{}/component#0}}/basename/explicit",
        hpx::get_locality_id()));
    performance_counter counter1(hpx::util::format(
        "/examples{{locality#{}/component#1}}/basename/explicit",
        hpx::get_locality_id()));

    //Read counter before component is created -- Doesn't work!!!!!!!!
    //std::cout << "Counter value: " << counter.get_value<double>().get() << std::endl;

    component0.add(5);
    component1.add(-5);

    std::cout << "Component 0 value: " << component0.get() << std::endl;
    std::cout << "Counter 0 value: " << counter0.get_value<double>().get() << std::endl << std::endl;
    std::cout << "Component 1 value: " << component1.get() << std::endl;
    std::cout << "Counter 1 value: " << counter1.get_value<double>().get() << std::endl << std::endl;

    component0.add(5);
    component1.add(-5);

    std::cout << "Component 0 value: " << component0.get() << std::endl;
    std::cout << "Counter 0 value: " << counter0.get_value<double>().get() << std::endl << std::endl;
    std::cout << "Component 1 value: " << component1.get() << std::endl;
    std::cout << "Counter 1 value: " << counter1.get_value<double>().get() << std::endl << std::endl;


    hpx::finalize();
    return 0;
}

int main(int argc, char* argv[])
{

    // Initialize and run HPX.
    return hpx::init();
}
