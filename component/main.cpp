#include <hpx/hpx_main.hpp>
#include <hpx/include/util.hpp>

#include "test.hpp"

#include <iostream>
#include <string>
#include <vector>


void print(test component){
	std::cout << component.get() 
	<< " from component " << component.get_id() 
	//<< "    " << hpx::get_locality_name(component.get_id()).get()
	<< std::endl;

}

//Test creating component in different localities
void test1(){

	// Find the localities connected to this application.
	std::vector<hpx::id_type> localities = hpx::find_all_localities();


	std::string s = "Hello";
	test component_client_1 = hpx::new_<server::test>(localities[0], s);
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


}

//Test how components' id work
void test2(){

	// Find the localities connected to this application.
	std::vector<hpx::id_type> localities = hpx::find_all_localities();

	//Create component
	std::string s = "Hello";
	test c1 = hpx::new_<server::test>(localities[0], s);
	std::cout << c1.get() << std::endl;

	//Register component name in agas
	std::string name = "component 1";
	hpx::agas::register_name(name, c1.get_id());

	//Compare original id with resolved id
	hpx::id_type id = hpx::agas::resolve_name(name).get();
	std::cout << c1.get_id() << std::endl;
	std::cout << id << std::endl;

	//Create new client
	test c2(hpx::agas::resolve_name(name).get());
	std::cout << c2.get() << std::endl;


	hpx::async([]() {
		std::string n = "component 1";
		test c3(hpx::agas::resolve_name(n).get());
		std::cout << c3.get() << std::endl;

	}, localities[0]);
	
	



	//test component_client_4 = hpx::new_<server::test>(localities[0], component_client_1.get_id());
	//std::cout << component_client_4.get() << std::endl;

}




int main(){

	test2();
	return 0;
}

