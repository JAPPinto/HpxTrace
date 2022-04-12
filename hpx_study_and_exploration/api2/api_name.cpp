
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>

#include <hpx/modules/program_options.hpp>

#include "examples.cpp"


#include <hpx/performance_counters/action_invocation_counter_discoverer.hpp>

/*

    Example that uses name_counter in the code

    With name_counter the correspondent component is identified through its registered name
    If the component does not yet exist, get_value() will return 0
*/




using hpx::performance_counters::performance_counter;



//void print() { async<print_action>(this->get_id()).get(); }


int hpx_main(hpx::program_options::variables_map& vm)
{

    //Create counter
    performance_counter implicit_counter(hpx::util::format(
        "/examples{{locality#{}/component}}/name/explicit",
        hpx::get_locality_id()));

    //Read counter before component is created
    std::cout << "Counter value: " << implicit_counter.get_value<double>().get() << std::endl;


    //Find all localities
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();


    //locality#x is irrelevant for now

    //Create component
    comp component = hpx::new_<server::comp>(localities.back());

    //Register component name in agas
    std::string name = "component";
    hpx::agas::register_name(name, component.get_id());


    component.add(5);
    std::cout << "Component value: " << component.get() << std::endl;

    std::cout << "Counter value: " << implicit_counter.get_value<double>().get() << std::endl;

    component.add(5);
    std::cout << "Component value: " << component.get() << std::endl;
    std::cout << "Counter value: " << implicit_counter.get_value<double>().get() << std::endl;


    std::cout << hpx::actions::detail::get_action_id_from_name("comp_read_action");



    std::map<std::string, void (*)(std::string)> funcMap; 
    funcMap["f1"] = server::comp::get_action;

    hpx::finalize();
    return 0;
}

int main(int argc, char* argv[])
{

    // Initialize and run HPX.
    return hpx::init();
}
