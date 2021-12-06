/*
Implementation of basic counters that monitor the number of heads and tails flipped.

To test run with the following options:
./ex4-simple_counter --hpx:print-counter=/test{locality#0/total}/heads --hpx:print-counter=/test{locality#0/total}/tails --hpx:print-counter-interval=1000


*/

#include <hpx/iostream.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/hpx_init.hpp>


std::int64_t heads(0);
std::int64_t tails(0);

//Function that's called each time the counter "/test/heads" is read
std::int64_t heads_counter(bool reset){
    return heads;
}
//Function that's called each time the counter "/test/tails" is read

std::int64_t tails_counter(bool reset){
    return heads;
}

std::vector<std::int64_t> heads_tails_counter(bool reset){
    std::vector<std::int64_t> result;
    result.push_back(heads);
    result.push_back(tails);
    return result;
}


void register_counter_type() {
    // Call the HPX API function to register the counter type.
    hpx::performance_counters::install_counter_type(
        "/test/heads",                                  // counter type name
        &heads_counter,                                 // function providing counter data
        "returns the number of heads"                   // description text (optional)
        "heads"                                         // unit of measure (optional)
    );
    // Call the HPX API function to register the counter type.
    hpx::performance_counters::install_counter_type(
        "/test/tails",                                  // counter type name
        &tails_counter,                                 // function providing counter data
        "returns the number of tails"                   // description text (optional)
        "tails"                                         // unit of measure (optional)
    );

    // Call the HPX API function to register the counter type.
    hpx::performance_counters::install_counter_type(
        "/example/heads-tails",                          // counter type name
        &heads_tails_counter,                           // function providing counter data
        "returns the number of heads and tails"         // description text (optional)
        "heads:tails"                                 // unit of measure (optional)
    );
}


int hpx_main()
{
    for (int i = 0; i < 300000000; ++i) {
        if ((std::rand() % 2) == 0)
            heads++;
        else
            tails++;
    }

    return hpx::finalize();
}


//HPX needs to be manually initialized so the counters can be registered first
int main(int argc, char* argv[]) {

    hpx::register_startup_function(&register_counter_type);

    // Initialize and run HPX.
    return hpx::init(argc, argv);
}
