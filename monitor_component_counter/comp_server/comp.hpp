
#ifndef __COMP_SERVER__
#define __COMP_SERVER__


#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>

#include <cstdint>

//inherits from hpx::components::locking_hook to ensure  thread safety 
//as it is a component, it needs to inherit from hpx::components::component_base

namespace server{
	class comp : public hpx::components::locking_hook< hpx::components::component_base<comp> >{

		private:
			int value = 0;

		public:
			void add(int n){
				value += n;
			}

			int get(){
				return value;
			}


			// Each of the exposed functions needs to be encapsulated into an
	        // action type, generating all required boilerplate code for threads,
	        // serialization, etc.
			HPX_DEFINE_COMPONENT_ACTION(comp, add);
			HPX_DEFINE_COMPONENT_ACTION(comp, get);
	};
}

//Declare the necessary component action boilerplate code
HPX_REGISTER_ACTION_DECLARATION( server::comp::add_action, comp_add_action);
HPX_REGISTER_ACTION_DECLARATION( server::comp::get_action, comp_get_action);

#endif
