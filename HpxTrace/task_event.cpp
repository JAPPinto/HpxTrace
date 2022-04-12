#include <task_wrapper.hpp>

using apex::task_wrapper;
using apex::profiler;


class task_event
{

public:
    task_event();
    task_event(std::shared_ptr<task_wrapper> tt_ptr){
        name = tt_ptr->task_id->get_name();
        parent_name = tt_ptr->parent->task_id->get_name();

        guid = tt_ptr->guid;
        parent_guid = tt_ptr->parent_guid;
        
        start_ns = 0;
        end_ns = 0;

        value = 0;
        allocations = 0;
        frees = 0;
        bytes_allocated = 0;
        bytes_freed = 0;

    }
    task_event(std::shared_ptr<profiler> &p){
        std::shared_ptr<task_wrapper> tt_ptr = p->tt_ptr;

        name = tt_ptr->task_id->get_name();
        parent_name = tt_ptr->parent->task_id->get_name();

        guid = tt_ptr->guid;
        parent_guid = tt_ptr->parent_guid;

        start_ns = p->start_ns;
        end_ns = p->end_ns;

        value = p->value;
        allocations = p->allocations;
        frees = p->frees;
        bytes_allocated = p->bytes_allocated;
        bytes_freed = p->bytes_freed;
    }

    std::string name;
    std::string parent_name;

    uint64_t guid;
    uint64_t parent_guid;
    
    uint64_t start_ns;
    uint64_t end_ns;

    double value;
    double allocations;
    double frees;
    double bytes_allocated;
    double bytes_freed;
    
};