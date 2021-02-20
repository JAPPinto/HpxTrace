/*
Simple example thats counts the total number of L1 cache mis by worker threads.

Note: for some reason with PAPI counters it is necessary to call start() otherwise get_value() results in HPX(invalid_status) 
*/

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/iostream.hpp>
#include <hpx/wrap_main.hpp>

int main() {
    const int n = 50000000;
    std::vector<double> v(n);

    //Initialize counter for each thread
    std::size_t const os_threads = hpx::get_os_thread_count();
    std::vector<hpx::performance_counters::performance_counter> counters(os_threads);
    for (int i = 0; i < os_threads; i++) {
        counters[i] = hpx::performance_counters::performance_counter("/papi{locality#0/worker-thread#" + std::to_string(i) + "}/PAPI_L1_TCM");
        counters[i].start();

    }


   hpx::for_loop(hpx::execution::par, 0, n, [&v](auto i) { v[i] = std::sqrt(i);});


    //Read counters
    for (int i = 0; i < os_threads; i++) {
        //auto a = counters[0].get_value<int>().get();
        hpx::cout << "worker-thread#" + std::to_string(i) + ": " << counters[i].get_value<int>().get() << hpx::endl;
    }

    return 0;
}
