#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/program_options.hpp>
#include <unistd.h>



#include <apex_api.hpp>
#include <apex_types.h>
#include <profiler.hpp>
#include <task_identifier.hpp>
#include <task_wrapper.hpp>

#include <hpx/modules/debugging.hpp>


using hpx::performance_counters::performance_counter;


//[fib_action
// forward declaration of the Fibonacci function
std::uint64_t test(std::uint64_t n);

// This is to generate the required boilerplate we need for the remote
// invocation to work.
HPX_PLAIN_ACTION(test, test_action);
//]

///////////////////////////////////////////////////////////////////////////////
//[fib_func
std::uint64_t test(std::uint64_t n)
{
std::cout << "test " << n << std::endl;
    hpx::naming::id_type const locality_id = hpx::find_here();

    if (n == 0)
        return 0;
    test_action fib;
    hpx::future<std::uint64_t> n1 =
        hpx::async(fib, locality_id, n - 1);
    return n1.get();

}

// 3
// 2 
//fib(3) fib(2) fib(1) fib(0)







std::atomic<std::uint64_t> count(0);

class event_data {
public:
  apex_event_type event_type_;
  int thread_id;
  void * data; /* generic data pointer */
  event_data() : thread_id(0), data(nullptr) {};
  virtual ~event_data() {};
};
class timer_event_data : public event_data {
public:
  apex::task_identifier * task_id;
  std::shared_ptr<apex::profiler> my_profiler;
  timer_event_data(apex::task_identifier * id);
  timer_event_data(std::shared_ptr<apex::profile> &the_profiler);
  ~timer_event_data();
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

int hpx_main(hpx::program_options::variables_map& vm)
{
//APEX_STOP_EVENT,APEX_RESUME_EVENT,APEX_YIELD_EVENT

    //When counter is read print information
    apex::register_policy({APEX_START_EVENT, APEX_RESUME_EVENT, APEX_STOP_EVENT, APEX_YIELD_EVENT},
        [](apex_context const& context)->int{
            std::shared_ptr<apex::task_wrapper>* pp = reinterpret_cast<std::shared_ptr<apex::task_wrapper>*>(context.data);
            std::shared_ptr<apex::task_wrapper> tw = *pp; 

            std::string name = tw->task_id->get_name();

            if(name != "test_action") return 0;


            if(context.event_type == APEX_START_EVENT)
                std::cout << "APEX_START_EVENT " << name << " " << tw->guid <<std::endl;
            else if(context.event_type == APEX_STOP_EVENT){

                std::cout << "APEX_STOP_EVENT " << name << " " << tw->guid <<std::endl;
                //apex_profile * prof = apex::get_profile(name);
                //std::cout << "calls " << prof->calls << std::endl;
                

            }
            else if(context.event_type == APEX_RESUME_EVENT)
                std::cout << "APEX_RESUME_EVENT " << name << " " << tw->guid <<std::endl;
            else if(context.event_type == APEX_YIELD_EVENT){
                std::cout << "APEX_YIELD_EVENT " << name << " " << tw->guid <<std::endl;
                //apex_profile * prof = apex::get_profile(name);
                //std::cout << "calls " << prof->calls << std::endl;
            }
            else
                 std::cout << "??? " << name << " " << tw->guid <<std::endl;

            //std::cout << hpx::util::backtrace(10).trace() << '\n';



            return 0;
    });


    //auto ap =  apex::apex::instance();


    int n = vm["n"].as<int>();

      // Wait for fib() to return the value
        test_action fib;
        std::uint64_t r = fib(hpx::find_here(), n);

    //workload
   // for (int i = 0; i < 1; ++i){
    //    std::uint64_t r = fib(hpx::find_here(), 5);
    //}



    hpx::finalize();
    return 0;
}



int main(int argc, char* argv[]) {


    // Configure application-specific options
    hpx::program_options::options_description
       desc_commandline("Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()
        ( "n",
          hpx::program_options::value<int>()->default_value(0),
          "script for tracing")
        ;


    // Initialize and run HPX
    int status =  hpx::init(desc_commandline, argc, argv);


    return 0;
}