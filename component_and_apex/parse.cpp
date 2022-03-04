#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <map>
#include <chrono>
#include <apex_api.hpp>
#include <regex>
#include <boost/bind.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include "variables_map.cpp"
#include "sample_value_event_data.hpp"
#include "Aggregation.cpp"


#define BOOST_SPIRIT_USE_PHOENIX_V3 1

using namespace std;


namespace API
{

    typedef boost::variant<double, std::string> Variant;

    //std::map<std::string, std::map<std::vector<boost::variant<double, std::string>>, double>> aggvars

    #define VARIANT_DOUBLE 0
    #define VARIANT_STRING 1




    std::map<std::string,double> dvars; //global variables
    std::map<std::string, std::string> stvars; //global variables
    //std::map<std::string, std::map<Variant,double>> aggvars; //aggregation variables
    std::map<std::string, Aggregation*> aggvars; //aggregation variables
    std::map<std::string,apex_event_type> event_types; 
    std::vector<hpx::util::interval_timer*> interval_timers; //for counter-create
    std::chrono::steady_clock::time_point start_time;

    namespace qi = boost::spirit::qi;

    namespace ascii = boost::spirit::ascii;
    namespace phx = boost::phoenix;

    #define NON_EXISTANT -1
    #define DOUBLE_VAR 0
    #define STRING_VAR 1

    int check_type(string name){
        auto d_it = dvars.find(name);
        if ( d_it != dvars.end()){
            return DOUBLE_VAR;
        }
        auto st_it = stvars.find(name);
        if ( st_it != stvars.end()){
            return STRING_VAR;
        }
        return NON_EXISTANT;
    }

    bool is_double_variable(string name){
        auto it = dvars.find(name);
        if ( it != dvars.end()){
            return true;
        }
        return false;
    }

        bool is_string_variable(string name){
        auto it = stvars.find(name);
        if ( it != stvars.end()){
            return true;
        }
        return false;
    }

    //check if is string literal, double variable or string variable
    Variant evaluate_string(std::string s){
        std::cout << "evaluate_string  " << s << "\n";



        for(auto it = dvars.cbegin(); it != dvars.cend(); ++it)
        {
            std::cout << "double " << it->first << " " << it->second << "\n";
        }
        cout << endl;
        for(auto it = stvars.cbegin(); it != stvars.cend(); ++it)
        {
            std::cout << "string " << it->first << " " << it->second << "\n";
        }

        auto d_it = dvars.find(s);
        if ( d_it != dvars.end()){
            std::cout << "evaluate_string double " << d_it->second << "\n";

            return d_it->second;
        }
            std::cout << "evaluate_string string " <<  stvars[s]<< "\n";

        return stvars[s];
    }


    void print_variable(std::string variable){

        std::map<std::string,double>::iterator d_it = dvars.find(variable);
        if ( d_it != dvars.end()){
            std::cout << "API PRINT double:" << d_it->second << std::endl;
        }
        else{
            std::cout << "API PRINT string: " << stvars[variable] << std::endl;
        }
    }

