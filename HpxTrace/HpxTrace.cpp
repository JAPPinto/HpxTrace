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
#include "variables_map.cpp"
#include "sample_value_event_data.hpp"
#include "Aggregations/AggregationsServer.hpp"
#include "ScalarVars/ScalarVarsServer.hpp"
#include "MapVars/MapVarsServer.hpp"



#include "Mutexes.cpp"

//#include "task_event.cpp"
#include "script_data.hpp"
#include "ScriptData.cpp"

#include "Map/MapClient.hpp"
#include "Mutexes/MutexesServer.hpp"

#include <boost/optional.hpp>

#include <boost/serialization/optional.hpp>



#define BOOST_SPIRIT_USE_PHOENIX_V3 1



namespace API
{

typedef boost::variant<double, std::string> Variant;
typedef std::vector<Variant> VariantList;

#define VARIANT_DOUBLE 0
#define VARIANT_STRING 1


std::map<std::string,apex_event_type> event_types; 
std::vector<hpx::util::interval_timer*> interval_timers; //for counter-create
std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();



namespace qi = boost::spirit::qi;

namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;

#define NON_EXISTANT -1
#define DOUBLE_VAR 0
#define STRING_VAR 1

using hpx::naming::id_type;


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
/*
void validate_aggregating_function(
    std::map<std::string, std::string> &aggregations,
    std::string name, 
    std::string new_function
    ){
        std::string previous_function = aggregations[name];
        if(previous_function == ""){
            aggregations[name] = new_function;

            if(new_function == "avg"){
                AverageAggregation* g = new AverageAggregation(new_function);
                aggvars[name] = g;
            }
            else if(new_function == "quantize"){
                Quantization* q = new Quantization(new_function);
                aggvars[name] = q;
            }
            //if count, sum, max, min
            else if (new_function != "lquantize"){
                ScalarAggregation* g = new ScalarAggregation(new_function);
                aggvars[name] = g;
            }
        }
        else if (new_function != previous_function){
            std::string error = "aggregation redefined\n";
            error += " current: @" + name + " = " + new_function + "() \n";
            error += "previous: @" + name + " = " + previous_function + "() \n";
            throw std::runtime_error(error);
        }    
}

void validate_lquantize_parameters(
    std::map<std::string, std::vector<int>> &lquantizes,
    std::string name, 
    int lower_bound, int upper_bound, int step
    ){
        auto it = lquantizes.find(name);
        std::vector<int> test = {lower_bound, upper_bound, step};
        if(it == lquantizes.end()){
            lquantizes[name] = test;
            LQuantization* q = new LQuantization("lquantize",lower_bound,upper_bound,step);
            aggvars[name] = q;
        }
        else if(it->second != test){
            std::string error = "lquantization parameters redefined\n";
            error += " current: @" + name + " = lquantize(_, " + std::to_string(lower_bound) +
             ", " + std::to_string(upper_bound) + ", " + std::to_string(step) + ")\n";
            error += "previous: @" + name + " = lquantize(_, " + std::to_string(it->second[0]) +
             ", " + std::to_string(it->second[1]) + ", " + std::to_string(it->second[2]) + ")\n";
            throw std::runtime_error(error);
        }
}
*/


template <typename Iterator>
bool validate_actions(Iterator first, Iterator last, ScriptData data) {
    using qi::double_;
    using qi::int_;
    using qi::char_;
    using qi::_1;
    using qi::_2;
    using qi::_3;
    using qi::_4;
    using qi::_5;
    using qi::_6;
    using qi::_val;
    using qi::_pass;
    using boost::spirit::lit;
    using qi::phrase_parse;
    using ascii::space;
    


    typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleString;
    typedef qi::rule<Iterator, ascii::space_type, double> RuleDouble;
    typedef qi::rule<Iterator, ascii::space_type> Rule;


    //String rules
    qi::rule<Iterator, std::string()> string_content = +(char_ - '"');
    RuleString string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];
    //Var rules

    RuleString probe_var = (char_("a-zA-Z") >> *char_("a-zA-Z0-9_"));
    RuleString local_var = (char_("&") >> *char_("a-zA-Z0-9_"));
    RuleString global_var = (char_("#") >> *char_("a-zA-Z0-9_"));
    RuleString var = probe_var | local_var | global_var;

    Rule value;
    qi::rule<Iterator, ascii::space_type> keys = value % ',';

    Rule probe_map = (probe_var >> '[' >> keys >> ']');
    Rule local_map = (local_var >> '[' >> keys >> ']');
    Rule global_map = (global_var >> '[' >> keys >> ']');
    Rule map = probe_map | local_map | global_map;

    //Function rules
    Rule string_function;
    Rule double_function;
    //Values rules
    Rule string_value = string | string_function | map | var;
    Rule double_value = double_ | double_function | map | var;

    //Expression rules

    Rule string_expression = 
        string_value                            
        >> *('+' >> string_value);

    Rule arithmetic_expression, term, factor;

    arithmetic_expression =
        term                            
        >> *(   ('+' >> term            )
            |   ('-' >> term            )
            )
        ;

