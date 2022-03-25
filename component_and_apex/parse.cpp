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
//#include "task_event.cpp"
#include "script_data.hpp"



#define BOOST_SPIRIT_USE_PHOENIX_V3 1


void register_local_policy(const apex_event_type when, std::function<int(apex_context const&)> f){
    apex::register_policy(when, f);
}

void register_local_policyy(int x, const apex_event_type when,std::function<int(apex_context const&)> f){
    x++;
}

//HPX_PLAIN_ACTION(register_local_policyy, register_local_policy_action);

//Register apex policy in all localities
void register_global_policy(const apex_event_type when, std::function<int(apex_context const&)> f){

    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();

    for(auto loc : localities){
        apex::register_policy(when, f);    
    }
}


//Register apex policy in all localities
/*std::set<apex_policy_handle*> register_policy(std::set<apex_event_type> when,
                    std::function<int(apex_context const&)> f){

    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
    std::set<apex_policy_handle*> handles;

    for(auto loc : localities){
        handles.insert(apex::register_policy(when, f));    
    }

    return handles;
}*/









namespace API
{

typedef boost::variant<double, std::string> Variant;

#define VARIANT_DOUBLE 0
#define VARIANT_STRING 1


std::map<std::string,double> dvars; //global variables
std::map<std::string, std::string> stvars; //global variables
std::map<std::string, Aggregation*> aggvars; //aggregation variables
std::map<std::string,apex_event_type> event_types; 
std::vector<hpx::util::interval_timer*> interval_timers; //for counter-create
std::chrono::steady_clock::time_point start_time;
script_data global_data;

namespace qi = boost::spirit::qi;

namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;

#define NON_EXISTANT -1
#define DOUBLE_VAR 0
#define STRING_VAR 1

bool is_double_variable(std::string name){
    auto it = dvars.find(name);
    if ( it != dvars.end()){
        return true;
    }
    return false;
}

bool is_string_variable(std::string name){
    auto it = stvars.find(name);
    if ( it != stvars.end()){
        return true;
    }
    return false;
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

void aggregate(std::string name, std::vector<Variant> keys, double value){
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
    using qi::phrase_parse;
    using ascii::space;
    


    typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleString;
    typedef qi::rule<Iterator, ascii::space_type, double> RuleDouble;
    typedef qi::rule<Iterator, ascii::space_type> Rule;


    //String rules
    qi::rule<Iterator, std::string()> string_content = +(char_ - '"');
    RuleString string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];
    //Var rules
    RuleString var = (char_("a-zA-Z_") >> *char_("a-zA-Z0-9_"));
   // RuleString string_var = (var[_pass = boost::phoenix::bind(&is_string_variable, _1)])[_val = phx::ref(stvars)[_1]];
    //RuleDouble double_var = (var[_pass = boost::phoenix::bind(&is_double_variable, _1)])[_val = phx::ref(dvars)[_1]];
    //Function rules
    Rule string_function;
    Rule double_function;
    //Values rules
    Rule string_value = string | string_function | var;
    Rule double_value = double_ | double_function | var;

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

    Rule value = arithmetic_expression 
          | string_expression;


    Rule assignment = (var >> '=' >> arithmetic_expression)
                | (var >> '=' >> string_expression);  



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


    qi::rule<Iterator, ascii::space_type> keys = value % ',';
     

    std::map<std::string, std::string> aggregations;
    std::map<std::string, std::vector<int>> lquantizes;



    Rule aggregation = 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "count()")
        [boost::phoenix::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "count")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "sum" >> '(' >> arithmetic_expression  >> ')')
        [boost::phoenix::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "sum")]
        |
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "avg" >> '(' >> arithmetic_expression >> ')')
        [boost::phoenix::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "avg")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "min" >> '(' >> arithmetic_expression >> ')')
        [boost::phoenix::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "min")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "max" >> '(' >> arithmetic_expression >> ')')
        [boost::phoenix::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "max")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "quantize" >> '(' >> arithmetic_expression >> ')')
        [boost::phoenix::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "quantize")]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "lquantize" >> '(' >>
                                                                      arithmetic_expression >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >>
                                                                      ')')
        [boost::phoenix::bind(&validate_aggregating_function, phx::ref(aggregations), _1, "lquantize"),
        boost::phoenix::bind(&validate_lquantize_parameters, phx::ref(lquantizes), _1, _2,_3,_4)
        ];




    Rule action = assignment | print | aggregation;

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
    std::cout << std::chrono::duration_cast<std::chrono::seconds> (now - start_time).count();
    return std::chrono::duration_cast<std::chrono::nanoseconds> (now - start_time).count();
}

