#include <vector>
#include <set>
#include <sys/time.h>
#include <stack>
#include <stdlib.h>
#include <iostream>
#include "tid_mananger.h"

TID_manager::TID_manager(const std::vector<Thread*>& q) : thread_list(q) {}

//Refer to header file for documentation.
int TID_manager::min_id_finder(){
    for(int i=0; i<MAX_THREAD_NUM; ++i){
        if(thread_list[i]==nullptr){
            return i;
        }
    }
    return -1;
}

//Refer to header file for documentation.
int TID_manager::get_size(){
    return thread_list.size();
}
