#include <hpx/config.hpp>
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




#include <boost/optional.hpp>

#include <boost/serialization/optional.hpp>

#include "parser/parser.cpp"
//#include "parser/validation_grammars.cpp"
//#include "parser/execution_grammars.cpp"


#include "HpxTrace.hpp"





namespace HpxTrace
{

typedef boost::variant<double, std::string> Variant;
typedef std::vector<Variant> VariantList;

#define VARIANT_DOUBLE 0
#define VARIANT_STRING 1


std::map<std::string,apex_event_type> event_types; 
std::vector<hpx::util::interval_timer*> interval_timers; //for counter-create
std::vector<apex_policy_handle*> apex_policies;

std::vector<int> parse_localities(std::string s, int number_of_localities){
    std::vector<int> localities;
    if(s == "") return localities;
    if(s == "[]"){
        for (int i = 0; i < number_of_localities; i++){
            localities.push_back(i);
        }
        return localities;
    }
    s = s.substr(1, s.length()-2);

    std::stringstream ss(s);
    while(ss.good()){
        std::string substr; getline( ss, substr, ',' );
        int i = std::stoi(substr);
        if(i >= 0 && i < number_of_localities){ 
            localities.push_back(i);
        }
    }
    return localities;
}



typedef std::pair<std::map<std::string,double>,std::map<std::string,std::string>> arguments;

void trigger_probe(std::string probe_name, 
    std::map<std::string,double> double_arguments,
    std::map<std::string,std::string> string_arguments){
    
        arguments args;
        args.first = double_arguments;
        args.second = string_arguments;

        std::uint64_t thread_id = reinterpret_cast<std::uint64_t>(hpx::threads::get_self_id().get());
        args.second["&thread"] = std::to_string(thread_id);

        apex::custom_event(event_types[probe_name], &args);
}
void trigger_probe(std::string probe_name){
    trigger_probe(probe_name, {}, {});
} 

void register_user_probe(std::string probe_name, std::string predicate, std::string actions, ScriptData data){

    apex_event_type event_type;
    auto it = event_types.find(probe_name);
    if ( it == event_types.end()){
        event_type = apex::register_custom_event(probe_name);
        event_types[probe_name] = event_type;
    }
    else{
        event_type = it-> second;
    }


    apex_policies.push_back(apex::register_policy(
        event_type,
        [predicate, actions, data](apex_context const& context)->int{
        
            arguments& args = *reinterpret_cast<arguments*>(context.data);

            ScalarVars probe_svars;
            MapVars probe_mvars;
            std::map<std::string,double>& double_arguments = args.first;
            for (auto const& arg : double_arguments){
                //arg.first -> variable name
                //arg.second -> variable value
                probe_svars.store_double(arg.first, arg.second);
            }
            std::map<std::string,std::string>& string_arguments = args.second;
            for (auto const& arg : string_arguments){
                //arg.first -> variable name
                //arg.second -> variable value
                probe_svars.store_string(arg.first, arg.second); 
            }


            if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars, data)){
                parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
            }

        return APEX_NOERROR;
        }
    ));
}

HPX_DEFINE_PLAIN_ACTION(register_user_probe, register_user_probe_action); 


bool read_counter(hpx::performance_counters::performance_counter counter, std::string* counter_name){

    int value = counter.get_value<int>().get();
    //reading the counter from the API does not tigger APEX_SAMPLE_VALUE event to it has to be triggered manually
    apex::sample_value(*counter_name, value);
    return true;
}


bool fill_counter_variables(std::string name, double value, ScalarVars& probe_svars){

    probe_svars.store_string("&counter_name", name);
    probe_svars.store_double("&counter_value", value);

    hpx::performance_counters::counter_path_elements p;

    hpx::performance_counters::counter_status status;
    hpx::error_code ec ;

    try{
        status = get_counter_path_elements(name, p);
    }
    catch(...){
        return false;
    }           

    if (!status_is_valid(status)) return false;

    probe_svars.store_string("&counter_type", '/' + p.objectname_ + '/' + p.countername_);
    probe_svars.store_string("&counter_parameters", p.parameters_);
    probe_svars.store_string("&counter_parent_instance_name", p.parentinstancename_);
    probe_svars.store_string("&counter_parent_instance_index", std::to_string(p.parentinstanceindex_));
    probe_svars.store_string("&counter_instance_name", p.instancename_);
    probe_svars.store_string("&counter_instance_index", std::to_string(p.instanceindex_));
    return true;
}




