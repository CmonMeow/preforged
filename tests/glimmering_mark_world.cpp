#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Glimmering World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;Scope caster,target;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool npc,float x=100,double health=480){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=x;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{health,480};
   d.damageable=d.conditions_allowed=true;d.fleshy=true;d.fleshy_evidence="synthetic Glimmering recipient";
   n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Mark caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);caster=*w.effect_scope(1);
  if(!npc){w.join(2,"Marked foe",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{480,480,100,100},100);}
  target=*w.effect_scope(npc?100:2);box->drain();
 }
};
int main()try{unsigned cases=0;
 for(bool npc:{false,true})for(unsigned rank=0;rank<=20;++rank){
  Fixture f(npc);const auto plan=*glimmering_mark(rank);
  check(f.w.stage_glimmering_mark(f.caster,f.target,plan,20,100));
  const auto owned=f.w.glimmering_mark_snapshot(f.target,100);
  check(owned&&owned->caster==f.caster&&owned->target==f.target&&owned->level==20&&owned->team==1&&owned->plan==plan);
  const auto body=*skills::particles::targeted_body_2022(f.target.actor,1,227);
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),body)==1);
  check(f.w.stage_glimmering_mark(f.caster,f.target,plan,20,500));
  const auto renewed=f.w.glimmering_mark_snapshot(f.target,500);check(renewed&&renewed->effect!=owned->effect&&renewed->applied==500);
  const auto effects=f.w.status_snapshot(f.target,500);check(effects&&std::count_if(effects->begin(),effects->end(),[](const auto& e){return e.skill==227;})==1);
  check(!f.w.glimmering_mark_snapshot(f.target,10500));++cases;
 }
 for(bool npc:{false,true}){
  Fixture f(npc);auto plan=*glimmering_mark(12);++plan.damage;
  check(!f.w.stage_glimmering_mark(f.caster,f.target,plan,20,100)&&!f.w.glimmering_mark_snapshot(f.target,100));
  plan=*glimmering_mark(12);auto bad=f.target;++bad.generation;
  check(!f.w.stage_glimmering_mark(f.caster,bad,plan,20,100));bad=f.caster;++bad.generation;
  check(!f.w.stage_glimmering_mark(bad,f.target,plan,20,100));
  check(f.w.stage_glimmering_mark(f.caster,f.target,plan,20,100));
  f.w.join(3,"New caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},200,3);f.w.set_vitals(3,{500,1000,100,100},100);
  const auto next=*f.w.effect_scope(3);check(f.w.stage_glimmering_mark(next,f.target,plan,20,500));
  check(f.w.glimmering_mark_snapshot(f.target,500)->caster==next);
  Fixture far(npc,1249);check(!far.w.stage_glimmering_mark(far.caster,far.target,plan,20,100));
 }
 {Fixture f(false);check(f.w.hex_breaker(f.target,*hex_breaker(12),20,100));
  check(f.w.stage_glimmering_mark(f.caster,f.target,*glimmering_mark(12),20,100));
  check(!f.w.glimmering_mark_snapshot(f.target,100)&&f.w.resources(1)->health<500);}
 {Fixture f(true,100,1);check(f.w.apply_condition(f.caster,f.target,combat::conditions::Kind::poison,100,10000));
  check(!f.w.stage_glimmering_mark(f.caster,f.target,*glimmering_mark(12),20,1100));
  check(f.w.npc_combat_snapshot(f.target)->health->current==0&&!f.w.glimmering_mark_snapshot(f.target,1100));}
 std::cout<<cases<<" player/NPC rank application+renewal cases; body400, identity, range, prevention and due death pass; application-layer assertions only\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