double round_(double d){return std::round(d);}
double ceil_(double d){return std::ceil(d);}
double floor_(double d){return std::floor(d);}
double fmod(double a, double b){return std::fmod(a,b);}


template <typename Iterator>
bool parse_actions(Iterator first, Iterator last, script_data global_data) {
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
    RuleString var = (char_("a-zA-Z_") >> *char_("a-zA-Z0-9_"));
    
    RuleString local_string_var = (var[_pass = boost::phoenix::bind(&is_string_variable, _1)])
                                  [_val = phx::ref(stvars)[_1]];
    RuleString global_string_var = (var[_pass = boost::phoenix::bind(&script_data::is_string, &global_data, _1)])
                                   [_val = boost::phoenix::bind(&script_data::get_string, &global_data, _1)];
    RuleString string_var = local_string_var | global_string_var;


    RuleDouble local_double_var = (var[_pass = boost::phoenix::bind(&is_double_variable, _1)])
                            [_val = phx::ref(dvars)[_1]];
    RuleDouble global_double_var = (var[_pass = boost::phoenix::bind(&script_data::is_double, &global_data, _1)])
                            [_val = boost::phoenix::bind(&script_data::get_double, &global_data, _1)];
    RuleDouble double_var = local_double_var | global_double_var;

    RuleDouble timestamp = lit("timestamp")[_val =  boost::phoenix::bind(&elapsed_time)];

    //Function rules
    RuleString string_function;
    RuleDouble double_function;
    //Values rules
    RuleString string_value = string | string_function | string_var;
    RuleDouble double_value = double_ | double_function | timestamp| double_var;

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
            |   ('%' >> factor          [_val = boost::phoenix::bind(&fmod, _val, _1)])
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





    Rule assignment = (var >> '=' >> arithmetic_expression)
                      [boost::phoenix::bind(&script_data::store_double, &global_data, _1, _2)]
                    | (var >> '=' >> string_expression)
                      [boost::phoenix::bind(&script_data::store_string, &global_data, _1, _2)];  



    Rule print = ("print(" >> value >> ')')[boost::phoenix::bind(&print_value, _1)];
    
    RuleString str = ("str(" >> arithmetic_expression >> ')')[_val = boost::phoenix::bind(&to_string, _1)];
    string_function = str;

    RuleDouble dbl = ("dbl(" >> string_expression >> ')')[_val = boost::phoenix::bind(&to_double, _1)];

    RuleDouble round = ("round(" >> arithmetic_expression >> ')')[_val = boost::phoenix::bind(&round_, _1)];
    RuleDouble ceil = ("ceil(" >> arithmetic_expression >> ')')[_val = boost::phoenix::bind(&ceil_, _1)];
    RuleDouble floor = ("floor(" >> arithmetic_expression >> ')')[_val = boost::phoenix::bind(&floor_, _1)];


    double_function = dbl | round | ceil | floor;

    Rule function = print;

    qi::rule<Iterator, ascii::space_type, std::vector<Variant>> keys = value % ',';
    //if lit is not used, compiler tries to do binary OR
    Rule agg_funcs = lit("sum") | "avg" | "min" | "max" | "quantize";

    Rule aggregation = 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "count()")
        [boost::phoenix::bind(&aggregate, _1, _2, 0)]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> 
        agg_funcs >> '(' >> arithmetic_expression >> ')')
        [boost::phoenix::bind(&aggregate, _1, _2, _3)]
        | 
        ('@' >> var >> '[' >> keys >> ']' >> "=" >> "lquantize" >> '(' >>
                                                                      arithmetic_expression >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >>
                                                                      ')')
        [boost::phoenix::bind(&aggregate, _1, _2, _3)];


    Rule action = assignment | print | aggregation;

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


bool compare(Variant a, std::string op, Variant b){
    std::cout << "compare " << a  << op  << b << " "<< "\n";

    if(op == "==") return a == b;
    if(op == "!=") return a != b;
    if(op == "<")  return a < b;
    if(op == "<=") return a <= b;
    if(op == ">")  return a > b;
    if(op == ">=") return a >= b;

    std::cout << "ERRO COMPARE " << a  << op  << b << " "<< "\n";
    return false;
}

