#include <cmath>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>


class Aggregation
{
    typedef boost::variant<double, std::string> Variant;
    protected:
        std::mutex mtx;
    public:   
        std::string function;

        Aggregation(std::string f) : function(f){}

        virtual void aggregate(std::vector<Variant> keys, double d){}

        virtual void print(){}

    friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive &a, const unsigned version){
            a & function;
        }
};

class ScalarAggregation : public Aggregation
{
    typedef boost::variant<double, std::string> Variant;

    public:
        std::map<std::vector<Variant>, double> values;

        ScalarAggregation(std::string f) : Aggregation(f){}


        void aggregate(std::vector<Variant> keys, double value){

            std::scoped_lock lock(mtx);

            if(function == "count"){
                values[keys]++;
            }
            else if(function == "sum"){
                values[keys] += value;
            }
            else if(function == "min"){
                values[keys] = std::min(values[keys], value);
            }
            else if(function == "max"){
                values[keys] = std::max(values[keys], value);
            }
        }

        void aggregate(std::map<std::vector<Variant>, double> aggregation){
            std::scoped_lock lock(mtx);

            if(function == "count" || function == "sum"){
                for (auto const& [keys, value] : aggregation){
                    values[keys] += value;
                }
            }
            else if(function == "min"){
                for (auto const& [keys, value] : aggregation){
                    values[keys] = std::min(values[keys], value);
                }
            }
            else if(function == "max"){
                for (auto const& [keys, value] : aggregation){
                    values[keys] = std::max(values[keys], value);
                }
            }
        }

        void print(){
            std::scoped_lock lock(mtx);

            std::cout << function << std::endl;

            for (std::pair<std::vector<Variant>, double> v : values){
                //arg.first -> keys
                //arg.second -> value
                for (Variant k : v.first){
                    std::cout << k  << " ";
                }
                std::cout << ": " << v.second << std::endl;
            }
        }

    friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive &a, const unsigned version){
            a & boost::serialization::base_object<Aggregation>(*this);
            a & values;
        }
};

class AverageAggregation : public Aggregation
{
    typedef boost::variant<double, std::string> Variant;

    public:
        //<average,count>
        std::map<std::vector<Variant>, std::pair<double,int>> values;

        AverageAggregation(std::string f) : Aggregation(f){}


        void aggregate(std::vector<Variant> keys, double d){

            std::scoped_lock lock(mtx);

            //average = average + ((value - average) / nValues)
            auto p =  values[keys];
            double avg = p.first;
            int count = p.second + 1;
            avg = avg  + ((d - avg)/count);
            values[keys] = std::make_pair(avg, count);
        }

        void aggregate(std::map<std::vector<Variant>, std::pair<double,int>> aggregation){
            std::scoped_lock lock(mtx);

            for (auto const& [keys, val] : aggregation){
                double avgA = values[keys].first;
                double avgB = val.first;
                int countA = values[keys].second;
                double countB = val.second;
                int new_count = countA + countB;
                double new_avg = (avgA * countA + avgB + countB) / new_count;
                values[keys] = std::make_pair(new_avg, new_count);
            }

        }

        void print(){
            std::scoped_lock lock(mtx);

            std::cout << function << std::endl;

            for (auto v : values){
                //arg.first -> keys
                //arg.second -> value
                for (Variant k : v.first){
                    std::cout << k  << " ";
                }
                //value -> <average,count>
                std::cout << ": " << v.second.first << std::endl;
            }
        }
};

class Quantization : public Aggregation
{
    typedef boost::variant<double, std::string> Variant;
    public:
        std::map<std::vector<Variant>, std::vector<int>> frequencies;

        Quantization(std::string f) : Aggregation(f){}

        void aggregate(std::vector<Variant> keys, double d){

            std::scoped_lock lock(mtx);

            auto it = frequencies.find(keys);
            if(it == frequencies.end()){
                std::vector<int> v(32,0);
                v[std::floor(std::log2(d))]++;
                frequencies[keys] = v;
            }
            else{
                frequencies[keys][std::floor(std::log2(d))]++;
            }

        } 

        void aggregate(std::map<std::vector<Variant>, std::vector<int>> aggregation){
            
            std::scoped_lock lock(mtx);

            for (auto const& [keys, val] : aggregation){
                auto it = frequencies.find(keys);
                if(it == frequencies.end()){
                    frequencies[keys] = val;
                }
                else{
                    for (int i = 0; i < 32; i++){
                        frequencies[keys][i] += val[i];
                    }
                }
            }
        }


        void print(){
            std::scoped_lock lock(mtx);

            std::cout << function << std::endl;

            for (auto v : frequencies){
                //print keys
                for (Variant k : v.first){
                    std::cout << k  << " ";
                }
                std::cout << ": " << std::endl;
                //print distribution
                int i, j;
                std::vector<int>& dist = v.second;
                //there will always be one element bigger than 0

                //remove initial zeroes
                for (i = 0; i < 32 && !dist[i]; i++);
                //remove final zeroes
                for (j = 31; j >= i && !dist[j]; j--);
                for (; i <= j; i++){
                    std::cout << std::pow(2,i) <<  " " << v.second[i] << std::endl;
                    
                }
            }
        }
};

class LQuantization : public Aggregation
{
    typedef boost::variant<double, std::string> Variant;
    public:
        std::map<std::vector<Variant>, std::vector<int>> frequencies;
        double lower_bound, upper_bound, step;


        LQuantization(std::string f, double lb, double up, double s):
            Aggregation(f), lower_bound(lb), upper_bound(up), step(s){} 

        

        void aggregate(std::vector<Variant> keys, double d)
        {
            std::scoped_lock lock(mtx);

            auto it = frequencies.find(keys);
            if ( it == frequencies.end()){

                int size = std::floor((upper_bound - lower_bound) / step) + 1;
                std::vector<int> v(size,0);
                v[std::floor((d -lower_bound)/step)]++;

                frequencies[keys] = v;
            }
            else{
                frequencies[keys][std::floor((d - lower_bound)/step)]++;
            }

        }

        void aggregate(std::map<std::vector<Variant>, std::vector<int>> aggregation){
            
            int size = std::floor((upper_bound - lower_bound) / step) + 1;

            std::scoped_lock lock(mtx);

            for (auto const& [keys, val] : aggregation){
                auto it = frequencies.find(keys);
                if(it == frequencies.end()){
                    frequencies[keys] = val;
                }
                else{
                    for (int i = 0; i < size-1; i++){
                        frequencies[keys][i] += val[i];
                    }
                }
            }
        }


        void print(){
            std::scoped_lock lock(mtx);
            
            std::cout << function << std::endl;

            for (auto v : frequencies){
                //print keys
                for (Variant k : v.first){
                    std::cout << k  << " ";
                }
                std::cout << ": " << std::endl;
                //print distribution
                int i, j;
                std::vector<int>& dist = v.second;
                //there will always be one element bigger than 0


                int size = std::floor((upper_bound - lower_bound) / step) + 1;
                std::cout << size << " " << upper_bound << " " << lower_bound << " " << step << "\n";

                //for (i = 0; i < size && !dist[i]; i++);
                //for (j = size-1; j >= i && !dist[j]; j--);
                i = 0;
                j = size-1;
                for (; i <= j; i++){
                    std::cout << lower_bound + i*step <<  " " << v.second[i] << std::endl;   
                }
            }
        }
};