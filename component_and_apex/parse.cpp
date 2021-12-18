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
#include "sample_value_event_data.hpp"

#define BOOST_SPIRIT_USE_PHOENIX_V3 1

using namespace std;


namespace API
{

    typedef boost::variant<double, std::string> Variant;

    #define VARIANT_DOUBLE 0
    #define VARIANT_STRING 1




    std::map<std::string,double> dvars; //global variables
    std::map<std::string, std::string> stvars; //global variables

    std::map<std::string,apex_event_type> event_types;




    std::vector<hpx::util::interval_timer*> interval_timers;

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
        auto d_it = dvars.find(name);
        if ( d_it != dvars.end()){
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



    template <typename Iterator>
    bool parse_probe(Iterator first, Iterator last) {
        using qi::double_;
        using qi::char_;
        using qi::_1;
        using qi::_2;
        using qi::_val;
        using qi::_pass;

        //using qi::string_;


        using qi::phrase_parse;
        using ascii::space;
        





        qi::rule<Iterator, ascii::space_type, std::string()> var = +char_("a-zA-Z_");
        qi::rule<Iterator, std::string()> string_content = +(char_ - '"');
        //+(char_ - '"');
        qi::rule<Iterator, ascii::space_type, std::string()> string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];
        //checks if "string" or string variable and returns value
        qi::rule<Iterator, ascii::space_type, std::string()> var_or_string = 
                                                                            string[_val = _1]
                                                                            |
                                                                            var[_val = phx::ref(stvars)[_1]]
                                                                            ;



        qi::rule<Iterator, ascii::space_type, std::string()> strjoin, str_function, var_func_string;
        qi::rule<Iterator, ascii::space_type, double> expression, term, factor ;
        qi::rule<Iterator, ascii::space_type> probe, statement, assignment, print, function;

        qi::rule<Iterator, ascii::space_type, Variant()> value ;

        value = double_[_val = _1] 
              | string[_val = _1] 
              | str_function[_val = _1]
              | expression [_val = _1]
              | var[_val = boost::phoenix::bind(&evaluate_string, _1)];


              //a = var
              //a = var + 3

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
            (var[_pass = boost::phoenix::bind(&is_double_variable, _1)])[_val = phx::ref(dvars)[_1]]
            |   double_                     [_val = _1]
            |   '(' >> expression           [_val = _1] >> ')'
            |   ('-' >> factor              [_val = -_1])
            |   ('+' >> factor              [_val = _1])
            ;


        //assignment = (var >> '=' >> var)[ boost::phoenix::bind(&f, _1, _2)] | 
    /*    assignment = 
                ( "double" >> var >> '=' >> expression)[phx::ref(dvars)[_1] = _2]
               // | ("string" >> var  >> '=' >> var);
                | 
                ("string" >> var >> '=' >> var_func_string)[phx::ref(stvars)[_1] = _2]                 
                ;
*/

        assignment = (var >> '=' >> value)[ boost::phoenix::bind(&store_variable, _1, _2)];  

        print = ("print(" >> value >> ')')[boost::phoenix::bind(&print_value, _1)];


        strjoin = ("strjoin(" >> value >> ',' >> value >> ')')[_val = boost::phoenix::bind(&strjoin_func, _1, _2)];

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
            std::cout << "FALHOU " << probe << "\n";
            return false;
        }