void register_counter_create_probe(
    std::string probe_arg1, std::string probe_arg2,
    std::string predicate, std::string actions, ScriptData data){

    std::string counter_name = probe_arg1;
    int period = std::stoi(probe_arg2);

            std::cout << "register_counter_create_probe " << counter_name << period << std::endl;


    apex_policies.push_back(apex::register_policy(
        APEX_SAMPLE_VALUE,
        [predicate, actions, counter_name, data](apex_context const& context)->int{
        
            apex::sample_value_event_data& dt = *reinterpret_cast<apex::sample_value_event_data*>(context.data);
                
            if(*dt.counter_name == counter_name){

                ScalarVars probe_svars;
                MapVars probe_mvars;
                fill_counter_variables(*dt.counter_name, dt.counter_value, probe_svars);

                if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars,data)){
                    parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
                }

            }
            return APEX_NOERROR;
        }
    ));

    hpx::performance_counters::performance_counter counter(counter_name);


    std::string* name_ptr = new std::string(counter_name);
    hpx::util::interval_timer* it = new hpx::util::interval_timer(
                                            hpx::util::bind_front(&read_counter, counter, name_ptr), 
                                            period*1000,  //microsecs  100000 - 0.1s
                                            "", true);

    it->start();

    interval_timers.push_back(it); 

}

void register_counter_probe(
    std::string probe_arg,
    std::string predicate, std::string actions, ScriptData data){

    apex_policies.push_back(apex::register_policy(
        APEX_SAMPLE_VALUE, 
        [predicate, actions, probe_arg, data](apex_context const& context)->int{
        
            apex::sample_value_event_data& dt = *reinterpret_cast<apex::sample_value_event_data*>(context.data);
            
            if(probe_arg == "" || *dt.counter_name == probe_arg){

                ScalarVars probe_svars;
                MapVars probe_mvars;         

                bool is_counter = fill_counter_variables(*dt.counter_name, dt.counter_value, probe_svars);
                if(!is_counter) return APEX_NOERROR;

                if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars,data)){
                    parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
                }
            }
            return APEX_NOERROR;
        }
    ));
}



void register_counter_type_probe(std::string probe_arg ,std::string predicate, std::string actions,
    ScriptData data
 ){


    apex_policies.push_back(apex::register_policy(
        APEX_SAMPLE_VALUE,
        [predicate, actions, probe_arg, data](apex_context const& context)->int{
            
            apex::sample_value_event_data& dt = *reinterpret_cast<apex::sample_value_event_data*>(context.data);
            
            hpx::performance_counters::counter_path_elements p;


            hpx::error_code ec; 
            hpx::performance_counters::counter_status status = 
                        get_counter_path_elements(*dt.counter_name, p, ec);


            std::string type_sampled = '/' + p.objectname_ + '/' + p.countername_;

            //if is counter and belongs to the desired type 
            if(&ec != &hpx::throws && type_sampled.find(probe_arg) != -1){

                ScalarVars probe_svars;
                MapVars probe_mvars;
                fill_counter_variables(*dt.counter_name, dt.counter_value, probe_svars);

                if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars,data)){
                    parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
                }
            }

            return APEX_NOERROR;
        }
    ));
    
}

void register_proc_probe(std::string probe_arg, std::string predicate, std::string actions, ScriptData data){
    
    std::string filter = probe_arg;

    apex_policies.push_back(apex::register_policy(
        APEX_SAMPLE_VALUE, 
        [predicate, actions, filter, data](apex_context const& context)->int{
        
            apex::sample_value_event_data& dt = *reinterpret_cast<apex::sample_value_event_data*>(context.data);
            
            if((*dt.counter_name).find(filter) != -1){

                ScalarVars probe_svars;            
                MapVars probe_mvars;

                probe_svars.store_string("&proc_name", *(dt.counter_name));
                probe_svars.store_double("&proc_value", dt.counter_value);

                if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars, data)){
                    parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
                }

            }
            return APEX_NOERROR;
        }
    ));
    
}

