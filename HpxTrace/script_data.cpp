#include <hpx/hpx.hpp>
#include <hpx/modules/runtime_components.hpp>

#include "script_data_server/script_data.hpp"


// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<server::script_data> script_data_type;

HPX_REGISTER_COMPONENT(script_data_type, script_data);

// Serialization support for script_data actions.
HPX_REGISTER_ACTION( script_data_type::wrapped_type::store_double_action, script_data_store_double_action);
HPX_REGISTER_ACTION( script_data_type::wrapped_type::get_double_action, script_data_get_double_action);
HPX_REGISTER_ACTION( script_data_type::wrapped_type::is_double_action, script_data_is_double_action);
HPX_REGISTER_ACTION( script_data_type::wrapped_type::store_string_action, script_data_store_string_action);
HPX_REGISTER_ACTION( script_data_type::wrapped_type::get_string_action, script_data_get_string_action);
HPX_REGISTER_ACTION( script_data_type::wrapped_type::is_string_action, script_data_is_string_action);