#ifndef _MAPVARSSERVER_H
#define _MAPVARSSERVER_H

#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <cstdint>
#include "MapVars.hpp"

//inherits from hpx::components::locking_hook to ensure  thread safety 
//as it is a component, it needs to inherit from hpx::components::component_base

class MapVarsServer : public hpx::components::locking_hook< hpx::components::component_base<MapVarsServer> >{

    typedef boost::variant<double, std::string> Variant;
    typedef std::vector<Variant> VariantList;


    private:
        MapVars vars;
    public:


        bool store_double(std::string name, VariantList keys, double d){
            return vars.store_double(name, keys, d);
        }
        bool store_string(std::string name, VariantList keys, std::string s){
            return vars.store_string(name, keys, s);
        }


        hpx::util::optional<double> get_double(std::string name, VariantList keys){
            return vars.get_double(name, keys);
        }

        hpx::util::optional<std::string> get_string(std::string name, VariantList keys){
            return vars.get_string(name, keys);

        }

        // Each of the exposed functions needs to be encapsulated into an
        // action type, generating all required boilerplate code for threads,
        // serialization, etc.
        HPX_DEFINE_COMPONENT_ACTION(MapVarsServer, store_double);
        HPX_DEFINE_COMPONENT_ACTION(MapVarsServer, get_double);
        HPX_DEFINE_COMPONENT_ACTION(MapVarsServer, store_string);
        HPX_DEFINE_COMPONENT_ACTION(MapVarsServer, get_string);
};


//Declare the necessary component action boilerplate code
HPX_REGISTER_ACTION_DECLARATION(MapVarsServer::store_double_action, HpxTrace_MapVarsServer_store_double_action);
HPX_REGISTER_ACTION_DECLARATION(MapVarsServer::get_double_action, HpxTrace_MapVarsServer_get_double_action)
HPX_REGISTER_ACTION_DECLARATION(MapVarsServer::store_string_action, HpxTrace_MapVarsServer_store_string_action);
HPX_REGISTER_ACTION_DECLARATION(MapVarsServer::get_string_action, HpxTrace_MapVarsServer_get_string_action)

#endif