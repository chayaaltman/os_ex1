#include "uthreads.h"
#include <stdlib.h>
#include <deque>
#include <iostream>
#include <sys/time.h>
#include "tid_mananger.h"
#include <algorithm>
#include <setjmp.h>
#include <signal.h>


#define INVALID_INPUT "invalid input"
#define INVALID_ARGUMENT "invalid argument"
#define MAX_SIZE_ERR "thread queue reached the max capacity"
#define ID_NOT_FOUND "ID does not exist"


#ifdef __x86_64__
typedef unsigned long address_t;

#define JB_SP 6
#define JB_PC 7

address_t translate_address(address_t addr) {
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

address_t translate_address(address_t addr) {
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

std::deque<int> threadQueue;
std::vector<Thread*> allThreads(MAX_THREAD_NUM, nullptr);

int running_tid = 0;
int global_quantum_usecs = 0;
int global_quantums = 0;


/**
 * @brief initializes the thread library.
 *
 * Once this function returns, the main thread (tid == 0) will be set as RUNNING. There is no need to
 * provide an entry_point or to create a stack for the main thread - it will be using the "regular" stack and PC.
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {
    if (quantum_usecs<=0 ){
        std::cerr << "thread library error: " << INVALID_INPUT << std::endl;
        return -1;
    }

    Thread* mainThread = new Thread(0, STATES::RUNNING, nullptr);
    mainThread->quantum_count = 1;

    allThreads[0] = mainThread;
    threadQueue.push_back(0);

    global_quantum_usecs = quantum_usecs;
    global_quantums = 1;

    return 0;
}

/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 * It is an error to call this function with a null entry_point.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn(thread_entry_point entry_point) {
    if(!entry_point){
        std::cerr << "thread library error: " << INVALID_ARGUMENT << std::endl;
        return -1;
    }

    TID_manager manager(allThreads);
    int next_id = manager.min_id_finder();

    if(next_id == -1){
        std::cerr << "thread library error: " << MAX_SIZE_ERR << std::endl;
        return -1;
    }


    Thread* thread = new Thread(next_id, STATES::READY, entry_point);
    thread->stack = new char[STACK_SIZE];
    address_t sp = (address_t) thread->stack + STACK_SIZE - sizeof(address_t);
    address_t pc = (address_t) entry_point;
    sigsetjmp(thread->env, 1);
    (thread->env[0].__jmpbuf)[JB_SP] = translate_address(sp);
    (thread->env[0].__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&thread->env[0].__saved_mask);

    allThreads[next_id] = thread;
    threadQueue.push_back(next_id);
    return next_id;
}


/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
    if (tid == 0) {
        for (int i = 0; i < MAX_THREAD_NUM; ++i) {
            delete allThreads[i];
            allThreads[i] = nullptr;
        }
        exit(0);
    }

    if (tid < 0 || tid >= MAX_THREAD_NUM || allThreads[tid] == nullptr) {
        std::cerr << "thread library error: " << ID_NOT_FOUND << std::endl;
        return -1;
    }

    threadQueue.erase(
        std::remove(threadQueue.begin(), threadQueue.end(), tid),
        threadQueue.end()
    );

    delete allThreads[tid];
    allThreads[tid] = nullptr;

    if (tid == running_tid) {
        running_tid = threadQueue.front();
        threadQueue.pop_front();

        Thread* next = allThreads[running_tid];
        next->state = STATES::RUNNING;
        next->quantum_count++;
        global_quantums++;

        siglongjmp(next->env, 1);
    }

    return 0;
}


/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made. Blocking a thread in
 * BLOCKED state has no effect and is *not* considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block(int tid) {

    std::cerr << "thread library error: " << "did not implement" << std::endl;
    return -1;
}


/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * When a thread transition to the READY state it is placed at the end of the READY queue.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid) {
    std::cerr << "thread library error: " << "did not implement" << std::endl;
    return -1;
}


/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY queue.
 * If the thread which was just RUNNING should also be added to the READY queue, or if multiple threads wake up
 * at the same time, the order in which they're added to the end of the READY queue doesn't matter.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * A call with num_quantums == 0 will immediately stop the thread and move it to the back of the execution queue.
 *
 * It is considered an error if the main thread (tid == 0) calls this function with num_quantums != 0.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums) {
    if(num_quantums != 0){
        return -1;
    }
    if(running_tid == 0 && num_quantums != 0){
        std::cerr << "thread library error: " << "main thread cannot sleep" << std::endl;
        return -1;
    }
    context_switch(true);
    return 0;
}


/**
 * @brief Helper context switch function
 * This function will implement the context switch mainly used for the sleep function.
 * It will push the current thread to the back of the queue, while changing it from 'RUNNING' to 'READY'.
 * And also manage the new 'RUNNING' thread.
 */

 void context_switch(bool requeue){
    Thread* current = allThreads[running_tid];

    int ret = sigsetjmp(current->env, 1);
    if (ret != 0) {
        return; 
    }

    if (requeue) {
        current->state = STATES::READY;
        threadQueue.push_back(running_tid);
    }

    running_tid = threadQueue.front();
    threadQueue.pop_front();

    Thread* next = allThreads[running_tid];
    next->state = STATES::RUNNING;
    next->quantum_count++;
    global_quantums++;

    siglongjmp(next->env, 1);
}


/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid() {
    return running_tid;
}


/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums() {
    return global_quantums;
}


/**
 * @brief Returns the number of quantums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid) {
    if (tid < 0 || tid >= MAX_THREAD_NUM || allThreads[tid] == nullptr) {
        std::cerr << "thread library error: " << ID_NOT_FOUND << std::endl;
        return -1;
    }
    return allThreads[tid]->quantum_count;
}