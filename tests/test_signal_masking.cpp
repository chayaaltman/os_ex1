#include "uthreads.h"
#include <iostream>

void dummy_thread() {
    uthread_terminate(uthread_get_tid());
}

void thread_stress_spawner() {
    for (int i = 0; i < 90; ++i) {
        int tid = uthread_spawn(dummy_thread);
        if (tid > 0) {
            uthread_block(tid);
            uthread_resume(tid);
        }
    }
    uthread_terminate(uthread_get_tid());
}

int main() {
    // 50 microseconds! The timer will fire aggressively while we modify structures
    if (uthread_init(50) != 0) return 1;
    
    int spawner_tid = uthread_spawn(thread_stress_spawner);
    
    // Main thread waits until the spawner finishes
    while (uthread_get_quantums(spawner_tid) != -1) {
        // If sigprocmask is missing or implemented poorly, 
        // the process will SegFault or deadlock inside this loop!
    }
    
    std::cout << "PASS: Survived rapid queue modifications without a SegFault" << std::endl;
    return 0;
}