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












// #include <deque>
// #include <set>
// #include <sys/time.h>
// #include <stack>
// #include <stdlib.h>
// #include <iostream>
// #include "tid_mananger.h"

//     TID_manager::TID_manager(const std::deque<Thread>& q) : queue(q) {}

//     int TID_manager::min_id_finder(){
//         if(queue.empty()){
//             std::cerr << "thread library error: " << EMPTY_STRUCTURE << std::endl;
//             return -1;
//         }

//         std::set<int> used_ids;

//         for(const auto& it : queue){
//             used_ids.insert(it.id);
//         }

//         int next_tid = 0;
//         while(used_ids.count(next_tid)){
//             next_tid++;
//         }
//         return next_tid;
//     }

//     int TID_manager::get_size(){
//         return queue.size();
//     }

//     TID_manager::~TID_manager() {
//         queue.clear();
//     }
