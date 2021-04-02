
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>

#include <hpx/modules/program_options.hpp>

#include "comp.hpp"


using hpx::performance_counters::performance_counter;


int hpx_main(hpx::program_options::variables_map& vm)
{
    //Create counter
    performance_counter implicit_counter(hpx::util::format(
        "/example{{locality#{}/total}}/immediate/implicit",
        hpx::get_locality_id()));

    //Read counter before component is created
    std::cout << "Counter value: " << implicit_counter.get_value<double>().get() << std::endl;


    //Create component
    comp component = hpx::new_<server::comp>(hpx::find_here());

    //Register component name in agas
    std::string name = "component";
    hpx::agas::register_name(name, component.get_id());


    component.add(5);
    std::cout << "Component value: " << component.get() << std::endl;

    std::cout << "Counter value: " << implicit_counter.get_value<double>().get() << std::endl;

    component.add(5);
    std::cout << "Component value: " << component.get() << std::endl;
    std::cout << "Counter value: " << implicit_counter.get_value<double>().get() << std::endl;


    hpx::finalize();
    return 0;
}

int main(int argc, char* argv[])
{

    // Initialize and run HPX.
    return hpx::init();
}
