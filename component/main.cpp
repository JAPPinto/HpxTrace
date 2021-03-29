#include <hpx/hpx_main.hpp>
#include <hpx/include/util.hpp>

#include "test.hpp"

#include <iostream>
#include <string>
#include <vector>


void print(test component){
	std::cout << component.get() 
	<< " from component " << component.get_id() 
	<< "    " << hpx::get_locality_name(component.get_id()).get()
	<< std::endl;

}


int main(){

	// Find the localities connected to this application.
	std::vector<hpx::id_type> localities = hpx::find_all_localities();

	// Create a test component either on this locality (if the
	// example is executed on one locality only) or on any of the remote
	// localities (otherwise).
	test component_client_1 = hpx::new_<server::test>(localities[0]);
	std::string s = "Hello";
	component_client_1.store(s);
	print(component_client_1);

	//Different component instance in the same locality
	test component_client_2 = hpx::new_<server::test>(localities[0]);
	print(component_client_2);

	if(localities.size() > 1){
		//Different component instance in a different locality
		test component_client_3 = hpx::new_<server::test>(localities[1]);
		std::string a = "Hi";
		component_client_3.store(a);
		print(component_client_3);
		
	}

	//test component_client_4 = hpx::new_<server::test>(localities[0], component_client_1.get_id());
	//std::cout << component_client_4.get() << std::endl;


	return 0;
}

