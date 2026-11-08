#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Mind Wrack target line "+std::to_string(at.line()));}
int main()try{
 for(bool pvp:{false,true})for(unsigned energy:{0u,1u,2u}){world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==2?60:0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{500,500,id==2?energy:100,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2),other=*w.effect_scope(3);check(w.energy_capacity(2,5,0,100,{80,-2})&&w.apply_mind_wrack(a,b,12,pvp,100));
  check(w.mind_wrack_targeted({other,b,42,1,200,true})&&w.resources(2)->energy==energy&&w.resources(2)->health==500);
  check(w.mind_wrack_targeted({a,b,28,1,200,true})&&w.resources(2)->energy==energy); // Hex does not qualify.
  check(w.mind_wrack_targeted({a,b,42,2,200,true}));const unsigned damage=energy?(pvp?0u:21u)+(energy==1?83u:0u):0u;
  check(w.resources(2)->energy==(energy?energy-1:0)&&w.resources(2)->health==500-damage&&w.status_snapshot(b,200)->size()==(energy==1?0u:1u));
  check(!w.mind_wrack_targeted({a,b,42,2,201,true})&&w.resources(2)->health==500-damage);
  check(!w.mind_wrack_targeted({a,b,42,3,199,true})&&!w.mind_wrack_targeted({{a.instance,a.actor,a.generation+1},b,42,3,201,true}));
 }
 // Flat protection applies to each actual damage component; an Energy-debt
 // UI zero is not a zero transition or a successful one-Energy loss.
 for(bool debt:{false,true})for(bool pvp:{false,true}){world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,id==2?1u:100u,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.energy_capacity(2,5,0,100,{80,-2}));if(debt)check(w.energy_capacity(2,5,0,100,{70,-2}));else check(w.protective_enchantment(b,b,*skills::regeneration::reviewed(299,0),0,100));check(w.apply_mind_wrack(a,b,12,pvp,100)&&w.mind_wrack_targeted({a,b,42,1,200,true}));check(w.resources(2)->energy==0&&w.resources(2)->health==(debt?500u:pvp?420u:402u));const auto status=*w.status_snapshot(b,200);check(std::count_if(status.begin(),status.end(),[](const auto& e){return e.skill==49||e.skill==2734;})==(debt?1:0));}
 // A no-hex confirmed use still consumes its source event ID.
 {world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,10,100},100);}const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.mind_wrack_targeted({a,b,42,7,100,true})&&w.apply_mind_wrack(a,b,12,false,200));check(!w.mind_wrack_targeted({a,b,42,7,200,true})&&w.resources(2)->energy==10);}
 for(bool negative:{false,true})for(bool pvp:{false,true}){
  world::Instance w;for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,id==1?30u:1u,30},100);}
  const auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.energy_capacity(2,6,0,100,{10,-2})&&w.activate_overcast_energy(b,231,0,1,1000));
  if(negative)for(unsigned event=2;event<=4;++event)check(w.activate_overcast_energy(b,231,0,event,1000));
  check(w.energy_capacity(2,6,0,1000,{10,2})&&w.settle_resources(b,1500)&&w.apply_mind_wrack(a,b,12,pvp,1500));
  const auto before=*w.overcast_energy_snapshot(b);check(w.mind_wrack_targeted({a,b,42,1,1500,true}));const auto after=*w.overcast_energy_snapshot(b);
  check(overcast::precise_current(after)==overcast::precise_current(before)-(negative?0:overcast::energy_unit));
  check(after.curve.burden_units()==before.curve.burden_units()&&after.resources.health==(negative||pvp?500u:479u));
  check(w.status_snapshot(b,1500)->size()==1); // Positive fraction remains; no zero burst.
  check(!w.mind_wrack_targeted({a,b,42,1,1500,true}));
 }
 const std::string hash(64,'a');std::istringstream a("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 60 0 0 - -\n");std::istringstream m("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/mind_wrack_target.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n");auto pop=world::load_npc_catalog(a);world::bind_combat_catalog(pop,combat::npcs::load_catalog(m,hash),hash);
 for(bool known:{false,true})for(bool pvp:{false,true}){auto defs=pop.for_map(146);if(known){defs[0].combat_definition->spawn_energy=combat::npcs::Energy{1,20};defs[0].combat_definition->energy_pips=0;}world::Instance w;w.configure_npcs(defs);w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);const auto c=*w.effect_scope(1),n=*w.effect_scope(100);check(w.apply_mind_wrack(c,n,12,pvp,100));check(w.mind_wrack_targeted({c,n,42,1,200,true})==known);check(w.npc_combat_snapshot(n)->health->current==(known?(pvp?417.:396.):500.));check(bool(w.npc_combat_snapshot(n)->effects&0x800)==!known);if(known)check(!w.mind_wrack_targeted({c,n,42,1,200,true}));}
 std::cout<<"PASS Mind Wrack actual confirmed target loss, separate PvE/PvP components, zero/one/two Energy, wrong caster/hex/retired/replayed events and known/unknown NPC Energy\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
