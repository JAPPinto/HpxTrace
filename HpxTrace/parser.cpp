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



namespace HpxTrace
{

typedef boost::variant<double, std::string> Variant;
typedef std::vector<Variant> VariantList;

#define VARIANT_DOUBLE 0
#define VARIANT_STRING 1




namespace qi = boost::spirit::qi;

namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;

#define NON_EXISTANT -1
#define DOUBLE_VAR 0
#define STRING_VAR 1

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

}