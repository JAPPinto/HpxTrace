#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <apex_api.hpp>
#include <regex>
#include <boost/bind.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>


#define BOOST_SPIRIT_USE_PHOENIX_V3 1

using namespace std;
    namespace qi = boost::spirit::qi;


typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleString;
typedef qi::rule<Iterator, ascii::space_type, double> RuleDouble;
typedef qi::rule<Iterator, ascii::space_type> Rule;


template <typename Iterator>
struct actions_grammar : grammar<Iterator, A1, A2, A3>
{
    actions_grammar() : my_grammar::base_type(start, name)
    {
        // Rule definitions
        start = string;
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


    std::map<std::string,double> dvars; //global variables
    std::map<std::string, std::string> stvars; //global variables

    //String rules
    qi::rule<Iterator, std::string()> string_content = +(char_ - '"');
    RuleString string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];
    //Var rules
    RuleString var = +char_("a-zA-Z_");
    RuleString string_var = (var[_pass = boost::phoenix::bind(&is_string_variable, _1)])[_val = phx::ref(stvars)[_1]];
    RuleDouble double_var = (var[_pass = boost::phoenix::bind(&is_double_variable, _1)])[_val = phx::ref(dvars)[_1]];
    //Function rules
    RuleString string_function;
    RuleDouble double_function;
    //Values rules
    RuleString string_value = string | string_function | string_var;
    RuleDouble double_value = double_ | double_function | double_var;
};