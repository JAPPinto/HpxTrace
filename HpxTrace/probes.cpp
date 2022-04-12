#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>
#include <stdlib.h>  
#include "HpxTrace.cpp"


#include <apex_api.hpp>


#include "argument.cpp"

//[fib_action
// forward declaration of the Fibonacci function
std::uint64_t test(std::uint64_t n);



// This is to generate the required boilerplate we need for the remote
// invocation to work.
HPX_PLAIN_ACTION(test, test_action);
//]

///////////////////////////////////////////////////////////////////////////////
//[fib_func
std::uint64_t test(std::uint64_t n)
{
//std::cout << "test " << n << " " <<  n % 2 << std::endl;

    //Find all localities
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();

    hpx::naming::id_type const locality_id = localities[localities.size()-1];
    //usleep(1000000);
    usleep(100000);

    if(hpx::find_here() == localities[0]){
        HpxTrace::trigger_probe("abc", {{"a",0}}, {{"s", "ola"}} );
    }
    else{
        HpxTrace::trigger_probe("abc", {{"a",1}}, {{"s", "ola"}} );
    } 


    if (n == 0)
        return 0;
    test_action fib;
    std::uint64_t x;
    if(localities.size() > 1){
        hpx::future<std::uint64_t> n1 = hpx::async(fib, localities[ n % 2], n - 1);
        x = n1.get();
    }
    else{
        hpx::future<std::uint64_t> n1 = hpx::async(fib, localities[0], n - 1);
        x = n1.get();
    }
   // for(auto loc : localities){
    //}

    return x;

}





using hpx::performance_counters::performance_counter;


std::atomic<std::uint64_t> count(0);

    


int hpx_main(hpx::program_options::variables_map& vm)
{

   

    std::vector<std::string> arguments_values = {"1","2","3"};



    std::string script = vm["script"].as<std::string>();

    if(script == "") script = "abc{x=3; x = x * 10;}";

    std::string file = vm["file"].as<std::string>();

    if(file != ""){
        std::ifstream t(file);
        std::stringstream buffer;
        buffer << t.rdbuf();
        script = buffer.str();
    }



    HpxTrace::init(script);


    //API::parse_script(script);


    //usleep(1000000);
    test(10);



    //API::trigger_probe("xyz", arguments_values);

    HpxTrace::finalize();



    hpx::finalize();
    return 0;
}

//BEGIN END


int main(int argc, char* argv[]) {


    // Configure application-specific options
    hpx::program_options::options_description
       desc_commandline("Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()
        ( "script",
          hpx::program_options::value<std::string>()->default_value(""),
          "script for tracing")
        ;

    desc_commandline.add_options()
        ( "file",
          hpx::program_options::value<std::string>()->default_value(""),
          "file with script for tracing")
        ;

    // Initialize and run HPX
    //int status =  hpx::init(desc_commandline, argc, argv);

    // Initialize and run HPX
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    int status =  hpx::init(argc, argv, init_args);




    return 0;




}