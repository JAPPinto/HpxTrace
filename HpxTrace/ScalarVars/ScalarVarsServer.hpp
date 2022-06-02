#ifndef _SCALARVARSSERVER_H
#define _SCALARVARSSERVER_H

#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>

#include <cstdint>
#include "ScalarVars.hpp"

//inherits from hpx::components::locking_hook to ensure  thread safety 
//as it is a component, it needs to inherit from hpx::components::component_base

class ScalarVarsServer : public hpx::components::locking_hook< hpx::components::component_base<ScalarVarsServer> >{

    private:
        ScalarVars vars;
    public:


        bool store_double(std::string name, double d){
            return vars.store_double(name, d);
        }
        bool store_string(std::string name, std::string s){
            return vars.store_string(name, s);
        }


        hpx::util::optional<double> get_double(std::string name){
            return vars.get_double(name);
        }

        hpx::util::optional<std::string> get_string(std::string name){
            return vars.get_string(name);

        }

        // Each of the exposed functions needs to be encapsulated into an
        // action type, generating all required boilerplate code for threads,
        // serialization, etc.
        HPX_DEFINE_COMPONENT_ACTION(ScalarVarsServer, store_double);
        HPX_DEFINE_COMPONENT_ACTION(ScalarVarsServer, get_double);
        HPX_DEFINE_COMPONENT_ACTION(ScalarVarsServer, store_string);
        HPX_DEFINE_COMPONENT_ACTION(ScalarVarsServer, get_string);
};


//Declare the necessary component action boilerplate code
HPX_REGISTER_ACTION_DECLARATION(ScalarVarsServer::store_double_action, ScalarVarsServer_store_double_action);
HPX_REGISTER_ACTION_DECLARATION(ScalarVarsServer::get_double_action, ScalarVarsServer_get_double_action)
HPX_REGISTER_ACTION_DECLARATION(ScalarVarsServer::store_string_action, ScalarVarsServer_store_string_action);
HPX_REGISTER_ACTION_DECLARATION(ScalarVarsServer::get_string_action, ScalarVarsServer_get_string_action)

#endif