    term =
        factor                          
        >> *(   ('*' >> factor          )
            |   ('/' >> factor          )
            |   ('%' >> factor          )
            )
        ;

    factor =
            double_value                 
        |   '(' >> arithmetic_expression  >> ')'
        |   ('-' >> factor               )
        |   ('+' >> factor               )
        ;

    value = arithmetic_expression 
          | string_expression;


    Rule assignment = ((map | var) >> '=' >> arithmetic_expression)
                | ((map | var) >> '=' >> string_expression);  




    
    Rule str = ("str(" >> arithmetic_expression >> ')');
    string_function = str;

    Rule dbl = ("dbl(" >> string_expression >> ')');

    RuleDouble round = ("round(" >> arithmetic_expression >> ')');
    RuleDouble ceil = ("ceil(" >> arithmetic_expression >> ')');
    RuleDouble floor = ("floor(" >> arithmetic_expression >> ')');


    double_function = dbl | round | ceil | floor;




    
     

    std::map<std::string, std::string> aggregations;
    std::map<std::string, std::vector<int>> lquantizes;



    Rule aggregation = 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "count()")
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "count")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "sum" >> '(' >> arithmetic_expression  >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "sum")]
        |
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "avg" >> '(' >> arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "avg")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "min" >> '(' >> arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "min")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "max" >> '(' >> arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "max")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "quantize" >> '(' >> arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "quantize")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "lquantize" >> '(' >>
                                                                      arithmetic_expression >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >>
                                                                      ')')
        [phx::bind(&validate_lquantization, phx::ref(data.aggregations), _1,_2,_3,_4)];

    qi::rule<Iterator, ascii::space_type> localities = int_ % ',';


    Rule print = ("print(" >> arithmetic_expression >> ')')
               | ("print(" >> string_expression >> ')')
               | (lit("print(") >> '@' >> var >> ')')
               | (lit("global_print(") >> '@' >> var >> ')')
               | (lit("global_print(") >> '@' >> var >> ',' >> localities >> ')');



    Rule lock =  lit("lock") >> '(' >> keys >> ')';
    Rule unlock =  lit("unlock") >> '(' >> keys >> ')';

    Rule global_lock = (lit("global_lock") >> '(' >> keys >> ')');
        
    Rule global_unlock = (lit("global_unlock") >> '(' >> keys >> ')');

    Rule locks = lock | unlock | global_lock | global_unlock;


    Rule action = assignment | print | aggregation | locks;

    Rule actions = action >> ';' >> *(action >> ';');



    bool r = phrase_parse(
        first,                          /*< start iterator >*/
        last,                           /*< end iterator >*/
        actions,                        /*< the parser >*/
        space
    );


    if (first != last){// fail if we did not get a full match
        std::string error = "Syntax error in action: \n" ;
        for (; (*first) != ';' && first != last; first++)
        {
            error += *first;
        }
        throw std::runtime_error(error);
    }


    return r;
}  

int elapsed_time(){
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds> (now - start_time).count();
}

double round_(double d){return std::round(d);}
double ceil_(double d){return std::ceil(d);}
double floor_(double d){return std::floor(d);}
double fmod(double a, double b){return std::fmod(a,b);}


