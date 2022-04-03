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
#include "Aggregation.cpp"
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


//std::map<std::string,double> probe_dvars; //global variables
//std::map<std::string, std::string> probe_stvars; //global variables
std::map<std::string, Aggregation*> aggvars; //aggregation variables
std::map<std::string,apex_event_type> event_types; 
std::vector<hpx::util::interval_timer*> interval_timers; //for counter-create
std::chrono::steady_clock::time_point start_time;
//script_data global_data;
Mutexes mutexes;



namespace qi = boost::spirit::qi;

namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;

#define NON_EXISTANT -1
#define DOUBLE_VAR 0
#define STRING_VAR 1

using hpx::naming::id_type;

bool is_dvar(std::map<std::string,double>& probe_dvars, std::string name){
    auto it = probe_dvars.find(name);
    if ( it != probe_dvars.end()){
        return true;
    }
    return false;
}


bool is_stvar( std::map<std::string,std::string>& probe_stvars, std::string name){
    auto it = probe_stvars.find(name);
    if ( it != probe_stvars.end()){
        return true;
    }
    return false;
}

bool get_comp_dvar(hpx::naming::id_type id, std::string name, double& value){
    hpx::util::optional op = MapServer<std::string,double>::get_var_action()(id, name);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

bool get_comp_stvar(hpx::naming::id_type id, std::string name, std::string& value){
    hpx::util::optional op = MapServer<std::string,std::string>::get_var_action()(id, name);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

void store_comp_dvar(hpx::naming::id_type id, std::string name, double value){
    MapServer<std::string,double>::store_var_action()(id, name, value);
}

void store_comp_stvar(hpx::naming::id_type id, std::string name, std::string value){
    MapServer<std::string,std::string>::store_var_action()(id, name, value);
}

bool get_dmap(hpx::naming::id_type id, std::string name, VariantList keys, double& value){
    keys.push_back(name);
    hpx::util::optional op = MapServer<VariantList,double>::get_var_action()(id, keys);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

bool get_stmap(hpx::naming::id_type id, std::string name, VariantList keys, std::string& value){
    keys.push_back(name);
    hpx::util::optional op = MapServer<VariantList,std::string>::get_var_action()(id, keys);
    if(!op.has_value()) return false;
    value = op.value();
    return true; 
}

void store_dmap(hpx::naming::id_type id, std::string name, VariantList keys, double value){
    keys.push_back(name);
    MapServer<VariantList,double>::store_var_action()(id, keys, value);
}

void store_stmap(hpx::naming::id_type id, std::string name, VariantList keys, std::string value){
    keys.push_back(name);
    MapServer<VariantList,std::string>::store_var_action()(id, keys, value);
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

void aggregate(std::string name, VariantList keys, double value){
    aggvars[name]->aggregate(keys, value);
}

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



template <typename Iterator>
bool validate_actions(Iterator first, Iterator last) {
    using qi::double_;
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


    Rule local_map = (local_var >> '[' >> keys >> ']');
    Rule global_map = (global_var >> '[' >> keys >> ']');
    Rule map = local_map | global_map;

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



    Rule print = ("print(" >> arithmetic_expression >> ')')
               | ("print(" >> string_expression >> ')');

    
    Rule str = ("str(" >> arithmetic_expression >> ')');
    string_function = str;

    Rule dbl = ("dbl(" >> string_expression >> ')');

    RuleDouble round = ("round(" >> arithmetic_expression >> ')');
    RuleDouble ceil = ("ceil(" >> arithmetic_expression >> ')');
    RuleDouble floor = ("floor(" >> arithmetic_expression >> ')');


    double_function = dbl | round | ceil | floor;


    Rule function = print;


    
     

    std::map<std::string, std::string> aggregations;
    std::map<std::string, std::vector<int>> lquantizes;



    Rule aggregation = 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "count()")
        [phx::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "count")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "sum" >> '(' >> arithmetic_expression  >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "sum")]
        |
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "avg" >> '(' >> arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "avg")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "min" >> '(' >> arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "min")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "max" >> '(' >> arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "max")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "quantize" >> '(' >> arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "quantize")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "lquantize" >> '(' >>
                                                                      arithmetic_expression >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >>
                                                                      ')')
        [phx::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "lquantize"),
        phx::bind(&validate_lquantize_parameters, phx::ref(lquantizes), _1, _2,_3,_4)
        ];


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
    std::map<std::string,double>& probe_dvars, std::map<std::string,std::string>& probe_stvars,
    ScriptData data) {
    using qi::double_;
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


    //probe -> probe_dvars and probe_stvars 
    //local -> data.local_dvars data.local_stvars
    //global -> data.global_dvars data.global_stvars

    //String rules
    qi::rule<Iterator, std::string()> string_content = +(char_ - '"');
    RuleString string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];
    //Var rules
    RuleString probe_var = (char_("a-zA-Z") >> *char_("a-zA-Z0-9_"));
    RuleString local_var = (char_("&") >> *char_("a-zA-Z0-9_"));
    RuleString global_var = (char_("#") >> *char_("a-zA-Z0-9_"));

    RuleString var = probe_var | local_var | global_var;

    RuleString probe_stvar = (probe_var[_pass = phx::bind(&is_stvar, phx::ref(probe_stvars), _1)])
                                  [_val = phx::ref(probe_stvars)[_1]];

    std::string s = "--";

    RuleString local_stvar = 
        (local_var [_pass = phx::bind(&get_comp_stvar, phx::ref(data.local_stvars), _1, phx::ref(s))])
        [_val = phx::ref(s)];

    RuleString global_stvar = 
        (global_var [_pass = phx::bind(&get_comp_stvar, phx::ref(data.global_stvars), _1, phx::ref(s))])
        [_val = phx::ref(s)];


    RuleString string_var = probe_stvar | local_stvar | global_stvar;


    RuleDouble probe_dvar = (probe_var[_pass = phx::bind(&is_dvar, phx::ref(probe_dvars), _1)])
                                  [_val = phx::ref(probe_dvars)[_1]];

    double d = -5;


    RuleDouble local_dvar = 
        (local_var[_pass = phx::bind(&get_comp_dvar, phx::ref(data.local_dvars), _1, phx::ref(d))])
        [_val = phx::ref(d)];

    RuleDouble global_dvar = 
        (global_var [_pass = phx::bind(&get_comp_dvar, phx::ref(data.global_dvars), _1, phx::ref(d))])
        [_val = phx::ref(d)];


    RuleDouble double_var = probe_dvar | local_dvar | global_dvar;


    RuleDouble timestamp = lit("timestamp")[_val =  phx::bind(&elapsed_time)];
    RuleString locality = lit("locality")[_val = phx::ref(data.locality_name)];

    qi::rule<Iterator, ascii::space_type, Variant()> value;
    qi::rule<Iterator, ascii::space_type, VariantList> keys = value % ',';


    RuleDouble local_dmap = (local_var >> '[' >> keys >> ']')
        [_pass = phx::bind(&get_dmap, phx::ref(data.local_dmaps), _1, _2, phx::ref(d)),
        _val = phx::ref(d)];

    RuleDouble global_dmap = (global_var >> '[' >> keys >> ']')
        [_pass = phx::bind(&get_dmap, phx::ref(data.global_dmaps), _1, _2, phx::ref(d)),
        _val = phx::ref(d)];
    
    RuleDouble double_map = local_dmap | global_dmap;
    
    RuleString local_stmap = (local_var >> '[' >> keys >> ']')
        [_pass = phx::bind(&get_stmap, phx::ref(data.local_stmaps), _1, _2, phx::ref(s)),
        _val = phx::ref(s)];


    RuleString global_stmap = (global_var >> '[' >> keys >> ']')
        [_pass = phx::bind(&get_stmap, phx::ref(data.global_stmaps), _1, _2, phx::ref(s)),
        _val = phx::ref(s)];

    RuleString string_map = local_stmap | global_stmap;


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





    Rule probe_var_assignment = (probe_var >> '=' >> arithmetic_expression)[phx::ref(probe_dvars)[_1] = _2]
                    | (probe_var >> '=' >> string_expression)[phx::ref(probe_stvars) [_1] = _2]; 

    Rule local_var_assignment = (local_var >> '=' >> arithmetic_expression)
                      [phx::bind(&store_comp_dvar, phx::ref(data.local_dvars), _1, _2)]
                    | (local_var >> '=' >> string_expression)
                      [phx::bind(&store_comp_stvar, phx::ref(data.local_stvars), _1, _2)];

    Rule global_var_assignment = (global_var >> '=' >> arithmetic_expression)
                      [phx::bind(&store_comp_dvar, phx::ref(data.global_dvars), _1, _2)]
                    | (global_var >> '=' >> string_expression)
                      [phx::bind(&store_comp_stvar, phx::ref(data.global_stvars), _1, _2)];


    Rule local_map_assignment = 
        (local_var >> '[' >> keys >> ']' >> '=' >> arithmetic_expression)
        [phx::bind(&store_dmap, phx::ref(data.local_dmaps), _1, _2, _3)]
        | 
        (local_var >> '[' >> keys >> ']' >> '=' >> string_expression)
        [phx::bind(&store_stmap, phx::ref(data.local_stmaps), _1, _2, _3)];

    Rule global_map_assignment = 
        (global_var >> '[' >> keys >> ']' >> '=' >> arithmetic_expression)
        [phx::bind(&store_dmap, phx::ref(data.global_dmaps), _1, _2, _3)]
        |
        (global_var >> '[' >> keys >> ']' >> '=' >> string_expression)
        [phx::bind(&store_stmap, phx::ref(data.global_stmaps), _1, _2, _3)];


    Rule assignment = probe_var_assignment | local_var_assignment | global_var_assignment
                    | local_map_assignment | global_map_assignment;

    Rule print = ("print(" >> value >> ')')[phx::bind(&print_value, _1)];
    
    RuleString str = ("str(" >> arithmetic_expression >> ')')[_val = phx::bind(&to_string, _1)];
    string_function = str;

    RuleDouble dbl = ("dbl(" >> string_expression >> ')')[_val = phx::bind(&to_double, _1)];

    RuleDouble round = ("round(" >> arithmetic_expression >> ')')[_val = phx::bind(&round_, _1)];
    RuleDouble ceil = ("ceil(" >> arithmetic_expression >> ')')[_val = phx::bind(&ceil_, _1)];
    RuleDouble floor = ("floor(" >> arithmetic_expression >> ')')[_val = phx::bind(&floor_, _1)];


    double_function = dbl | round | ceil | floor;

    Rule function = print;

    //if lit is not used, compiler tries to do binary OR
    Rule agg_funcs = lit("sum") | "avg" | "min" | "max" | "quantize";

    Rule aggregation = 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "count()")
        [phx::bind(&aggregate, _1, _2, 0)]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> 
        agg_funcs >> '(' >> arithmetic_expression >> ')')
        [phx::bind(&aggregate, _1, _2, _3)]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "lquantize" >> '(' >>
                                                                      arithmetic_expression >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >>
                                                                      ')')
        [phx::bind(&aggregate, _1, _2, _3)];

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
        std::string error = "Variable with wrong type/uninitialized variable in action: \n" ;
        for (; (*first) != ';' && first != last; first++)
        {
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
    std::map<std::string,double>& probe_dvars, std::map<std::string,std::string>& probe_stvars,
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

    RuleString probe_stvar = (probe_var[_pass = phx::bind(&is_stvar, phx::ref(probe_stvars), _1)])
                                  [_val = phx::ref(probe_stvars)[_1]];

    std::string s = "--";

    RuleString local_stvar = 
        (local_var [_pass = phx::bind(&get_comp_stvar, phx::ref(data.local_stvars), _1, phx::ref(s))])
        [_val = phx::ref(s)];

    RuleString global_stvar = 
        (global_var [_pass = phx::bind(&get_comp_stvar, phx::ref(data.global_stvars), _1, phx::ref(s))])
        [_val = phx::ref(s)];


    RuleString string_var = probe_stvar | local_stvar | global_stvar;


    RuleDouble probe_dvar = (probe_var[_pass = phx::bind(&is_dvar, phx::ref(probe_dvars), _1)])
                                  [_val = phx::ref(probe_dvars)[_1]];

    double d = -5;


    RuleDouble local_dvar = 
        (local_var[_pass = phx::bind(&get_comp_dvar, phx::ref(data.local_dvars), _1, phx::ref(d))])
        [_val = phx::ref(d)];

    RuleDouble global_dvar = 
        (global_var [_pass = phx::bind(&get_comp_dvar, phx::ref(data.global_dvars), _1, phx::ref(d))])
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



typedef std::pair<std::map<std::string,double>,std::map<std::string,std::string>> arguments;

void trigger_probe(std::string probe_name, 
    std::map<std::string,double> double_arguments = {},
    std::map<std::string,std::string> string_arguments = {}){
    
        arguments args;
        args.first = double_arguments;
        args.second = string_arguments;
        apex::custom_event(event_types[probe_name], &args);
}

void register_user_probe(std::string probe_name, std::string predicate, std::string actions, script_data comp,
    ScriptData data
 ){

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
      [predicate, actions, comp, data](apex_context const& context)->int{
            arguments& args = *reinterpret_cast<arguments*>(context.data);

            std::map<std::string,double> double_arguments = args.first;
            std::map<std::string,double> probe_dvars;
            for (auto const& arg : double_arguments){
                //arg.first -> variable name
                //arg.second -> variable value
                probe_dvars[arg.first] = arg.second; 
            }

            std::map<std::string,std::string> string_arguments  = args.second;
            std::map<std::string,std::string> probe_stvars;
            for (auto const& arg : string_arguments){
                //arg.first -> variable name
                //arg.second -> variable value
                probe_stvars[arg.first] = arg.second; 
            }


            if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_dvars, probe_stvars, data)){
                parse_actions(actions.begin(), actions.end(), probe_dvars, probe_stvars, data);
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


void fill_counter_variables(std::string name, double value, 
        std::map<std::string,double>& probe_dvars, 
        std::map<std::string,std::string>& probe_stvars){

    probe_stvars["counter_name"] = name;
    probe_dvars["counter_value"] = value;

    hpx::performance_counters::counter_path_elements p;

    hpx::performance_counters::counter_status status = 
                get_counter_path_elements(name, p);



    probe_stvars["counter_type"] = '/' + p.objectname_ + '/' + p.countername_;
    probe_stvars["counter_parameters"] = p.parameters_;
    probe_stvars["counter_parent_instance_name"] = p.parentinstancename_;
    probe_stvars["counter_parent_instance_index"] = std::to_string(p.parentinstanceindex_);
    probe_stvars["counter_instance_name"] = p.instancename_;
    probe_stvars["counter_instance_index"] = std::to_string(p.instanceindex_);
}




void register_counter_create_probe(std::string probe_name ,std::string predicate, std::string actions, 
    script_data comp,
    ScriptData data
    ){

    std::regex rgx("counter\\-create::([^:]+)::([0-9]+)::");
    std::smatch match;
    std::regex_search(probe_name, match, rgx);

    std::string counter_name = match[1];
    int period = std::stoi(match[2]);
    std::string* name_ptr = new std::string(counter_name);



    apex::register_policy(APEX_SAMPLE_VALUE, [predicate, actions, name_ptr, comp, data](apex_context const& context)->int{
        
        sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
            
        if(*dt.counter_name == *name_ptr){

            std::map<std::string,double> probe_dvars;
            std::map<std::string,std::string> probe_stvars;

            fill_counter_variables(*name_ptr, dt.counter_value, probe_dvars, probe_stvars);

            if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_dvars, probe_stvars,data)){
                parse_actions(actions.begin(), actions.end(), probe_dvars, probe_stvars, data);
            }

        }


        return APEX_NOERROR;
    });

    hpx::performance_counters::performance_counter counter(counter_name);



    hpx::util::interval_timer* it = new hpx::util::interval_timer(hpx::util::bind_front(&read_counter, counter, name_ptr), period
        , "", true); //microsecs  100000 - 0.1s

    it->start();

    interval_timers.push_back(it); 

}

