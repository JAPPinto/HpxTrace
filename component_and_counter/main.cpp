#include <hpx/hpx_main.hpp>
#include <hpx/include/util.hpp>

#include "comp.hpp"

#include <iostream>
#include <string>
#include <vector>


comp component;

//does not work
//what():  this client_base has no valid shared state: HPX(no_state)
std::int64_t f(bool reset){
	return component.get();
}


int main(){

	std::vector<hpx::id_type> localities = hpx::find_all_localities();

	component = hpx::new_<server::comp>(localities.back());

	component.add(5);

	std::cout << component.get() << std::endl;


    using hpx::performance_counters::performance_counter;

    performance_counter implicit_counter(hpx::util::format(
        "/example{{locality#{}/total}}/immediate/implicit",
        hpx::get_locality_id()));

    implicit_counter.start();

    std::cout <<  implicit_counter.get_value<double>().get() << std::endl;


	return 0;
}