HPX_DEFINE_PLAIN_ACTION(register_proc_probe, register_proc_probe_action); 


void fill_task_variables(std::shared_ptr<apex::task_wrapper> tw,
                         apex_event_type event_type,
                         ScalarVars& probe_svars)
{

    std::string task_name = tw->task_id->get_name();
    probe_svars.store_string("&name", task_name);
    probe_svars.store_string("&parent_name", tw->parent->task_id->get_name());
    probe_svars.store_string("&guid", std::to_string(tw->guid));
    probe_svars.store_string("&parent_guid", std::to_string(tw->parent_guid));
    probe_svars.store_string("&thread", std::to_string(tw->thread_id));


    if(event_type == APEX_STOP_EVENT || event_type == APEX_YIELD_EVENT){
        if(event_type == APEX_STOP_EVENT) probe_svars.store_string("&event", "stop");
        else if(event_type == APEX_YIELD_EVENT) probe_svars.store_string("&event", "yield");
        apex::profiler *prof = tw->prof; 
        probe_svars.store_double("&start", (double) prof->start_ns);
        probe_svars.store_double("&end", (double) prof->end_ns);        
        probe_svars.store_double("&allocations", prof->allocations);
        probe_svars.store_double("&frees", prof->frees);
        probe_svars.store_double("&bytes_allocated", prof->bytes_allocated);
        probe_svars.store_double("&bytes_freed", prof->bytes_freed);
        
    }
    else{
        if(event_type == APEX_START_EVENT) probe_svars.store_string("&event", "start");
        else if(event_type == APEX_RESUME_EVENT) probe_svars.store_string("&event", "resume");
    }
}


void register_task_probe(std::string probe_arg1, std::string probe_arg2,
            std::string predicate, std::string actions, ScriptData data){

    std::set<apex_event_type> events;

    if(probe_arg1 == ""){
        events.insert(APEX_START_EVENT);
        events.insert(APEX_RESUME_EVENT);
        events.insert(APEX_STOP_EVENT);
        events.insert(APEX_YIELD_EVENT);
    }

    else{    
        probe_arg1 = probe_arg1.substr(1, probe_arg1.length()-2);
        std::stringstream ss(probe_arg1);
        while(ss.good()){
            std::string substr;
            getline( ss, substr, ',' );
            if(substr == "start") events.insert(APEX_START_EVENT);
            else if(substr == "resume") events.insert(APEX_RESUME_EVENT);
            else if(substr == "stop") events.insert(APEX_STOP_EVENT);
            else if(substr == "yield") events.insert(APEX_YIELD_EVENT);
        }
    }

    std::string task_filter = probe_arg2;

    std::set<apex_policy_handle*> pls = apex::register_policy(
        events,
        [predicate, actions, task_filter, data](apex_context const& context)->int{
        
            std::shared_ptr<apex::task_wrapper> tw = 
                *reinterpret_cast<std::shared_ptr<apex::task_wrapper>*>(context.data);
            std::string task_name = tw->task_id->get_name();

            if(task_filter != task_name && task_filter != "") return 0;

            ScalarVars probe_svars;        
            MapVars probe_mvars;

            fill_task_variables(tw, context.event_type, probe_svars);

            if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars, data)){
                parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
            }

            return APEX_NOERROR;
        }
    );

    for(auto pl : pls){
        apex_policies.push_back(pl);
    }
    
}

HPX_DEFINE_PLAIN_ACTION(register_task_probe, register_task_probe_action); 

void fill_message_variables(apex::message_event_data event_data,
                            apex_event_type event_type,
                            ScalarVars& probe_svars)
{

    if(event_type == APEX_SEND) probe_svars.store_string("&event", "send");
    else if(event_type == APEX_RECV) probe_svars.store_string("&event", "receive");
    probe_svars.store_string("&action", event_data.action_name);
    probe_svars.store_string("&tag", std::to_string(event_data.tag));
    probe_svars.store_string("&source_rank", std::to_string(event_data.source_rank));
    probe_svars.store_string("&source_thread", std::to_string(event_data.source_thread));
    probe_svars.store_string("&target", std::to_string(event_data.target));   
    probe_svars.store_double("&size", event_data.size);
}


