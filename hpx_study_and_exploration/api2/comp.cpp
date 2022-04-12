#include <hpx/hpx.hpp>
#include <hpx/runtime/components/component_factory.hpp>

#include "comp_server/comp.hpp"


// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<server::comp> comp_type;

HPX_REGISTER_COMPONENT(comp_type, comp);

// Serialization support for comp actions.
HPX_REGISTER_ACTION( comp_type::wrapped_type::add_action, comp_add_action);
HPX_REGISTER_ACTION( comp_type::wrapped_type::get_action, comp_get_action);
HPX_REGISTER_ACTION( comp_type::wrapped_type::read_action, comp_read_action);

