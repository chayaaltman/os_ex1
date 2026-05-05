#include "uthreads.h"
#include <iostream>

static int t1_woke = 0;

void thread_func() {
    uthread_sleep(2); 
    t1_woke = 1;
    uthread_terminate(uthread_get_tid());
}

int main() {
    if (uthread_init(10000) != 0) return 1;
    
    // Quantum 1: Main running. Spawned T1.
    // Quantum 2: Main yields. T1 runs, calls sleep(2), goes to sleep.
    uthread_sleep(0); 
    if (t1_woke != 0) {
        std::cout << "FAIL: T1 woke up too early (after 1 yield)" << std::endl;
        return 1;
    }

    // Quantum 3: Main yields. T1 still sleeping.
    uthread_sleep(0);
    if (t1_woke != 0) {
        std::cout << "FAIL: T1 woke up too early (after 2 yields)" << std::endl;
        return 1;
    }

    // Quantum 4: Main yields. T1 should be READY and run now.
    uthread_sleep(0);
    if (t1_woke == 1) {
        std::cout << "PASS: Basic Sleep" << std::endl;
        return 0;
    }

    std::cout << "FAIL: T1 never woke up" << std::endl;
    return 1;
}