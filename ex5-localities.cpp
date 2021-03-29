/*

*/

#include <hpx/iostream.hpp>
#include <hpx/hpx_init.hpp>
#include "component/test.hpp"




int hpx_main()
{

    std::cout << hpx::get_num_localities().get() << " localities\n";
    std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();

    for(auto l : localities)
        std::cout << " locality " << l.get_gid().to_string() << std::endl;


    std::cout << hpx::get_num_worker_threads() << " worker threads\n";

    std::cout << hpx::get_os_thread_count()  << " os threads\n";

    return hpx::finalize();
}


int main(int argc, char* argv[]) {


    // Initialize and run HPX.
    return hpx::init(argc, argv);
}
