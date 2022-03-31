#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/optional.hpp>
//inherits from hpx::components::locking_hook to ensure  thread safety 
//as it is a component, it needs to inherit from hpx::components::component_base
template <typename T>
class VarsCompServer
	: public hpx::components::locking_hook< hpx::components::component_base<VarsCompServer<T>>>{



	private:
		std::map<std::string,T> vars;

	public:
    	typedef T argument_type;
		
		void store_var(std::string name, argument_type value){
			vars[name] = value;
		}

		hpx::util::optional<argument_type> get_var(std::string name){
			if (exists(name)) return vars[name];
			return {};
		}
		bool exists(std::string name){
		    auto it = vars.find(name);
		    if ( it != vars.end()){
		        return true;
		    }
		    return false;
		}


		// Each of the exposed functions needs to be encapsulated into an
        // action type, generating all required boilerplate code for threads,
        // serialization, etc.
		HPX_DEFINE_COMPONENT_ACTION(VarsCompServer, store_var);
		HPX_DEFINE_COMPONENT_ACTION(VarsCompServer, get_var);
		HPX_DEFINE_COMPONENT_ACTION(VarsCompServer, exists);

};


//Declare the necessary component action boilerplate code
#define REGISTER_VARSCOMP_DECLARATON(type)                                    \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        VarsCompServer<type>::store_var_action,                               \
        BOOST_PP_CAT(__VarsCompServer_store_var_action_, type));              \
                                                                              \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        VarsCompServer<type>::get_var_action,                                 \
        BOOST_PP_CAT(__VarsCompServer_get_var_action_, type));                \
                                                                              \
    HPX_REGISTER_ACTION_DECLARATION(                                          \
        VarsCompServer<type>::exists_action,                                  \
        BOOST_PP_CAT(__VarsCompServer_exists_action_, type));                 \

/**/

#define REGISTER_VARSCOMP(type)                                               \
    HPX_REGISTER_ACTION(                                                      \
        VarsCompServer<type>::store_var_action,                               \
        BOOST_PP_CAT(__VarsCompServer_store_var_action_, type));              \
                                                                              \
    HPX_REGISTER_ACTION(                                                      \
        VarsCompServer<type>::get_var_action,                                 \
        BOOST_PP_CAT(__VarsCompServer_get_var_action_, type));                \
                                                                              \
    HPX_REGISTER_ACTION(                                                      \
        VarsCompServer<type>::exists_action,                                  \
        BOOST_PP_CAT(__VarsCompServer_exists_action_, type));                 \
                                                                              \
    typedef ::hpx::components::component<                                     \
        VarsCompServer<type>                                                  \
    > BOOST_PP_CAT(__VarsCompServer_, type);                                  \
    HPX_REGISTER_COMPONENT(BOOST_PP_CAT(__VarsCompServer_, type))             



using std::string;

REGISTER_VARSCOMP_DECLARATON(double);
REGISTER_VARSCOMP(double);
REGISTER_VARSCOMP_DECLARATON(string);
REGISTER_VARSCOMP(string);

