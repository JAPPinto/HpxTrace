
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>

#include <hpx/modules/program_options.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>


void implicit_counter(){

    using hpx::performance_counters::performance_counter;

    performance_counter implicit_counter(hpx::util::format(
        "/example{{locality#{}/total}}/immediate/implicit",
        hpx::get_locality_id()));

    implicit_counter.start();

    std::cout <<  implicit_counter.get_value<double>().get() << std::endl;

    hpx::this_thread::suspend(std::chrono::milliseconds(500));

    std::cout <<  implicit_counter.get_value<double>().get() << std::endl;

}

void explicit_counter(){

    using hpx::performance_counters::performance_counter;

    performance_counter explicit_counter(hpx::util::format(
        "/example{{locality#{}/instance#{}}}/immediate/explicit",
        hpx::get_locality_id(), 0));


    explicit_counter.start();

    std::cout <<  explicit_counter.get_value<double>().get() << std::endl;

    hpx::this_thread::suspend(std::chrono::milliseconds(500));

    std::cout <<  explicit_counter.get_value<double>().get() << std::endl;

}


int hpx_main(hpx::program_options::variables_map& vm)
{



    // Initiate shutdown of the runtime system.
    explicit_counter();
    hpx::finalize();
    return 0;
}

int main(int argc, char* argv[])
{


    // Initialize and run HPX.
    return hpx::init();
}