template <typename Iterator>
bool parse_actions(Iterator first, Iterator last,
    ScalarVars probe_svars,
    MapVars probe_mvars,
    ScriptData data) {
    using qi::double_;
    using qi::char_;
    using qi::int_;
    using qi::_1;
    using qi::_2;
    using qi::_3;
    using qi::_4;
    using qi::_5;
    using qi::_6;
    using qi::_val;
    using qi::_pass;
    using boost::spirit::lit;
    using qi::phrase_parse;
    using ascii::space;
    


    typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleString;
    typedef qi::rule<Iterator, ascii::space_type, double> RuleDouble;
    typedef qi::rule<Iterator, ascii::space_type, bool> RuleBool;
    typedef qi::rule<Iterator, ascii::space_type> Rule;


    //probe -> probe_dvars and probe_stvars 
    //local -> data.local_scalar_vars data.local_scalar_vars
    //global -> data.global_scalar_vars data.global_scalar_vars

    //String rules
    qi::rule<Iterator, std::string()> string_content = +(char_ - '"');
    RuleString string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];
    //Var rules
    RuleString probe_var = (char_("a-zA-Z") >> *char_("a-zA-Z0-9_"));
    RuleString local_var = (char_("&") >> *char_("a-zA-Z0-9_"));
    RuleString global_var = (char_("#") >> *char_("a-zA-Z0-9_"));

    RuleString var = probe_var | local_var | global_var;

    std::string s = "--";

    RuleString probe_stvar = 
        (probe_var [_pass = phx::bind(&get_probe_stvar, phx::ref(probe_svars), _1, phx::ref(s))])
        [_val = phx::ref(s)];

    RuleString local_stvar = 
        (local_var [_pass = phx::bind(&get_comp_stvar, phx::ref(data.local_scalar_vars), _1, phx::ref(s))])
        [_val = phx::ref(s)];

    RuleString global_stvar = 
        (global_var [_pass = phx::bind(&get_comp_stvar, phx::ref(data.global_scalar_vars), _1, phx::ref(s))])
        [_val = phx::ref(s)];


    RuleString string_var = probe_stvar | local_stvar | global_stvar;


    double d = -5;

    RuleDouble probe_dvar = 
        (probe_var [_pass = phx::bind(&get_probe_dvar, phx::ref(probe_svars), _1, phx::ref(d))])
        [_val = phx::ref(d)];

    RuleDouble local_dvar = 
        (local_var[_pass = phx::bind(&get_comp_dvar, phx::ref(data.local_scalar_vars), _1, phx::ref(d))])
        [_val = phx::ref(d)];

    RuleDouble global_dvar = 
        (global_var [_pass = phx::bind(&get_comp_dvar, phx::ref(data.global_scalar_vars), _1, phx::ref(d))])
        [_val = phx::ref(d)];


    RuleDouble double_var = probe_dvar | local_dvar | global_dvar;


    RuleDouble timestamp = lit("timestamp")[_val =  phx::bind(&elapsed_time)];
    RuleString locality = lit("locality")[_val = phx::ref(data.locality_name)];

    qi::rule<Iterator, ascii::space_type, Variant()> value;
    qi::rule<Iterator, ascii::space_type, VariantList> keys = value % ',';


    RuleDouble probe_dmap = (probe_var >> '[' >> keys >> ']')
        [_pass = phx::bind(&get_probe_dmap, phx::ref(probe_mvars), _1, _2, phx::ref(d)),
        _val = phx::ref(d)];

    RuleDouble local_dmap = (local_var >> '[' >> keys >> ']')
        [_pass = phx::bind(&get_comp_dmap, phx::ref(data.local_map_vars), _1, _2, phx::ref(d)),
        _val = phx::ref(d)];

    RuleDouble global_dmap = (global_var >> '[' >> keys >> ']')
        [_pass = phx::bind(&get_comp_dmap, phx::ref(data.global_map_vars), _1, _2, phx::ref(d)),
        _val = phx::ref(d)];
    
    RuleDouble double_map = probe_dmap | local_dmap | global_dmap;


    RuleString probe_stmap = (probe_var >> '[' >> keys >> ']')
        [_pass = phx::bind(&get_probe_stmap, phx::ref(probe_mvars), _1, _2, phx::ref(s)),
        _val = phx::ref(s)];

    RuleString local_stmap = (local_var >> '[' >> keys >> ']')
        [_pass = phx::bind(&get_comp_stmap, phx::ref(data.local_map_vars), _1, _2, phx::ref(s)),
        _val = phx::ref(s)];


    RuleString global_stmap = (global_var >> '[' >> keys >> ']')
        [_pass = phx::bind(&get_comp_stmap, phx::ref(data.global_map_vars), _1, _2, phx::ref(s)),
        _val = phx::ref(s)];

    RuleString string_map = probe_stmap | local_stmap | global_stmap;


    //Function rules
    RuleString string_function;
    RuleDouble double_function;
    //Values rules
    RuleString string_value = string | string_function | locality | string_map | string_var;
    RuleDouble double_value = double_ | double_function | timestamp| double_map | double_var;

    //Expression rules

    RuleString string_expression = 
        string_value                            [_val = _1]
        >> *('+' >> string_value                [_val = _val + _1]);

    RuleDouble arithmetic_expression, term, factor;

    arithmetic_expression =
        term                            [_val = _1]
        >> *(   ('+' >> term            [_val = _val + _1])
            |   ('-' >> term            [_val = _val - _1])
            )
        ;

    term =
        factor                          [_val = _1]
        >> *(   ('*' >> factor          [_val = _val * _1])
            |   ('/' >> factor          [_val = _val / _1])
            |   ('%' >> factor          [_val = phx::bind(&fmod, _val, _1)])
            )
        ;

    factor =
            double_value                 [_val = _1]
        |   '(' >> arithmetic_expression [_val = _1] >> ')'
        |   ('-' >> factor               [_val = -_1])
        |   ('+' >> factor               [_val = _1])
        ;


    value = arithmetic_expression[_val = _1] 
          | string_expression[_val = _1];




    RuleBool probe_var_assignment = (probe_var >> '=' >> arithmetic_expression)
                      [_val = phx::bind(&store_probe_dvar, phx::ref(probe_svars), _1, _2)]
                    | (probe_var >> '=' >> string_expression)
                      [_val = phx::bind(&store_probe_stvar, phx::ref(probe_svars), _1, _2)];

    RuleBool local_var_assignment = (local_var >> '=' >> arithmetic_expression)
                      [_val = phx::bind(&store_comp_dvar, phx::ref(data.local_scalar_vars), _1, _2)]
                    | (local_var >> '=' >> string_expression)
                      [_val = phx::bind(&store_comp_stvar, phx::ref(data.local_scalar_vars), _1, _2)];

    RuleBool global_var_assignment = (global_var >> '=' >> arithmetic_expression)
                      [_val = phx::bind(&store_comp_dvar, phx::ref(data.global_scalar_vars), _1, _2)]
                    | (global_var >> '=' >> string_expression)
                      [_val = phx::bind(&store_comp_stvar, phx::ref(data.global_scalar_vars), _1, _2)];

    RuleBool probe_map_assignment = 
        (probe_var >> '[' >> keys >> ']' >> '=' >> arithmetic_expression)
        [_val = phx::bind(&store_probe_dmap, phx::ref(probe_mvars), _1, _2, _3)]
        | 
        (probe_var >> '[' >> keys >> ']' >> '=' >> string_expression)
        [_val = phx::bind(&store_probe_stmap, phx::ref(probe_mvars), _1, _2, _3)];

    RuleBool local_map_assignment = 
        (local_var >> '[' >> keys >> ']' >> '=' >> arithmetic_expression)
        [_val = phx::bind(&store_comp_dmap, phx::ref(data.local_map_vars), _1, _2, _3)]
        | 
        (local_var >> '[' >> keys >> ']' >> '=' >> string_expression)
        [_val = phx::bind(&store_stmap, phx::ref(data.local_map_vars), _1, _2, _3)];

    RuleBool global_map_assignment = 
        (global_var >> '[' >> keys >> ']' >> '=' >> arithmetic_expression)
        [_val = phx::bind(&store_comp_dmap, phx::ref(data.global_map_vars), _1, _2, _3)]
        |
        (global_var >> '[' >> keys >> ']' >> '=' >> string_expression)
        [_val = phx::bind(&store_stmap, phx::ref(data.global_map_vars), _1, _2, _3)];

    bool scalar_assignment_error = false;
    bool map_assignment_error = false;


    Rule scalar_var_assignment = (probe_var_assignment | local_var_assignment | global_var_assignment)
                                 [_pass = _1, phx::ref(scalar_assignment_error) = !_1];

    Rule map_var_assignment = (probe_map_assignment | local_map_assignment | global_map_assignment)
                                 [_pass = _1, phx::ref(map_assignment_error) = !_1];

    Rule assignment = scalar_var_assignment | map_var_assignment;

    
    RuleString str = ("str(" >> arithmetic_expression >> ')')[_val = phx::bind(&to_string, _1)];
    string_function = str;

    RuleDouble dbl = ("dbl(" >> string_expression >> ')')[_val = phx::bind(&to_double, _1)];

    RuleDouble round = ("round(" >> arithmetic_expression >> ')')[_val = phx::bind(&round_, _1)];
    RuleDouble ceil = ("ceil(" >> arithmetic_expression >> ')')[_val = phx::bind(&ceil_, _1)];
    RuleDouble floor = ("floor(" >> arithmetic_expression >> ')')[_val = phx::bind(&floor_, _1)];


    double_function = dbl | round | ceil | floor;


    //if lit is not used, compiler tries to do binary OR
    Rule agg_funcs = lit("sum") | "avg" | "min" | "max" | "quantize";

    Rule aggregation = 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "count()")
        [phx::bind(&aggregate, phx::ref(data.local_aggregation), _1, _2, 0)]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> 
        agg_funcs >> '(' >> arithmetic_expression >> ')')
        [phx::bind(&aggregate, phx::ref(data.local_aggregation), _1, _2, _3)]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "lquantize" >> '(' >>
                                                                      arithmetic_expression >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >>
                                                                      ')')
        [phx::bind(&aggregate, phx::ref(data.local_aggregation), _1, _2, _3)];


    qi::rule<Iterator, ascii::space_type,std::vector<int>> localities = int_ % ',';


    Rule print = ("print(" >> value >> ')')[phx::bind(&print_value, _1)]
               | (lit("print(") >> '@' >> var >> ')')
                 [phx::bind(&print_aggregation, phx::ref(data.local_aggregation), _1)]
               | (lit("global_print(") >> '@' >> var >> ')')
                 [phx::bind(&global_print_aggregation, phx::ref(data.aggregations), _1)]
               | (lit("global_print(") >> '@' >> var >> ',' >> localities >> ')')
                 [phx::bind(&partial_print_aggregation, phx::ref(data.aggregations), _2, _1)];



    Rule lock = (lit("lock") >> '(' >> keys >> ')')
        [phx::bind(&comp_lock, phx::ref(data.local_mutexes),_1)];
        
    Rule unlock = (lit("unlock") >> '(' >> keys >> ')')
        [phx::bind(&comp_unlock, phx::ref(data.local_mutexes),_1)];

    Rule global_lock = (lit("global_lock") >> '(' >> keys >> ')')
        [phx::bind(&comp_lock, phx::ref(data.global_mutexes),_1)];
        
    Rule global_unlock = (lit("global_unlock") >> '(' >> keys >> ')')
        [phx::bind(&comp_unlock, phx::ref(data.global_mutexes),_1)];

    Rule locks = lock | unlock | global_lock | global_unlock;

    Rule action = assignment | print | aggregation | locks;

    Rule actions = action >> ';' >> *(action >> ';');



    bool r = phrase_parse(
        first,                          /*< start iterator >*/
        last,                           /*< end iterator >*/
        actions,                        /*< the parser >*/
        space
    );


    if (first != last){// fail if we did not get a full match
        std::string error;
        if(scalar_assignment_error){
            error = "Variable type and assigned value type don't match: \n" ;
        }
        else if(map_assignment_error){
            error = "Map type and assigned value type don't match: \n" ;
        }
        else{
            error = "Variable with wrong type/uninitialized variable in action: \n" ;
        }
        for (; (*first) != ';' && first != last; first++){
            error += *first;
        }
        throw std::runtime_error(error);
    }


    return r;
}



