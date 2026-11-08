#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mind Wrack resource event line "+std::to_string(at.line()));}
int main()try{
 for(bool fractional:{false,true})for(bool later_caster:{false,true}){world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,id==1?100u:1u,100},100);}
  const auto source=*w.effect_scope(1),victim=*w.effect_scope(2);
  check(w.energy_capacity(2,5,0,100,{80,fractional?-1:-2})&&w.apply_mind_wrack(source,victim,12,false,100));
  if(later_caster)check(w.settle_resources(source,1000));
  const bool payable=fractional||!later_caster;
  check(w.consume_energy(2,1,victim,600)==payable&&w.resources(2)->energy==(payable?0u:1u));
  check(w.resources(2)->health==((fractional||!payable)?500u:417u)&&w.status_snapshot(victim,600)->size()==((fractional||!payable)?1u:0u));
  if(fractional&&!later_caster){
   check(w.restore_energy(2,100,victim,600)&&w.resources(2)->energy==100);
   check(w.consume_energy(2,100,victim,600)&&w.resources(2)->energy==0&&w.resources(2)->health==417&&w.status_snapshot(victim,600)->empty());
  }
 }
 for(bool debt:{false,true}){world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,id==1?100u:debt?1u:10u,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.energy_capacity(2,5,0,100,{80,-2})&&w.apply_mind_wrack(a,b,12,false,100));check(w.energy_capacity(2,5,0,200,{70,-2}));check(w.resources(2)->energy==0&&w.resources(2)->health==(debt?500u:417u)&&w.status_snapshot(b,200)->size()==(debt?1u:0u));}
 for(bool lethal:{false,true}){world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==2&&lethal?50u:500u,500,id==1?100u:5u,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.energy_capacity(2,5,0,100,{80,-2})&&w.apply_mind_wrack(a,b,12,false,100)&&w.degeneration_hex(a,b,*degeneration_hex(66,12),100));check(w.attack_activity(b,500,200)==!lethal);check(w.resources(2)->energy==0&&w.resources(2)->health==(lethal?0u:417u));}
 {world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,id==2?10u:100u,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2),c=*w.effect_scope(3);check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));check(w.energy_capacity(2,5,0,100,{80,-2})&&w.apply_mind_wrack(a,b,12,false,100)&&w.apply_condition(a,c,combat::conditions::Kind::bleeding,100,10000));check(w.ally_cleansing(b,c,*ally_cleansing(295,0),200));check(w.resources(2)->energy==0&&w.resources(2)->health==417&&!w.has_condition(c,combat::conditions::Kind::bleeding,200));}
 const std::string hash(64,'a');std::istringstream a("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 60 0 0 - -\n");std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/mind_wrack_resource_events.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n");auto pop=world::load_npc_catalog(a);world::bind_combat_catalog(pop,combat::npcs::load_catalog(m,hash),hash);
 for(bool lethal:{false,true}){auto defs=pop.for_map(146);defs[0].combat_definition->spawn_energy=combat::npcs::Energy{5,20};defs[0].combat_definition->energy_pips=0;if(lethal)defs[0].combat_definition->spawn_health=combat::Health{50,500};world::Instance w;w.configure_npcs(defs);w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);const auto c=*w.effect_scope(1),n=*w.effect_scope(100);check(w.apply_mind_wrack(c,n,12,false,100)&&w.degeneration_hex(c,n,*degeneration_hex(66,12),100));auto token=w.begin_npc_action(n,combat::npcs::control::Action::attack,200,500);check(bool(token));check(w.npc_combat_snapshot(n)->health->current==(lethal?0.:417.));check(w.complete_npc_action(*token,500)==!lethal);w.tick(600);check(w.npc_combat_snapshot(n)->health->current==(lethal?0.:417.));}
 std::cout<<"PASS Mind Wrack equipment exact-zero/debt, player attack-hex loss, cleansing cost and NPC attack Energy journal with once-only death/token handling\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
