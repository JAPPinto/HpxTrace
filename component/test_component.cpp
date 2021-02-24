#include <hpx/hpx.hpp>
#include <hpx/runtime/components/component_factory.hpp>

#include "server/test_component.hpp"


// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<server::testComponent> testComponent_type;

HPX_REGISTER_COMPONENT(testComponent_type, testComponent);

// Serialization support for testComponent actions.
HPX_REGISTER_ACTION( testComponent_type::wrapped_type::store_action, testComponent_store_action);
HPX_REGISTER_ACTION( testComponent_type::wrapped_type::get_action, testComponent_get_action);