template <typename Iterator>
bool validate_predicate(Iterator first, Iterator last) {

    using qi::double_;
    using qi::char_;
    using qi::_1;
    using qi::_2;
    using qi::_3; 
    using qi::_val;
    using qi::_pass;
    using qi::phrase_parse;
    using ascii::space;
    

    typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleString;
    typedef qi::rule<Iterator, ascii::space_type, double> RuleDouble;
    typedef qi::rule<Iterator, ascii::space_type> Rule;


    //String rules
    qi::rule<Iterator, std::string()> string_content = +(char_ - '"');
    RuleString string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];
    //Var rules
    RuleString probe_var = (char_("a-zA-Z") >> *char_("a-zA-Z0-9_"));
    RuleString local_var = (char_("&") >> *char_("a-zA-Z0-9_"));
    RuleString global_var = (char_("#") >> *char_("a-zA-Z0-9_"));

    RuleString var = probe_var | local_var | global_var;
    //Function rules
    Rule string_function;
    Rule double_function;
    //Values rules
    Rule string_value = string | string_function | var;
    Rule double_value = double_ | double_function | var;

    //Expression rules

    Rule string_expression = 
        string_value                            
        >> *('+' >> string_value                );

    Rule arithmetic_expression, term, factor;

    arithmetic_expression =
        term                            
        >> *(   ('+' >> term            )//[_1 + 10]
            |   ('-' >> term            )
            )
        ;

    term =
        factor                          
        >> *(   ('*' >> factor          )
            |   ('/' >> factor          )
            |   ('%' >> factor          )
            )
        ;

    factor =
            double_value                 
        |   '(' >> arithmetic_expression  >> ')'
        |   ('-' >> factor               )
        |   ('+' >> factor               )
        ;


    qi::rule<Iterator, ascii::space_type> value ;
    value = arithmetic_expression 
          | string_expression;

    qi::rule<Iterator, ascii::space_type, std::string()> op;
    qi::rule<Iterator, ascii::space_type> predicate, comparison, ands, ors, parentheses, expression, subexpression;


    op = +char_("=<>!");

    comparison = (string_expression >> "==" >> string_expression)
               | (string_expression >> "!=" >> string_expression)
               | (arithmetic_expression >> "==" >> arithmetic_expression)
               | (arithmetic_expression >> "!=" >> arithmetic_expression)
               | (arithmetic_expression >> "<"  >> arithmetic_expression)
               | (arithmetic_expression >> "<=" >> arithmetic_expression)
               | (arithmetic_expression >> ">"  >> arithmetic_expression)
               | (arithmetic_expression >> ">=" >> arithmetic_expression);


    bool result;

    parentheses = comparison | ('(' >> ors >> ')');


    ands = parentheses >> *("&&" >> parentheses);
    ors = ands >> *("||" >> ands);


    predicate = ('/' >> ors  >> '/');


    bool r = phrase_parse(
        first,                          /*< start iterator >*/
        last,                           /*< end iterator >*/
        predicate,                      /*< the parser >*/
        space
    );


    if (first != last){// fail if we did not get a full match
        std::string error = "Syntax error in predicate: \n" ;
        for (; first != last; first++)
        {
            error += *first;
        }
        throw std::runtime_error(error);
    }


    return result;
}