        return r;
    }


    bool compare(Variant a, std::string op, Variant b){
        std::cout << "compare " << a  << op  << b << " "<< "\n";

        if(a.type() == typeid(double)) std::cout << "AAAAA\n";

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
    bool parse_predicate(Iterator first, Iterator last) {

        using qi::double_;
        using qi::char_;
        using qi::_1;
        using qi::_2;
        using qi::_3;
        using qi::_val;
        using qi::phrase_parse;
        using ascii::space;
        

        typedef boost::variant<double, std::string> Variant;


        qi::rule<Iterator, ascii::space_type, std::string()> var = +char_("a-zA-Z_");
        qi::rule<Iterator, std::string()> string_content = +(char_ - '"');
                                   
        qi::rule<Iterator, ascii::space_type, std::string()> string = qi::lexeme[('"' >> string_content >> '"')][_val = _1];


        qi::rule<Iterator, ascii::space_type, std::string()> op;
        qi::rule<Iterator, ascii::space_type, bool> predicate, comparison, ands, ors, temp, expression, subexpression;
        qi::rule<Iterator, ascii::space_type, Variant()> value ;

        value = double_[_val = _1] | string[_val = _1] | var[_val = boost::phoenix::bind(&evaluate_string, _1)];



        op = +char_("=<>!");

        comparison = (value >> op >> value)[_val = boost::phoenix::bind(&compare, _1, _2, _3)];


        temp = comparison | ('(' >> ors >> ')') ;

        bool result;

        ands = temp[_val = _1] >> *("&&" >> temp)[_val &= _1];
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
        std::cout << "register_probe " << probe_predicate << std::endl;
                if(parse_predicate(probe_predicate.begin(), probe_predicate.end())){
                    parse_probe(script.begin(), script.end());
                }
                


            return APEX_NOERROR;
            });

    }



    bool read_counter(hpx::performance_counters::performance_counter counter, std::string* counter_name){

        int value = counter.get_value<int>().get();
        std::cout << "READ COUNTER " << *counter_name << " " << value << std::endl;
        //reading the counter from the API does not tigger APEX_SAMPLE_VALUE event to it has to be triggered manually
        apex::sample_value(*counter_name, value);
        return true;
    }


    void register_counter_probe(std::string probe_name ,std::string probe_predicate, std::string script){

        std::regex rgx("counter::([^:]+)::([0-9]+)");
        std::smatch match;
        std::regex_search(probe_name, match, rgx);
        std::cout << "REGISTER COUNTER " << match[0] << " " << match[1] << " " << match[2] << std::endl; 

        std::string counter_name = match[1];
        int period = std::stoi(match[2]);
        std::string* name_ptr = new std::string(counter_name);



        apex::register_policy(APEX_SAMPLE_VALUE, [script, probe_predicate, name_ptr](apex_context const& context)->int{
            
            sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
                
            if(*dt.counter_name == *name_ptr){

                stvars["counter_name"] = *name_ptr;
                dvars["counter_value"] = dt.counter_value;

                if(parse_predicate(probe_predicate.begin(), probe_predicate.end())){
                    std::cout << "APEX_SAMPLE_VALUE" << *(dt.counter_name) << " " << dt.counter_value << std::endl;
                    parse_probe(script.begin(), script.end());
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





    void parse_script(std::string script){

        std::regex rgx1("^[\\n\\r\\s]*([a-zA-Z0-9]+)(/[^/]*/)?\\{([^{}]*)\\}");
        std::regex rgx2("^[\\n\\r\\s]*(counter::[^:]+::[0-9]+)(/[^/]*/)?\\{([^{}]*)\\}");

        std::smatch match;



        while (std::regex_search(script, match, rgx1) || std::regex_search(script, match, rgx2)){
            std::string probe_name = match[1];
            std::string probe_predicate = match[2];
            std::string probe_script = match[3];
            std::cout << probe_name << " " << probe_script << "\n";

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

            else if(probe_name.find("counter") != -1){
                std::cout << "COUNTER " << probe_name << std::endl; 
                register_counter_probe(probe_name, probe_predicate, probe_script);
            }
            else{
                
                std::cout << "ELSE " << probe_name << std::endl; 

                register_probe(probe_name, probe_predicate, probe_script);

            }


            script = match.suffix();

        }










    }


    void finalize(){
        for (auto  element : interval_timers) {
            std::cout << "finalize\n";
            element->~interval_timer();
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