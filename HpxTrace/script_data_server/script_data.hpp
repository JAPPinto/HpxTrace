#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>

#include <cstdint>

//inherits from hpx::components::locking_hook to ensure  thread safety 
//as it is a component, it needs to inherit from hpx::components::component_base

namespace server{
	class script_data : public hpx::components::locking_hook< hpx::components::component_base<script_data> >{

		private:
			std::map<std::string,double> dvars; //global variables
			std::map<std::string, std::string> stvars; //global variables
			//std::map<std::string, Aggregation*> aggvars; //aggregation variables
			//std::map<std::string,apex_event_type> event_types; 
			//std::vector<hpx::util::interval_timer*> interval_timers; //for counter-create
			//std::chrono::steady_clock::time_point start_time;

		public:
			void store_double(std::string name, double d){
				dvars[name] = d;
			}

			double get_double(std::string name){
				return dvars[name];
			}
			bool is_double(std::string name){
			    auto it = dvars.find(name);
			    if ( it != dvars.end()){
			        return true;
			    }
			    return false;
			}

			bool is_string(std::string name){
			    auto it = stvars.find(name);
			    if ( it != stvars.end()){
			        return true;
			    }
			    return false;
			}
			
			void store_string(std::string name, std::string s){
				stvars[name] = s;
			}

			std::string get_string(std::string name){
				return stvars[name];
			}

			// Each of the exposed functions needs to be encapsulated into an
	        // action type, generating all required boilerplate code for threads,
	        // serialization, etc.
			HPX_DEFINE_COMPONENT_ACTION(script_data, store_double);
			HPX_DEFINE_COMPONENT_ACTION(script_data, get_double);
			HPX_DEFINE_COMPONENT_ACTION(script_data, is_double);
			HPX_DEFINE_COMPONENT_ACTION(script_data, store_string);
			HPX_DEFINE_COMPONENT_ACTION(script_data, get_string);
			HPX_DEFINE_COMPONENT_ACTION(script_data, is_string);
	};
}

//Declare the necessary component action boilerplate code
HPX_REGISTER_ACTION_DECLARATION( server::script_data::store_double_action, script_data_store_double_action);
HPX_REGISTER_ACTION_DECLARATION( server::script_data::get_double_action, script_data_get_double_action)
HPX_REGISTER_ACTION_DECLARATION( server::script_data::is_double_action, script_data_is_double_action)
HPX_REGISTER_ACTION_DECLARATION( server::script_data::store_string_action, script_data_store_string_action);
HPX_REGISTER_ACTION_DECLARATION( server::script_data::get_string_action, script_data_get_string_action)
HPX_REGISTER_ACTION_DECLARATION( server::script_data::is_string_action, script_data_is_string_action)

