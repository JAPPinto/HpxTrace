#ifndef _SCALARVARS_H
#define _SCALARVARS_H

class ScalarVars{

    private:
        std::map<std::string,double> dvars;
        std::map<std::string, std::string> stvars;

        bool string_exists(std::string name){
            auto it = stvars.find(name);
            if ( it != stvars.end()){
                return true;
            }
            return false;
        }

        bool double_exists(std::string name){
            auto it = dvars.find(name);
            if ( it != dvars.end()){
                return true;
            }
            return false;
        }
    public:


        bool store_double(std::string name, double d){
            if(string_exists(name)) return false;
            dvars[name] = d;
            return true;
        }
        bool store_string(std::string name, std::string s){
            if(double_exists(name)) return false;
            stvars[name] = s;
            return true;
        }


        hpx::util::optional<double> get_double(std::string name){
            auto it = dvars.find(name);
            if ( it != dvars.end()){
                return it->second;
            }
            return {};
        }

        hpx::util::optional<std::string> get_string(std::string name){
            auto it = stvars.find(name);
            if ( it != stvars.end()){
                return it->second;
            }
            return {};
        }
};

#endif