template <typename Iterator>
bool parse_predicate(Iterator first, Iterator last,
    ScalarVars probe_svars,
    ScriptData data) {
    using qi::double_;
    using qi::char_;
    using qi::_1;
    using qi::_2;
    using qi::_3; 
    using qi::_val;
    using qi::_pass;
    using qi::phrase_parse;
    using ascii::space;
    


    typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleString;
    typedef qi::rule<Iterator, ascii::space_type, double> RuleDouble;
    typedef qi::rule<Iterator, ascii::space_type> Rule;


    //String rules
    qi::rule<Iterator, std::string()> string_content = +(char_ - '"');
    RuleString string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];
    //Var rules
    RuleString probe_var = (char_("a-zA-Z") >> *char_("a-zA-Z0-9_"));
    RuleString local_var = (char_("&") >> *char_("a-zA-Z0-9_"));
    RuleString global_var = (char_("#") >> *char_("a-zA-Z0-9_"));

    RuleString var = probe_var | local_var | global_var;


    std::string s = "--";

    RuleString probe_stvar = 
        (probe_var [_pass = phx::bind(&get_probe_stvar, phx::ref(probe_svars), _1, phx::ref(s))])
        [_val = phx::ref(s)];

    RuleString local_stvar = 
        (local_var [_pass = phx::bind(&get_comp_stvar, phx::ref(data.local_scalar_vars), _1, phx::ref(s))])
        [_val = phx::ref(s)];

    RuleString global_stvar = 
        (global_var [_pass = phx::bind(&get_comp_stvar, phx::ref(data.global_scalar_vars), _1, phx::ref(s))])
        [_val = phx::ref(s)];


    RuleString string_var = probe_stvar | local_stvar | global_stvar;


    double d = -5;

    RuleDouble probe_dvar = 
        (probe_var [_pass = phx::bind(&get_probe_dvar, phx::ref(probe_svars), _1, phx::ref(d))])
        [_val = phx::ref(d)];

    RuleDouble local_dvar = 
        (local_var[_pass = phx::bind(&get_comp_dvar, phx::ref(data.local_scalar_vars), _1, phx::ref(d))])
        [_val = phx::ref(d)];

    RuleDouble global_dvar = 
        (global_var [_pass = phx::bind(&get_comp_dvar, phx::ref(data.global_scalar_vars), _1, phx::ref(d))])
        [_val = phx::ref(d)];


    RuleDouble double_var = probe_dvar | local_dvar | global_dvar;


    //Function rules
    RuleString string_function;
    RuleDouble double_function;
    //Values rules
    RuleString string_value = string | string_function | string_var;
    RuleDouble double_value = double_ | double_function | double_var;

    //Expression rules

    RuleString string_expression = 
        string_value                            [_val = _1]
        >> *('+' >> string_value                [_val = _val + _1]);

    RuleDouble arithmetic_expression, term, factor;

    arithmetic_expression =
        term                            [_val = _1]
        >> *(   ('+' >> term            [_val = _val + _1])
            |   ('-' >> term            [_val = _val - _1])
            )
        ;

    term =
        factor                          [_val = _1]
        >> *(   ('*' >> factor          [_val = _val * _1])
            |   ('/' >> factor          [_val = _val / _1])
            |   ('%' >> factor          [_val = phx::bind(&fmod, _val, _1)])
            )
        ;

    factor =
            double_value                 [_val = _1]
        |   '(' >> arithmetic_expression [_val = _1] >> ')'
        |   ('-' >> factor               [_val = -_1])
        |   ('+' >> factor               [_val = _1])
        ;


    qi::rule<Iterator, ascii::space_type, Variant()> value ;
    value = arithmetic_expression[_val = _1] 
          | string_expression[_val = _1];

    qi::rule<Iterator, ascii::space_type, std::string()> op;
    qi::rule<Iterator, ascii::space_type, bool> predicate, comparison, ands, ors, parentheses, expression, subexpression;


    op = +char_("=<>!");

    comparison = (string_expression >> "==" >> string_expression)[_val = _1 == _2]
               | (string_expression >> "!=" >> string_expression)[_val = _1 != _2]
               | (arithmetic_expression >> "==" >> arithmetic_expression)[_val = _1 == _2]
               | (arithmetic_expression >> "!=" >> arithmetic_expression)[_val = _1 != _2]
               | (arithmetic_expression >> "<"  >> arithmetic_expression)[_val = _1 <  _2]
               | (arithmetic_expression >> "<=" >> arithmetic_expression)[_val = _1 <= _2]
               | (arithmetic_expression >> ">"  >> arithmetic_expression)[_val = _1 >  _2]
               | (arithmetic_expression >> ">=" >> arithmetic_expression)[_val = _1 >= _2];




    bool result;

    parentheses = comparison | ('(' >> ors >> ')') ;


    ands = parentheses[_val = _1] >> *("&&" >> parentheses)[_val &= _1];
    ors = ands[_val = _1] >> *("||" >> ands)[_val |= _1];


    predicate = ('/' >> ors  >> '/')[phx::ref(result) = _1];


    bool r = phrase_parse(
        first,                          /*< start iterator >*/
        last,                           /*< end iterator >*/
        predicate,                      /*< the parser >*/
        space
    );


    if (first != last){// fail if we did not get a full match
        std::cout << "FALHOU \n";
        return false;
    }



    return result;
}


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
    std::map<std::string,double> double_arguments = {},
    std::map<std::string,std::string> string_arguments = {}){
    
        arguments args;
        args.first = double_arguments;
        args.second = string_arguments;
        apex::custom_event(event_types[probe_name], &args);
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


    apex::register_policy(event_type,
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
        });
}

