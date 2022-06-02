#include <hpx/modules/program_options.hpp>

#ifndef _HPXTRACE_H
#define _HPXTRACE_H

namespace HpxTrace
{
    void register_command_line_options(hpx::program_options::options_description& desc_commandline);

    void init(std::string script);
    void init(hpx::program_options::variables_map& vm);

    void finalize();

    void trigger_probe(std::string probe_name, 
    std::map<std::string,double> double_arguments = {},
    std::map<std::string,std::string> string_arguments = {});

}

#endif