void register_counter_probe(std::string probe_name ,std::string predicate, std::string actions, script_data comp,
    ScriptData data
 ){

    std::regex rgx("counter::([^:]+)::");
    std::smatch match;
    

    std::string counter_name;


    //has counter name as argument
    if(std::regex_search(probe_name, match, rgx)){

        counter_name = match[1];


        std::string* name_ptr = new std::string(counter_name);



        apex::register_policy(APEX_SAMPLE_VALUE, [predicate, actions, name_ptr, comp, data](apex_context const& context)->int{
            
            sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
            
            if(*dt.counter_name == *name_ptr){

                std::map<std::string,double> probe_dvars;
                std::map<std::string,std::string> probe_stvars;

                fill_counter_variables(*name_ptr, dt.counter_value, probe_dvars, probe_stvars);

                if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_dvars, probe_stvars,data)){
                    parse_actions(actions.begin(), actions.end(), probe_dvars, probe_stvars, data);
                }
            }


            return APEX_NOERROR;
        });
    }
    else{

    }
}

void register_counter_type_probe(std::string probe_name ,std::string predicate, std::string actions, script_data comp,
    ScriptData data
 ){

    std::regex rgx("counter\\-type::([^:]+)::");
    std::smatch match;
    

    std::string counter_type;


    //has counter name as argument
    if(std::regex_search(probe_name, match, rgx)){

        counter_type = match[1];


        std::string* type_ptr = new std::string(counter_type);



        apex::register_policy(APEX_SAMPLE_VALUE, [predicate, actions, type_ptr, comp, data](apex_context const& context)->int{
            
            sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
            
            hpx::performance_counters::counter_path_elements p;


            hpx::error_code ec; 
            hpx::performance_counters::counter_status status = 
                        get_counter_path_elements(*dt.counter_name, p, ec);


            std::string type_sampled = '/' + p.objectname_ + '/' + p.countername_;

            //if is counter and belongs to the desired type 
            if(&ec != &hpx::throws && type_sampled.find(*(type_ptr)) != -1){

                std::map<std::string,double> probe_dvars;
                std::map<std::string,std::string> probe_stvars;

                fill_counter_variables(*dt.counter_name, dt.counter_value, probe_dvars, probe_stvars);

                if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_dvars, probe_stvars,data)){
                    parse_actions(actions.begin(), actions.end(), probe_dvars, probe_stvars, data);
                }
            }


            return APEX_NOERROR;
        });
    }
}

