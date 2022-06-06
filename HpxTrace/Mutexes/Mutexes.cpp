#include <hpx/hpx.hpp>
#include <hpx/modules/runtime_components.hpp>

#include "MutexesServer.hpp"


// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<MutexesServer> MutexesServer_type;

HPX_REGISTER_COMPONENT(MutexesServer_type, MutexesServer);

// Serialization support for MutexesServer actions.
HPX_REGISTER_ACTION( MutexesServer_type::wrapped_type::lock_action, HpxTrace_MutexesServer_lock_action);
HPX_REGISTER_ACTION( MutexesServer_type::wrapped_type::unlock_action, HpxTrace_MutexesServer_unlock_action);


