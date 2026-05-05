#include "uthreads.h"
#include <iostream>

int main() {
    uthread_init(10000);

    if (uthread_sleep(1) != -1) {
        std::cout << "FAIL: main thread was allowed to sleep(1)" << std::endl;
        return 1;
    }

    if (uthread_sleep(0) != 0) {
        std::cout << "FAIL: main thread could not sleep(0) [yield]" << std::endl;
        return 1;
    }

    std::cout << "PASS: Main thread sleep constraints enforced" << std::endl;
    return 0;
}