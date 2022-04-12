#include <map>


class VariablesMap
{
public:
	VariablesMap();
	~VariablesMap();

	template <typename T>
	void put(std::string name, T value){
		if (typeid(T) == typeid(double)){
			numeric_variables[name] = value;
		}
		else if (typeid(T) == typeid(std::string)){
			string_variables[name] = value;
		}
	}

	double get(std::string name){
		
		auto nv_it = numeric_variables.find(name);
		if ( nv_it == numeric_variables.end()){
			return nv_it->second;
		}
		return -1;
		/*auto sv_it = string_variables.find(name);
		if (sv_it == string_variables.end()){
			return sv_it->second;
		}*/
	}
	

private:

    std::map<std::string,double> numeric_variables;
    std::map<std::string,std::string> string_variables;
};