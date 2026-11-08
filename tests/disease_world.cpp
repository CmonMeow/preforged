#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Disease World line "+std::to_string(at.line()));
}
int main()try{
 using combat::conditions::Kind;
 world::Instance w;
 for(unsigned id=1;id<=4;++id){
  const float x=id==1?1000.f:id==2?0.f:id==3?160.f:320.f;
  w.join(id,"Player",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
  w.set_vitals(id,{1000,1000,100,100},100);
 }
 const auto caster=*w.effect_scope(1),source=*w.effect_scope(2),near=*w.effect_scope(3),far=*w.effect_scope(4);
 check(w.apply_condition(caster,source,Kind::disease,100,1000));
 w.tick(200);
 check(w.has_condition(near,Kind::disease,200)&&!w.has_condition(far,Kind::disease,200));
 w.tick(300);
 check(w.has_condition(far,Kind::disease,300));
 check(!w.has_condition(near,Kind::disease,1100)&&!w.has_condition(far,Kind::disease,1100));
 // The later transfer must retain the recipient's longer independent timer.
 check(w.apply_condition(caster,source,Kind::disease,1200,1000));
 check(w.apply_condition(caster,near,Kind::disease,1200,2000));
 w.tick(1300);
 check(w.has_condition(source,Kind::disease,3000)&&!w.has_condition(source,Kind::disease,3200));
 check(w.has_condition(near,Kind::disease,3000));
 world::Instance reaction;
 for(unsigned id=1;id<=4;++id){
  const float x=id==1?1000.f:id==2?0.f:id==3?160.f:320.f;
  reaction.join(id,"Player",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
  reaction.set_vitals(id,{id==3?10u:1000u,1000,100,100},100);
 }
 const auto r_caster=*reaction.effect_scope(1),r_source=*reaction.effect_scope(2),r_near=*reaction.effect_scope(3),r_far=*reaction.effect_scope(4);
 check(reaction.degeneration_hex(r_caster,r_near,*skills::effects::degeneration_hex(19,12),100));
 check(reaction.apply_condition(r_caster,r_source,Kind::disease,100,1000));
 reaction.tick(200);check(reaction.resources(3)->health==0);
 reaction.tick(300);check(!reaction.has_condition(r_far,Kind::disease,300));
 for(unsigned mode=0;mode<7;++mode){
  world::Instance mixed;world::Instance::Npc n;n.id=n.definition.id=100;n.x=320;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1.,1000};d.damageable=d.conditions_allowed=true;
  d.fleshy=mode!=4;d.fleshy_evidence="fixture";d.creature_type=mode==3?"norn":"human";d.creature_type_evidence="fixture";
  if(mode!=1){d.disease_immune=mode==2;d.disease_immunity_evidence="fixture";}
  n.combat_definition=d;mixed.configure_npcs({n});
  for(unsigned id=1;id<=3;++id){mixed.join(id,"Player",id==1?1000.f:id==2?0.f:160.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);mixed.set_vitals(id,{1000,1000,100,100},100);}
  const auto origin=*mixed.effect_scope(1),infected=*mixed.effect_scope(2),npc=*mixed.effect_scope(100);
  check(mixed.apply_condition(origin,infected,Kind::disease,100,10000));
  mixed.tick(200);check(!mixed.has_condition(npc,Kind::disease,200));
  if(mode>=5){mixed.leave(1);mixed.join(1,"Replacement",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);mixed.set_vitals(1,{1000,1000,100,100},100);check(*mixed.effect_scope(1)!=origin);}
  mixed.tick(300);check(mixed.has_condition(npc,Kind::disease,300)==(mode==0||mode>=5));
  if(mode==6)check(mixed.condition_spell(*mixed.effect_scope(1),npc,*skills::effects::condition_spell(117,0),350));
  mixed.tick(1000);
  check(mixed.take_npc_deaths(origin).size()==unsigned(mode==0));
  if(mode>=5)check(mixed.take_npc_deaths(*mixed.effect_scope(1)).empty());
 }
 {
  world::Instance chain;std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u}){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?160.f:480.f;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{id==100?1000.:1.,1000};d.damageable=d.conditions_allowed=true;
   d.fleshy=true;d.fleshy_evidence="fixture";d.creature_type="human";d.creature_type_evidence="fixture";d.disease_immune=false;d.disease_immunity_evidence="fixture";
   n.combat_definition=d;npcs.push_back(n);
  }
  chain.configure_npcs(npcs);
  for(unsigned id=1;id<=3;++id){chain.join(id,"Player",id==1?1000.f:id==2?0.f:320.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);chain.set_vitals(id,{1000,1000,100,100},100);}
  const auto origin=*chain.effect_scope(1),infected=*chain.effect_scope(2),via=*chain.effect_scope(3),a=*chain.effect_scope(100),b=*chain.effect_scope(101);
  check(chain.apply_condition(origin,infected,Kind::disease,100,10000));
  chain.tick(200);check(chain.has_condition(a,Kind::disease,200)&&!chain.has_condition(via,Kind::disease,200));
  chain.tick(300);check(chain.has_condition(via,Kind::disease,300)&&!chain.has_condition(b,Kind::disease,300));
  chain.tick(400);check(chain.has_condition(b,Kind::disease,400));
  chain.tick(1000);check(chain.take_npc_deaths(origin).size()==1);
  check(!chain.has_condition(via,Kind::disease,10100));
 }
 std::cout<<"PASS World tick Disease spread,snapshot chains,absolute expiry and longest existing infection; reviewed NPC recipients,unknown/immune/nonflesh/type rejection and original credit across chains/reused actor IDs\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
