
#include <hpx/assert.hpp>
#include <hpx/include/components.hpp>

#include "server/test_component.hpp"

#include <utility>

//Clients, like servers, need to inherit from a base class, this time, hpx::components::client_base:
class testComponent : public hpx::components::client_base<testComponent, server::testComponent>{
    //typedef the base class for readability
    typedef hpx::components::client_base<testComponent, server::testComponent> base_type;

    public:

        //this->get_id() references a data member of the hpx::components::client_base base class which identifies the server accumulator instance

        //Non-blocking
        void store(hpx::launch::apply_policy, std::string content){

            HPX_ASSERT(this->get_id());
            hpx::apply<server::testComponent::store_action>(this->get_id(), content);
        }

        //Synchronous
        void store(std::string content){

            HPX_ASSERT(this->get_id());
            server::testComponent::store_action()(this->get_id(), content);
        }



        //Asynchronous
        std::string get(hpx::launch::async_policy){

            HPX_ASSERT(this->get_id());
            hpx::async<server::testComponent::store_action>(hpx::launch::async, this->get_id());
        }

        //Synchronous
        std::string add(){

            HPX_ASSERT(this->get_id());
            server::testComponent::store_action()(this->get_id());
        }

};