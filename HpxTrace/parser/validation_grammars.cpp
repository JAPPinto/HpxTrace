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
struct validation_grammar : qi::grammar<Iterator, ascii::space_type, std::string()>
{

    typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleSt;
    typedef qi::rule<Iterator, ascii::space_type, double> RuleN;
    typedef qi::rule<Iterator, ascii::space_type> Rule;


    validation_grammar() : validation_grammar::base_type{var}
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


template <typename Iterator>
struct predicate_validation_grammar : qi::grammar<Iterator, ascii::space_type>
{

    typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleSt;
    typedef qi::rule<Iterator, ascii::space_type, double> RuleN;
    typedef qi::rule<Iterator, ascii::space_type> Rule;


    predicate_validation_grammar() : predicate_validation_grammar::base_type{predicate}
    {
        comparison = (vg.string_expression >> "==" >> vg.string_expression)
               | (vg.string_expression >> "!=" >> vg.string_expression)
               | (vg.arithmetic_expression >> "==" >> vg.arithmetic_expression)
               | (vg.arithmetic_expression >> "!=" >> vg.arithmetic_expression)
               | (vg.arithmetic_expression >> "<"  >> vg.arithmetic_expression)
               | (vg.arithmetic_expression >> "<=" >> vg.arithmetic_expression)
               | (vg.arithmetic_expression >> ">"  >> vg.arithmetic_expression)
               | (vg.arithmetic_expression >> ">=" >> vg.arithmetic_expression);

        parentheses = comparison | ('(' >> ors >> ')');
        ands = parentheses >> *("&&" >> parentheses);
        ors = ands >> *("||" >> ands);

        predicate = ('/' >> ors  >> '/');
    }

    Rule predicate, comparison, ands, ors, parentheses, expression, subexpression;


    base_grammar<Iterator> bg;
    validation_grammar<Iterator> vg;
};


template <typename Iterator>
bool ffp(Iterator first, Iterator last)
{

  predicate_validation_grammar<std::string::iterator> g;

    bool r = phrase_parse(
        first,                              /*< start iterator >*/
        last,                               /*< end iterator >*/
        g,                                  /*< the parser >*/
        ascii::space
    );

    if (first != last){// fail if we did not get a full match
        std::string error = "Syntax error in predicate: \n" ;
        for (; first != last; first++)
        {
            error += *first;
        }
        throw std::runtime_error(error);
    }
    return r;
}


template <typename Iterator>
struct actions_validation_grammar : qi::grammar<Iterator, ascii::space_type>
{

    typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleSt;
    typedef qi::rule<Iterator, ascii::space_type, double> RuleN;
    typedef qi::rule<Iterator, ascii::space_type> Rule;


    actions_validation_grammar(ScriptData sd) : actions_validation_grammar::base_type{actions}
    {
        data = sd;
        Rule var = bg.var;
        map = vg.map;
        Rule keys = vg.keys;
        Rule arithmetic_expression = vg.arithmetic_expression;
        Rule string_expression = vg.string_expression;
        aggregation = bg.var;
        assignment = ((vg.map | bg.var) >> '=' >>
                     (vg.arithmetic_expression | vg.string_expression));

        string_content = bg.string_content;//+(char_ - '"');



        aggregation = 
        ('@' >> bg.local_var >> '[' >> vg.keys >> ']' >> "=" >> "count()")
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "count")]
        | 
        ('@' >> bg.local_var >> '[' >> vg.keys >> ']' >> "=" >> "sum" >> '(' >> vg.arithmetic_expression  >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "sum")]
        |
        ('@' >> bg.local_var >> '[' >> vg.keys >> ']' >> "=" >> "avg" >> '(' >> vg.arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "avg")]
        | 
        ('@' >> bg.local_var >> '[' >> vg.keys >> ']' >> "=" >> "min" >> '(' >> vg.arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "min")]
        | 
        ('@' >> bg.local_var >> '[' >> vg.keys >> ']' >> "=" >> "max" >> '(' >> vg.arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "max")]
        | 
        ('@' >> bg.local_var >> '[' >> vg.keys >> ']' >> "=" >> "quantize" >> '(' >> vg.arithmetic_expression >> ')')
        [phx::bind(&validate_aggregating_function, phx::ref(data.aggregations), _1, "quantize")]
        | 
        ('@' >> bg.local_var >> '[' >> vg.keys >> ']' >> "=" >> "lquantize" >> '(' >>
                                                                      vg.arithmetic_expression >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >> ',' >>
                                                                      double_ >>
                                                                      ')')
        [phx::bind(&validate_lquantization, phx::ref(data.aggregations), _1,_2,_3,_4)];


        localities = int_ % ',';

        print = ("print(" >> vg.arithmetic_expression >> ')')
           | ("print(" >> string_expression >> ')')
           | (lit("print(") >> '@' >> bg.var >> ')')
           | (lit("global_print(") >> '@' >> bg.var >> ')')
           | (lit("global_print(") >> '@' >> bg.var >> ',' >> localities >> ')');
        lock =  lit("lock") >> '(' >> vg.keys >> ')';
        unlock =  lit("unlock") >> '(' >> vg.keys >> ')';

        global_lock = (lit("global_lock") >> '(' >> vg.keys >> ')');
                
        global_unlock = (lit("global_unlock") >> '(' >> vg.keys >> ')');

        locks = lock | unlock | global_lock | global_unlock;

        action = assignment | print | aggregation | locks;
        actions = action >> ';' >> *(action >> ';');


    }
    ScriptData data;
    Rule map;
    Rule x, string_content;
    base_grammar<Iterator> bg;
    validation_grammar<Iterator> vg;

    Rule assignment, aggregation, localities, print, lock, unlock, global_lock, global_unlock, locks;
    Rule action, actions;
};


template <typename Iterator>
bool fff(Iterator first, Iterator last, ScriptData data)
{

  static actions_validation_grammar<std::string::iterator> g(data);

    bool r = phrase_parse(
        first,                              /*< start iterator >*/
        last,                               /*< end iterator >*/
        g,                                  /*< the parser >*/
        ascii::space
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


}