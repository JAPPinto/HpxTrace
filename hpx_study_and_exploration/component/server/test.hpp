#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>

#include <cstdint>

//inherits from hpx::components::locking_hook to ensure  thread safety 
//as it is a component, it needs to inherit from hpx::components::component_base

namespace server{
	class test : public hpx::components::locking_hook< hpx::components::component_base<test> >{

		private:
			std::string content = "";

		public:
			test() : content("") {}

			test(std::string initial_content) : content(initial_content) {}

			void store(std::string new_content){
				content = new_content;
			}

			std::string get(){
				return content;
			}


			// Each of the exposed functions needs to be encapsulated into an
	        // action type, generating all required boilerplate code for threads,
	        // serialization, etc.
			HPX_DEFINE_COMPONENT_ACTION(test, store);
			HPX_DEFINE_COMPONENT_ACTION(test, get);
	};
}

//Declare the necessary component action boilerplate code
HPX_REGISTER_ACTION_DECLARATION( server::test::store_action, test_store_action);
HPX_REGISTER_ACTION_DECLARATION( server::test::get_action, test_get_action);
