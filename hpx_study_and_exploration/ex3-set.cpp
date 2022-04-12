/*
Simple example thats counts the total number of user threads executed by worker threads.
Now using performance_counter_set
*/

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/iostream.hpp>
#include <hpx/wrap_main.hpp>
#include <hpx/include/performance_counters.hpp>

int main() {
    const int n = 10000000;
    std::vector<double> v(n);


    std::string name = "/threads{locality#0/worker-thread#*/total}/count/cumulative";
    hpx::performance_counters::performance_counter_set counters(name);

    //Doesn't work, why?
    //hpx::performance_counters::performance_counter_set counters("/threads{locality#0/worker-thread#*/total}/count/cumulative");
   /* 


    hpx::for_loop(hpx::execution::par, 0, n, [&v](auto i) { v[i] = std::sqrt(i);});


    std::vector<int> values = counters.get_values<int>().get();

    for (int i = 0; i < values.size(); i++) {
        hpx::cout << "worker-thread#" + std::to_string(i) + ": " << values[i] << hpx::endl;
    }*/

    return 0;
}