bool and_operator(bool a, bool b){return a && b;}


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
    RuleString var = (char_("a-zA-Z_") >> *char_("a-zA-Z0-9_"));
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
bool parse_predicate(Iterator first, Iterator last) {

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
    RuleString var = (char_("a-zA-Z_") >> *char_("a-zA-Z0-9_"));
    RuleString string_var = (var[_pass = boost::phoenix::bind(&is_string_variable, _1)])[_val = phx::ref(stvars)[_1]];
    RuleDouble double_var = (var[_pass = boost::phoenix::bind(&is_double_variable, _1)])[_val = phx::ref(dvars)[_1]];
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
            |   ('%' >> factor          [_val = boost::phoenix::bind(&fmod, _val, _1)])
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

    //comparison = (value >> op >> value)[_val = boost::phoenix::bind(&compare, _1, _2, _3)];
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

void register_user_probe(std::string probe_name, std::string predicate, std::string actions, script_data comp){

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
      [predicate, actions, comp](apex_context const& context)->int{
            //std::cout << context.event_type << std::endl;
            //vector<MyClass*>& v = *reinterpret_cast<vector<MyClass*> *>(voidPointerName);
            arguments& args = *reinterpret_cast<arguments*>(context.data);
            std::map<std::string,double> double_arguments = args.first;
            std::map<std::string,std::string> string_arguments  = args.second;



            for (auto const& arg : double_arguments){
                //arg.first -> variable name
                //arg.second -> variable value
                dvars[arg.first] = arg.second; 
            }

            for (auto const& arg : string_arguments){
                //arg.first -> variable name
                //arg.second -> variable value
                stvars[arg.first] = arg.second; 
            }

            stvars["locality"] = hpx::get_locality_name();

            if(predicate == "" || parse_predicate(predicate.begin(), predicate.end())){
                parse_actions(actions.begin(), actions.end(), comp);
            }

            stvars["locality"] = "";

            


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


void fill_counter_variables(std::string name, double value){
    stvars["counter_name"] = name;
    dvars["counter_value"] = value;

    hpx::performance_counters::counter_path_elements p;

    hpx::performance_counters::counter_status status = 
                get_counter_path_elements(name, p);

    std::cout << "AAAA " << p.parentinstancename_ << " " << p.parentinstanceindex_
     << " " << p.instancename_ <<  " " << p.instanceindex_  << std::endl; 


    stvars["counter_type"] = '/' + p.objectname_ + '/' + p.countername_;
    stvars["counter_parameters"] = p.parameters_;
    stvars["counter_parent_instance_name"] = p.parentinstancename_;
    stvars["counter_parent_instance_index"] = std::to_string(p.parentinstanceindex_);
    stvars["counter_instance_name"] = p.instancename_;
    stvars["counter_instance_index"] = std::to_string(p.instanceindex_);
}

void erase_counter_variables(){
    dvars.erase("counter_value");
    stvars.erase("counter_name");

    stvars.erase("counter_type");
    stvars.erase("counter_parameters");
    stvars.erase("counter_parent_instance_name");
    stvars.erase("counter_parent_instance_index");
    stvars.erase("counter_instance_name");
    stvars.erase("counter_instance_index");

}


void register_counter_create_probe(std::string probe_name ,std::string probe_predicate, std::string script){

    std::regex rgx("counter\\-create::([^:]+)::([0-9]+)::");
    std::smatch match;
    std::regex_search(probe_name, match, rgx);
    std::cout << "REGISTER COUNTER " << match[0] << " " << match[1] << " " << match[2] << std::endl; 

    std::string counter_name = match[1];
    int period = std::stoi(match[2]);
    std::string* name_ptr = new std::string(counter_name);



    apex::register_policy(APEX_SAMPLE_VALUE, [script, probe_predicate, name_ptr](apex_context const& context)->int{
        
        sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
            
        if(*dt.counter_name == *name_ptr){

            fill_counter_variables(*name_ptr, dt.counter_value);

            if(probe_predicate == "" || parse_predicate(probe_predicate.begin(), probe_predicate.end())){
                std::cout << "APEX_SAMPLE_VALUE" << *(dt.counter_name) << " " << dt.counter_value << std::endl;
                parse_actions(script.begin(), script.end(), global_data);
            }

            erase_counter_variables();
        }


        return APEX_NOERROR;
    });

    hpx::performance_counters::performance_counter counter(counter_name);



    hpx::util::interval_timer* it = new hpx::util::interval_timer(hpx::util::bind_front(&read_counter, counter, name_ptr), period
        , "", true); //microsecs  100000 - 0.1s

    it->start();

    interval_timers.push_back(it); 

}

void register_counter_probe(std::string probe_name ,std::string probe_predicate, std::string script){

    std::regex rgx("counter::([^:]+)::");
    std::smatch match;
    

    std::string counter_name;


    //has counter name as argument
    if(std::regex_search(probe_name, match, rgx)){

        counter_name = match[1];

        std::cout << "REGISTER COUNTER " << match[0] << " " << match[1] << std::endl; 

        std::string* name_ptr = new std::string(counter_name);



        apex::register_policy(APEX_SAMPLE_VALUE, [script, probe_predicate, name_ptr](apex_context const& context)->int{
            
            sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
            
            if(*dt.counter_name == *name_ptr){

                fill_counter_variables(*name_ptr, dt.counter_value);

                if(probe_predicate == "" || parse_predicate(probe_predicate.begin(), probe_predicate.end())){
                    std::cout << "APEX_SAMPLE_VALUE" << *(dt.counter_name) << " " << dt.counter_value << std::endl;
                    parse_actions(script.begin(), script.end(), global_data);
                }
                erase_counter_variables();
            }


            return APEX_NOERROR;
        });
    }
    else{

    }
}

void register_counter_type_probe(std::string probe_name ,std::string probe_predicate, std::string script){

    std::regex rgx("counter\\-type::([^:]+)::");
    std::smatch match;
    

    std::string counter_type;


    //has counter name as argument
    if(std::regex_search(probe_name, match, rgx)){

        counter_type = match[1];

        std::cout << "REGISTER COUNTER " << match[0] << " " << match[1] << std::endl; 

        std::string* type_ptr = new std::string(counter_type);



        apex::register_policy(APEX_SAMPLE_VALUE, [script, probe_predicate, type_ptr](apex_context const& context)->int{
            
            sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
            
            hpx::performance_counters::counter_path_elements p;


            hpx::error_code ec; 
            hpx::performance_counters::counter_status status = 
                        get_counter_path_elements(*dt.counter_name, p, ec);


            std::string type_sampled = '/' + p.objectname_ + '/' + p.countername_;

            //if is counter and belongs to the desired type 
            if(&ec != &hpx::throws && type_sampled.find(*(type_ptr)) != -1){
                fill_counter_variables(*dt.counter_name, dt.counter_value);

                if(probe_predicate == "" || parse_predicate(probe_predicate.begin(), probe_predicate.end())){
                    std::cout << "APEX_SAMPLE_VALUE" << *(dt.counter_name) << " " << dt.counter_value << std::endl;
                    parse_actions(script.begin(), script.end(), global_data);
                }

                erase_counter_variables();
            }


            return APEX_NOERROR;
        });
    }
}

void register_proc_probe(std::string probe_name ,std::string probe_predicate, std::string script){

    std::regex rgx("proc::([^:]*)::");
    std::smatch match;
    
    std::string filter = "";

    if(std::regex_search(probe_name, match, rgx)){
        filter = std::string(match[1]);
    }

    std::string counter_name;



    apex::register_policy(APEX_SAMPLE_VALUE, [script, probe_predicate, filter](apex_context const& context)->int{
        
        sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
        
        if((*dt.counter_name).find(filter) != -1){

            stvars["proc_name"] = *(dt.counter_name);
            dvars["proc_value"] = dt.counter_value;

            if(probe_predicate == "" || parse_predicate(probe_predicate.begin(), probe_predicate.end())){
                std::cout << "APEX_SAMPLE_VALUE" << *(dt.counter_name) << " " << dt.counter_value << std::endl;
                parse_actions(script.begin(), script.end(), global_data);
            }
            stvars["proc_name"] = "";
            dvars["proc_value"] = 0;
        }
        return APEX_NOERROR;
    });
    
}

void clear_task_variables(){
    stvars["name"] = "";
    stvars["parent_name"] = "";
    stvars["guid"] = "";
    stvars["parent_guid"] = "";

    stvars["locality"] = "";

    stvars["event"] = "";

    dvars["start"] = 0;
    dvars["end"] = 0;        
    dvars["allocations"] = 0;
    dvars["frees"] = 0;
    dvars["bytes_allocated"] = 0;
    dvars["bytes_freed"] = 0; 
}


void fill_task_variables(std::shared_ptr<apex::task_wrapper> tw, apex_event_type event_type){
    std::string task_name = tw->task_id->get_name();
    stvars["name"] = task_name;
    stvars["parent_name"] = tw->parent->task_id->get_name();
    stvars["guid"] = std::to_string(tw->guid);
    stvars["parent_guid"] = std::to_string(tw->parent_guid);

    stvars["locality"] = hpx::get_locality_name();

    if(event_type == APEX_STOP_EVENT || event_type == APEX_YIELD_EVENT){
        if(event_type == APEX_STOP_EVENT) stvars["event"] = "stop";
        else if(event_type == APEX_YIELD_EVENT) stvars["event"] = "yield";
        apex::profiler *prof = tw->prof; 
        dvars["start"] = (double) prof->start_ns;
        dvars["end"] = (double) prof->end_ns;        
        dvars["allocations"] = prof->allocations;
        dvars["frees"] = prof->frees;
        dvars["bytes_allocated"] = prof->bytes_allocated;
        dvars["bytes_freed"] = prof->bytes_freed;
    }
    else{
        if(event_type == APEX_START_EVENT) stvars["event"] = "start";
        else if(event_type == APEX_RESUME_EVENT) stvars["event"] = "resume";
    }
}


void register_task_probe(std::string probe_name ,std::string predicate, std::string actions, script_data comp){
    std::regex rgx("task\\[([^\\]]*)\\](?:::([^{]*))?");

    std::smatch match;

    std::regex_search(probe_name, match, rgx);
        std::cout << "event:" << std::string(match[1]) << std::endl;
        std::cout << "task:" << std::string(match[2]) << std::endl;
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

    //Initialize task variables so if users try to read profiler variables at start/resume it returns empty values
    clear_task_variables();

    apex::register_policy(events, [predicate, actions, task_filter, comp](apex_context const& context)->int{
        
        std::shared_ptr<apex::task_wrapper> tw = *reinterpret_cast<std::shared_ptr<apex::task_wrapper>*>(context.data);
        std::string task_name = tw->task_id->get_name();

        if(task_filter != task_name && task_filter != "") return 0;

        fill_task_variables(tw, context.event_type);


        if(predicate == "" || parse_predicate(predicate.begin(), predicate.end())){
            parse_actions(actions.begin(), actions.end(), comp);
        }

        clear_task_variables();


        return APEX_NOERROR;
    });
    
}

HPX_DEFINE_PLAIN_ACTION(register_task_probe, register_task_probe_action); 

void fill_message_variables(apex::message_event_data event_data, apex_event_type event_type){
    stvars["locality"] = hpx::get_locality_name();

    if(event_type == APEX_SEND) stvars["event"] = "send";
    else if(event_type == APEX_RECV) stvars["event"] = "receive";

    dvars["tag"] = event_data.tag;
    dvars["size"] = event_data.size;
    dvars["source_rank"] = event_data.source_rank;
    dvars["source_thread"] = event_data.source_thread;
    dvars["target"] = event_data.target;
}

void clear_message_variables(apex::message_event_data event_data){
    stvars["locality"] = "";

    stvars["event"] = "";

    dvars["tag"] = 0;
    dvars["size"] = 0;
    dvars["source_rank"] = 0;
    dvars["source_thread"] = 0;
    dvars["target"] = 0;
}

void register_message_probe(std::string probe_name, std::string predicate, std::string actions, script_data comp){
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




    apex::register_policy(events, [predicate, actions, comp](apex_context const& context)->int{
        

        apex::message_event_data event_data = *reinterpret_cast<apex::message_event_data*>(context.data);

        fill_message_variables(event_data, context.event_type);

        if(predicate == "" || parse_predicate(predicate.begin(), predicate.end())){
            parse_actions(actions.begin(), actions.end(), comp);
        }

        clear_message_variables(event_data);

        return APEX_NOERROR;
    });
    
}

HPX_DEFINE_PLAIN_ACTION(register_message_probe, register_message_probe_action); 
 


void parse_script(std::string script){



    start_time = std::chrono::steady_clock::now();
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();


    //(?: -> non-capturing group
    std::regex rgx_cc("\\s*(counter\\-create::[^:]+::[0-9]+::)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_c("\\s*(counter(?:::[^:]*::)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_ct("\\s*(counter\\-type(?:::[^:]*::)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_proc("\\s*(proc(?:::[^:]*::)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_task("\\s*(task\\[[^\\]]*\\](?:::[^{]*)?)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_message("\\s*(message\\[[^\\]]*\\])\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");
    std::regex rgx_probe("\\s*([a-zA-Z0-9]+)\\s*(/[^/]*/)?\\s*\\{([^{}]*)\\}");



    std::smatch match;


    int n_probes = 0;

    while (std::regex_search(script, match, rgx_cc) 
    || std::regex_search(script, match, rgx_c) 
    || std::regex_search(script, match, rgx_ct)
    || std::regex_search(script, match, rgx_proc) 
    || std::regex_search(script, match, rgx_task) 
    || std::regex_search(script, match, rgx_message)     
    || std::regex_search(script, match, rgx_probe)){


        std::string probe_name = match[1];
        std::string probe_predicate = match[2];
        std::string probe_script = match[3];

        if(probe_predicate != "") 
            validate_predicate(probe_predicate.begin(), probe_predicate.end());
        validate_actions(probe_script.begin(), probe_script.end());



        if(probe_name == "BEGIN"){

            std::cout << "BEGIN " << probe_name << std::endl; 

            parse_actions(probe_script.begin(), probe_script.end(), global_data);
        }

        else if(probe_name == "END"){
            std::cout << "END " << probe_name << std::endl; 

            std::string end_script = probe_script;

            hpx::register_shutdown_function(
              [end_script]()->void{
     
                
                parse_actions(end_script.begin(), end_script.end(), global_data);    
            });
        }

        else if(probe_name.find("counter-create") != -1){
            std::cout << "COUNTER CREATE " << probe_name << std::endl; 
            register_counter_create_probe(probe_name, probe_predicate, probe_script);
        }
        else if(probe_name.find("counter-type") != -1){
            std::cout << "COUNTER TYPE " << probe_name << std::endl; 
            register_counter_type_probe(probe_name, probe_predicate, probe_script);
        }
        else if(probe_name.find("counter") != -1){
            std::cout << "COUNTER " << probe_name << std::endl; 
            register_counter_probe(probe_name, probe_predicate, probe_script);
        }
        else if(probe_name.find("proc") != -1){
            std::cout << "PROC " << probe_name << std::endl; 
            register_proc_probe(probe_name, probe_predicate, probe_script);
        }
        else if(probe_name.find("task") != -1){
            std::cout << "TASK " << probe_name << std::endl; 
            for(auto loc : localities){
                register_task_probe_action register_action;
                register_action(loc, probe_name, probe_predicate, probe_script, global_data);
            }
        }
        else if(probe_name.find("message") != -1){
            std::cout << "MESSAGE " << probe_name << std::endl; 
            for(auto loc : localities){
                register_message_probe_action register_action;
                register_action(loc, probe_name, probe_predicate, probe_script, global_data);
            }
        }
        else{
            std::cout << "ELSE " << probe_name << std::endl; 
            for(auto loc : localities){
                register_user_probe_action register_action;
                register_action(loc, probe_name, probe_predicate, probe_script, global_data);
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

HPX_DEFINE_PLAIN_ACTION(parse_script, parse_script_action); 



void init(std::string script){
    //Find all localities
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
    //hpx::id_type server_id = hpx::new_<server::script_data>(hpx::find_here());
    global_data = hpx::new_<script_data>(hpx::find_here());
    /*parse_script_action psa;
    for(auto loc : localities){
        hpx::async(psa, loc, script).get();
    }*/
    parse_script(script);
}

//destruct interval_timers
void finalize(){
    for (auto  element : interval_timers) {
        std::cout << "finalize\n";
        element->~interval_timer();
    }
}
}
HPX_REGISTER_ACTION(API::register_user_probe_action, api_register_user_probe_action);
HPX_REGISTER_ACTION(API::register_task_probe_action, api_register_task_probe_action);
HPX_REGISTER_ACTION(API::register_message_probe_action, api_register_message_probe_action);

HPX_REGISTER_ACTION(API::parse_script_action, api_parse_script_action);