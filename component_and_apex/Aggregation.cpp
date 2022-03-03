#include <cmath>

class Aggregation
{
    typedef boost::variant<double, std::string> Variant;
    public:   
        std::string name;
        std::string function;

        Aggregation(std::string n, std::string f) : name(n) , function(f){}

        virtual void aggregate(std::vector<Variant> keys, double d){}

        virtual void print(){}
};

class ScalarAggregation : public Aggregation
{
    typedef boost::variant<double, std::string> Variant;

    public:
        std::map<std::vector<Variant>, double> values;

        ScalarAggregation(std::string n, std::string f) : Aggregation(n,f){}


        void aggregate(std::vector<Variant> keys, double d){

                if(function == "count"){
                    values[keys]++;
                }
                else if(function == "sum"){
                    values[keys] += d;
                }
                else if(function == "min"){
                    values[keys] = std::min(values[keys], d);
                }
                else if(function == "max"){
                    values[keys] = std::max(values[keys], d);
                }
            }

        void print(){

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
};

class AverageAggregation : public Aggregation
{
    typedef boost::variant<double, std::string> Variant;

    public:
        //<average,count>
        std::map<std::vector<Variant>, std::pair<double,int>> values;

        AverageAggregation(std::string n, std::string f) : Aggregation(n,f){}


        void aggregate(std::vector<Variant> keys, double d){

            //average = average + ((value - average) / nValues)
            auto p =  values[keys];
            double avg = p.first;
            int count = p.second + 1;
            avg = avg  + ((d - avg)/count);
            values[keys] = std::make_pair(avg, count);
        }

        void print(){

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

        Quantization(std::string n, std::string f) : Aggregation(n,f){}

        void aggregate(std::vector<Variant> keys, double d){

             auto it = frequencies.find(keys);
                    if ( it == frequencies.end()){
                        std::vector<int> v(32,0);
                        v[std::floor(std::log2(d))]++;
                        frequencies[keys] = v;
                    }
                    else{
                        frequencies[keys][std::floor(std::log2(d))]++;
                    }

        } 

        void print(){

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

                
                for (i = 0; i < 32 && !dist[i]; i++);
                for (j = 31; j >= i && !dist[j]; j--);
                for (; i <= j; i++)
                {
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


        LQuantization(std::string n, std::string f, double lb, double up, double s):
            Aggregation(n,f), lower_bound(lb), upper_bound(up), step(s){} 

        

        void aggregate(std::vector<Variant> keys, double d)
        {

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


        void print(){

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
                for (; i <= j; i++)
                {
                    std::cout << lower_bound + i*step <<  " " << v.second[i] << std::endl;   
                }
            }
        }
};