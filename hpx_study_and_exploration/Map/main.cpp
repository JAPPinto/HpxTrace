#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include "MapClient.hpp"

REGISTER_MAP_DECLARATION(string,int);
REGISTER_MAP(string,int);

int hpx_main()
{
    hpx::naming::id_type locality = hpx::find_here();

    MapClient<std::string,int> m = hpx::new_<MapClient<std::string,int>>(locality);
    m.store("a", 26);

    m.get(hpx::launch::async, "a").then([](hpx::future<hpx::util::optional<int>>&& v){
        std::cout << v.get().value()  << std::endl;
    });

    hpx::finalize();
    return 0;
}

int main(int argc, char* argv[]) {

    int status =  hpx::init();
    return 0;
}