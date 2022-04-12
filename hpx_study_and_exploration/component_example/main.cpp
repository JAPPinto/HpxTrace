#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>

#include "client.hpp"



void new_server_example(){

    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    hpx::id_type id = hpx::new_<CompServer>(localities[0], 5).get();

    CompServer::add_action()(id, 1);

    std::cout << CompServer::get_action()(id) << std::endl;
}

void new_client_example(){

    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    CompClient c = hpx::new_<CompClient>(localities[0], 5);

    c.add(2);
    std::cout << c.get() << std::endl;
    
}


void multiple_clients(){
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    //hpx::id_type server_id = hpx::new_<CompServer>(localities[0], 5).get();

    CompClient c1 = CompClient(hpx::new_<CompServer>(localities[0], 5));
    //CompClient c2 = CompClient(server_id);

}


int hpx_main()
{

    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    new_client_example();
    
    //CompClient c = hpx::new_<CompServer>(localities[0],2);
    //CompClient c = CompClient(10);
    //CompClient c = CompClient(localities[0],20);
    //CompClient c = hpx::new_<CompClient>(localities[0], 5);


   // c.add(1);
    //std::cout << c.get() << std::endl;

    hpx::finalize();
    return 0;
}


int main(int argc, char* argv[]) {

    hpx::init(argc, argv);

    return 0;

}