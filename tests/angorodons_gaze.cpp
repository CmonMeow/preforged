#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_angorodons_gaze.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Angorodon's Gaze line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;effects::Scope source,target,cause;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();bool npc;
 Fixture(bool is_npc,unsigned health=500,unsigned energy=50):npc(is_npc){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),500};d.damageable=d.conditions_allowed=true;
   n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Necromancer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,500,energy,100},100);
  check(w.energy_capacity(1,4,0,100,{80,-2}));check(w.resources(1)->energy==energy&&w.resources(1)->max_energy==100);
  w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{health,500,100,100},100);
  source=*w.effect_scope(1);cause=*w.effect_scope(2);target=*w.effect_scope(npc?100:2);box->drain();
 }
 void condition(std::uint64_t at,std::uint64_t duration){check(w.apply_condition(cause,source,combat::conditions::Kind::blind,at,duration));}
 double health(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
};
int main()try{
 check(!effects::angorodons_gaze(21));
 for(unsigned rank=0;rank<=20;++rank){const auto p=*effects::angorodons_gaze(rank);combat::conditions::State state;
  check(effects::angorodons_gaze_energy(p,state,1000)==0);
  check(state.apply(combat::conditions::Kind::blind,2,1000,1000));
  check(effects::angorodons_gaze_energy(p,state,1000)==p.energy&&effects::angorodons_gaze_energy(p,state,2000)==0);
  auto forged=p;++forged.energy;check(!effects::angorodons_gaze_energy(forged,state,1000));
 }
 for(bool npc:{false,true})for(unsigned rank=0;rank<=20;++rank)for(bool conditioned:{false,true}){
  Fixture f(npc);if(conditioned)f.condition(500,5000);const auto plan=*effects::angorodons_gaze(rank);
  const auto live=*effects::life_steal(2189,rank);check(live.amount==plan.amount&&live.energy_if_condition==plan.energy);
  std::vector<world::Packet> out;check(f.w.life_steal(f.source,f.target,live,1000,&out));
  check(f.health()==500-plan.amount&&f.w.resources(1)->health==100+plan.amount);
  check(f.w.resources(1)->energy==50+(conditioned?plan.energy:0));
  check(out==std::vector<world::Packet>{*particles::targeted_body_2022(f.target.actor,1,2189)});
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<3;++mode){Fixture f(npc,mode==2?4:500,mode==1?100:50);
  f.condition(1000,mode==0?1000:5000);check(f.w.life_steal(f.source,f.target,*effects::life_steal(2189,12),2000));
  check(f.w.resources(1)->energy==(mode==0?50u:mode==1?100u:60u));
  if(mode==2)check(f.health()==0&&f.w.resources(1)->health==105);
 }
 for(bool npc:{false,true}){
  // A hex alone is not a condition,and life stealing is not received healing.
  Fixture f(npc);check(f.w.degeneration_hex(f.cause,f.source,*effects::degeneration_hex(785,12),1000));
  check(f.w.life_steal(f.source,f.target,*effects::life_steal(2189,12),2000));
  check(f.w.resources(1)->energy==50&&f.w.resources(1)->health==142);
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<3;++mode){Fixture f(npc);f.condition(500,5000);
  auto target=f.target;auto plan=*effects::life_steal(2189,12);
  if(mode==0)++target.generation;if(mode==1)++plan.energy_if_condition;if(mode==2)target=f.source;
  check(!f.w.life_steal(f.source,target,plan,1000));
  check(f.health()==500&&f.w.resources(1)->energy==50&&f.w.resources(1)->health==100);
 }
 for(bool npc:{false,true}){
  Fixture f(npc,500,20);f.condition(500,5000);
  check(f.w.consume_energy(1,5,f.source,1000)); //Already paid for the activating skill.
  check(f.w.activate_overcast_energy(f.source,231,0,1,1500));
  check(f.w.energy_capacity(1,4,0,1500,{50,-2})); //100->70preserves deficit15->-15.
  auto before=f.w.overcast_energy_snapshot(f.source);check(before&&overcast::precise_current(*before)==-15*std::int64_t(overcast::energy_unit));
  check(f.w.life_steal(f.source,f.target,*effects::life_steal(2189,12),2000));
  const auto after=f.w.overcast_energy_snapshot(f.source);check(after&&overcast::precise_current(*after)==-5*std::int64_t(overcast::energy_unit));
 }
 for(bool npc:{false,true}){
  Fixture reward(npc),control(npc);
  for(auto* f:{&reward,&control}){check(f->w.energy_capacity(1,4,0,100,{80,-1}));check(f->w.activate_overcast_energy(f->source,231,0,1,1000));}
  reward.condition(1500,5000);
  check(reward.w.life_steal(reward.source,reward.target,*effects::life_steal(2189,12),2000));
  check(control.w.life_steal(control.source,control.target,*effects::life_steal(2189,12),2000));
  check(reward.w.settle_resources(reward.source,2000)&&control.w.settle_resources(control.source,2000));
  const auto a=reward.w.overcast_energy_snapshot(reward.source),b=control.w.overcast_energy_snapshot(control.source);
  check(a&&b&&overcast::precise_current(*a)-overcast::precise_current(*b)==10*std::int64_t(overcast::energy_unit));
  check(a->regeneration.remainder()==b->regeneration.remainder()&&a->fractional_tail==b->fractional_tail);
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){Fixture f(npc);casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0;
  h.resources=[&](auto id){return f.w.resources(id);};std::vector<world::Packet> events;
  h.events=[&](const auto& p){events.insert(events.end(),p.begin(),p.end());};
  h.consume_energy_at=[&](unsigned cost,auto at){check(cost==5);return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return id==f.target.actor&&range==1248;};
  h.life_steal=[](auto,const auto&,auto){throw std::runtime_error("Angorodon bypassed typed completion");return false;};
  if(mode!=3)h.complete_life_steal=[&](auto target,const auto& p,auto at)->casting::EffectCompletion{
   ++calls;std::vector<world::Packet> out;const bool accepted=f.w.life_steal(f.source,target,p,at,&out);return {accepted,std::move(out)};};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[2189]=1;Slots bar{};bar[0]={2189,0};attributes::Ranks ranks{};ranks[6]=12;
  if(mode==5)f.condition(500,1500); //Ends exactly at completion.
  check(runtime.start(1000,{2189,0,f.target.actor},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
  check(f.w.resources(1)->energy==(mode==3?50u:45u));if(mode==3)continue;
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1500,h);if(mode==4)h.complete_life_steal={};if(mode==6)f.condition(1500,5000);
  runtime.update(1999,h);check(!calls);runtime.update(2000,h);runtime.update(2000,h);
  const bool success=mode==0||mode==5||mode==6;check(calls==unsigned(success));
  check(f.w.resources(1)->energy==(mode==6?55u:45u));
  const auto body=*particles::targeted_body_2022(f.target.actor,1,2189);check(std::count(events.begin(),events.end(),body)==unsigned(success));
  if(success)check(std::find(events.begin(),events.end(),casting::terminal(58,1))<std::find(events.begin(),events.end(),body));
 }
 {Fixture f(true);f.condition(1000,10000);
  check(f.w.degeneration_hex(f.cause,f.target,*skills::effects::degeneration_hex(31,0),3000));
  const auto energy=f.w.resources(1)->energy,health=f.w.resources(1)->health;
  const auto target_health=f.w.npc_combat_snapshot(f.target)->health->current;
  check(!f.w.life_steal(f.source,f.target,*skills::effects::life_steal(2189,12),2000));
  check(f.w.resources(1)->energy==energy&&f.w.resources(1)->health==health&&
    f.w.npc_combat_snapshot(f.target)->health->current==target_health);
 }
 std::cout<<"PASS Angorodon's Gaze ranks,completioncondition/expiry,playerNPC transfer/Energy/full/lethal and actual typed Runtime completion\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
