#include <cmath>

class Aggregation
{
    typedef boost::variant<double, std::string> Variant;
	public:   
		std::string name;
		std::string type;
	    std::map<std::vector<Variant>, double> values;
	    int avg_count; 

	    //Aggregation() : type(""){}

	    virtual void aggregate(std::vector<Variant> keys, std::string function, double d){

		    if(type == ""){
	            type = function;
	        }
	        else if(type != function){
	            throw  "aggregation " + name + " is of type " + type;
	        }
	        if(type == "count"){
	        	values[keys]++;
	        }
	        else if(type == "sum"){
	        	values[keys] += d;
	        }
	        else if(type == "avg"){
	        	//average = average + ((value - average) / nValues)
	        	int avg = values[keys];
	        	avg_count++;
	        	values[keys] = avg  + ((d - avg)/avg_count);
	        }
	        else if(type == "min"){
	        	values[keys] = std::min(values[keys], d);
	        }
	        else if(type == "max"){
	        	values[keys] = std::max(values[keys], d);
	        }
	    }

	    virtual void print(){

            std::cout << type << std::endl;

    	    for (std::pair<std::vector<Variant>, double> v : values){
                //arg.first -> name
                //arg.second -> value
                for (Variant k : v.first){
                	std::cout << k  << " ";
                }
                std::cout << ": " << v.second << std::endl;
               // agg.second.print();


            }
	    }

	//virtual void aggregate(double d);
	//void print();

	
};

class Quantization : public Aggregation
{
    typedef boost::variant<double, std::string> Variant;
	public:
	    //std::map<std::vector<Variant>, std::vector<std::pair<double,int>>> values;
	    std::map<std::vector<Variant>, std::map<double,int>> values;
	    std::map<std::vector<Variant>, std::vector<int>> frequencies;


	    void aggregate(std::vector<Variant> keys, std::string function, double d){

		    if(type == ""){
	            type = function;
	        }
	        else if(type != function){
	            throw  "aggregation " + name + " is of type " + type;
	        }



			 auto it = frequencies.find(keys);
			        if ( it == frequencies.end()){
			            std::vector<int> v(32,0);
			            v[std::floor(std::log2(d))]++;
			            frequencies[keys] = v;
			        }
			        else{
			            frequencies[keys][std::floor(std::log2(d))]++;
			        }

			values[keys][d]++;	       
	    }


	    void print(){

            std::cout << type << std::endl;

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
