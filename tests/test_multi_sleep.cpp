#include "uthreads.h"
#include <iostream>

static int wake_count = 0;

void thread_func() {
    uthread_sleep(2);
    wake_count++;
    uthread_terminate(uthread_get_tid());
}

int main() {
    uthread_init(10000);
    uthread_spawn(thread_func); // T1
    uthread_spawn(thread_func); // T2

    // Yield multiple times to let both sleeps expire
    for(int i=0; i<6; ++i) uthread_sleep(0);

    if (wake_count == 2) {
        std::cout << "PASS: Multiple threads woke up successfully" << std::endl;
        return 0;
    }

    std::cout << "FAIL: Only " << wake_count << " threads woke up" << std::endl;
    return 1;
}