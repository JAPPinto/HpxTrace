#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

using hpx::naming::id_type;

class ScriptData
{


public:

    id_type locality;
    std::string locality_name;

    id_type local_scalar_vars;
    id_type global_scalar_vars;
    

    id_type local_map_vars;
    id_type global_map_vars;

    id_type local_mutexes;
    id_type global_mutexes;
    id_type local_aggregation;
    std::vector<id_type> aggregations;

    ScriptData(){}

    ScriptData(id_type loc){
            locality_name = hpx::get_locality_name(loc).get();
        }

    //local_aggregations //juntas no final
        
    //needed so it can be used as an action argument
    template<class Archive>
    void serialize(Archive &a, const unsigned version){
        a & locality & locality_name 
        & local_scalar_vars & global_scalar_vars
        & local_map_vars & global_map_vars
        & local_mutexes & global_mutexes
        & local_aggregation & aggregations;
   }
};