void register_proc_probe(std::string probe_name ,std::string predicate, std::string actions, script_data comp,
    ScriptData data
 ){

    std::regex rgx("proc::([^:]*)::");
    std::smatch match;
    
    std::string filter = "";

    if(std::regex_search(probe_name, match, rgx)){
        filter = std::string(match[1]);
    }

    std::string counter_name;



    apex::register_policy(APEX_SAMPLE_VALUE, [predicate, actions, filter, comp, data](apex_context const& context)->int{
        
        sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
        
        if((*dt.counter_name).find(filter) != -1){

            std::map<std::string,double> probe_dvars;
            std::map<std::string,std::string> probe_stvars;

            probe_stvars["proc_name"] = *(dt.counter_name);
            probe_dvars["proc_value"] = dt.counter_value;

            if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_dvars, probe_stvars, data)){
                parse_actions(actions.begin(), actions.end(), probe_dvars, probe_stvars, data);
            }

        }
        return APEX_NOERROR;
    });
    
}


void fill_task_variables(std::shared_ptr<apex::task_wrapper> tw,
                         apex_event_type event_type,
                         std::map<std::string,double>& probe_dvars, 
                         std::map<std::string,std::string>& probe_stvars)
{

    std::string task_name = tw->task_id->get_name();
    probe_stvars["name"] = task_name;
    probe_stvars["parent_name"] = tw->parent->task_id->get_name();
    probe_stvars["guid"] = std::to_string(tw->guid);
    probe_stvars["parent_guid"] = std::to_string(tw->parent_guid);


    if(event_type == APEX_STOP_EVENT || event_type == APEX_YIELD_EVENT){
        if(event_type == APEX_STOP_EVENT) probe_stvars["event"] = "stop";
        else if(event_type == APEX_YIELD_EVENT) probe_stvars["event"] = "yield";
        apex::profiler *prof = tw->prof; 
        probe_dvars["start"] = (double) prof->start_ns;
        probe_dvars["end"] = (double) prof->end_ns;        
        probe_dvars["allocations"] = prof->allocations;
        probe_dvars["frees"] = prof->frees;
        probe_dvars["bytes_allocated"] = prof->bytes_allocated;
        probe_dvars["bytes_freed"] = prof->bytes_freed;
    }
    else{
        if(event_type == APEX_START_EVENT) probe_stvars["event"] = "start";
        else if(event_type == APEX_RESUME_EVENT) probe_stvars["event"] = "resume";
    }
}


