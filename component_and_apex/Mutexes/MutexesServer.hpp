#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>


class MutexesServer
	: public hpx::components::locking_hook< hpx::components::component_base<MutexesServer>>{

    typedef boost::variant<double, std::string> Variant;

	private:
		std::map<std::vector<Variant>,std::mutex> mutexes; 
		std::mutex mtx;
	public:

		void lock(std::vector<Variant> key){
		std::cout << "lock " << key[0] <<  std::endl;
			mutexes[key].lock();
		}

		void unlock(std::vector<Variant> key){
		std::cout << "unlock " << key[0]<<  std::endl;
			mutexes[key].unlock();
		} 
			
		HPX_DEFINE_COMPONENT_ACTION(MutexesServer, lock);
		HPX_DEFINE_COMPONENT_ACTION(MutexesServer, unlock);

};
