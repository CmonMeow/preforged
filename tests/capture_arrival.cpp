#include "../tools/capture/arrival_tracker.h"
#include <iostream>
int main(){
 capture::ArrivalTracker t;
 if(!t.observe(1,{1,2,3},1).empty())return 1;
 if(t.observe(1,{1,2,3,4},1)!=std::vector<std::uint32_t>{4})return 2;
 if(!t.observe(1,{1,4},1).empty()||!t.observe(1,{1,2,4},1).empty())return 3;
 if(!t.observe(2,{1,2,4},1).empty())return 4;
 if(!t.observe(2,{1,2,4,5},5).empty())return 5;
 std::cout<<"PASS initial population excluded, local excluded, once per instance and reappearance excluded\n";
}