void register_task_probe(std::string probe_name ,std::string predicate, std::string actions, script_data comp,
    ScriptData data
 ){
    std::regex rgx("task\\[([^\\]]*)\\](?:::([^{]*))?");

    std::smatch match;

    std::regex_search(probe_name, match, rgx);
    std::set<apex_event_type> events;

    std::stringstream ss(match[1]);
    while(ss.good()){
        std::string substr;
        getline( ss, substr, ',' );
        if(substr == "start") events.insert(APEX_START_EVENT);
        else if(substr == "resume") events.insert(APEX_RESUME_EVENT);
        else if(substr == "stop") events.insert(APEX_STOP_EVENT);
        else if(substr == "yield") events.insert(APEX_YIELD_EVENT);
    }
    

    std::string task_filter = match[2];



    apex::register_policy(events, [predicate, actions, task_filter, comp, data](apex_context const& context)->int{
        
        std::shared_ptr<apex::task_wrapper> tw = *reinterpret_cast<std::shared_ptr<apex::task_wrapper>*>(context.data);
        std::string task_name = tw->task_id->get_name();

        if(task_filter != task_name && task_filter != "") return 0;

        std::map<std::string,double> probe_dvars;
        std::map<std::string,std::string> probe_stvars;

        fill_task_variables(tw, context.event_type, probe_dvars, probe_stvars);


        if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_dvars, probe_stvars, data)){
            parse_actions(actions.begin(), actions.end(), probe_dvars, probe_stvars, data);
        }



        return APEX_NOERROR;
    });
    
}

