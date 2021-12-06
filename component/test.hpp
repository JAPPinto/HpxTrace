    
#include <hpx/assert.hpp>
#include <hpx/include/components.hpp>

#include "server/test.hpp"

#include <utility>

//Clients, like servers, need to inherit from a base class, this time, hpx::components::client_base:
class test : public hpx::components::client_base<test, server::test>{
    //typedef the base class for readability
    typedef hpx::components::client_base<test, server::test> base_type;

    public:


        /// Default construct an empty client side representation (not connected to any existing component).
        test() {}

        /// Create a client side representation for the existing server::test instance with the given GID.
        test(hpx::future<hpx::id_type> && id) : base_type(std::move(id)) {}
        test(hpx::id_type && id) : base_type(std::move(id)) {}



        //this->get_id() references a data member of the hpx::components::client_base base class which identifies the server accumulator instance

        //Non-blocking
        void store(hpx::launch::apply_policy, std::string content){

            HPX_ASSERT(this->get_id());
            hpx::apply<server::test::store_action>(this->get_id(), content);
        }

        //Synchronous
        void store(std::string content){

            HPX_ASSERT(this->get_id());
            server::test::store_action()(this->get_id(), content);
        }



        //Asynchronous
        hpx::future<std::string> get(hpx::launch::async_policy){

            HPX_ASSERT(this->get_id());
            return hpx::async<server::test::get_action>(hpx::launch::async, this->get_id());
        }

        //Synchronous
        std::string get(){

            HPX_ASSERT(this->get_id());
            return server::test::get_action()(this->get_id());
        }

};