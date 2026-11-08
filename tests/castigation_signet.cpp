#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_castigation_signet.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Castigation Signet line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;effects::Scope source,target;bool npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool is_npc,bool light=false,unsigned health=1000,bool known=true,unsigned energy=50):npc(is_npc){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),1000};d.damageable=d.conditions_allowed=true;
   if(known){d.holy_traits.light_sensitive=light;d.light_sensitive_evidence="fixture";}n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Monk",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,energy,100},100);
  check(w.energy_capacity(1,3,0,100,{80,-2})&&w.resources(1)->energy==energy&&w.resources(1)->max_energy==100);
  if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{health,1000,100,100},100);
   if(known)check(w.player_holy_equipment(*w.effect_scope(2),0));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
 void action(unsigned mode){if(!mode)return;const auto until=mode==6?1500u:mode>=3?1000u:3000u;
  if(npc){const auto token=w.begin_npc_action(target,mode==2?combat::npcs::control::Action::spell:combat::npcs::control::Action::attack,200,until);check(bool(token));if(mode==3)check(w.complete_npc_action(*token,1000));}
  else if(mode==2)check(w.cast_activity(target,194,200,3000));else{check(w.attack_activity(target,until,200));if(mode==3)check(w.attack_activity(target,{},1000));}}
 double health(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
};
int main()try{
 check(!effects::castigation_signet(21));
 for(unsigned rank=0;rank<=20;++rank){const auto p=*effects::castigation_signet(rank);check(p.damage==26+2*rank&&effects::castigation_energy(p,true)==p.energy&&effects::castigation_energy(p,false)==0);
  auto forged=p;++forged.energy;check(!effects::castigation_energy(forged,true));}
 for(bool npc:{false,true})for(bool light:{false,true})for(unsigned mode=0;mode<5;++mode)for(unsigned rank:{0u,12u,20u}){
  if(!npc&&light)continue;Fixture f(npc,light);f.action(mode);const auto p=*effects::castigation_signet(rank);const auto live=*effects::holy_damage(2006,rank);
  check(live.damage==p.damage&&live.energy_if_attacking==p.energy);std::vector<world::Packet> out;
  check(f.w.holy_damage(f.source,f.target,live,20,1000,&out));
  check(f.health()==1000-p.damage*(light?2:1)&&f.w.resources(1)->energy==50+(mode==1?p.energy:0));
  check(out==std::vector<world::Packet>{*particles::targeted_body_2022(f.target.actor,1,2006)});
 }
 for(bool npc:{false,true}){Fixture f(npc,false,10);f.action(1);check(f.w.holy_damage(f.source,f.target,*effects::holy_damage(2006,12),20,1000));check(f.health()==0&&f.w.resources(1)->energy==58);}
 for(bool npc:{false,true}){Fixture f(npc,false,1000,false);f.action(1);check(!f.w.holy_damage(f.source,f.target,*effects::holy_damage(2006,12),20,1000));check(f.health()==1000&&f.w.resources(1)->energy==50);}
 for(bool npc:{false,true}){Fixture f(npc,false,1000,true,100);f.action(1);check(f.w.holy_damage(f.source,f.target,*effects::holy_damage(2006,12),20,1000));check(f.w.resources(1)->energy==100);}
 for(bool npc:{false,true}){Fixture f(npc,false,1000,true,20);f.action(1);
  check(f.w.activate_overcast_energy(f.source,231,0,1,1500)&&f.w.energy_capacity(1,3,0,1500,{50,-2}));
  const auto before=f.w.overcast_energy_snapshot(f.source);check(before&&overcast::precise_current(*before)==-10*std::int64_t(overcast::energy_unit));
  check(f.w.holy_damage(f.source,f.target,*effects::holy_damage(2006,12),20,2000));
  const auto after=f.w.overcast_energy_snapshot(f.source);check(after&&overcast::precise_current(*after)==-2*std::int64_t(overcast::energy_unit));}
 for(bool npc:{false,true}){Fixture reward(npc),control(npc);reward.action(1);
  for(auto* f:{&reward,&control}){check(f->w.energy_capacity(1,3,0,100,{80,-1}));check(f->w.activate_overcast_energy(f->source,231,0,1,1000));}
  check(reward.w.holy_damage(reward.source,reward.target,*effects::holy_damage(2006,12),20,2000));
  check(control.w.holy_damage(control.source,control.target,*effects::holy_damage(2006,12),20,2000));
  check(reward.w.settle_resources(reward.source,2000)&&control.w.settle_resources(control.source,2000));
  const auto a=reward.w.overcast_energy_snapshot(reward.source),b=control.w.overcast_energy_snapshot(control.source);
  check(a&&b&&overcast::precise_current(*a)-overcast::precise_current(*b)==8*std::int64_t(overcast::energy_unit));
  check(a->regeneration.remainder()==b->regeneration.remainder()&&a->fractional_tail==b->fractional_tail);}
 for(bool npc:{false,true})for(bool forged:{false,true}){Fixture f(npc);f.action(1);auto target=f.target;auto plan=*effects::holy_damage(2006,12);
  if(forged)++plan.energy_if_attacking;else ++target.generation;
  std::vector<world::Packet> out;check(!f.w.holy_damage(f.source,target,plan,20,1000,&out));
  check(f.health()==1000&&f.w.resources(1)->energy==50&&out.empty());}
 {Fixture f(true);f.w.join(3,"Other source",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  f.w.set_vitals(3,{1000,1000,100,100},100);const auto other=*f.w.effect_scope(3);
  check(f.w.degeneration_hex(other,f.target,*effects::degeneration_hex(31,0),3000));
  const auto energy=f.w.resources(1)->energy,health=f.w.resources(1)->health;const auto target_health=f.health();
  std::vector<world::Packet> out;check(!f.w.holy_damage(f.source,f.target,*effects::holy_damage(2006,12),20,2000,&out));
  check(f.w.resources(1)->energy==energy&&f.w.resources(1)->health==health&&f.health()==target_health&&out.empty());}
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){Fixture f(npc);f.action(mode==5?6:1);casting::Hooks h;casting::Runtime runtime(1);unsigned calls=0;std::vector<world::Packet> sent;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){sent.insert(sent.end(),p.begin(),p.end());};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return id==f.target.actor&&range==1248;};
  h.consume_energy_at=[&](auto cost,auto now){check(cost==0);return f.w.consume_energy(1,cost,f.source,now);};
  if(mode!=3)h.holy_damage=[&](auto target,const auto& p,auto now)->casting::EffectCompletion{++calls;std::vector<world::Packet> out;const bool ok=f.w.holy_damage(f.source,target,p,20,now,&out);return {ok,std::move(out)};};
  Slots bar{};bar[0]={2006,0};Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[2006]=1;attributes::Ranks ranks{};ranks[14]=12;
  const auto started=runtime.start(1000,{2006,0,f.target.actor},bar,policy,ranks,true,h);
  const auto expected=mode==3?casting::Reason::unsupported:casting::Reason::accepted;
  if(started!=expected)throw std::runtime_error("Castigation start npc="+std::to_string(npc)+" mode="+std::to_string(mode)+" reason="+std::to_string(static_cast<unsigned>(started))+" expected="+std::to_string(static_cast<unsigned>(expected)));
  check(f.w.resources(1)->energy==50);if(mode==3)continue;if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1500,h);if(mode==4)h.holy_damage={};
  runtime.update(1999,h);check(!calls);runtime.update(2000,h);runtime.update(2000,h);const bool success=mode==0||mode==5;check(calls==unsigned(success));
  check(f.w.resources(1)->energy==(mode==0?58u:50u));const auto body=*particles::targeted_body_2022(f.target.actor,1,2006);
  check(std::count(sent.begin(),sent.end(),body)==unsigned(success));if(success)check(std::find(sent.begin(),sent.end(),casting::terminal(58,1))<std::find(sent.begin(),sent.end(),body));
  if(mode==0){check(runtime.start(2750,{2006,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);
   check(runtime.start(21999,{2006,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::recharge);
   check(runtime.start(22000,{2006,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);}
 }
 std::cout<<"PASS Castigation Signet holy traits,attack-time Energy,idle/spell/completed/exact-end,lethal/unknown traits and typed Runtime completion\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