HPX_DEFINE_PLAIN_ACTION(register_user_probe, register_user_probe_action); 


bool read_counter(hpx::performance_counters::performance_counter counter, std::string* counter_name){

    int value = counter.get_value<int>().get();
    //reading the counter from the API does not tigger APEX_SAMPLE_VALUE event to it has to be triggered manually
    apex::sample_value(*counter_name, value);
    return true;
}


bool fill_counter_variables(std::string name, double value, ScalarVars& probe_svars){

    probe_svars.store_string("counter_name", name);
    probe_svars.store_double("counter_value", value);

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

    probe_svars.store_string("counter_type", '/' + p.objectname_ + '/' + p.countername_);
    probe_svars.store_string("counter_parameters", p.parameters_);
    probe_svars.store_string("counter_parent_instance_name", p.parentinstancename_);
    probe_svars.store_string("counter_parent_instance_index", std::to_string(p.parentinstanceindex_));
    probe_svars.store_string("counter_instance_name", p.instancename_);
    probe_svars.store_string("counter_instance_index", std::to_string(p.instanceindex_));
    return true;
}




void register_counter_create_probe(
    std::string probe_arg1, std::string probe_arg2,
    std::string predicate, std::string actions, ScriptData data){

    std::string counter_name = probe_arg1;
    int period = std::stoi(probe_arg2);

            std::cout << "register_counter_create_probe " << counter_name << period << std::endl;


    apex::register_policy(APEX_SAMPLE_VALUE, [predicate, actions, counter_name, data](apex_context const& context)->int{
        
        sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
            
        if(*dt.counter_name == counter_name){

            ScalarVars probe_svars;
            MapVars probe_mvars;
            fill_counter_variables(*dt.counter_name, dt.counter_value, probe_svars);

            if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars,data)){
                parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
            }

        }


        return APEX_NOERROR;
    });

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

    apex::register_policy(APEX_SAMPLE_VALUE, [predicate, actions, probe_arg, data](apex_context const& context)->int{
        
        sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
        
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
    });
}



