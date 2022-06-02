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

#include "actions.cpp"
#include "base_grammar.cpp"


#define BOOST_SPIRIT_USE_PHOENIX_V3 1



namespace HpxTrace
{

typedef boost::variant<double, std::string> Variant;
typedef std::vector<Variant> VariantList;

namespace qi = boost::spirit::qi;

namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;

using hpx::naming::id_type;
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


template <typename Iterator>
struct execution_grammar : qi::grammar<Iterator, ascii::space_type, std::string()>
{

    typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleSt;
    typedef qi::rule<Iterator, ascii::space_type, double> RuleN;
    typedef qi::rule<Iterator, ascii::space_type> Rule;


    execution_grammar() : execution_grammar::base_type{var}
    {

        keys = value % ',';
        //Map rules
        probe_map = (bg.probe_var >> '[' >> keys >> ']');
        local_map = (bg.local_var >> '[' >> keys >> ']');
        global_map = (bg.global_var >> '[' >> keys >> ']');
        map = probe_map | local_map | global_map;

        //Function rules

        //Values rules
        string_value = bg.string | string_function | map | bg.var;
        double_value = double_ | double_function | map | bg.var;

        //Expression rules

        string_expression = 
            string_value                            
            >> *('+' >> string_value);


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

 


        str = ("str(" >> arithmetic_expression >> ')');

        dbl = ("dbl(" >> string_expression >> ')');

        round = ("round(" >> arithmetic_expression >> ')');
        ceil = ("ceil(" >> arithmetic_expression >> ')');
        floor = ("floor(" >> arithmetic_expression >> ')');


        double_function = dbl | round | ceil | floor;
        string_function = str;
        value = arithmetic_expression | string_expression;


        var = bg.var;
    }

    RuleSt var;
    base_grammar<Iterator> bg;
    Rule value,
         keys,
         probe_map, local_map, global_map, map,
         string_value, double_value,
         string_function, double_function,
         string_expression,
         arithmetic_expression, term, factor,
         str, dbl, round, ceil, floor;

};



}