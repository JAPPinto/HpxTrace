#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>

#include "comp.hpp"
#include "parse.cpp"


#include <apex_api.hpp>


#include "argument.cpp"
//#include "parse.cpp"


using hpx::performance_counters::performance_counter;


std::atomic<std::uint64_t> count(0);





















int hpx_main(hpx::program_options::variables_map& vm)
{
    std::vector<std::string> arguments_values = {"1","2","3"};


    //run_code();

    // extract command line argument, i.e. fib(N)
    std::string script = vm["script"].as<std::string>();

    if(script == "") script = "abc{x=3; x = x * 10;}";


    API::parse_script(script);

            std::cout << "________________" << "\n";


    API::trigger_probe("abc", arguments_values);
    API::trigger_probe("abc", arguments_values);
    API::trigger_probe("abc", arguments_values);
    

    hpx::finalize();
    return 0;
}


int main(int argc, char* argv[]) {


    // Configure application-specific options
    hpx::program_options::options_description
       desc_commandline("Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()
        ( "script",
          hpx::program_options::value<std::string>()->default_value(""),
          "script for tracing")
        ;



    // Initialize and run HPX
    int status =  hpx::init(desc_commandline, argc, argv);


    return 0;




}