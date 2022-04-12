#include <hpx/hpx.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "../Aggregation.cpp"

class LquantizeResult{

public:
    std::string function;
    int lower_bound, upper_bound, step;
    //needed so it can be used as an action argument
    template<class Archive>
    void serialize(Archive &a, const unsigned version){
        a & function & lower_bound & upper_bound & step;
   }


};

class AggregationsServer
    : public hpx::components::component_base<AggregationsServer>{

    typedef boost::variant<double, std::string> Variant;
    typedef std::vector<Variant> VariantList;

    private:
        std::map<std::string, Aggregation*> aggs;

        std::string find_function(std::string name){
            auto it = aggs.find(name);
            if(it == aggs.end()) return "";
            return it->second->function;
        }

    public:
        //new_x functions return empty string for new aggregations and the function name for existing ones

        std::string new_aggregation(std::string func, std::string name){

            std::string prev_func = find_function(name); 
            if(prev_func != "") return prev_func;

            if(func == "count" || func == "sum" || func == "min" || func == "max"){
                aggs[name] = new ScalarAggregation(func);
            }

            else if(func == "avg"){
                aggs[name] = new AverageAggregation("avg");
            }
            else if(func == "quantize"){
                aggs[name] = new Quantization("quantize");
            }

            return func;
        }

        LquantizeResult new_lquantize(std::string name, int lower_bound, int upper_bound, int step){
            LquantizeResult r;

            std::string f = find_function(name); 
            if(f == ""){
                aggs[name] = new LQuantization("lquantize", lower_bound,upper_bound,step);
            }
            else if(f == "lquantize"){
                LQuantization* lq =  static_cast<LQuantization*>(aggs[name]);
                r.lower_bound = lq->lower_bound;
                r.upper_bound = lq->upper_bound;
                r.step = lq->step;
            }
            r.function = f;
            return r;
        }

        bool aggregate(std::string name, VariantList keys, double value){
            auto it = aggs.find(name);
            if(it == aggs.end()) return false; //throw ("Aggregation " + name + "doesn't exist");
            it->second->aggregate(keys, value);

            return true;
        }


        std::map<std::vector<Variant>, double> get_scalar(std::string name){
        std::map<std::vector<Variant>, double> res;
            if(find_function(name) == "") return res;
            return (static_cast<ScalarAggregation*>(aggs[name]))->values;
        }

        std::map<std::vector<Variant>, std::pair<double,int>> get_average(std::string name){
        std::map<std::vector<Variant>, std::pair<double,int>> res;
            if(find_function(name) == "") return res;
            return (static_cast<AverageAggregation*>(aggs[name]))->values;
        }

        std::map<std::vector<Variant>, std::vector<int>> get_quantization(std::string name){
        std::map<std::vector<Variant>, std::vector<int>> res;
            if(find_function(name) == "") return res;
            return (static_cast<Quantization*>(aggs[name]))->frequencies;
        }

        std::map<std::vector<Variant>, std::vector<int>> get_lquantization(std::string name){
        std::map<std::vector<Variant>, std::vector<int>> res;
            if(find_function(name) == "") return res;
            return (static_cast<LQuantization*>(aggs[name]))->frequencies;
        }

        void print(std::string name){
            if(find_function(name) != ""){
                aggs[name]->print();
            }
        }




        
        HPX_DEFINE_COMPONENT_ACTION(AggregationsServer, new_aggregation);
        HPX_DEFINE_COMPONENT_ACTION(AggregationsServer, new_lquantize);

        HPX_DEFINE_COMPONENT_ACTION(AggregationsServer, aggregate);


        HPX_DEFINE_COMPONENT_ACTION(AggregationsServer, get_scalar);
        HPX_DEFINE_COMPONENT_ACTION(AggregationsServer, get_average);
        HPX_DEFINE_COMPONENT_ACTION(AggregationsServer, get_quantization);
        HPX_DEFINE_COMPONENT_ACTION(AggregationsServer, get_lquantization);

        HPX_DEFINE_COMPONENT_ACTION(AggregationsServer, find_function);
        HPX_DEFINE_COMPONENT_ACTION(AggregationsServer, print);



};

void print_aggregation(hpx::naming::id_type id, std::string name){
    AggregationsServer::print_action()(id, name);
}




void global_print_aggregation(std::vector<hpx::naming::id_type> ids, std::string name){
            std::cout << "global_print_aggregation "<< name << std::endl;

    std::string f = AggregationsServer::find_function_action()(ids[0], name);
    if(f == "avg"){
        AverageAggregation ag(f);
        for(auto id : ids){
            ag.aggregate(AggregationsServer::get_average_action()(id, name));
        }
        ag.print();
    }
    else if(f == "quantize"){
        Quantization ag(f);
        for(auto id : ids){
            ag.aggregate(AggregationsServer::get_quantization_action()(id, name));
        }
        ag.print();
    }
    else if(f == "lquantize"){
        LquantizeResult res;
        res = AggregationsServer::new_lquantize_action()(ids[0], name, 0, 0, 0);

        LQuantization ag(f, res.lower_bound, res.upper_bound, res.step);
        for(auto id : ids){
            ag.aggregate(AggregationsServer::get_lquantization_action()(id, name));
        }
        ag.print();
    }
    else{
        ScalarAggregation ag(f);
        for(auto id : ids){
            ag.aggregate(AggregationsServer::get_scalar_action()(id, name));
        }
        ag.print();
    }
}


void partial_print_aggregation(std::vector<hpx::naming::id_type> ids, std::vector<int> loc_indexes,
                              std::string name){

    std::vector<hpx::naming::id_type> filtered_ids;
    for(int i : loc_indexes){
        filtered_ids.push_back(ids[i]);
    }

    global_print_aggregation(filtered_ids, name);

}