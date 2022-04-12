#include <hpx/hpx.hpp>
#include <hpx/modules/runtime_components.hpp>

#include "ScalarVarsServer.hpp"


// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<ScalarVarsServer> ScalarVarsServer_type;

HPX_REGISTER_COMPONENT(ScalarVarsServer_type, ScalarVarsServer);

// Serialization support for ScalarVarsServer actions.
HPX_REGISTER_ACTION( ScalarVarsServer_type::wrapped_type::store_double_action, ScalarVarsServer_store_double_action);
HPX_REGISTER_ACTION( ScalarVarsServer_type::wrapped_type::store_string_action, ScalarVarsServer_store_string_action);
HPX_REGISTER_ACTION( ScalarVarsServer_type::wrapped_type::get_double_action, ScalarVarsServer_get_double_action);
HPX_REGISTER_ACTION( ScalarVarsServer_type::wrapped_type::get_string_action, ScalarVarsServer_get_string_action);


