#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>


//#include "comp.hpp"
#include "examples.cpp"


#include <apex_api.hpp>


using hpx::performance_counters::performance_counter;


//precisa de receber um componente de qualquer tipo
//void monitor_component(comp, name?, params)


void test(){

    usleep(1000000);
    
    comp c = hpx::new_<server::comp>(hpx::find_here());
    hpx::agas::register_name("component", c.get_id());


    for (int i = 0; i < 1000000; i++) {
        c.add(1);
    }

}





int hpx_main() {


    test();


    hpx::finalize();
    return 0;
}


int main(int argc, char* argv[]) {

    register_component_counter<::server::comp::get_action>("/example/name");

    hpx::init(argc, argv);

    return 0;
}


REGISTER_COMPONENT_COUNTER_FACTORY(::server::comp::get_action);