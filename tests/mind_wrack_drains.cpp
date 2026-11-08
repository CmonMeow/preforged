#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mind Wrack drain line "+std::to_string(at.line()));}
int main()try{
 for(bool pvp:{false,true})for(bool lethal:{false,true}){world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{id==2&&lethal?5u:500u,500,id==2?1u:100u,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.energy_capacity(2,5,0,100,{80,-2})&&w.apply_mind_wrack(a,b,12,pvp,100));check(w.energy_drain(a,b,*energy_drain(42,0),20,200));check(w.resources(2)->energy==0&&w.resources(2)->health==(lethal?0u:408u)&&w.status_snapshot(b,200)->empty());}
 // Area drain components commit before zero-Energy reactions. Energy Surge
 // drains only its selected target; the adjacent damage recipient keeps Energy.
 for(unsigned skill:{39u,59u}){world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{id==2&&skill==39?5u:500u,500,id==1?100u:1u,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2),c=*w.effect_scope(3);for(unsigned id:{2u,3u})check(w.energy_capacity(id,5,0,100,{80,-2})&&w.apply_mind_wrack(a,*w.effect_scope(id),12,false,100));check(w.energy_drain(a,b,*energy_drain(skill,0),20,200));check(w.resources(2)->health==(skill==39?0u:417u)&&w.resources(3)->health==(skill==39?491u:417u)&&w.resources(3)->energy==(skill==39?1u:0u));}
 // Existing integer skill consequences must not erase fractional current
 // or negative debt when the owned Overcast state is committed.
 for(unsigned skill:{39u,42u,59u,79u,80u,40u})for(bool negative:{false,true}){
  world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,id==1?20u:3u,30},100);}
  const auto a=*w.effect_scope(1),b=*w.effect_scope(2),c=*w.effect_scope(3);
  for(auto owner:{b,c}){check(w.energy_capacity(owner.actor,6,0,100,{10,-2})&&w.activate_overcast_energy(owner,231,0,1,1000));
   if(negative)for(unsigned event=2;event<=4;++event)check(w.activate_overcast_energy(owner,231,0,event,1000));
   check(w.energy_capacity(owner.actor,6,0,1000,{10,2})&&w.settle_resources(owner,1500));}
  const auto before=*w.overcast_energy_snapshot(b),other=*w.overcast_energy_snapshot(c);const auto plan=*energy_drain(skill,0);
  const auto integer=*drain_energy(plan,before.resources);
  check(w.energy_drain(a,b,plan,20,1500));const auto after=*w.overcast_energy_snapshot(b),adjacent=*w.overcast_energy_snapshot(c);
  check(overcast::precise_current(after)==overcast::precise_current(before)-std::int64_t(integer.lost)*overcast::energy_unit);
  check(after.curve.burden_units()==before.curve.burden_units()&&after.regeneration.remainder()==before.regeneration.remainder());
  const auto area_loss=plan.drain_each?integer.lost:0;
  check(overcast::precise_current(adjacent)==overcast::precise_current(other)-std::int64_t(area_loss)*overcast::energy_unit);
 }
 const std::string hash(64,'a');std::istringstream a("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 60 0 0 - -\n");std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/mind_wrack_drains.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n");auto pop=world::load_npc_catalog(a);world::bind_combat_catalog(pop,combat::npcs::load_catalog(m,hash),hash);
 for(bool pvp:{false,true}){auto defs=pop.for_map(146);defs[0].combat_definition->spawn_energy=combat::npcs::Energy{2,20};defs[0].combat_definition->energy_pips=0;world::Instance w;w.configure_npcs(defs);w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{100,500,100,100},100);const auto c=*w.effect_scope(1),n=*w.effect_scope(100);check(w.apply_mind_wrack(c,n,12,pvp,100));check(w.energy_drain(c,n,*energy_drain(40,0),20,200));check(w.npc_combat_snapshot(n)->health->current==417&&!(w.npc_combat_snapshot(n)->effects&0x800)&&w.resources(1)->health==140);}
 std::cout<<"PASS Mind Wrack player Energy Burn and known-NPC Feast zero bursts, no own-loss bonus on external drains and no resurrection after lethal primary damage\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
