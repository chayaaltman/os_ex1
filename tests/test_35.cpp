

#include "uthreads.h"
#include <iostream>
#include <limits>

// Thread 2 just yields and waits to be terminated externally
void thread_to_be_killed() {
    // should never reach here - thread 1 terminates us while we're in ready queue
    std::cout << "Thread 2: ERROR - should have been terminated before running" << std::endl;
    exit(1);
}

// Thread 1 terminates thread 2, then terminates itself
void killer_thread() {
    std::cout << "Thread 1: running" << std::endl;

    // thread 2 was never scheduled, so its quantum count should be 0
    int t2_quantums = uthread_get_quantums(2);
    std::cout << "Thread 2 quantums: " << t2_quantums << " (expected 0)" << std::endl;
    if (t2_quantums != 0) {
        std::cout << "ERROR: thread 2 quantum count wrong" << std::endl;
        exit(1);
    }

    // total: quantum 1 (main) + quantum 2 (thread 1 now) = 2
    int total = uthread_get_total_quantums();
    std::cout << "Total quantums: " << total << " (expected 2)" << std::endl;
    if (total != 2) {
        std::cout << "ERROR: total quantum count wrong" << std::endl;
        exit(1);
    }

    // thread 1 itself has run once
    int t1_quantums = uthread_get_quantums(1);
    std::cout << "Thread 1 quantums: " << t1_quantums << " (expected 1)" << std::endl;
    if (t1_quantums != 1) {
        std::cout << "ERROR: thread 1 quantum count wrong" << std::endl;
        exit(1);
    }

    // externally terminate thread 2 while it's still in ready queue
    if (uthread_terminate(2) != 0) {
        std::cout << "ERROR: failed to terminate thread 2" << std::endl;
        exit(1);
    }
    std::cout << "Thread 1: killed thread 2 successfully" << std::endl;

    // verify thread 2 is gone
    if (uthread_get_quantums(2) != -1) {
        std::cout << "ERROR: thread 2 should no longer exist" << std::endl;
        exit(1);
    }

    // self-terminate
    std::cout << "Thread 1: self-terminating" << std::endl;
    uthread_terminate(1);
}

int main() {
    if (uthread_init(std::numeric_limits<int>::max()) != 0) {
        std::cout << "ERROR: uthread_init failed" << std::endl;
        exit(1);
    }

    if (uthread_spawn(killer_thread) != 1) {
        std::cout << "ERROR: failed to spawn thread 1" << std::endl;
        exit(1);
    }
    if (uthread_spawn(thread_to_be_killed) != 2) {
        std::cout << "ERROR: failed to spawn thread 2" << std::endl;
        exit(1);
    }

    std::cout << "Main: yielding" << std::endl;
    uthread_sleep(0); // quantum 2: thread 1 runs, kills thread 2, self-terminates
                      // quantum 3: main resumes (thread 2 never ran)

    std::cout << "Main: back" << std::endl;

    if (uthread_get_quantums(1) != -1) {
        std::cout << "ERROR: thread 1 should no longer exist" << std::endl;
        exit(1);
    }
    if (uthread_get_quantums(2) != -1) {
        std::cout << "ERROR: thread 2 should no longer exist" << std::endl;
        exit(1);
    }

    int total = uthread_get_total_quantums();
    std::cout << "Total quantums: " << total << " (expected 3)" << std::endl;
    if (total != 3) {
        std::cout << "ERROR: total quantum count wrong at end" << std::endl;
        exit(1);
    }

    std::cout << "All checks passed!" << std::endl;
    uthread_terminate(0);
}

