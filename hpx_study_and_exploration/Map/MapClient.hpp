#include <hpx/assert.hpp>
#include <hpx/include/components.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/optional.hpp>

#include "MapServer.hpp"

#include <utility>

template <typename K, typename V>
class MapClient : public hpx::components::client_base<MapClient<K,V>, MapServer<K,V>>{

    typedef hpx::components::client_base<MapClient<K,V>, MapServer<K,V>> base_type;
    typedef typename MapServer<K,V>::key_type key_type;
    typedef typename MapServer<K,V>::value_type value_type;
    
    public:

        MapClient() {}
        MapClient(hpx::future<hpx::id_type> && id) : base_type(std::move(id)) {}
        MapClient(hpx::id_type && id) : base_type(std::move(id)) {}


        //Synchronous
        void store(key_type key, value_type value){
            HPX_ASSERT(this->get_id());
            typedef typename MapServer<K,V>::store_action action_type;
            action_type()(this->get_id(), key, value);
        }

        //Asynchronous
        hpx::future<void> store(hpx::launch::async_policy, key_type key, value_type value){
            HPX_ASSERT(this->get_id());
            typedef typename MapServer<K,V>::store_action action_type;
            return hpx::async<action_type>(hpx::launch::async, this->get_id(), key, value);
        }

        //Non-blocking
        hpx::future<void> store(hpx::launch::apply_policy, key_type key, value_type value){
            HPX_ASSERT(this->get_id());
            typedef typename MapServer<K,V>::store_action action_type;
            return hpx::apply<action_type>(this->get_id(), key, value);
        }

        //Synchronous
        hpx::util::optional<value_type> get(key_type key){
            HPX_ASSERT(this->get_id());
            typedef typename MapServer<K,V>::get_action action_type;
            return action_type()(this->get_id(), key);
        }

        //Asynchronous
        hpx::future<hpx::util::optional<value_type>> get(hpx::launch::async_policy, key_type key){
            HPX_ASSERT(this->get_id());
            typedef typename MapServer<K,V>::get_action action_type;
            return hpx::async<action_type>(hpx::launch::async, this->get_id(), key);
        }
};