
#include "uthreads.h"
#include <iostream>
#include <limits>

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

// ─── threads ────────────────────────────────────────────────────────────────

// used by test 1: basic block/resume
// thread 1 blocks itself, waits for main to resume it
void thread_self_block() {
    std::cout << "[T1] running, will block itself" << std::endl;
    uthread_block(uthread_get_tid()); // blocks itself → context switch fires
    // only reaches here after main resumes it
    std::cout << "[T1] resumed successfully" << std::endl;
    uthread_terminate(uthread_get_tid());
}

// used by test 2: terminated while blocked
// thread 2 blocks itself, then main terminates it while blocked
void thread_block_then_die() {
    std::cout << "[T2] running, will block itself" << std::endl;
    uthread_block(uthread_get_tid());
    // should never reach here
    std::cout << "[T2] ERROR - should have been terminated while blocked" << std::endl;
    exit(1);
}

// used by test 3: one thread blocks another
int t3_victim_ran = 0;
void thread_victim() {
    t3_victim_ran++;
    std::cout << "[T4] victim running (should only run once before block)" << std::endl;
    uthread_sleep(0); // yield so blocker can run
    // only reaches here after resume
    std::cout << "[T4] victim resumed" << std::endl;
    uthread_terminate(uthread_get_tid());
}

void thread_blocker() {
    std::cout << "[T3] blocker running, will block T4" << std::endl;
    // block the victim (tid 4) externally
    check(uthread_block(4) == 0, "block external thread succeeds");
    std::cout << "[T3] blocked T4, self-terminating" << std::endl;
    uthread_terminate(uthread_get_tid());
}


int main() {
    std::cout << "\n=== ERROR HANDLING TESTS ===" << std::endl;
    {
        if (uthread_init(std::numeric_limits<int>::max()) != 0) {
            std::cout << "ERROR: uthread_init failed" << std::endl;
            exit(1);
        }

        // resume non-existent thread
        check(uthread_resume(99) == -1,  "resume non-existent thread returns -1");

        // block non-existent thread
        check(uthread_block(99) == -1,   "block non-existent thread returns -1");

        // block main thread
        check(uthread_block(0) == -1,    "block main thread returns -1");

        // resume main thread (RUNNING) — no effect, not an error
        check(uthread_resume(0) == 0,    "resume RUNNING thread is not an error");

        //uthread_terminate(0);
    }

    std::cout << "\n=== TEST 1: SELF-BLOCK AND RESUME ===" << std::endl;
    {
        if (uthread_init(std::numeric_limits<int>::max()) != 0) {
            std::cout << "ERROR: uthread_init failed" << std::endl;
            exit(1);
        }

        int tid1 = uthread_spawn(thread_self_block);
        check(tid1 == 1, "spawned thread 1");

        uthread_sleep(0); // yield → T1 runs and blocks itself → back to main

        // T1 is now BLOCKED and NOT in ready queue
        // resuming a READY thread is a no-op — but T1 is BLOCKED so resume should work
        std::cout << "[Main] resuming T1" << std::endl;
        check(uthread_resume(tid1) == 0, "resume blocked thread succeeds");

        // resuming again (now READY) should be a no-op, not an error
        check(uthread_resume(tid1) == 0, "resume READY thread is not an error");

        uthread_sleep(0); // yield → T1 runs, prints resumed, self-terminates → back to main

        check(uthread_get_quantums(tid1) == -1, "T1 is gone after self-terminate");

        //uthread_terminate(0);
    }

    std::cout << "\n=== TEST 2: TERMINATE WHILE BLOCKED ===" << std::endl;
    {
        if (uthread_init(std::numeric_limits<int>::max()) != 0) {
            std::cout << "ERROR: uthread_init failed" << std::endl;
            exit(1);
        }

        int tid2 = uthread_spawn(thread_block_then_die);
        check(tid2 == 1, "spawned thread 2");

        uthread_sleep(0); // yield → T2 runs and blocks itself → back to main

        // T2 is now BLOCKED (not in ready queue)
        // terminate it while blocked — erase/remove should be a no-op on ready queue
        check(uthread_terminate(tid2) == 0, "terminate blocked thread succeeds");
        check(uthread_get_quantums(tid2) == -1, "T2 is gone after terminate");

        //uthread_terminate(0);
    }

    std::cout << "\n=== TEST 3: EXTERNAL BLOCK BY ANOTHER THREAD ===" << std::endl;
    {
        if (uthread_init(std::numeric_limits<int>::max()) != 0) {
            std::cout << "ERROR: uthread_init failed" << std::endl;
            exit(1);
        }

        // spawn order: T3=blocker, T4=victim
        // ready queue after spawns: [3, 4]
        int tid3 = uthread_spawn(thread_blocker);
        int tid4 = uthread_spawn(thread_victim);
        check(tid3 == 1, "spawned blocker as T1");
        check(tid4 == 2, "spawned victim as T2");

        uthread_sleep(0); // main yields → T3 runs, blocks T4, self-terminates → T4 runs
                          // but T4 is blocked before it can run again → back to main

        // T4 should be blocked now, T3 gone
        check(uthread_get_quantums(tid3) == -1, "blocker T3 is gone");

        // T4 exists but is blocked
        check(uthread_get_quantums(tid4) != -1, "victim T4 still exists (blocked)");

        // resume T4
        std::cout << "[Main] resuming victim T4" << std::endl;
        check(uthread_resume(tid4) == 0, "resume victim succeeds");

        uthread_sleep(0); // yield → T4 runs, prints resumed, self-terminates → back to main

        check(uthread_get_quantums(tid4) == -1, "victim T4 gone after resume+terminate");

        uthread_terminate(0);
    }

    std::cout << "\n=== TEST 4: DOUBLE BLOCK IS NO-OP ===" << std::endl;
    {
        if (uthread_init(std::numeric_limits<int>::max()) != 0) {
            std::cout << "ERROR: uthread_init failed" << std::endl;
            exit(1);
        }

        // spawn a thread that blocks itself
        int tid = uthread_spawn(thread_self_block);
        uthread_sleep(0); // yield → thread blocks itself → back to main

        // block an already-BLOCKED thread — should be no-op, not an error
        check(uthread_block(tid) == 0, "blocking already-BLOCKED thread is not an error");

        // resume it
        check(uthread_resume(tid) == 0, "resume after double-block succeeds");

        uthread_sleep(0); // let it finish
        //uthread_terminate(0);
    }

    std::cout << "\n=== DONE - Total errors: " << errors << " ===" << std::endl;
    return errors == 0 ? 0 : 1;
    uthread_terminate(0);
}