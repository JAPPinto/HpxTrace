#ifndef _BASEGRAMMAR_
#define _BASEGRAMMAR_

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
struct base_grammar : qi::grammar<Iterator, ascii::space_type, std::string()>
{

    typedef qi::rule<Iterator, ascii::space_type, std::string()> RuleSt;
    typedef qi::rule<Iterator, ascii::space_type, double> RuleN;
    typedef qi::rule<Iterator, ascii::space_type> Rule;


    base_grammar() : base_grammar::base_type{var}
    {
        //String rules
        string_content = +(char_ - '"');
        string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];
        //Var rules
        probe_var = (char_("&") >> char_("a-zA-Z") >> *char_("a-zA-Z0-9_"));
        local_var = (char_("a-zA-Z") >> *char_("a-zA-Z0-9_"));
        global_var = (char_("#") >> char_("a-zA-Z") >> *char_("a-zA-Z0-9_"));

        var = probe_var | local_var | global_var;
    }

    qi::rule<Iterator, std::string()> string_content;
    RuleSt string;
    RuleSt probe_var, local_var, global_var, var;

};

}
#endif