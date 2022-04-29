#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/optional.hpp>


template <typename T>
class SMapServer
	: public hpx::components::locking_hook<hpx::components::component_base<SMapServer<T>>>{

	private:
		std::map<std::string,T> values;

	public:
    	typedef T value_type;
		
		void store(std::string key, value_type value){
			values[key] = value;
		}

		hpx::util::optional<value_type> get(std::string key){
			if (exists(key)) return values[key];
			return {};
		}
		bool exists(std::string key){
		    auto it = values.find(key);
		    if ( it != values.end()){
		        return true;
		    }
		    return false;
		}


		// Each of the exposed functions needs to be encapsulated into an
        // action type, generating all required boilerplate code for threads,
        // serialization, etc.
		HPX_DEFINE_COMPONENT_ACTION(SMapServer, store);
		HPX_DEFINE_COMPONENT_ACTION(SMapServer, get);

};

//Declare the necessary component action boilerplate code
#define REGISTER_SMAPSERVER_DECLARATON(type)                    \
    HPX_REGISTER_ACTION_DECLARATION(                            \
        SMapServer<type>::store_action,                         \
        BOOST_PP_CAT(__SMapServer_store_action_, type));        \
                                                                \
    HPX_REGISTER_ACTION_DECLARATION(                            \
        SMapServer<type>::get_action,                           \
        BOOST_PP_CAT(__SMapServer_get_action_, type));          \
/**/

#define REGISTER_SMAPSERVER(type)                               \
    HPX_REGISTER_ACTION(                                        \
        SMapServer<type>::store_action,                         \
        BOOST_PP_CAT(__SMapServer_store_action_, type));        \
                                                                \
    HPX_REGISTER_ACTION(                                        \
        SMapServer<type>::get_action,                           \
        BOOST_PP_CAT(__SMapServer_get_action_, type));          \
                                                                \
                                                                \
    typedef ::hpx::components::component<                       \
        SMapServer<type>                                        \
    > BOOST_PP_CAT(__SMapServer_, type);                        \
    HPX_REGISTER_COMPONENT(BOOST_PP_CAT(__SMapServer_, type))             



using std::string;

REGISTER_SMAPSERVER_DECLARATON(double);
REGISTER_SMAPSERVER(double);
REGISTER_SMAPSERVER_DECLARATON(string);
REGISTER_SMAPSERVER(string);