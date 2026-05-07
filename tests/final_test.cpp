#include "uthreads.h"
#include "tid_mananger.h"
#include <iostream>
#include <limits>
#include <deque>
#include <vector>
#include <sys/time.h>
#include <signal.h>
// ─── library internals ──────────────────────────────────────────────────────
// Assuming these exactly match the global variables in your uthreads.cpp
extern std::vector<Thread*> allThreads;
extern std::deque<int> threadQueue;
extern int running_tid;
extern int global_quantums;
extern int global_quantum_usecs;
extern Thread* duplicate_thread;
void reset_library() {
    // 1. Disable the hardware timer so it doesn't fire while we reset
    // 1. Block signals to prevent a stale SIGVTALRM from firing during cleanup
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGVTALRM);
    sigprocmask(SIG_BLOCK, &set, NULL);
    // 2. Disable the hardware timer so it doesn't fire after we unblock
    struct itimerval zero_timer = {0};
    setitimer(ITIMER_VIRTUAL, &zero_timer, NULL);
    // 2. Clear all data structures
    // 3. Clean up duplicate_thread to avoid dangling pointer
    if (duplicate_thread != nullptr) {
        delete duplicate_thread;
        duplicate_thread = nullptr;
    }
    // 4. Clear all data structures
    for (int i = 0; i < MAX_THREAD_NUM; ++i) {
        delete allThreads[i];
        allThreads[i] = nullptr;
    }
    threadQueue.clear();
    running_tid = 0;
    global_quantums = 0;
    global_quantum_usecs = 0;
    // 5. Ignore SIGVTALRM so any stale pending signal is discarded on unblock
    signal(SIGVTALRM, SIG_IGN);
    // 6. Unblock signals now that everything is clean
    sigprocmask(SIG_UNBLOCK, &set, NULL);
}
// ─── helpers ────────────────────────────────────────────────────────────────
static int errors = 0;
void check(bool condition, const char* msg) {
    if (!condition) {
        std::cout << "FAIL: " << msg << std::endl;
        errors++;
    } else {
        std::cout << "PASS: " << msg << std::endl;
    }
}
long get_time_usecs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
// ─── TEST 1: basic preemption ───────────────────────────────────────────────
// Prove threads preempt automatically without explicit yield/sleep calls
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
// ─── TEST 2: timer reset edge case ──────────────────────────────────────────
// Prove that if T1 yields early, T2 gets a FULL quantum, not a partial one
static volatile long t2_elapsed_time = 0;
void thread_early_yielder() {
    // Sleep forces an immediate context switch before the timer naturally expires
    while (uthread_get_quantums(2)!=1){}
    uthread_terminate(uthread_get_tid());
}
void thread_full_quantum_tester() {
    long start_time = get_time_usecs();
    int initial_quantums = uthread_get_quantums(uthread_get_tid());
    
    // Spin until the timer preempts us
    while(uthread_get_quantums(uthread_get_tid()) == initial_quantums) {}
    
    t2_elapsed_time = get_time_usecs() - start_time;
    uthread_terminate(uthread_get_tid());
}
// ─── TEST 3: signal masking stress test ─────────────────────────────────────
// Fire the timer incredibly fast while modifying the queue. 
// If sigprocmask is missing, this will SegFault.
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
// ─── main ───────────────────────────────────────────────────────────────────
int main() {
    // ── TEST 1: basic preemption ────────────────────────────────────────────
    std::cout << "\n=== TEST 1: BASIC PREEMPTION (TiK ToK) ===" << std::endl;
    {
        t1_ran = 0;
        t2_ran = 0;
        // 10,000 microseconds = 10ms per quantum
        check(uthread_init(10000) == 0, "T1: Library initialized");
        
        uthread_spawn(thread_preempt_1);
        uthread_spawn(thread_preempt_2);
        // Main thread spins until total quantums hit 10
        while (uthread_get_total_quantums() < 10) {}
        check(t1_ran > 0, "T1: Thread 1 was scheduled automatically");
        check(t2_ran > 0, "T1: Thread 2 was scheduled automatically");
        reset_library();
    }
    // ── TEST 2: timer reset edge case ───────────────────────────────────────
    std::cout << "\n=== TEST 2: TIMER RESET VERIFICATION ===" << std::endl;
    {
        t2_elapsed_time = 0;
        // 100,000 usecs = 0.1 seconds per quantum
        uthread_init(100000); 
        
        uthread_spawn(thread_early_yielder);
        uthread_spawn(thread_full_quantum_tester);
        // Main thread sleeps to let T1 and T2 run
        uthread_sleep(5); 
        // We expect T2's elapsed time to be close to the full 100,000 usecs.
        // If the timer wasn't reset, it will be vastly smaller.
        // Allowing a generous buffer (80,000) for CPU scheduling overhead.
        check(t2_elapsed_time > 80000, "T2: Timer was reset; next thread received a full quantum");
        reset_library();
    }
    // ── TEST 3: signal masking stress test ──────────────────────────────────
    std::cout << "\n=== TEST 3: SIGNAL MASKING STRESS TEST ===" << std::endl;
    {
        // 50 microseconds! The timer will fire aggressively while we modify std::deque
        uthread_init(50); 
        
        int spawner_tid = uthread_spawn(thread_stress_spawner);
        
        // Main thread waits until the spawner finishes
        while (uthread_get_quantums(spawner_tid) != -1) {
            // If we don't SegFault in this loop, signal masking works perfectly!
        }
        check(true, "T3: Survived rapid queue modifications without a SegFault");
        reset_library();
    }
    // ── DONE ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== DONE - Total Stage 6 errors: " << errors << " ===" << std::endl;
    return errors == 0 ? 0 : 1;
}