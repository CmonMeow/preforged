#include "gw_npc_wander.h"
#include "gw_npc_aggro_memory.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::ai;
void check(bool yes){if(!yes)throw std::runtime_error("NPC wander policy regression");}
int main()try{
    wander::Policy p{{{100,0},{0,100}},500,2000,"synthetic private patrol fixture"};
    check(p.valid(200));check(!p.valid(100));
    wander::State s(p,1000);check(!s.destination({10,20},1499));
    auto g=s.destination({10,20},1500);check(g&&g->x==110&&g->y==20);
    s.finish(1600);check(!s.destination({10,20},2099));
    g=s.destination({10,20},2100);check(g&&g->x==10&&g->y==120);
    // An unreachable leg times out, waits, then tries the next authored leg.
    check(!s.destination({10,20},4100));check(!s.destination({10,20},4599));
    g=s.destination({10,20},4600);check(g&&g->x==110);
    s.suspend(4700);check(!s.destination({10,20},5199));
    check(!s.destination({10,20},4600)); // no historical motion
    check(s.destination({10,20},5200).has_value());
    p.offsets.push_back({INFINITY,0});check(!p.valid(200));
    aggro::Memory memory({1,100,1});
    check(!memory.provoke({1,100,1}));check(!memory.provoke({2,1,1}));
    check(memory.provoke({1,1,1}));
    std::vector<aggro::Observation> foes{{{1,1,1},true,true,true,500}};
    const auto selected=memory.observe(foes,100,1000);check(selected&&selected->target.has_value());
    foes[0].distance=1100;check(!memory.observe(foes,100,1000)->engaged);
    check(!memory.observe(std::vector<aggro::Observation>{{{1,1,2},true,true,true,500}},100,1000)->target);
    std::cout<<"NPC authored wander cadence, interruption and blocked-leg retry passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