    void print_value(Variant v){
        std::cout << "API PRINT:" << v << std::endl;
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


/*
        if(std::is_arithmetic(v.type())){
            std::cout << "API PRINT double:" << boost::get<double>(v) << std::endl;
            return;
        }

        std::map<std::string,double>::iterator d_it = dvars.find(variable);
        if ( d_it != dvars.end()){
            std::cout << "API PRINT double:" << d_it->second << std::endl;

        }
        else{
            std::cout << "API PRINT string: " << stvars[variable] << std::endl;
        }
    }*/





    void print_string_value(std::string s){
        std::cout << "API PRINT STRING: " << s << std::endl;
    }

    std::string strjoin_func(Variant a, Variant b){
        if(a.type() != typeid(string) || b.type() != typeid(string)){
            throw "strjoin: invalid argument "; 
        }
        std::cout << "strjoin " << boost::get<string>(a)+boost::get<string>(b) << std::endl;

        return boost::get<string>(a)+boost::get<string>(b);
    }


    std::string get_var_or_string(std::string s){
        std::cout << "get_var_or_string " << s << std::endl;


        std::smatch match;
        std::regex rgx("{([.-}]*)}");
        if(std::regex_search(s, match, rgx)){
            return match[1];
        }
        else{
            return stvars[s];
        }
    }

    void store_variable(string name, Variant value){
        std::cout << "store_variable " << name << value.which()  << std::endl;


        if(value.which() == VARIANT_DOUBLE){
            if(check_type(name) == STRING_VAR) 
                throw  name + " variable already exists as a string";
            std::cout << "store_variable double " << name << boost::get<double>(value)  << std::endl;

            dvars[name] = boost::get<double>(value);
        }
        if(value.which() == VARIANT_STRING){
            if(check_type(name) == DOUBLE_VAR) 
                throw  name + " variable already exists as a double";
            std::cout << "store_variable string " << name << boost::get<string>(value)  << std::endl;

            stvars[name] = boost::get<string>(value);

        }
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
                    AverageAggregation* g = new AverageAggregation(name, new_function);
                    aggvars[name] = g;
                }
                else if(new_function == "quantize"){
                    Quantization* q = new Quantization(name, new_function);
                    aggvars[name] = q;
                }
                //if count, sum, max, min
                else if (new_function != "lquantize"){
                    ScalarAggregation* g = new ScalarAggregation(name, new_function);
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
                LQuantization* q = new LQuantization(name,"lquantize",lower_bound,upper_bound,step);
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

        //using qi::string_;


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


        //RETIRAR VAR DE VALUE?
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
        double_function = dbl;

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
            last,                              /*< end iterator >*/
            actions,   /*< the parser >*/
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


    template <typename Iterator>
    bool parse_probe(Iterator first, Iterator last) {
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
        RuleString string_var = (var[_pass = boost::phoenix::bind(&is_string_variable, _1)])[_val = phx::ref(stvars)[_1]];
        RuleDouble double_var = (var[_pass = boost::phoenix::bind(&is_double_variable, _1)])[_val = phx::ref(dvars)[_1]];

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





        Rule assignment = (var >> '=' >> arithmetic_expression)[phx::ref(dvars)[_1] = _2]
                    | (var >> '=' >> string_expression)[phx::ref(stvars)[_1] = _2];  



        Rule print = ("print(" >> value >> ')')[boost::phoenix::bind(&print_value, _1)];
        
        RuleString str = ("str(" >> arithmetic_expression >> ')')[_val = boost::phoenix::bind(&to_string, _1)];
        string_function = str;

        RuleDouble dbl = ("dbl(" >> string_expression >> ')')[_val = boost::phoenix::bind(&to_double, _1)];
        double_function = dbl;

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
            last,                              /*< end iterator >*/
            actions,   /*< the parser >*/
            space
        );


        if (first != last){// fail if we did not get a full match
            std::string error = "Variable with wrong type/uninitialized in action: \n" ;
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

    bool and_operator(bool a, bool b){

        return a && b;
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

        //using qi::string_;


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
        else{
            std::cout << "DEU " << result <<  "\n";

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

    void register_probe(std::string probe_name, std::string probe_predicate, std::string script){

        apex_event_type event_type = apex::register_custom_event(probe_name);
        event_types[probe_name] = event_type;

        apex::register_policy(event_type,
          [script, probe_predicate](apex_context const& context)->int{
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
                if(probe_predicate == "" || parse_predicate(probe_predicate.begin(), probe_predicate.end())){
                    parse_probe(script.begin(), script.end());
                }
                


            return APEX_NOERROR;
            });

    }



    bool read_counter(hpx::performance_counters::performance_counter counter, std::string* counter_name){

        int value = counter.get_value<int>().get();
        //reading the counter from the API does not tigger APEX_SAMPLE_VALUE event to it has to be triggered manually
        apex::sample_value(*counter_name, value);
        return true;
    }


    void fill_counter_variables(string name, double value){
        stvars["counter_name"] = name;
        dvars["counter_value"] = value;

        hpx::performance_counters::counter_path_elements p;

        hpx::performance_counters::counter_status status = 
                    get_counter_path_elements(name, p);

        stvars["counter_type"] = '/' + p.objectname_ + '/' + p.countername_;
        stvars["counter_parameters"] = p.parameters_;
        stvars["counter_parent_instance_name"] = p.parentinstancename_;
        stvars["counter_parent_instance_index"] = p.parentinstanceindex_;
        stvars["counter_instance_name"] = p.instancename_;
        stvars["counter_instance_index"] = p.instanceindex_;
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
                    parse_probe(script.begin(), script.end());
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
                        parse_probe(script.begin(), script.end());
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

                hpx::performance_counters::counter_status status = 
                            get_counter_path_elements(*dt.counter_name, p);

                string type_sampled = '/' + p.objectname_ + '/' + p.countername_;


                //if sampled counter belongs to the desired type 
                if(type_sampled.find(*(type_ptr)) != -1){
                    cout << "PATHS DEU" << endl;


                    fill_counter_variables(*dt.counter_name, dt.counter_value);

                    if(probe_predicate == "" || parse_predicate(probe_predicate.begin(), probe_predicate.end())){
                        std::cout << "APEX_SAMPLE_VALUE" << *(dt.counter_name) << " " << dt.counter_value << std::endl;
                        parse_probe(script.begin(), script.end());
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
        
        string filter = "";

        if(std::regex_search(probe_name, match, rgx)){
            filter = string(match[1]);
        }

        std::string counter_name;

 std::cout << "APEX_SAMPLE_VALUE filter" << filter << endl;


        apex::register_policy(APEX_SAMPLE_VALUE, [script, probe_predicate, filter](apex_context const& context)->int{
            
            sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
            
            if((*dt.counter_name).find(filter) != -1){

                //fill_counter_variables(*name_ptr, dt.counter_value);

                if(probe_predicate == "" || parse_predicate(probe_predicate.begin(), probe_predicate.end())){
                    std::cout << "APEX_SAMPLE_VALUE" << *(dt.counter_name) << " " << dt.counter_value << std::endl;
                    parse_probe(script.begin(), script.end());
                }
                //erase_counter_variables();
            }


            return APEX_NOERROR;
        });
        
    }

    void parse_script(std::string script){



        start_time = std::chrono::steady_clock::now();

        std::regex rgx_cc("^[\\n\\r\\s]*(counter\\-create::[^:]+::[0-9]+::)(/[^/]*/)?\\{([^{}]*)\\}");
        std::regex rgx_c("^[\\n\\r\\s]*(counter(?:::[^:]*::)?)(/[^/]*/)?\\{([^{}]*)\\}");
        std::regex rgx_ct("^[\\n\\r\\s]*(counter\\-type(?:::[^:]*::)?)(/[^/]*/)?\\{([^{}]*)\\}");
        std::regex rgx_proc("^[\\n\\r\\s]*(proc(?:::[^:]*::)?)(/[^/]*/)?\\{([^{}]*)\\}");
        std::regex rgx_user("^[\\n\\r\\s]*([a-zA-Z0-9]+)(/[^/]*/)?\\{([^{}]*)\\}");




        std::smatch match;



        while (std::regex_search(script, match, rgx_cc) 
        || std::regex_search(script, match, rgx_c) 
        || std::regex_search(script, match, rgx_ct)
        || std::regex_search(script, match, rgx_proc)  
        || std::regex_search(script, match, rgx_user)){


            std::string probe_name = match[1];
            std::string probe_predicate = match[2];
            std::string probe_script = match[3];
            std::cout << probe_name << " " << probe_script << "\n";

            if(probe_predicate != "") 
                validate_predicate(probe_predicate.begin(), probe_predicate.end());
            validate_actions(probe_script.begin(), probe_script.end());



            if(probe_name == "BEGIN"){

                std::cout << "BEGIN " << probe_name << std::endl; 



                parse_probe(probe_script.begin(), probe_script.end());
            }

            else if(probe_name == "END"){
                std::cout << "END " << probe_name << std::endl; 

                std::string end_script = probe_script;

                hpx::register_shutdown_function(
                  [end_script]()->void{
         
                    
                    parse_probe(end_script.begin(), end_script.end());    
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
            else{
                
                std::cout << "ELSE " << probe_name << std::endl; 

                register_probe(probe_name, probe_predicate, probe_script);

            }


            script = match.suffix();

        }



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

    void finalize(){
        for (auto  element : interval_timers) {
            std::cout << "finalize\n";
            element->~interval_timer();
        }
    }
}