HPX_DEFINE_PLAIN_ACTION(register_task_probe, register_task_probe_action); 

void fill_message_variables(apex::message_event_data event_data,
                            apex_event_type event_type,
                            std::map<std::string,double>& probe_dvars,
                            std::map<std::string,std::string>& probe_stvars)
{

    if(event_type == APEX_SEND) probe_stvars["event"] = "send";
    else if(event_type == APEX_RECV) probe_stvars["event"] = "receive";

    probe_dvars["tag"] = event_data.tag;
    probe_dvars["size"] = event_data.size;
    probe_dvars["source_rank"] = event_data.source_rank;
    probe_dvars["source_thread"] = event_data.source_thread;
    probe_dvars["target"] = event_data.target;
}


void register_message_probe(std::string probe_name, std::string predicate, std::string actions, script_data comp,
    ScriptData data
 ){
    std::regex rgx("message\\[([^\\]]*)\\]");

    std::smatch match;

    std::regex_search(probe_name, match, rgx);
    std::set<apex_event_type> events;

    std::stringstream ss(match[1]);
    while(ss.good()){
        std::string substr;
        getline( ss, substr, ',' );
        if(substr == "send") events.insert(APEX_SEND);
        else if(substr == "receive") events.insert(APEX_RECV);
    }




    apex::register_policy(events, [predicate, actions, comp, data](apex_context const& context)->int{
        

        apex::message_event_data event_data = *reinterpret_cast<apex::message_event_data*>(context.data);

        std::map<std::string,double> probe_dvars;
        std::map<std::string,std::string> probe_stvars;

        fill_message_variables(event_data, context.event_type, probe_dvars, probe_stvars);


        if(predicate == "" || parse_predicate(predicate.begin(), predicate.end(), probe_dvars, probe_stvars, data)){
            parse_actions(actions.begin(), actions.end(), probe_dvars, probe_stvars, data);
        }


        return APEX_NOERROR;
    });
    
}

