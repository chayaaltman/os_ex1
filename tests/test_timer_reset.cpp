#include "uthreads.h"
#include <iostream>
#include <sys/time.h>

static volatile long t2_elapsed_time = 0;

long get_time_usecs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void thread_early_yielder() {
    // Calling sleep(0) forces an immediate context switch 
    // *before* the hardware timer naturally expires.
    uthread_sleep(0);
    uthread_terminate(uthread_get_tid());
}

void thread_full_quantum_tester() {
    long start_time = get_time_usecs();
    int initial_quantums = uthread_get_quantums(uthread_get_tid());
    
    // Spin until the timer preempts us naturally
    while(uthread_get_quantums(uthread_get_tid()) == initial_quantums) {}
    
    t2_elapsed_time = get_time_usecs() - start_time;
    uthread_terminate(uthread_get_tid());
}

int main() {
    // 100,000 usecs = 0.1 seconds per quantum
    if (uthread_init(100000) != 0) return 1;
    
    uthread_spawn(thread_early_yielder);
    uthread_spawn(thread_full_quantum_tester);
    
    // Main thread sleeps to get out of the way
    uthread_sleep(5); 
    
    // We expect T2's elapsed time to be close to the full 100,000 usecs.
    if (t2_elapsed_time > 80000) {
        std::cout << "PASS: Timer was reset; next thread received a full quantum (" << t2_elapsed_time << " usecs)" << std::endl;
        return 0;
    } else {
        std::cout << "FAIL: Timer was NOT reset! Thread ran for only " << t2_elapsed_time << " usecs" << std::endl;
        return 1;
    }
}