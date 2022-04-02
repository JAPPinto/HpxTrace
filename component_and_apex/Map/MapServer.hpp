#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/optional.hpp>
//inherits from hpx::components::locking_hook to ensure  thread safety 
//as it is a component, it needs to inherit from hpx::components::component_base
template <typename K, typename V>
class MapServer
	: public hpx::components::locking_hook< hpx::components::component_base<MapServer<K,V>>>{



	private:
		std::map<K,V> vars;

	public:
    	typedef K key_type ;
    	typedef V value_type;
		
		void store_var(key_type name, value_type value){
			vars[name] = value;
		}

		hpx::util::optional<value_type> get_var(key_type name){
			if (exists(name)) return vars[name];
			return {};
		}
		bool exists(key_type name){
		    auto it = vars.find(name);
		    if ( it != vars.end()){
		        return true;
		    }
		    return false;
		}


		// Each of the exposed functions needs to be encapsulated into an
        // action type, generating all required boilerplate code for threads,
        // serialization, etc.
		HPX_DEFINE_COMPONENT_ACTION(MapServer, store_var);
		HPX_DEFINE_COMPONENT_ACTION(MapServer, get_var);
		HPX_DEFINE_COMPONENT_ACTION(MapServer, exists);

};

using std::string;

#define REGISTER_MAP_DECLARATION(a, b)                            \
	typedef MapServer<a, b> HPX_PP_CAT(MapServer, __LINE__);      \
    HPX_REGISTER_ACTION_DECLARATION(                              \
    	HPX_PP_CAT(MapServer, __LINE__)::get_var_action,          \
    	HPX_PP_CAT(__MapServer_get_var_action_, b))               \
    HPX_REGISTER_ACTION_DECLARATION(                              \
    	HPX_PP_CAT(MapServer, __LINE__)::store_var_action,        \
    	HPX_PP_CAT(__MapServer_store_var_action_, b))             \


#define REGISTER_MAP(a, b)                                        \
	typedef MapServer<a,b> HPX_PP_CAT(MapServer, __LINE__);       \
    HPX_REGISTER_ACTION(                                          \
        HPX_PP_CAT(MapServer, __LINE__)::get_var_action,          \
        HPX_PP_CAT(__MapServer_get_value_action_,b))              \
    HPX_REGISTER_ACTION(                                          \
        HPX_PP_CAT(MapServer, __LINE__)::store_var_action,        \
        HPX_PP_CAT(__MapServer_store_value_action_,b))            \
    typedef ::hpx::components::component<HPX_PP_CAT(              \
        MapServer, __LINE__)>                                     \
        HPX_PP_CAT(__MapServer_, b);                              \
    HPX_REGISTER_COMPONENT(HPX_PP_CAT(__MapServer_, b))           \

REGISTER_MAP_DECLARATION(string,double);
REGISTER_MAP(string,double);
REGISTER_MAP_DECLARATION(string,string);
REGISTER_MAP(string,string);
