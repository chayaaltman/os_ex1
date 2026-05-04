cmake_minimum_required(VERSION 4.2)
project(Ex1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Ex1
        demos/demo_itimer.c
        demos/demo_jmp.c
        demos/demo_singInt_handler.c
        games/ants.cpp
        games/thread_tron.cpp
        tests/test_1_kick_tires.cpp
        tests/test_2_thread_id_management.cpp
        tests/test_3_self_preemption.cpp
        uthreads.cpp
        uthreads.h)
