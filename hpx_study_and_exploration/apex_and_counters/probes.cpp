#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>


#include <apex_api.hpp>




using hpx::performance_counters::performance_counter;


std::atomic<std::uint64_t> count(0);

class event_data {
public:
  apex_event_type event_type_;
  int thread_id;
  void * data; /* generic data pointer */
  event_data() : thread_id(0), data(nullptr) {};
  virtual ~event_data() {};
};
//Represents counter reading
class sample_value_event_data : public event_data {
public:
  std::string * counter_name;
  double counter_value;
  bool is_threaded;
  bool is_counter;
  sample_value_event_data(int thread_id, std::string counter_name, double counter_value, bool threaded);
  ~sample_value_event_data();
};

int hpx_main()
{


    //When counter is read print information
    apex::register_policy(APEX_SAMPLE_VALUE,
        [](apex_context const& context)->int{
            sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);
            //filter non HPX counter readings
            if((*dt.counter_name)[0] == '/'){
                std::cout << "APEX_SAMPLE_VALUE 1 " << *(dt.counter_name) << " " << dt.counter_value << std::endl;
            }

            return 0;
    });

    //Test if multiple policies can apply to a single counter query
    apex::register_policy(APEX_SAMPLE_VALUE,
        [](apex_context const& context)->int{
            sample_value_event_data& dt = *reinterpret_cast<sample_value_event_data*>(context.data);

            if((*dt.counter_name)[0] == '/'){
                std::cout << "APEX_SAMPLE_VALUE 2 " << *(dt.counter_name) << " " << dt.counter_value << std::endl;
            }
            return 0;
    });


    std::string counter_name = "/threads{locality#0/total}/count/cumulative";
    hpx::performance_counters::performance_counter counter(counter_name);


    //read counter each 0.5s seconds
    hpx::util::interval_timer it([&counter, &counter_name]()->bool{
        int value = counter.get_value<int>().get();
        std::cout << "READ COUNTER " << counter_name << " " << value << std::endl;

        //reading the counter from the API does not tigger APEX_SAMPLE_VALUE event to it has to be triggered manually
        apex::sample_value(counter_name, value);
        return true;
            
    }, 500000); //0.5s
    
    it.start();


    //workload
    for (int i = 0; i < 1000000000000; ++i);



    hpx::finalize();
    return 0;
}



int main(int argc, char* argv[]) {

    // Initialize and run HPX
    int status =  hpx::init(argc, argv);

    return 0;
}