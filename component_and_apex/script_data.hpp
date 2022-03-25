
#include <hpx/assert.hpp>
#include <hpx/include/components.hpp>

#include "script_data_server/script_data.hpp"

#include <utility>

//Clients, like servers, need to inherit from a base class, this time, hpx::components::client_base:
class script_data : public hpx::components::client_base<script_data, server::script_data>{
    //typedef the base class for readability
    typedef hpx::components::client_base<script_data, server::script_data> base_type;

    public:


        /// Default construct an empty client side representation (not connected to any existing component).
        script_data() {}

        /// Create a client side representation for the existing server::script_data instance with the given GID.
        script_data(hpx::future<hpx::id_type> && id) : base_type(std::move(id)) {}
        script_data(hpx::id_type && id) : base_type(std::move(id)) {}


        //Synchronous
        void store_double(std::string name, double d){

            HPX_ASSERT(this->get_id());
            server::script_data::store_double_action()(this->get_id(), name, d);
        }

        //Synchronous
        double get_double(std::string name){

            HPX_ASSERT(this->get_id());
            return server::script_data::get_double_action()(this->get_id(), name);
        }

        //Synchronous
        bool is_double(std::string name){

            HPX_ASSERT(this->get_id());
            return server::script_data::is_double_action()(this->get_id(), name);
        }
        
        //Synchronous
        void store_string(std::string name, std::string s){

            HPX_ASSERT(this->get_id());
            server::script_data::store_string_action()(this->get_id(), name, s);
        }

        //Synchronous
        std::string get_string(std::string name){

            HPX_ASSERT(this->get_id());
            return server::script_data::get_string_action()(this->get_id(), name);
        }

        //Synchronous
        bool is_string(std::string name){

            HPX_ASSERT(this->get_id());
            return server::script_data::is_string_action()(this->get_id(), name);
        }

};