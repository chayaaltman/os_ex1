
#ifndef TID_MANAGER
#define TID_MANAGER

#include <setjmp.h>
#include <vector>
#include "uthreads.h"

#define STACK_SIZE 4096
#define EMPTY_STRUCTURE "Invalid queue"

typedef void (*thread_entry_point)(void);

enum STATES {
    RUNNING,
    READY,
    BLOCKED
};

struct Thread{
    STATES state;
    int id;
    char* stack;
    thread_entry_point entry_point_func = nullptr;
    int quantum_count =0;
    sigjmp_buf env;

    Thread(int _id, STATES _state, thread_entry_point _func) 
        : state(_state), id(_id), entry_point_func(_func), quantum_count(0) {
        
        if (_id == 0) {
            stack = nullptr;
        } else {
            stack = new char[STACK_SIZE];
        }
    }

    ~Thread() {
        if (stack != nullptr) {
            delete[] stack;
        }
    }
    
};



class TID_manager{
    private:
    const std::vector<Thread*>& thread_list;

    public:
    TID_manager(const std::vector<Thread*>& q);
    int min_id_finder();
    int get_size();
};

#endif