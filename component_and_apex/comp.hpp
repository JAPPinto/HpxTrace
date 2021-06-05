
#include <hpx/assert.hpp>
#include <hpx/include/components.hpp>

#include "comp_server/comp.hpp"

#include <utility>

//Clients, like servers, need to inherit from a base class, this time, hpx::components::client_base:
class comp : public hpx::components::client_base<comp, server::comp>{
    //typedef the base class for readability
    typedef hpx::components::client_base<comp, server::comp> base_type;

    public:


        /// Default construct an empty client side representation (not connected to any existing component).
        comp() {}

        /// Create a client side representation for the existing server::comp instance with the given GID.
        comp(hpx::future<hpx::id_type> && id) : base_type(std::move(id)) {}
        comp(hpx::id_type && id) : base_type(std::move(id)) {}




        //Non-blocking
        void add(hpx::launch::apply_policy, int n){

            HPX_ASSERT(this->get_id());
            hpx::apply<server::comp::add_action>(this->get_id(), n);
        }

        //Synchronous
        void add(int n){

            HPX_ASSERT(this->get_id());
            server::comp::add_action()(this->get_id(), n);
        }



        //Asynchronous
        hpx::future<int> get(hpx::launch::async_policy){

            HPX_ASSERT(this->get_id());
            return hpx::async<server::comp::get_action>(hpx::launch::async, this->get_id());
        }

        //Synchronous
        int get(){

            HPX_ASSERT(this->get_id());
            return server::comp::get_action()(this->get_id());
        }

        int get(bool reset){

            HPX_ASSERT(this->get_id());
            return server::comp::get_action()(this->get_id());
        }

};