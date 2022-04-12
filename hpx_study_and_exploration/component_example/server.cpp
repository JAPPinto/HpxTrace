#include <hpx/hpx.hpp>
#include <hpx/runtime/components/component_factory.hpp>

#include "server.hpp"


CompServer::CompServer() : value(0) { }
CompServer::CompServer(int n) : value(n) { }

void CompServer::add(int n){
	value += n;
}

int CompServer::get(){
	return value;
}

typedef hpx::components::component<CompServer> comp_type;

// Serialization support for comp actions.
//HPX_REGISTER_ACTION( comp_type::wrapped_type::add_action, comp_add_action);
//HPX_REGISTER_ACTION( comp_type::wrapped_type::get_action, comp_get_action);

HPX_REGISTER_ACTION(CompServer::add_action, comp_add_action);
HPX_REGISTER_ACTION(CompServer::get_action, comp_get_action);


HPX_REGISTER_COMPONENT(comp_type, Comp);
// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE();

