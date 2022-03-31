#include <hpx/assert.hpp>
#include <hpx/include/components.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/optional.hpp>

#include "VarsCompServer.hpp"

#include <utility>

//Clients, like servers, need to inherit from a base class, this time, hpx::components::client_base:
template <typename T>
class VarsCompClient : public hpx::components::client_base<VarsCompClient<T>, VarsCompServer<T>>{


    //typedef the base class for readability
    typedef hpx::components::client_base<VarsCompClient<T>, VarsCompServer<T>> base_type;
    typedef typename VarsCompServer<T>::argument_type argument_type;
    
    public:


        /// Default construct an empty client side representation (not connected to any existing component).
        VarsCompClient() {}

        //VarsCompClient(hpx::id_type const& id) : base_type(id){}

        /// Create a client side representation for the existing VarsCompServer instance with the given GID.
        VarsCompClient(hpx::future<hpx::id_type> && id) : base_type(std::move(id)) {}
        VarsCompClient(hpx::id_type && id) : base_type(std::move(id)) {}


        //Synchronous
        void store_var(std::string name, argument_type value){




            HPX_ASSERT(this->get_id());
            VarsCompServer<T>::store_var_action()(this->get_id(), name, value);
        }

        //Synchronous
        hpx::util::optional<argument_type> get_var(std::string name){

            HPX_ASSERT(this->get_id());
            return VarsCompServer<T>::get_var_action()(this->get_id(), name);
        }

        //Synchronous
        bool exists(std::string name){

            HPX_ASSERT(this->get_id());
            return VarsCompServer<T>::exists_action()(this->get_id(), name);
        }
    

};