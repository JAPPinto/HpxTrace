#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>

#include <cstdint>

//inherits from hpx::components::locking_hook to ensure  thread safety 
//as it is a component, it needs to inherit from hpx::components::component_base

class CompServer : public hpx::components::locking_hook< hpx::components::component_base<CompServer> >{

	private:
		int value;

	public:

		CompServer();
		CompServer(int n);

		void add(int n);

		int get();

		// Each of the exposed functions needs to be encapsulated into an
        // action type, generating all required boilerplate code for threads,
        // serialization, etc.
		HPX_DEFINE_COMPONENT_ACTION(CompServer, add);
		HPX_DEFINE_COMPONENT_ACTION(CompServer, get);
};


//Declare the necessary component action boilerplate code
HPX_REGISTER_ACTION_DECLARATION(CompServer::add_action, comp_add_action);
HPX_REGISTER_ACTION_DECLARATION(CompServer::get_action, comp_get_action);
