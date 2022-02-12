class Aggregation
{
    typedef boost::variant<double, std::string> Variant;
	public:   
		std::string name;
		std::string type;
	    std::map<std::vector<Variant>, double> values;
	    int avg_count; 

	    //Aggregation() : type(""){}

	    void aggregate(std::vector<Variant> keys, std::string function, double d){
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

	    void print(){

            std::cout << type << std::endl;
    	    for (auto v : values){
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