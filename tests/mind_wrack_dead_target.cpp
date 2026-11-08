#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mind Wrack dead target line "+std::to_string(at.line()));}
int main()try{const std::string hash(64,'a');std::istringstream a("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 60 0 0 - -\n");std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/mind_wrack_dead_target.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 20 60 1 1 test test test - test 60 60 test test\n");auto pop=world::load_npc_catalog(a);world::bind_combat_catalog(pop,combat::npcs::load_catalog(m,hash),hash);
 for(bool known:{false,true})for(bool pvp:{false,true}){auto defs=pop.for_map(146);if(known){defs[0].combat_definition->spawn_energy=combat::npcs::Energy{1,20};defs[0].combat_definition->energy_pips=0;}world::Instance w;w.configure_npcs(defs);w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);const auto c=*w.effect_scope(1),n=*w.effect_scope(100);check(w.apply_mind_wrack(c,n,12,pvp,100));const auto token=w.begin_npc_action(n,combat::npcs::control::Action::spell,100,2100);check(bool(token)&&w.cry_frustration(c,n,*cry_frustration(12),20,200));check(w.npc_combat_snapshot(n)->health->current==0);
  check(w.mind_wrack_targeted({c,n,57,1,200,true}));check(!w.mind_wrack_targeted({c,n,57,1,201,true}));check(w.npc_combat_snapshot(n)->health->current==0&&!w.complete_npc_action(*token,2100));
 }
 std::cout<<"PASS Mind Wrack completed killing use accepted once without a dead-NPC Energy query/reaction, PvE/PvP and known/unknown Energy\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
