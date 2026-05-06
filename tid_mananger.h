
#ifndef TID_MANAGER
#define TID_MANAGER

#include <setjmp.h>
#include <vector>
#include "uthreads.h"

#define STACK_SIZE 4096
#define EMPTY_STRUCTURE "Invalid queue"

/**
 * Represents a pointer to a function that serves as the entry point for a thread.
 */
typedef void (*thread_entry_point)(void);

/**
 * Enum representing the possible states of a thread: RUNNING, READY, and BLOCKED.
 */
enum STATES {
    RUNNING,
    READY,
    BLOCKED
};

/**
 * Represents a thread in the system, containing its state, ID, stack, entry point function, quantum count,
 * environment for context switching, wake quantum, and flags for sleep and block status for use in various functions throughout
 * the exercise.
 */
struct Thread{
    STATES state;
    int id;
    char* stack;
    thread_entry_point entry_point_func = nullptr;
    int quantum_count =0;
    sigjmp_buf env;
    int wake_quantum =-1;
    bool sleep_flag = false;
    bool blocked_flag = false;

    /**
     * Constructor for the Thread struct, initializing the thread's state, ID, entry point function, and quantum count.
     * If the thread ID is 0, the stack is set to nullptr; otherwise, a new stack of size STACK_SIZE is allocated for the thread.
     * @param _id The ID of the thread.
     * @param _state The initial state of the thread.
     * @param _func The entry point function for the thread.
     */
    Thread(int _id, STATES _state, thread_entry_point _func) 
        : state(_state), id(_id), entry_point_func(_func), quantum_count(0) {
        
        if (_id == 0) {
            stack = nullptr;
        } else {
            stack = new char[STACK_SIZE];
        }
    }

    /**
     * Destructor for the Thread struct, responsible for deallocating the stack memory if it was allocated.
     */
    ~Thread() {
        if (stack != nullptr) {
            delete[] stack;
        }
        stack = nullptr;
    }
    
};


/**
 * This class is responsible for managing thread IDs, providing functionality to find the minimum available thread ID
 * and to get the size of the thread list.
 * The TID_manager class takes a vector of Thread pointers as input and provides methods to manage thread IDs effectively.
 * The min_id_finder method is used to find the minimum available thread ID.
 * The get_size method returns the size of the thread list.
 */
class TID_manager{
    private:
    const std::vector<Thread*>& thread_list;

    public:
    TID_manager(const std::vector<Thread*>& q);
    int min_id_finder();
    int get_size();
};

#endif
