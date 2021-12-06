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


namespace API
{


    std::map<std::string,double> gv; //global variables

    std::map<std::string,apex_event_type> event_types;




    namespace qi = boost::spirit::qi;

    namespace ascii = boost::spirit::ascii;
    namespace phx = boost::phoenix;






    ///////////////////////////////////////////////////////////////////////////
    //  Our number list parser
    ///////////////////////////////////////////////////////////////////////////
    //[tutorial_numlist1
    template <typename Iterator>
    bool parse_probe(Iterator first, Iterator last) {
        using qi::double_;
        using qi::char_;
        using qi::_1;
        using qi::_2;
        using qi::_val;

        //using qi::string_;


        using qi::phrase_parse;
        using ascii::space;
        




        //qi::rule<Iterator, double> expression, single;
        //qi::rule<Iterator> assignment;

        qi::rule<Iterator, ascii::space_type, std::string()> var = +char_("a-zA-Z");
/*
        qi::rule<Iterator, double> number = qi::lexeme_d[ (+double_) ];
        single = double_[_val = _1] | var[_val = phx::ref(gv)[_1]];
        expression = (expression >> '+' >> expression) | single;
        expression = single[_val = _1] | 
                     (expression >> '+');// >> expression)[_val = _1 + _2];
        


*/

        qi::rule<Iterator, ascii::space_type, double> expression, term, factor, assignment;

        expression =
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
            var                             [_val = phx::ref(gv)[_1]]
            |   double_                         [_val = _1]
            |   '(' >> expression           [_val = _1] >> ')'
            |   ('-' >> factor              [_val = -_1])
            |   ('+' >> factor              [_val = _1])
            ;

        BOOST_SPIRIT_DEBUG_NODE(expression);
        BOOST_SPIRIT_DEBUG_NODE(term);
        BOOST_SPIRIT_DEBUG_NODE(factor);
        
        assignment = (var >> '=' >> expression)[phx::ref(gv)[_1] = _2];


        qi::rule<Iterator, ascii::space_type> probe;

        probe = assignment >> ';' >> *(assignment >> ';');




        bool r = phrase_parse(
            first,                          /*< start iterator >*/
            last,                              /*< end iterator >*/
            probe,   /*< the parser >*/
            space
        );


        if (first != last) // fail if we did not get a full match
            return false;


        return r;
    }
    //]

    void print(std::string variable){
        std::cout << gv[variable] << std::endl;
    }


    void trigger_probe(std::string probe_name, std::vector<std::string> arguments_values){
        //trigger_event();


        for (int i = 0; i < arguments_values.size(); i++) {
            //std::cout << arguments_values[i] << std::endl;
                    
        }

        apex::custom_event(event_types[probe_name], &arguments_values);
    }

    void register_probe(std::string probe_name, std::vector<std::string> arguments_names, std::string script){

        apex_event_type event_type = apex::register_custom_event(probe_name);
        event_types[probe_name] = event_type;

        apex::register_policy(event_type,
          [arguments_names, script](apex_context const& context)->int{
                //std::cout << context.event_type << std::endl;
                //vector<MyClass*>& v = *reinterpret_cast<vector<MyClass*> *>(voidPointerName);
                std::vector<std::string>& arguments_values = *reinterpret_cast<std::vector<std::string>*>(context.data);
                for (int i = 0; i < arguments_names.size(); i++) {
                    //std::cout << arguments_names[i] << " " << arguments_values[i] << std::endl;
                    
                }


                parse_probe(script.begin(), script.end());


                std::cout << gv["x"] << "\n";





            return APEX_NOERROR;
            });

    }

    void parse_script(const std::string& script){
        std::regex rgx("([a-zA-Z0-9]+)\\{([^{}]*)\\}");
        std::smatch match;

        std::regex_search(script.begin(), script.end(), match, rgx);

        std::cout << match[1] << "\n" << match[2] << "\n";


        std::string probe_name = match[1];
        std::string probe_script = match[2];



        std::map<std::string,double> gv;

        std::vector<std::string> arguments_names = {"i","j","k"};
        register_probe(probe_name, arguments_names, probe_script);



    }





}

////////////////////////////////////////////////////////////////////////////
//  Main program
////////////////////////////////////////////////////////////////////////////

/*
int
main()
{
    std::cout << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "\t\tA comma separated list parser for Spirit...\n\n";
    std::cout << "/////////////////////////////////////////////////////////\n\n";

    std::cout << "Give me a comma separated list of numbers.\n";
    std::cout << "Type [q or Q] to quit\n\n";

    std::string str;
    std::map<std::string,double> gv;
    while (getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        if (client::parse_probe(str.begin(), str.end(), gv))
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            std::cout << str << " Parses OK: " << std::endl;
        }
        else
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}
*/