HPX_DEFINE_PLAIN_ACTION(register_message_probe, register_message_probe_action); 
 

void execute_begin_probe(std::string actions, ScriptData data){
    std::map<std::string,double> probe_dvars;
    std::map<std::string,std::string> probe_stvars;

    
    parse_actions(actions.begin(), actions.end(), 
        probe_dvars, probe_stvars, data);
}

HPX_DEFINE_PLAIN_ACTION(execute_begin_probe, execute_begin_probe_action); 


void register_end_probe(std::string actions, ScriptData data){
    std::map<std::string,double> probe_dvars;
    std::map<std::string,std::string> probe_stvars;


    hpx::register_shutdown_function(
        [actions, data]()->void{
            std::map<std::string,double> probe_dvars;
            std::map<std::string,std::string> probe_stvars;

            parse_actions(actions.begin(), actions.end(), 
                probe_dvars, probe_stvars, data
            );   
        }
    );
        
}

HPX_DEFINE_PLAIN_ACTION(register_end_probe, register_end_probe_action); 


void process_begin_or_end_probe(std::string probe_name, std::string probe_actions,
    std::vector<std::pair<id_type,ScriptData>>& localities_data){


        std::regex rgx("(BEGIN|END)\\[([^\\]]*)\\]");

        std::smatch match;

        std::regex_search(probe_name, match, rgx);
        std::vector<int> localities;

        if(match[2] == ""){
            if(match[1] == "BEGIN"){
                for(auto l : localities_data){
                    execute_begin_probe_action execute_action;
                    execute_action(l.first, probe_actions, l.second);
                }    
            }
            else{
                for(auto l : localities_data){

                    register_end_probe_action register_action;
                    register_action(l.first, probe_actions, l.second);
                }    
            }
            return;
        }

        std::stringstream ss(match[2]);
        while(ss.good()){
            std::string substr;
            getline( ss, substr, ',' );
            int i = std::stoi(substr);
            if(i >= 0 && i < localities_data.size())
                localities.push_back(i);
        }
        if(match[1] == "BEGIN"){
            for(int l : localities){
                execute_begin_probe_action execute_action;
                auto d = localities_data[l];
                execute_action(localities_data[l].first, probe_actions, localities_data[l].second);
            }
        }
        else{
            for(int l : localities){
                register_end_probe_action register_action;
                auto d = localities_data[l];
                register_action(localities_data[l].first, probe_actions, localities_data[l].second);
            }
        }
}



