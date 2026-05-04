#include "uthreads.h"
#include <iostream>
#include <limits>


void do_nothing() {

}

int main(int argc, char **argv) {
    int result = uthread_init(std::numeric_limits<int>::max());
    if (result != 0) {
        std::cout << "uthread_init failed with code " << result << std::endl;
        exit(-1);
    }

    std::cout << "Creating " << MAX_THREAD_NUM << " threads" << std::endl;
    for (int i = 1; i < MAX_THREAD_NUM; i++) {
        int spawned_tid = uthread_spawn(do_nothing);
        if (i != spawned_tid) {
            std::cout << "Expected thread " << i << " to be created, go thread " << spawned_tid << std::endl;
            exit(1);
        }
    }
    std::cout << "Trying to create one more thread" << std::endl;
    int spawned_tid = uthread_spawn(do_nothing);
    if (spawned_tid != -1) {
        std::cout << "Expected -1 error for creating too many threads, got tid " << spawned_tid << std::endl;
        exit(1);
    }
    std::cout << "Deleting threads 10 and 20, then creating a new one" << std::endl;
    int errcode;
    errcode = uthread_terminate(10);
    if (errcode != 0) {
        std::cout << "Failure terminating thread 10" << std::endl;
        exit(1);
    }
    errcode = uthread_terminate(20);
    if (errcode != 0) {
        std::cout << "Failure terminating thread 10" << std::endl;
        exit(1);
    }
    int new_thread_id = uthread_spawn(do_nothing);
    if (new_thread_id != 10) {
        std::cout << "expected new thread to have ID 10, got " << new_thread_id << std::endl;
        exit(1);
    }

    std::cout << "Repeatedly spawning and terminating threads" << std::endl;
for (int i = 0; i < 1000; i++) {
    int tid = uthread_spawn(do_nothing);
    if (tid == -1) {
        std::cout << "Failed spawning thread on iteration " << i << std::endl;
        exit(1);
    }

    errcode = uthread_terminate(tid);
    if (errcode != 0) {
        std::cout << "Failed terminating thread " << tid
                  << " on iteration " << i << std::endl;
        exit(1);
    }
}
std::cout << "Trying to terminate a thread that does not exist" << std::endl;
errcode = uthread_terminate(MAX_THREAD_NUM + 100);
if (errcode != -1) {
    std::cout << "Expected -1 when terminating non-existing thread, got " << errcode << std::endl;
    exit(1);
}

    std::cout << "Success" << std::endl;
}
