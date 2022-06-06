#include <hpx/hpx.hpp>
#include <hpx/modules/runtime_components.hpp>

#include "MapVarsServer.hpp"


// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<MapVarsServer> MapVarsServer_type;

HPX_REGISTER_COMPONENT(MapVarsServer_type, MapVarsServer);

// Serialization support for MapVarsServer actions.
HPX_REGISTER_ACTION( MapVarsServer_type::wrapped_type::store_double_action, HpxTrace_MapVarsServer_store_double_action);
HPX_REGISTER_ACTION( MapVarsServer_type::wrapped_type::store_string_action, HpxTrace_MapVarsServer_store_string_action);
HPX_REGISTER_ACTION( MapVarsServer_type::wrapped_type::get_double_action, HpxTrace_MapVarsServer_get_double_action);
HPX_REGISTER_ACTION( MapVarsServer_type::wrapped_type::get_string_action, HpxTrace_MapVarsServer_get_string_action);