void register_counter_type_probe(std::string probe_arg ,std::string predicate, std::string actions,
    ScriptData data
 ){


    apex::register_policy(APEX_SAMPLE_VALUE, [predicate, actions, probe_arg, data](apex_context const& context)->int{
        
        sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
        
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
    });
    
}

void register_proc_probe(std::string probe_arg, std::string predicate, std::string actions, ScriptData data){
    
    std::string filter = probe_arg;

    apex::register_policy(APEX_SAMPLE_VALUE, [predicate, actions, filter, data](apex_context const& context)->int{
        
        sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
        
        if((*dt.counter_name).find(filter) != -1){

            ScalarVars probe_svars;            
            MapVars probe_mvars;

            probe_svars.store_string("proc_name", *(dt.counter_name));
            probe_svars.store_double("proc_value", dt.counter_value);

            if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars, data)){
                parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
            }

        }
        return APEX_NOERROR;
    });
    
}

HPX_DEFINE_PLAIN_ACTION(register_proc_probe, register_proc_probe_action); 


void fill_task_variables(std::shared_ptr<apex::task_wrapper> tw,
                         apex_event_type event_type,
                         ScalarVars& probe_svars)
{

    std::string task_name = tw->task_id->get_name();
    probe_svars.store_string("name", task_name);
    probe_svars.store_string("parent_name", tw->parent->task_id->get_name());
    probe_svars.store_string("guid", std::to_string(tw->guid));
    probe_svars.store_string("parent_guid", std::to_string(tw->parent_guid));


    if(event_type == APEX_STOP_EVENT || event_type == APEX_YIELD_EVENT){
        if(event_type == APEX_STOP_EVENT) probe_svars.store_string("event", "stop");
        else if(event_type == APEX_YIELD_EVENT) probe_svars.store_string("event", "yield");
        apex::profiler *prof = tw->prof; 
        probe_svars.store_double("start", (double) prof->start_ns);
        probe_svars.store_double("end", (double) prof->end_ns);        
        probe_svars.store_double("allocations", prof->allocations);
        probe_svars.store_double("frees", prof->frees);
        probe_svars.store_double("bytes_allocated", prof->bytes_allocated);
        probe_svars.store_double("bytes_freed", prof->bytes_freed);
    }
    else{
        if(event_type == APEX_START_EVENT) probe_svars.store_string("event", "start");
        else if(event_type == APEX_RESUME_EVENT) probe_svars.store_string("event", "resume");
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

    apex::register_policy(events, [predicate, actions, task_filter, data](apex_context const& context)->int{
        
        std::shared_ptr<apex::task_wrapper> tw = *reinterpret_cast<std::shared_ptr<apex::task_wrapper>*>(context.data);
        std::string task_name = tw->task_id->get_name();

        if(task_filter != task_name && task_filter != "") return 0;

        ScalarVars probe_svars;        
        MapVars probe_mvars;

        fill_task_variables(tw, context.event_type, probe_svars);


        if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars, data)){
            parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
        }



        return APEX_NOERROR;
    });
    
}

HPX_DEFINE_PLAIN_ACTION(register_task_probe, register_task_probe_action); 

