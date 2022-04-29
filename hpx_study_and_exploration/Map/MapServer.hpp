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
        
        void store(key_type key, value_type value){
            vars[key] = value;
        }

        hpx::util::optional<value_type> get(key_type key){
            if (exists(key)) return vars[key];
            return {};
        }
        bool exists(key_type key){
            auto it = vars.find(key);
            if ( it != vars.end()){
                return true;
            }
            return false;
        }

        HPX_DEFINE_COMPONENT_ACTION(MapServer, store);
        HPX_DEFINE_COMPONENT_ACTION(MapServer, get);
        HPX_DEFINE_COMPONENT_ACTION(MapServer, exists);

};

using std::string;

#define REGISTER_MAP_DECLARATION(a, b)                                \
    typedef MapServer<a, b> HPX_PP_CAT(MapServer, __LINE__);          \
    HPX_REGISTER_ACTION_DECLARATION(                                  \
        HPX_PP_CAT(MapServer, __LINE__)::get_action,                  \
        HPX_PP_CAT(__MapServer_get_action_, HPX_PP_CAT(a,b)))         \
    HPX_REGISTER_ACTION_DECLARATION(                                  \
        HPX_PP_CAT(MapServer, __LINE__)::store_action,                \
        HPX_PP_CAT(__MapServer_store_action_, HPX_PP_CAT(a,b)))       \


#define REGISTER_MAP(a, b)                                            \
    typedef MapServer<a,b> HPX_PP_CAT(MapServer, __LINE__);           \
    HPX_REGISTER_ACTION(                                              \
        HPX_PP_CAT(MapServer, __LINE__)::get_action,                  \
        HPX_PP_CAT(__MapServer_get_action_,HPX_PP_CAT(a,b)))          \
    HPX_REGISTER_ACTION(                                              \
        HPX_PP_CAT(MapServer, __LINE__)::store_action,                \
        HPX_PP_CAT(__MapServer_store_action_,HPX_PP_CAT(a,b)))        \
    typedef ::hpx::components::component<HPX_PP_CAT(                  \
        MapServer, __LINE__)>                                         \
        HPX_PP_CAT(__MapServer_, HPX_PP_CAT(a,b));                    \
    HPX_REGISTER_COMPONENT(HPX_PP_CAT(__MapServer_, HPX_PP_CAT(a,b))) \

REGISTER_MAP_DECLARATION(string,double);
REGISTER_MAP(string,double);
REGISTER_MAP_DECLARATION(string,string);
REGISTER_MAP(string,string);


REGISTER_MAP_DECLARATION(int,int);
REGISTER_MAP(int,int);


typedef boost::variant<double, std::string> Variant;
typedef std::vector<Variant> VariantList;

REGISTER_MAP_DECLARATION(VariantList,double);
REGISTER_MAP(VariantList,double);
REGISTER_MAP_DECLARATION(VariantList,string);
REGISTER_MAP(VariantList,string);