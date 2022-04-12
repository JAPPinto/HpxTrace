#include <hpx/hpx.hpp>
#include <hpx/runtime/components/component_factory.hpp>

#include "server/test.hpp"


// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<server::test> test_type;

HPX_REGISTER_COMPONENT(test_type, test);

// Serialization support for test actions.
HPX_REGISTER_ACTION( test_type::wrapped_type::store_action, test_store_action);
HPX_REGISTER_ACTION( test_type::wrapped_type::get_action, test_get_action);
