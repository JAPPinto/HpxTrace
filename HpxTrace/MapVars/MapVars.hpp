#ifndef _MAPVARS_H
#define _MAPVARS_H

class MapVars{

    typedef boost::variant<double, std::string> Variant;
    typedef std::vector<Variant> VariantList;

    private:
        std::map<std::string, std::map<VariantList,double>> dmaps;
        std::map<std::string, std::map<VariantList,std::string>> stmaps;

        std::map<std::string, std::string> stvars;

        bool string_map_exists(std::string map_name){
            auto it = stmaps.find(map_name);
            if ( it != stmaps.end()){
                return true;
            }
            return false;
        }

        bool double_map_exists(std::string map_name){
            auto it = dmaps.find(map_name);
            if ( it != dmaps.end()){
                return true;
            }
            return false;
        }
    public:


        bool store_double(std::string map_name, VariantList keys, double d){
            if(string_map_exists(map_name)) return false;
            dmaps[map_name][keys] = d;
            return true;
        }
        bool store_string(std::string map_name, VariantList keys, std::string s){
            if(double_map_exists(map_name)) return false;
            stmaps[map_name][keys] = s;
            return true;
        }

        hpx::util::optional<double> get_double(std::string map_name, VariantList keys){
            if(double_map_exists(map_name))
                return dmaps[map_name][keys];
            else
                return {};
        }

        hpx::util::optional<std::string> get_string(std::string map_name, VariantList keys){
            if(string_map_exists(map_name))
                return stmaps[map_name][keys];
            else
                return {};
        }
};

#endif