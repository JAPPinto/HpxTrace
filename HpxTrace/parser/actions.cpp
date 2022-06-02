#ifndef _ACTIONS_
#define _ACTIONS_

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <apex_api.hpp>
#include <regex>
#include <boost/bind.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/phoenix/bind/bind_member_function.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <task_identifier.hpp>
#include <task_wrapper.hpp>
#include <event_listener.hpp>

#include "../ScalarVars/ScalarVarsServer.hpp"
#include "../MapVars/MapVarsServer.hpp"
#include "../Aggregations/AggregationsServer.hpp"
#include "../ScriptData.cpp"
#include "../Mutexes/MutexesServer.hpp"


#include <boost/optional.hpp>
#include <boost/serialization/optional.hpp>





namespace HpxTrace
{

typedef boost::variant<double, std::string> Variant;
typedef std::vector<Variant> VariantList;


using hpx::naming::id_type;


std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();


bool get_probe_dvar(ScalarVars& vars, std::string name, double& value){
    hpx::util::optional op = vars.get_double(name);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

bool get_probe_stvar(ScalarVars& vars, std::string name, std::string& value){
    hpx::util::optional op = vars.get_string(name);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}


bool store_probe_dvar(ScalarVars& vars, std::string name, double value){
    return vars.store_double(name, value);
}

bool store_probe_stvar(ScalarVars& vars, std::string name, std::string value){
    return vars.store_string(name, value);
}


bool get_comp_dvar(hpx::naming::id_type id, std::string name, double& value){
    hpx::util::optional op = ScalarVarsServer::get_double_action()(id, name);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

bool get_comp_stvar(hpx::naming::id_type id, std::string name, std::string& value){
    hpx::util::optional op = ScalarVarsServer::get_string_action()(id, name);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

bool store_comp_dvar(hpx::naming::id_type id, std::string name, double value){
    return ScalarVarsServer::store_double_action()(id, name, value);
}

bool store_comp_stvar(hpx::naming::id_type id, std::string name, std::string value){
    return ScalarVarsServer::store_string_action()(id, name, value);
}

bool get_probe_dmap(MapVars& maps, std::string name, VariantList keys, double& value){
    hpx::util::optional op = maps.get_double(name, keys);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

bool get_probe_stmap(MapVars& maps, std::string name, VariantList keys, std::string& value){
    hpx::util::optional op = maps.get_string(name, keys);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

bool store_probe_dmap(MapVars& maps, std::string name, VariantList keys, double value){
    return maps.store_double(name, keys, value);
}

bool store_probe_stmap(MapVars& maps, std::string name, VariantList keys, std::string value){
    return maps.store_string(name, keys, value);
}

bool get_comp_dmap(hpx::naming::id_type id, std::string name, VariantList keys, double& value){
    hpx::util::optional op = MapVarsServer::get_double_action()(id, name, keys);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

bool get_comp_stmap(hpx::naming::id_type id, std::string name, VariantList keys, std::string& value){
    hpx::util::optional op = MapVarsServer::get_string_action()(id, name, keys);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

bool store_comp_dmap(hpx::naming::id_type id, std::string name, VariantList keys, double value){
    return MapVarsServer::store_double_action()(id, name, keys, value);
}

bool store_stmap(hpx::naming::id_type id, std::string name, VariantList keys, std::string value){
    return MapVarsServer::store_string_action()(id, name, keys, value);
}

void comp_lock(hpx::naming::id_type id, VariantList key){
    MutexesServer::lock_action()(id, key);
}


void comp_unlock(hpx::naming::id_type id, VariantList key){
    MutexesServer::unlock_action()(id, key);
}


void print_value(Variant v){
    std::stringstream msg;
    msg << "API PRINT: " << v << std::endl;
    std::cout << msg.str();
}




std::string to_string(double d){
    std::string str = std::to_string (d);
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    str.erase(str.find_last_not_of('.') + 1, std::string::npos);
    return str;
}

double to_double(std::string s){
    return std::stod(s);
}

void aggregate(hpx::naming::id_type id, std::string name, VariantList keys, double value){
    AggregationsServer::aggregate_action()(id, name, keys, value);
}

void validate_aggregating_function(
    std::vector<hpx::naming::id_type> ids,
    std::string name, 
    std::string new_func
    ){

        std::string prev_func;
        //Melhorar para ser paralelo
        for(auto id : ids){
            prev_func = AggregationsServer::new_aggregation_action()(id, new_func, name);
        }

        if (prev_func != "" && new_func != prev_func){
            std::string error = "aggregation redefined";
            error += " current: @" + name + " = " + new_func + "() \n";
            error += "previous: @" + name + " = " + prev_func + "() \n";
            throw std::runtime_error(error);
        }    
}

void validate_lquantization(
    std::vector<hpx::naming::id_type> ids,
    std::string name, 
    int lower_bound, int upper_bound, int step
    )
{


    LquantizeResult res;
    //Melhorar para ser paralelo
    for(auto id : ids){
        res = AggregationsServer::new_lquantize_action()(id, name, lower_bound, upper_bound, step);
    }

    if(res.function != "lquantize"){
        std::string error = "aggregation redefined";
        error += " current: @" + name + " = " + "lquantize" + "() \n";
        error += "previous: @" + name + " = " + res.function + "() \n";
        throw std::runtime_error(error);
    }
    else if(res.lower_bound != lower_bound || res.upper_bound != upper_bound || res.step != step){
        std::string error = "lquantization parameters redefined\n";
        error += " current: @" + name + " = lquantize(_, " + std::to_string(lower_bound) +
            ", " + std::to_string(upper_bound) + ", " + std::to_string(step) + ")\n";
        error += "previous: @" + name + " = lquantize(_, " + std::to_string(res.lower_bound) +
            ", " + std::to_string(res.upper_bound) + ", " + std::to_string(res.step) + ")\n";
        throw std::runtime_error(error);
    }



}


int elapsed_time(){
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds> (now - start_time).count();
}

double round_(double d){return std::round(d);}
double ceil_(double d){return std::ceil(d);}
double floor_(double d){return std::floor(d);}
double fmod(double a, double b){return std::fmod(a,b);}

}

#endif