void register_message_probe(
    std::string probe_arg,
    std::string predicate, std::string actions, ScriptData data){

    std::set<apex_event_type> events;

    if(probe_arg == ""){
        events.insert(APEX_SEND);
        events.insert(APEX_RECV);
    }
    else{    
        probe_arg = probe_arg.substr(1, probe_arg.length()-2);
        std::stringstream ss(probe_arg);
        while(ss.good()){
            std::string substr;
            getline( ss, substr, ',' );
            if(substr == "send") events.insert(APEX_SEND);
            else if(substr == "receive") events.insert(APEX_RECV);
        }
    }



    std::set<apex_policy_handle*> pls = apex::register_policy(
        events,
        [predicate, actions, data](apex_context const& context)->int{

            apex::message_event_data event_data = *reinterpret_cast<apex::message_event_data*>(context.data);

            ScalarVars probe_svars;        
            MapVars probe_mvars;

            fill_message_variables(event_data, context.event_type, probe_svars);

            if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars, data)){
                parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
            }

            return APEX_NOERROR;
        }
    );
    for(auto pl : pls){
        apex_policies.push_back(pl);
    }
}

HPX_DEFINE_PLAIN_ACTION(register_message_probe, register_message_probe_action); 
 

void execute_begin_probe(std::string actions, ScriptData data){
    ScalarVars probe_svars;    
    MapVars probe_mvars;
    parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
}

HPX_DEFINE_PLAIN_ACTION(execute_begin_probe, execute_begin_probe_action); 


void register_end_probe(std::string actions, ScriptData data){

    hpx::register_shutdown_function(
        [actions, data]()->void{

            ScalarVars probe_svars;
            MapVars probe_mvars;

            parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);   
        }
    );
        
}

HPX_DEFINE_PLAIN_ACTION(register_end_probe, register_end_probe_action); 


void parse_script(std::string script, std::vector<ScriptData>& localities_data){

    //(?: -> non-capturing group
    std::regex rgx_0args("\\s*([a-zA-Z0-9-_]+)(\\[[^\\]]*\\])?()()\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_1args("\\s*([a-zA-Z0-9-]+)(\\[[^\\]]*\\])?::([^:]+)::()\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_2args("\\s*([a-zA-Z0-9-]+)(\\[[^\\]]*\\])?::([^:]+)::([^:]+)::\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");

    std::smatch match;
    std::vector<std::string> matches;

    int n_probes = 0;
    std::regex_constants::match_flag_type fl = std::regex_constants::match_continuous;


    while (
           std::regex_search(script, match, rgx_0args, fl) 
        || std::regex_search(script, match, rgx_1args, fl) 
        || std::regex_search(script, match, rgx_2args, fl) 
    ){

        std::string probe_name = match[1];
        std::string probe_localities = match[2];
        std::string probe_arg1 = match[3];
        std::string probe_arg2 = match[4];
        std::string probe_predicate = match[5];
        std::string probe_actions = match[6];



        if(probe_predicate != "") 
            validate_predicate(probe_predicate.begin(), probe_predicate.end());
        validate_actions(probe_actions.begin(), probe_actions.end(), localities_data[0]);

        //ValidationParser vp;
        //vp.validate_actions(probe_actions.begin(), probe_actions.end(), localities_data[0]);


        //Remainder of script
        matches.push_back(probe_name);
        matches.push_back(probe_localities);
        matches.push_back(probe_arg1);
        matches.push_back(probe_arg2);
        matches.push_back(probe_predicate);
        matches.push_back(probe_actions);
        script = match.suffix();

        n_probes++;
            
    }
    int j = 6;
    for (int i = 0; i < n_probes; i++){

        std::string probe_name = matches[i*j];
        std::string probe_localities = matches[i*j + 1];
        std::string probe_arg1 = matches[i*j + 2];
        std::string probe_arg2 = matches[i*j + 3];
        std::string probe_predicate = matches[i*j + 4];
        std::string probe_actions = matches[i*j + 5];

        std::vector<int> locs = parse_localities(probe_localities, localities_data.size());



//std::vector<int> parse_localities(std::string s, int number_of_localities){

        if(probe_name == "BEGIN"){
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                execute_begin_probe_action execute_action;
                execute_action(sc.locality, probe_actions, sc);
            }  
        }
        else if(probe_name == "END"){
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                register_end_probe_action register_action;
                register_action(sc.locality, probe_actions, sc);
            }  
        }

        else if(probe_name == "counter"){
            register_counter_probe(probe_arg1, probe_predicate, probe_actions, localities_data[0]);
        }
        else if(probe_name == "counter-create"){
            register_counter_create_probe(probe_arg1, probe_arg2, probe_predicate, probe_actions, localities_data[0]);
        }
        else if(probe_name == "counter-type"){
            register_counter_type_probe(probe_arg1, probe_predicate, probe_actions, localities_data[0]);
        }

        else if(probe_name == "proc"){
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                register_proc_probe_action register_action;
                register_action(sc.locality, probe_arg1, probe_predicate, probe_actions, sc);
            }  
        }

        else if(probe_name == "message"){
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                register_message_probe_action register_action;
                register_action(sc.locality, probe_arg1, probe_predicate, probe_actions, sc);
            }
        }
        else if(probe_name == "task"){
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                register_task_probe_action register_action;
                register_action(sc.locality, probe_arg1, probe_arg2, probe_predicate, probe_actions, sc);
            }
        }
        else{
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                register_user_probe_action register_action;
                register_action(sc.locality, probe_name, probe_predicate, probe_actions, sc);
            }
        }


    }

    

    if(n_probes == 0){
        throw std::runtime_error("invalid probes");
    }

}



