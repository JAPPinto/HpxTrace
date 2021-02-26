#include <hpx/hpx_main.hpp>
#include <hpx/include/util.hpp>

#include "test.hpp"

#include <iostream>
#include <string>
#include <vector>


int main(){

	// Find the localities connected to this application.
	std::vector<hpx::id_type> localities = hpx::find_all_localities();

	// Create a test component either on this locality (if the
	// example is executed on one locality only) or on any of the remote
	// localities (otherwise).
	test component = hpx::new_<server::test>(localities.back());

	std::string s = "Hello";
	component.store(s);

	std::cout << component.get() << std::endl;

	return 0;
}

