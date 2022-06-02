#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>
#include <stdlib.h>  
#include "HpxTrace.hpp"


#include <apex_api.hpp>


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

        std::cout << "AAAAAAAAAAAA " << reinterpret_cast<std::uint64_t>(hpx::threads::get_self_id().get()) << std::endl;
        //apex::resume("FFFFF");
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
        //hpx::future<std::uint64_t> n1 = hpx::async(fib, localities[ n % 2], n - 1);
        //x = n1.get();
        x = fib( localities[ n % 2], n - 1);
    }
    else{
        //hpx::future<std::uint64_t> n1 = hpx::async(fib, localities[0], n - 1);
        //x = n1.get();
        x = fib( localities[0], n - 1);

    }
   // for(auto loc : localities){
    //}

    return x;

}





using hpx::performance_counters::performance_counter;


std::atomic<std::uint64_t> count(0);




int hpx_main(hpx::program_options::variables_map& vm)
{

    HpxTrace::init(vm);

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
       desc_commandline;

    HpxTrace::register_command_line_options(desc_commandline);

    // Initialize and run HPX
    //int status =  hpx::init(desc_commandline, argc, argv);

    // Initialize and run HPX
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    int status =  hpx::init(argc, argv, init_args);




    return 0;




}