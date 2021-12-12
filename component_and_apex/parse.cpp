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
#include "variables_map.cpp"

#define BOOST_SPIRIT_USE_PHOENIX_V3 1

namespace API
{


    std::map<std::string,double> dvars; //global variables
    std::map<std::string, std::string> stvars; //global variables

    std::map<std::string,apex_event_type> event_types;




    namespace qi = boost::spirit::qi;

    namespace ascii = boost::spirit::ascii;
    namespace phx = boost::phoenix;




    void print_variable(std::string variable){

        std::map<std::string,double>::iterator d_it = dvars.find(variable);
        if ( d_it != dvars.end()){
            std::cout << "DOUBLE" << std::endl;

            std::cout << "API PRINT double:" << d_it->second << std::endl;
        }
        else{
            std::cout << "API PRINT string: " << stvars[variable] << std::endl;
        }
    }



    void print_string_value(std::string s){
        std::cout << "API PRINT STRING: " << s << std::endl;
    }

    std::string strjoin_func(std::string a, std::string b){
        std::cout << "strjoin " << a+b << std::endl;

        return a + b;
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
        





        qi::rule<Iterator, ascii::space_type, std::string()> var = +char_("a-zA-Z");
        qi::rule<Iterator, std::string()> string_content = +(char_ - '"');
        //+(char_ - '"');
        qi::rule<Iterator, ascii::space_type, std::string()> string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];
        //checks if "string" or string variable and returns value
        qi::rule<Iterator, ascii::space_type, std::string()> var_or_string = 
                                                                            string[_val = _1]
                                                                            |
                                                                            var[_val = phx::ref(stvars)[_1]]
                                                                            ;




/*
        qi::rule<Iterator, double> number = qi::lexeme_d[ (+double_) ];
        single = double_[_val = _1] | var[_val = phx::ref(dvars)[_1]];
        expression = (expression >> '+' >> expression) | single;
        expression = single[_val = _1] | 
                     (expression >> '+');// >> expression)[_val = _1 + _2];
        


*/  
        qi::rule<Iterator, ascii::space_type, std::string()> strjoin, str_function, var_func_string;
        qi::rule<Iterator, ascii::space_type, double> expression, term, factor ;
        qi::rule<Iterator, ascii::space_type> probe, statement, assignment, print, function;



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
            var                             [_val = phx::ref(dvars)[_1]]
            |   double_                     [_val = _1]
            |   '(' >> expression           [_val = _1] >> ')'
            |   ('-' >> factor              [_val = -_1])
            |   ('+' >> factor              [_val = _1])
            ;


        //assignment = (var >> '=' >> var)[ boost::phoenix::bind(&f, _1, _2)] | 
        assignment = 
                ( "double" >> var >> '=' >> expression)[phx::ref(dvars)[_1] = _2]
               // | ("string" >> var  >> '=' >> var);
                | 
                ("string" >> var >> '=' >> var_func_string)[phx::ref(stvars)[_1] = _2]                 
                ;

        print = ("print(" >> var >> ')')[&print_variable]
                |
                ("print(" >> (string|str_function) >> ')')[&print_string_value]; 





        strjoin = ("strjoin(" >> var_or_string >> ',' >> var_or_string >> ')')[_val = boost::phoenix::bind(&strjoin_func, _1, _2)];

        str_function = strjoin;

        var_func_string = var_or_string | str_function;


        function = print | str_function;

        statement = assignment | function;

        probe = statement >> ';' >> *(statement >> ';');



        BOOST_SPIRIT_DEBUG_NODE(expression);
        BOOST_SPIRIT_DEBUG_NODE(term);
        BOOST_SPIRIT_DEBUG_NODE(factor);



        bool r = phrase_parse(
            first,                          /*< start iterator >*/
            last,                              /*< end iterator >*/
            probe,   /*< the parser >*/
            space
        );


        if (first != last){// fail if we did not get a full match
            std::cout << "FALHOU\n";
            return false;
        }


        return r;
    }
    //]


    typedef std::pair<std::map<std::string,double>,std::map<std::string,std::string>> arguments;

    void trigger_probe(std::string probe_name, 
        std::map<std::string,double> double_arguments = {},
        std::map<std::string,std::string> string_arguments = {}){
        
            arguments args;
            args.first = double_arguments;
            args.second = string_arguments;
            apex::custom_event(event_types[probe_name], &args);
    }

    void register_probe(std::string probe_name, std::string script){

        apex_event_type event_type = apex::register_custom_event(probe_name);
        event_types[probe_name] = event_type;

        apex::register_policy(event_type,
          [script](apex_context const& context)->int{
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



                parse_probe(script.begin(), script.end());







            return APEX_NOERROR;
            });

    }

    void parse_script(std::string script){
        std::regex rgx("([a-zA-Z0-9]+)\\{([^{}]*)\\}");
        std::smatch match;



        while (std::regex_search(script, match, rgx)){
            std::string probe_name = match[1];
            std::string probe_script = match[2];
            //std::cout << probe_name << "\n";

            if(probe_name == "BEGIN"){
                parse_probe(probe_script.begin(), probe_script.end());


            }

            else if(probe_name == "END"){
                std::string end_script = probe_script;

                hpx::register_shutdown_function(
                  [end_script]()->void{
         
                    
                    parse_probe(end_script.begin(), end_script.end());


                    
                });
            }


            register_probe(probe_name, probe_script);

            script = match.suffix();

        }










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
    std::map<std::string,double> dvars;
    while (getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        if (client::parse_probe(str.begin(), str.end(), dvars))
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