#include "uthreads.h"
#include <iostream>
#include <vector>

static std::vector<int> logs;

void thread_zero() {
    logs.push_back(1); 
    uthread_sleep(0);  // Should go to back of READY queue
    logs.push_back(3); 
    uthread_terminate(uthread_get_tid());
}

void thread_one() {
    logs.push_back(2); 
    uthread_sleep(1);  // Should go to SLEEP list
    logs.push_back(4); 
    uthread_terminate(uthread_get_tid());
}

int main() {
    uthread_init(10000);
    uthread_spawn(thread_zero);
    uthread_spawn(thread_one);

    for(int i=0; i<5; ++i) uthread_sleep(0);

    // Expected order: 
    // 1. T1 runs, logs 1, yields.
    // 2. T2 runs, logs 2, sleeps.
    // 3. T1 runs again (it was READY), logs 3.
    // 4. T2 wakes up later, logs 4.
    if (logs.size() == 4 && logs[0]==1 && logs[1]==2 && logs[2]==3 && logs[3]==4) {
        std::cout << "PASS: Sleep ordering is correct" << std::endl;
        return 0;
    }

    std::cout << "FAIL: Execution order was incorrect" << std::endl;
    return 1;
}