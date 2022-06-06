#include <hpx/hpx.hpp>
#include <hpx/modules/runtime_components.hpp>

#include "AggregationsServer.hpp"


// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE();

typedef hpx::components::component<AggregationsServer> AggregationsServer_type;

HPX_REGISTER_COMPONENT(AggregationsServer_type, AggregationsServer);

// Serialization support for AggregationsServer actions.
HPX_REGISTER_ACTION( AggregationsServer_type::wrapped_type::new_aggregation_action, HpxTrace_AggregationsServer_new_aggregation_action);
HPX_REGISTER_ACTION( AggregationsServer_type::wrapped_type::new_lquantize_action, HpxTrace_AggregationsServer_new_lquantize_action);



HPX_REGISTER_ACTION( AggregationsServer_type::wrapped_type::aggregate_action, HpxTrace_AggregationsServer_aggregate_action);

HPX_REGISTER_ACTION( AggregationsServer_type::wrapped_type::get_scalar_action, HpxTrace_AggregationsServer_get_scalar_action);
HPX_REGISTER_ACTION( AggregationsServer_type::wrapped_type::get_average_action, HpxTrace_AggregationsServer_get_average_action);
HPX_REGISTER_ACTION( AggregationsServer_type::wrapped_type::get_quantization_action, HpxTrace_AggregationsServer_get_quantization_action);
HPX_REGISTER_ACTION( AggregationsServer_type::wrapped_type::get_lquantization_action, HpxTrace_AggregationsServer_get_lquantization_action);

HPX_REGISTER_ACTION( AggregationsServer_type::wrapped_type::find_function_action, HpxTrace_AggregationsServer_find_function_action);
HPX_REGISTER_ACTION( AggregationsServer_type::wrapped_type::print_action, HpxTrace_AggregationsServer_print_action);


