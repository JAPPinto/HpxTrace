
#include <hpx/assert.hpp>
#include <hpx/include/components.hpp>

#include "server.hpp"

#include <utility>

//Clients, like servers, need to inherit from a base class, this time, hpx::components::client_base:
class CompClient : public hpx::components::client_base<CompClient, CompServer>{
    //typedef the base class for readability
    typedef hpx::components::client_base<CompClient, CompServer> base_type;

    public:


        //CompClient(int n = 0) : base_type(hpx::local_new<CompServer>(n)) {}
        //CompClient(hpx::id_type locality, int n = 0) : base_type(hpx::new_<CompServer>(locality, n)) {}

        /// Create a client side representation for the existing CompServer instance with the given GID.
        CompClient(hpx::future<hpx::id_type> && id) : base_type(std::move(id)) {
             std::cout << "a" << std::endl;
        }
        CompClient(hpx::id_type && id) : base_type(std::move(id)) {
            std::cout << "b" << std::endl;
        }




        //Non-blocking
        void add(hpx::launch::apply_policy, int n){

            HPX_ASSERT(this->get_id());
            hpx::apply<CompServer::add_action>(this->get_id(), n);
        }

        //Asynchronous
        hpx::future<void> add(hpx::launch::async_policy, int n){

            HPX_ASSERT(this->get_id());
            return hpx::async<CompServer::add_action>(hpx::launch::async, this->get_id(), n);
        }

        //Synchronous
        void add(int n){

            HPX_ASSERT(this->get_id());
            CompServer::add_action()(this->get_id(), n);
        }



        //Asynchronous
        hpx::future<int> get(hpx::launch::async_policy){

            HPX_ASSERT(this->get_id());
            return hpx::async<CompServer::get_action>(hpx::launch::async, this->get_id());
        }

        //Synchronous
        int get(){

            HPX_ASSERT(this->get_id());
            return CompServer::get_action()(this->get_id());
        }

        int get(bool reset){

            HPX_ASSERT(this->get_id());
            return CompServer::get_action()(this->get_id());
        }

};