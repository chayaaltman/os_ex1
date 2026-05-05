#include "uthreads.h"
#include <iostream>

static int t2_ran = 0;

void thread_func() {
    uthread_sleep(2);
    t2_ran = 1;
    uthread_terminate(uthread_get_tid());
}

int main() {
    uthread_init(10000);
    int tid2 = uthread_spawn(thread_func);

    uthread_sleep(0); // T2 runs, sleeps, main resumes
    uthread_block(tid2); // Explicitly block T2 while it's sleeping

    // Yield 5 times (well past the 2 quantum sleep)
    for(int i=0; i<5; ++i) uthread_sleep(0);

    if (t2_ran != 0) {
        std::cout << "FAIL: T2 ran while explicitly blocked, even though sleep expired" << std::endl;
        return 1;
    }

    uthread_resume(tid2);
    uthread_sleep(0); // Now it should run

    if (t2_ran == 1) {
        std::cout << "PASS: Sleep then Block" << std::endl;
        return 0;
    }

    return 1;
}