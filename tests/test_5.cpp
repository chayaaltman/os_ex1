#include "uthreads.h"
#include "tid_mananger.h"
#include <iostream>
#include <limits>
#include <deque>
#include <vector>

// ─── library internals ──────────────────────────────────────────────────────
extern std::vector<Thread*> allThreads;
extern std::deque<int> threadQueue;
extern int running_tid;
extern int global_quantums;
extern int global_quantum_usecs;

void reset_library() {
    for (int i = 0; i < MAX_THREAD_NUM; ++i) {
        delete allThreads[i];
        allThreads[i] = nullptr;
    }
    threadQueue.clear();
    running_tid = 0;
    global_quantums = 0;
    global_quantum_usecs = 0;
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

// ─── shared state ───────────────────────────────────────────────────────────

static int order_log[20];
static int order_idx = 0;

void log_event(int event) {
    order_log[order_idx++] = event;
}

// ─── TEST 1: basic non-zero sleep ───────────────────────────────────────────
// T1 sleeps for 2 quantums, main yields twice, T1 should wake on 3rd quantum

static int t1_woke = 0;

void thread_sleep_2() {
    log_event(1); // T1 first run
    uthread_sleep(2); // sleep for 2 quantums
    t1_woke = 1;
    log_event(4); // T1 wakes up
    uthread_terminate(uthread_get_tid());
}

// ─── TEST 2: sleep then block — must wait for BOTH ──────────────────────────
// T2 sleeps for 2 quantums, main blocks it mid-sleep,
// T2 should NOT wake when sleep expires, only after resume too

static int t2_ran_after_sleep = 0;

void thread_sleep_then_blocked() {
    log_event(10);
    uthread_sleep(2); // sleep 2 quantums
    // only reaches here if both sleep expired AND resumed
    t2_ran_after_sleep = 1;
    log_event(14);
    uthread_terminate(uthread_get_tid());
}

// ─── TEST 3: multiple threads waking at same time ───────────────────────────

static int t3_wake_count = 0;

void thread_multi_sleep_a() {
    uthread_sleep(2);
    t3_wake_count++;
    uthread_terminate(uthread_get_tid());
}

void thread_multi_sleep_b() {
    uthread_sleep(2);
    t3_wake_count++;
    uthread_terminate(uthread_get_tid());
}

// ─── TEST 4: sleep 0 vs sleep 1 ordering ────────────────────────────────────

static int t4_log[10];
static int t4_idx = 0;

void thread_sleep_zero() {
    t4_log[t4_idx++] = 1; // ran first time
    uthread_sleep(0);
    t4_log[t4_idx++] = 3; // ran second time
    uthread_terminate(uthread_get_tid());
}

void thread_sleep_one() {
    t4_log[t4_idx++] = 2; // ran first time
    uthread_sleep(1);
    t4_log[t4_idx++] = 4; // ran after 1 quantum sleep
    uthread_terminate(uthread_get_tid());
}

// ─── TEST 5: main thread sleep(n) is an error ───────────────────────────────

// ─── TEST 6: sleep then terminate ───────────────────────────────────────────
// a sleeping thread gets terminated — should not reappear in ready queue

void thread_sleep_get_terminated() {
    uthread_sleep(5); // sleep for a long time
    // should never reach here
    std::cout << "ERROR: sleeping thread should have been terminated" << std::endl;
    exit(1);
}

// ─── main ───────────────────────────────────────────────────────────────────

int main() {

    // ── TEST 1: basic non-zero sleep ────────────────────────────────────────
    std::cout << "\n=== TEST 1: BASIC NON-ZERO SLEEP ===" << std::endl;
    {
        order_idx = 0;
        t1_woke = 0;

        uthread_init(std::numeric_limits<int>::max());
        // quantums: 1=main, 2=T1(sleeps 2), 3=main yield1, 4=main yield2, 5=T1 wakes
        int tid1 = uthread_spawn(thread_sleep_2);

        // quantum 1: main running
        check(uthread_get_total_quantums() == 1, "T1: total quantums starts at 1");

        uthread_sleep(0); // quantum 2: T1 runs, calls sleep(2), wake_quantum = 2+2 = 4
                          // quantum 3: main resumes
        log_event(2);
        check(t1_woke == 0, "T1: thread hasn't woken yet after 1 yield");

        uthread_sleep(0); // quantum 4: nobody ready except... check wake
                          // at start of context_switch, global_quantums=3, wake_quantum=4
                          // 3 < 4 so T1 still sleeping. main goes back to queue.
                          // quantum 4 starts: main runs again? 
                          // Actually at quantum 4 start T1 wakes (4>=4)
        log_event(3);
        check(t1_woke == 0, "T1: thread hasn't woken after 2nd yield (wakes on next switch)");

        uthread_sleep(0); // quantum 5: T1 is now in ready queue, runs, sets t1_woke=1
                          // quantum 6: main resumes
        check(t1_woke == 1, "T1: thread woke up after sleep(2) expired");
        check(uthread_get_quantums(tid1) == -1, "T1: thread self-terminated after waking");

        reset_library();
    }

    // ── TEST 2: sleep then block ─────────────────────────────────────────────
    std::cout << "\n=== TEST 2: SLEEP THEN BLOCK ===" << std::endl;
    {
        t2_ran_after_sleep = 0;

        uthread_init(std::numeric_limits<int>::max());
        int tid2 = uthread_spawn(thread_sleep_then_blocked);

        uthread_sleep(0); // yield → T2 runs, calls sleep(2) → back to main
        // T2 is now sleeping (wake_quantum = current+2)

        // block T2 while it's sleeping
        check(uthread_block(tid2) == 0, "T2: can block a sleeping thread");
        check(t2_ran_after_sleep == 0, "T2: hasn't run after sleep+block");

        // yield enough times for sleep to expire
        uthread_sleep(0);
        uthread_sleep(0);
        uthread_sleep(0);

        // sleep has expired but T2 is still explicitly blocked
        check(t2_ran_after_sleep == 0, "T2: still blocked even after sleep expired");

        // now resume — T2 should become READY
        check(uthread_resume(tid2) == 0, "T2: resume after sleep expired");

        uthread_sleep(0); // yield → T2 runs → back to main

        check(t2_ran_after_sleep == 1, "T2: ran after both sleep expired and resumed");
        check(uthread_get_quantums(tid2) == -1, "T2: self-terminated");

        reset_library();
    }

    // ── TEST 3: multiple threads waking at same time ─────────────────────────
    std::cout << "\n=== TEST 3: MULTIPLE THREADS WAKE SIMULTANEOUSLY ===" << std::endl;
    {
        t3_wake_count = 0;

        uthread_init(std::numeric_limits<int>::max());
        int tida = uthread_spawn(thread_multi_sleep_a);
        int tidb = uthread_spawn(thread_multi_sleep_b);

        // both threads sleep for 2 quantums each
        // they both start in quantum 2 and 3 respectively,
        // so they wake at different times — let's just verify both eventually wake
        uthread_sleep(0); // Q2: Ta runs, sleeps(2)
        uthread_sleep(0); // Q3: Tb runs, sleeps(2)
        uthread_sleep(0); // Q4: main, Ta wakes (wake_quantum=4)
        uthread_sleep(0); // Q5: Ta runs, increments count, terminates. Tb wakes.
        uthread_sleep(0); // Q6: Tb runs, increments count, terminates.
        uthread_sleep(0); // Q7: main

        check(t3_wake_count == 2, "T3: both sleeping threads eventually woke up");
        check(uthread_get_quantums(tida) == -1, "T3: thread A terminated");
        check(uthread_get_quantums(tidb) == -1, "T3: thread B terminated");

        reset_library();
    }

    // ── TEST 4: sleep(0) vs sleep(1) ordering ───────────────────────────────
    std::cout << "\n=== TEST 4: SLEEP(0) VS SLEEP(1) ORDERING ===" << std::endl;
    {
        t4_idx = 0;

        uthread_init(std::numeric_limits<int>::max());
        // T5=sleep_zero, T6=sleep_one
        uthread_spawn(thread_sleep_zero); // tid 1
        uthread_spawn(thread_sleep_one);  // tid 2

        uthread_sleep(0); // Q2: T5 runs, logs 1, sleep(0) → back of queue
                          // Q3: T6 runs, logs 2, sleep(1) → blocked until Q4+1=5
                          // Q4: T5 runs again, logs 3, terminates
                          // Q5: T6 wakes, logs 4, terminates
                          // Q6: main
        uthread_sleep(0);
        uthread_sleep(0);
        uthread_sleep(0);
        uthread_sleep(0);

        check(t4_log[0] == 1, "T4: sleep_zero ran first");
        check(t4_log[1] == 2, "T4: sleep_one ran second");
        check(t4_log[2] == 3, "T4: sleep_zero resumed before sleep_one");
        check(t4_log[3] == 4, "T4: sleep_one resumed last");

        reset_library();
    }

    // ── TEST 5: main thread sleep(n!=0) is error ─────────────────────────────
    std::cout << "\n=== TEST 5: MAIN THREAD SLEEP ERROR ===" << std::endl;
    {
        uthread_init(std::numeric_limits<int>::max());

        check(uthread_sleep(1) == -1,  "T5: main cannot sleep(1)");
        check(uthread_sleep(10) == -1, "T5: main cannot sleep(10)");
        check(uthread_sleep(0) == 0,   "T5: main CAN sleep(0)");

        reset_library();
    }

    // ── TEST 6: terminate a sleeping thread ──────────────────────────────────
    std::cout << "\n=== TEST 6: TERMINATE SLEEPING THREAD ===" << std::endl;
    {
        uthread_init(std::numeric_limits<int>::max());
        int tid = uthread_spawn(thread_sleep_get_terminated);

        uthread_sleep(0); // yield → thread runs, calls sleep(5) → back to main

        // terminate it while it's sleeping
        check(uthread_terminate(tid) == 0, "T6: can terminate a sleeping thread");
        check(uthread_get_quantums(tid) == -1, "T6: sleeping thread is gone");

        // yield a few times — thread should NOT reappear
        uthread_sleep(0);
        uthread_sleep(0);
        uthread_sleep(0);
        uthread_sleep(0);
        uthread_sleep(0);

        std::cout << "PASS: T6: terminated sleeping thread never reappeared" << std::endl;

        reset_library();
    }

    // ── DONE ─────────────────────────────────────────────────────────────────
    std::cout << "\n=== DONE - Total errors: " << errors << " ===" << std::endl;
    return errors == 0 ? 0 : 1;
}