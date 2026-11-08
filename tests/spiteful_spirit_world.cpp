#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Spiteful Spirit World line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<5;++mode){
  world::Instance w;
  for(unsigned id=1;id<=3;++id){w.join(id,"Actor",id==2?60.f:0.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(2),other=*w.effect_scope(3);std::vector<world::Packet> visuals;
  if(mode==3)check(w.hex_breaker(target,*hex_breaker(12),20,100));
  check(w.spiteful_spirit(caster,target,*spiteful_spirit(0),20,100,&visuals));
  auto entries=w.status_snapshot(target,100);check(bool(entries));
  if(mode==3){check(entries->empty()&&visuals.empty()&&w.resources(1)->health==961);continue;}
  check(entries->size()==1&&entries->front().skill==121&&entries->front().cause==1&&entries->front().expires==8100);
  check(visuals==std::vector<world::Packet>{*skills::particles::targeted_body_2022(2,1,121)});
  const auto effect=entries->front().instance;
  check(!w.status_mutation(target,121,999,0,100,8100,true));
  check(!w.status_mutation(target,299,effect,0,100,8100,true));
  if(mode==1){check(w.status_mutation(target,121,effect,0,200,std::nullopt,false));check(w.status_snapshot(target,200)->empty());}
  if(mode==2){
   check(w.spiteful_spirit(other,target,*spiteful_spirit(12),20,1000,&visuals));entries=w.status_snapshot(target,1000);
   check(entries&&entries->size()==1&&entries->front().instance!=effect&&entries->front().cause==3&&entries->front().rank==12&&entries->front().expires==19000);
   w.leave(1);w.tick(8100);check(w.status_snapshot(target,8100)->size()==1);w.tick(19000);check(w.status_snapshot(target,19000)->empty());
  }else if(mode==4){
   check(w.hex_breaker(target,*hex_breaker(12),20,200));check(w.spiteful_spirit(other,target,*spiteful_spirit(12),20,300,&visuals));
   entries=w.status_snapshot(target,300);check(entries&&entries->size()==1&&entries->front().instance==effect&&entries->front().applied==100&&visuals.empty());
  }else{w.tick(8100);check(w.status_snapshot(target,8100)->empty());}
  check(w.resources(2)->health==1000); // This stage adds no invented degeneration/reaction damage.
 }
 for(unsigned mode=0;mode<7;++mode){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Target",mode==2?1300.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,mode==1?1:2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto caster=*w.effect_scope(1);auto target=*w.effect_scope(mode==0?1:2);if(mode==3)++target.generation;
  auto plan=*spiteful_spirit(0);if(mode==4)++plan.damage;
  check(!w.spiteful_spirit(caster,target,plan,mode==5?0u:mode==6?256u:20u,100));check(w.status_snapshot(*w.effect_scope(2),100)->empty());
 }
 for(unsigned mode=0;mode<4;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=mode==1?combat::npcs::Relation::neutral:combat::npcs::Relation::hostile_to_players;
  if(mode!=2)d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=mode!=2;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(100);auto chosen=target;if(mode==3)++chosen.generation;
  check(w.spiteful_spirit(caster,chosen,*spiteful_spirit(0),20,100)==(mode==0));
  if(mode==0){check(w.npc_combat_snapshot(target)->effects&0x800);w.tick(8100);check(!(w.npc_combat_snapshot(target)->effects&0x800)&&w.npc_combat_snapshot(target)->health->current==1000);}
 }
 {
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  const combat::npcs::Lifetime life{1,1,7,146,100};combat::npcs::LiveActor live(d,life);
  check(live.apply_spiteful_spirit(life,*spiteful_spirit(0),1,1,100));
  check(live.health_clock(life)==100&&live.advance_conditions(life,100).has_value());
  check(!live.apply_spiteful_spirit(life,*spiteful_spirit(12),1,2,200));
  check(live.advance_conditions(life,200).has_value());check(live.apply_spiteful_spirit(life,*spiteful_spirit(12),1,2,200));
  check(!live.hex_effect(life,1)&&live.hex_effect(life,2)->expires==18200);
  check(live.remove_hex_effect(life,2,200)&&!live.hex_effect(life,2));
 }
 std::cout<<"PASS Spiteful Spirit owned player/NPC hex,expiry/removal/renewal,prevention,ID ownership,settled-NPC clock and invalid target facts;reaction/Runtime remain unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
