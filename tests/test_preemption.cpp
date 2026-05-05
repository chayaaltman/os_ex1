#include "uthreads.h"
#include <iostream>

static volatile int t1_ran = 0;
static volatile int t2_ran = 0;

void thread_preempt_1() {
    while (uthread_get_total_quantums() < 10) {
        t1_ran = uthread_get_quantums(uthread_get_tid());
    }
    uthread_terminate(uthread_get_tid());
}

void thread_preempt_2() {
    while (uthread_get_total_quantums() < 10) {
        t2_ran = uthread_get_quantums(uthread_get_tid());
    }
    uthread_terminate(uthread_get_tid());
}

int main() {
    if (uthread_init(10000) != 0) {
        std::cout << "FAIL: Library initialization failed" << std::endl;
        return 1;
    }

    uthread_spawn(thread_preempt_1);
    uthread_spawn(thread_preempt_2);

    // Main thread spins until total quantums hit 10
    while (uthread_get_total_quantums() < 10) {}

    if (t1_ran > 0 && t2_ran > 0) {
        std::cout << "PASS: Basic Preemption (TiK ToK)" << std::endl;
        return 0;
    } else {
        std::cout << "FAIL: Threads were not scheduled automatically. t1: " << t1_ran << ", t2: " << t2_ran << std::endl;
        return 1;
    }
}