void fill_message_variables(apex::message_event_data event_data,
                            apex_event_type event_type,
                            ScalarVars& probe_svars)
{

    if(event_type == APEX_SEND) probe_svars.store_string("event", "send");
    else if(event_type == APEX_RECV) probe_svars.store_string("event", "receive");

    probe_svars.store_double("tag", event_data.tag);
    probe_svars.store_double("size", event_data.size);
    probe_svars.store_double("source_rank", event_data.source_rank);
    probe_svars.store_double("source_thread", event_data.source_thread);
    probe_svars.store_double("target", event_data.target);   
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



    apex::register_policy(events, [predicate, actions, data](apex_context const& context)->int{
        
        apex::message_event_data event_data = *reinterpret_cast<apex::message_event_data*>(context.data);

        ScalarVars probe_svars;        
        MapVars probe_mvars;

        fill_message_variables(event_data, context.event_type, probe_svars);

        if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_svars, data)){
            parse_actions(actions.begin(), actions.end(), probe_svars, probe_mvars, data);
        }

        return APEX_NOERROR;
    });
    
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
    std::regex rgx_0args("\\s*([a-zA-Z0-9-]+)(\\[[^\\]]*\\])?()()\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_1args("\\s*([a-zA-Z0-9-]+)(\\[[^\\]]*\\])?::([^:]+)::()\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_2args("\\s*([a-zA-Z0-9-]+)(\\[[^\\]]*\\])?::([^:]+)::([^:]+)::\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");

    std::regex rgx_begin("\\s*(BEGIN)\\[([^\\]]*)\\]()()\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_end("\\s*(END)\\[([^\\]]*)\\]()()\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_cc("\\s*(counter\\-create::[^:]+::[0-9]+::)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_c("\\s*(counter(?:::[^:]*::)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_ct("\\s*(counter\\-type(?:::[^:]*::)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_proc("\\s*(proc(?:::[^:]*::)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_task("\\s*(task\\[[^\\]]*\\](?:::[^{]*)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_message("\\s*(message\\[[^\\]]*\\])\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_probe("\\s*([a-zA-Z0-9]+)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");



    std::smatch match;
    std::vector<std::string> matches;

    int n_probes = 0;
    std::regex_constants::match_flag_type fl = std::regex_constants::match_continuous;


    while (
           std::regex_search(script, match, rgx_0args, fl) 
        || std::regex_search(script, match, rgx_1args, fl) 
        || std::regex_search(script, match, rgx_2args, fl) 
        /*|| std::regex_search(script, match, rgx_end, fl) 
        || std::regex_search(script, match, rgx_cc, fl) 
        || std::regex_search(script, match, rgx_c, fl) 
        || std::regex_search(script, match, rgx_ct, fl)
        || std::regex_search(script, match, rgx_proc, fl) 
        || std::regex_search(script, match, rgx_task, fl) 
        || std::regex_search(script, match, rgx_message, fl)     
        || std::regex_search(script, match, rgx_probe, fl)*/
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
            std::cout << "BEGIN " << probe_name << std::endl;
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                execute_begin_probe_action execute_action;
                execute_action(sc.locality, probe_actions, sc);
            }  
        }
        else if(probe_name == "END"){
            std::cout << "END " << probe_name << std::endl;
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                register_end_probe_action register_action;
                register_action(sc.locality, probe_actions, sc);
            }  
        }

        else if(probe_name == "counter"){
            std::cout << "COUNTER " << probe_name << std::endl; 
            register_counter_probe(probe_arg1, probe_predicate, probe_actions, localities_data[0]);
        }
        else if(probe_name == "counter-create"){
            std::cout << "COUNTER CREATE " << probe_name << std::endl; 
            register_counter_create_probe(probe_arg1, probe_arg2, probe_predicate, probe_actions, localities_data[0]);
        }
        else if(probe_name == "counter-type"){
            std::cout << "COUNTER TYPE " << probe_name << std::endl; 
            register_counter_type_probe(probe_arg1, probe_predicate, probe_actions, localities_data[0]);
        }

        else if(probe_name == "proc"){
            std::cout << "PROC " << probe_name << std::endl;
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                register_proc_probe_action register_action;
                register_action(sc.locality, probe_arg1, probe_predicate, probe_actions, sc);
            }  
        }

        else if(probe_name == "message"){
            std::cout << "MESSAGE " << probe_name << std::endl; 
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                register_message_probe_action register_action;
                register_action(sc.locality, probe_arg1, probe_predicate, probe_actions, sc);
            }
        }
        else if(probe_name == "task"){
            std::cout << "TASK " << probe_name << std::endl; 
            for(int i : locs){
                ScriptData& sc = localities_data[i];
                register_task_probe_action register_action;
                register_action(sc.locality, probe_arg1, probe_arg2, probe_predicate, probe_actions, sc);
            }
        }
        else{
            std::cout << "USER PROBE " << probe_name << std::endl; 
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


    //Print aggregations
    /*hpx::register_shutdown_function(
        []()->void{
            for (auto agg : aggvars){
                //arg.first -> aggregation name
                //arg.second -> aggregation object
                std::cout << "Aggregation " << agg.first << ":" << std::endl;
                agg.second->print();
            }    
        }
    );
    */
}



void init(std::string script){
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
}

//destruct interval_timers
void finalize(){
    for (auto  element : interval_timers) {
        std::cout << "finalize\n";
        element->~interval_timer();
    }
}
}

