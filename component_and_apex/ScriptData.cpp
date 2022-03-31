#include "VarsComp/VarsCompClient.hpp"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

using hpx::naming::id_type;

class ScriptData
{


public:

	id_type locality;
	std::string locality_name;
	id_type global_dvars;
	id_type local_dvars;
	id_type global_stvars;
	id_type local_stvars;
    std::map<std::string,std::string> probe_stvars;

    ScriptData(){}

	ScriptData(id_type loc,	id_type gdv, id_type ldv, id_type gsv, id_type lsv) :
		locality(loc), global_dvars(gdv), local_dvars(ldv), global_stvars(gsv), local_stvars(lsv) {
			locality_name = hpx::get_locality_name(loc).get();
			std::cout << "__________" << locality_name << std::endl;
		}

	//local_aggregations //juntas no final
		
	//needed so it can be used as an action argument
 	template<class Archive>
	void serialize(Archive &a, const unsigned version){
		a & locality & global_dvars & local_dvars & global_stvars & local_stvars;
   }
};