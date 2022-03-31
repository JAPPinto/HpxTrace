#include <mutex>

class Mutexes
{
    typedef boost::variant<double, std::string> Variant;

private:
	std::map<std::vector<Variant>,std::mutex> mutexes; 
	std::mutex mtx;
public:

	void lock(std::vector<Variant> key){
	std::cout << "lock a" <<  std::endl;

		//mtx.lock();
		mutexes[key].lock();
		//mtx.unlock();	
	//std::cout << "lock b"<<  std::endl;

	}

	void unlock(std::vector<Variant> key){
	std::cout << "unlock a"<<  std::endl;

		//mtx.lock();
		mutexes[key].unlock();
		//mtx.unlock();	
	//std::cout << "unlock b"<<  std::endl;

	}
	
};