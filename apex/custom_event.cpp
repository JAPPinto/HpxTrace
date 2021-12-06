////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2014-2015 Oregon University
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

// Naive SMP version implemented with futures.

#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/util.hpp>

#include <apex_api.hpp>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <list>
#include <random>
#include <vector>
#include <typeinfo>
#include <string> 

int hpx_main(){


    apex_event_type t;

    for (int i = 0; i < 200; i++) {
        t = apex::register_custom_event (std::to_string(i));
        std::cout << i << " " << t << '\n';
        /* code */
    }

  

    return hpx::finalize(); // Handles HPX shutdown
}


///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  

    return hpx::init(argc, argv);
}