void init(std::string script){

    std::cout << "Initializing HpxTrace" << std::endl;
    //Find all localities
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();


    typedef VariantList VariantList;
    id_type global_scalar_vars = hpx::new_<ScalarVarsServer>(hpx::find_here()).get();
    id_type global_map_vars = hpx::new_<MapVarsServer>(hpx::find_here()).get();
    id_type global_mutexes = hpx::new_<MutexesServer>(hpx::find_here()).get();

    std::vector<id_type> aggregations;
    for(auto loc : localities){
        aggregations.push_back(hpx::new_<AggregationsServer>(loc).get());
    }

    std::vector<ScriptData> localities_data;
    int i = 0;
    for(auto loc : localities){
        id_type local_scalar_vars = hpx::new_<ScalarVarsServer>(loc).get();
        id_type local_map_vars = hpx::new_<MapVarsServer>(loc).get();

        id_type local_mutexes = hpx::new_<MutexesServer>(loc).get();

        ScriptData sc(loc);

        sc.global_scalar_vars = global_scalar_vars;
        sc.local_scalar_vars = local_scalar_vars;

        sc.global_map_vars = global_map_vars;
        sc.local_map_vars = local_map_vars;

        sc.local_mutexes = local_mutexes;
        sc.global_mutexes = global_mutexes;

        sc.local_aggregation = aggregations[i];
        i++;
        sc.aggregations = aggregations;


        localities_data.push_back(sc);
    }


    parse_script(script, localities_data);

    std::cout << "HpxTrace Initialized" << std::endl;
}

void init(hpx::program_options::variables_map& vm){

    std::string script = vm["script"].as<std::string>();

    std::string file = vm["file"].as<std::string>();

    if(file != ""){
        std::ifstream t(file);
        std::stringstream buffer;
        buffer << t.rdbuf();
        script = buffer.str();
    }

    if(script != ""){
        init(script);
    }


}

void register_command_line_options(hpx::program_options::options_description& desc_commandline){
    desc_commandline.add_options()
        ( "script",
          hpx::program_options::value<std::string>()->default_value(""),
          "script for tracing")
        ;

    desc_commandline.add_options()
        ( "file",
          hpx::program_options::value<std::string>()->default_value(""),
          "file with script for tracing")
        ;
}


void deregister_policies(){
    for (auto policy : apex_policies) {
        apex::deregister_policy(policy);
    }
}

HPX_DEFINE_PLAIN_ACTION(deregister_policies, deregister_policies_action); 



//destruct interval_timers
void finalize(){
    
    std::cout << "HpxTrace finalizing\n";

    for (auto  element : interval_timers) {
        element->~interval_timer();
    }
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
    for(hpx::naming::id_type loc : localities){
        deregister_policies_action deregister_action;
        deregister_action(loc);
    }
    std::cout << "HpxTrace finalized\n";
            
}

}