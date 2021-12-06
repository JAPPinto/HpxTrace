    bool explicit_example_counter_discoverer(
        hpx::performance_counters::counter_info const& info,
        //f is called for each discovered performance counter instance
        hpx::performance_counters::discover_counter_func const& f, 
        hpx::performance_counters::discover_counters_mode mode, 
        hpx::error_code& ec
        ) {

        std::cout << "discover" << std::endl;

        hpx::performance_counters::counter_info i = info;



    	// compose the counter name templates

    	//A counter_path_elements holds the elements of a full name for a counter instance.
    	///objectname{parentinstancename::parentindex/instancename#instanceindex}/countername#parameters
        hpx::performance_counters::counter_path_elements p;

        //Fill the given counter_path_elements instance from the given full name of a counter. 
        //Verifica se o instance name segue o formato correto (/, #, etc)
        hpx::performance_counters::counter_status status =
            get_counter_path_elements(info.fullname_, p, ec);

        //invalid counter name
        if (!status_is_valid(status)) return false;

        //discover_counters_mode = discover_counters_minimal oir discover_counters_full
        //Se o nome não estiver completo, adiciona a parte em falta, só que isto nunca acontece
        if (mode == hpx::performance_counters::discover_counters_minimal ||
            p.parentinstancename_.empty() || p.instancename_.empty())
        {
            //Parent instance nunca está vazio???
            if (p.parentinstancename_.empty())
            {
                p.parentinstancename_ = "locality#*";
                p.parentinstanceindex_ = -1;
            }

            if (p.instancename_.empty())
            {
                p.instancename_ = "instance#*";
                p.instanceindex_ = -1;
            }

            status = get_counter_name(p, i.fullname_, ec);
            if (!status_is_valid(status) || !f(i, ec) || ec)
                return false;
        }
        //Posso expandir e criar isntace 0,1,2,3
        else if(p.instancename_ == "instance#*") {
            HPX_ASSERT(mode == hpx::performance_counters::discover_counters_full);

            // FIXME: expand for all instances
            p.instancename_ = "instance";
            p.instanceindex_ = 0;
            status = get_counter_name(p, i.fullname_, ec);
            if (!status_is_valid(status) || !f(i, ec) || ec)
                return false;
        }
        else if (!f(i, ec) || ec) {
            return false;
        }

        if (&ec != &hpx::throws)
            ec = hpx::make_success_code();

        return true;    // everything is ok
    }