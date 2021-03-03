
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>

#include <hpx/modules/program_options.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>


int hpx_main(hpx::program_options::variables_map& vm)
{


    using hpx::performance_counters::performance_counter;

    performance_counter example_counter(hpx::util::format(
        "/example{{locality#{}/total}}/immediate/implicit",
        hpx::get_locality_id()));

    example_counter.start();

    std::cout <<  example_counter.get_value<double>().get() << std::endl;

    hpx::this_thread::suspend(std::chrono::milliseconds(500));

    std::cout <<  example_counter.get_value<double>().get() << std::endl;



    // Initiate shutdown of the runtime system.
    hpx::finalize();
    return 0;
}

int main(int argc, char* argv[])
{


    // Initialize and run HPX.
    return hpx::init();
}
