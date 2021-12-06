/*
Simple example thats counts the total number of user threads executed by worker threads.
*/

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/iostream.hpp>
#include <hpx/wrap_main.hpp>

int main() {
    const int n = 10000000;
    std::vector<double> v(n);
/* 
    //Initialize counter for each thread
    std::size_t const os_threads = hpx::get_os_thread_count();
    std::vector<hpx::performance_counters::performance_counter> counters(os_threads);
    for (int i = 0; i < os_threads; i++) {
        counters[i] = hpx::performance_counters::performance_counter("/threads{locality#0/worker-thread#" + std::to_string(i) + "/total}/count/cumulative");
    }


   hpx::for_loop(hpx::execution::par, 0, n, [&v](auto i) { v[i] = std::sqrt(i);});


    //Read counters
    for (int i = 0; i < os_threads; i++) {
        hpx::cout << "worker-thread#" + std::to_string(i) + ": " << counters[i].get_value<int>().get() << hpx::endl;
    }
*/ 
    return 0;
}
