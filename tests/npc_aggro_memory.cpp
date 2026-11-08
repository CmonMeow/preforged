#include "gw_npc_aggro_memory.h"
#include <array>
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw::combat::ai::aggro;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("NPC aggro memory line "+std::to_string(at.line()));
}
const Scope source{1,100,1},first{1,1,1},second{1,2,1};
Observation foe(Scope scope,double distance){return {scope,true,true,true,distance};}
int main()try{
    Memory memory(source);
    std::array<Observation,2> seen{{foe(first,400),foe(second,800)}};
    auto selected=*memory.observe(seen,1000,2000);
    check(selected.target==first&&selected.engaged&&memory.remembers(second));
    seen[0].distance=2500;seen[1].distance=1500;
    selected=*memory.observe(seen,1000,2000);
    check(selected.target==second&&selected.engaged&&!memory.remembers(first));
    // Newcomers outside acquire are not invented members of the encounter.
    seen[0]=foe({1,3,1},1100);selected=*memory.observe(seen,1000,2000);
    check(selected.target==second&&!memory.remembers(seen[0].target));
    seen[1].distance.reset();selected=*memory.observe(seen,1000,2000);
    check(!selected.target&&selected.engaged&&memory.remembers(second));
    seen[1].distance=2000;selected=*memory.observe(seen,1000,2000);
    check(selected.target==second); // follow deadline inclusive
    seen[1].alive=false;selected=*memory.observe(seen,1000,2000);
    check(!selected.target&&!selected.engaged&&memory.size()==0);
    seen={foe(first,500),foe(second,500)};selected=*memory.observe(seen,1000,2000);
    check(selected.target==first); // deterministic authored equal-distance tie
    auto malformed=seen;malformed[1]=malformed[0];
    check(!memory.observe(malformed,1000,2000)&&memory.size()==2);
    check(!memory.observe(seen,1000,999)&&memory.size()==2);
    seen[0].target.generation++;seen[0].distance=1500;
    seen[1].published=false;selected=*memory.observe(seen,1000,2000);
    check(!selected.target&&!selected.engaged&&!memory.remembers(first));
    seen={foe(first,500),foe(second,500)};check(memory.observe(seen,1000,2000)->engaged);
    check(!memory.observe({},1000,2000)->engaged&&memory.size()==0);
    check(memory.observe(seen,1000,2000)->engaged);memory.clear();check(memory.size()==0);
    std::cout<<"NPC per-lifetime shared aggro memory passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
