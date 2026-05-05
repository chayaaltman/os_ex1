#include "uthreads.h"
#include <iostream>

void sleeper() {
    uthread_sleep(10);
    std::cout << "FAIL: Sleeping thread woke up after termination!" << std::endl;
    exit(1); 
}

int main() {
    uthread_init(10000);
    int tid = uthread_spawn(sleeper);

    uthread_sleep(0); // Let sleeper run and go to sleep
    
    if (uthread_terminate(tid) != 0) {
        std::cout << "FAIL: Could not terminate sleeping thread" << std::endl;
        return 1;
    }

    // Yield many times; the sleeper should never log the FAIL message
    for(int i=0; i<15; ++i) uthread_sleep(0);

    if (uthread_get_quantums(tid) == -1) {
        std::cout << "PASS: Terminated sleeper is gone" << std::endl;
        return 0;
    }

    return 1;
}