void parse_script(std::string script, script_data global_data, std::vector<std::pair<id_type,ScriptData>>& localities_data){



    start_time = std::chrono::steady_clock::now();
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();


    //(?: -> non-capturing group
    std::regex rgx_begin("\\s*(BEGIN(?:\\[[^\\]]*\\]))\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_end("\\s*(END(?:\\[[^\\]]*\\]))\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_cc("\\s*(counter\\-create::[^:]+::[0-9]+::)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_c("\\s*(counter(?:::[^:]*::)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_ct("\\s*(counter\\-type(?:::[^:]*::)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_proc("\\s*(proc(?:::[^:]*::)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_task("\\s*(task\\[[^\\]]*\\](?:::[^{]*)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_message("\\s*(message\\[[^\\]]*\\])\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_probe("\\s*([a-zA-Z0-9]+)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");



    std::smatch match;


    int n_probes = 0;
    std::regex_constants::match_flag_type fl = std::regex_constants::match_continuous;


    while (
            std::regex_search(script, match, rgx_begin, fl) 
        || std::regex_search(script, match, rgx_end, fl) 
        || std::regex_search(script, match, rgx_cc, fl) 
        || std::regex_search(script, match, rgx_c, fl) 
        || std::regex_search(script, match, rgx_ct, fl)
        || std::regex_search(script, match, rgx_proc, fl) 
        || std::regex_search(script, match, rgx_task, fl) 
        || std::regex_search(script, match, rgx_message, fl)     
        || std::regex_search(script, match, rgx_probe, fl)
    ){


        std::string probe_name = match[1];
        std::string probe_predicate = match[2];
        std::string probe_script = match[3];

        if(probe_predicate != "") 
            validate_predicate(probe_predicate.begin(), probe_predicate.end());
        validate_actions(probe_script.begin(), probe_script.end());
            



        if((probe_name.find("BEGIN") != -1) || (probe_name.find("END") != -1)){
            std::cout << "BEGINEND " << probe_name << std::endl;
            process_begin_or_end_probe(probe_name, probe_script, localities_data);
        }

        else if(probe_name.find("counter-create") != -1){
            std::cout << "COUNTER CREATE " << probe_name << std::endl; 
            register_counter_create_probe(probe_name, probe_predicate, probe_script, global_data, localities_data[0].second);
        }
        else if(probe_name.find("counter-type") != -1){
            std::cout << "COUNTER TYPE " << probe_name << std::endl; 
            register_counter_type_probe(probe_name, probe_predicate, probe_script, global_data, localities_data[0].second);
        }
        else if(probe_name.find("counter") != -1){
            std::cout << "COUNTER " << probe_name << std::endl; 
            register_counter_probe(probe_name, probe_predicate, probe_script, global_data, localities_data[0].second);
        }
        else if(probe_name.find("proc") != -1){
            std::cout << "PROC " << probe_name << std::endl; 
            register_proc_probe(probe_name, probe_predicate, probe_script, global_data, localities_data[0].second);
        }
        else if(probe_name.find("task") != -1){
            std::cout << "TASK " << probe_name << std::endl; 
            for(auto d : localities_data){
                register_task_probe_action register_action;
                register_action(d.first, probe_name, probe_predicate, probe_script, global_data, d.second);
            }
        }
        else if(probe_name.find("message") != -1){
            std::cout << "MESSAGE " << probe_name << std::endl; 
            for(auto d : localities_data){
                register_message_probe_action register_action;
                register_action(d.first, probe_name, probe_predicate, probe_script, global_data, d.second);
            }
        }
        else{
            std::cout << "ELSE " << probe_name << std::endl; 
            for(auto d : localities_data){
                register_user_probe_action register_action;
                register_action(d.first, probe_name, probe_predicate, probe_script, global_data, d.second);
            }
        }
        //Remainder of script
        script = match.suffix();

        n_probes++;

    }

    if(n_probes == 0){
        throw std::runtime_error("invalid probes");
    }


    //Print aggregations
    hpx::register_shutdown_function(
        []()->void{
            for (auto agg : aggvars){
                //arg.first -> aggregation name
                //arg.second -> aggregation object
                std::cout << "Aggregation " << agg.first << ":" << std::endl;
                agg.second->print();
            }    
        }
    );
}



void init(std::string script){
    //Find all localities
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
    //hpx::id_type server_id = hpx::new_<server::script_data>(hpx::find_here());
    script_data global_data = hpx::new_<script_data>(hpx::find_here());
    /*parse_script_action psa;
    for(auto loc : localities){
        hpx::async(psa, loc, script).get();
    }*/

    typedef VariantList VariantList;
    id_type global_dvars = hpx::new_<MapServer<std::string,double>>(hpx::find_here()).get();
    id_type global_stvars = hpx::new_<MapServer<std::string,std::string>>(hpx::find_here()).get();
    id_type global_dmaps = hpx::new_<MapServer<VariantList,double>>(hpx::find_here()).get();
    id_type global_stmaps = hpx::new_<MapServer<VariantList,std::string>>(hpx::find_here()).get();
    id_type global_mutexes = hpx::new_<MutexesServer>(hpx::find_here()).get();


    std::vector<std::pair<id_type,ScriptData>> localities_data;

    for(auto loc : localities){
        id_type local_dvars = hpx::new_<MapServer<std::string,double>>(loc).get();
        id_type local_stvars = hpx::new_<MapServer<std::string,std::string>>(loc).get();
        id_type local_dmaps = hpx::new_<MapServer<VariantList,double>>(loc).get();
        id_type local_stmaps = hpx::new_<MapServer<VariantList,std::string>>(loc).get();
        id_type local_mutexes = hpx::new_<MutexesServer>(loc).get();

        ScriptData sc(loc, global_dvars, local_dvars, global_stvars, local_stvars);
        sc.local_mutexes = local_mutexes;
        sc.global_mutexes = global_mutexes;
        
        sc.local_dmaps = local_dmaps;
        sc.local_stmaps = local_stmaps;
        sc.global_dmaps = global_dmaps;
        sc.global_stmaps = global_stmaps;


        localities_data.push_back(std::make_pair(loc,sc));
    }


    parse_script(script, global_data, localities_data);
}

//destruct interval_timers
void finalize(){
    for (auto  element : interval_timers) {
        std::cout << "finalize\n";
        element->~interval_timer();
    }
}
}

