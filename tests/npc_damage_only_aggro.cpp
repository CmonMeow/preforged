#include "gw_npc_aggro_memory.h"
#include <array>
#include <stdexcept>
#include <iostream>
using namespace fw::gw::combat::ai::aggro;
void check(bool yes){if(!yes)throw std::runtime_error("damage-only memory");}
int main(){
 const Scope source{1,100,1},attacker{1,1,1},bystander{1,2,1};Memory memory(source);
 std::array<Observation,2> seen{{{attacker,true,true,true,500},{bystander,true,true,true,0}}};
 check(!memory.observe(seen,1000,1500,false)->engaged);
 check(memory.provoke(attacker));
 check(memory.observe(seen,1000,1500,false)->target==attacker);
 check(!memory.remembers(bystander));
 seen[0].distance=1400;check(memory.observe(seen,1000,1500,false)->target==attacker);
 seen[0].distance.reset();auto unknown=*memory.observe(seen,1000,1500,false);
 check(unknown.engaged&&!unknown.target);
 seen[0].distance=1501;check(!memory.observe(seen,1000,1500,false)->engaged);
 seen[0].distance=0;check(!memory.observe(seen,1000,1500,false)->engaged);
 check(memory.provoke(attacker));seen[0].target.generation++;
 check(!memory.observe(seen,1000,1500,false)->engaged);
 check(memory.observe(seen,1000,1500)->engaged); // default proximity unchanged
 std::cout<<"Damage-only aggro retains provoked lifetimes without recruiting bystanders\n";
}
