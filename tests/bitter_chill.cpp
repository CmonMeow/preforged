#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Bitter Chill line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;Scope source,target;bool npc;
 Fixture(bool is_npc=false,unsigned own=500,double foe=510,unsigned ownmax=1000,unsigned foemax=1000,bool armor=true):npc(is_npc){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{foe,foemax};d.damageable=d.conditions_allowed=true;
   if(armor){d.cold_armor=60;d.cold_armor_evidence="fixture";}n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{own,ownmax,50,50},100);
  source=*w.effect_scope(1);check(w.energy_capacity(1,4,0,100,{30,-2}));
  if(!npc){w.join(2,"Target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{unsigned(foe),foemax,50,50},100);if(armor)check(w.elemental_armor(*w.effect_scope(2),{},60));}
  target=*w.effect_scope(npc?100:2);
 }
 double health()const{return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
};
int main()try{
 for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u})for(unsigned health:{499u,500u,501u,510u}){
  Fixture f(npc,500,health);auto plan=*elemental(1068,rank);ElementalOutcome result{true};std::vector<Packet> visuals;
  check(!f.w.elemental(f.source,f.target,plan,20,100)&&f.health()==health);
  auto stale=f.target;++stale.generation;check(!f.w.elemental(f.source,stale,plan,20,100,nullptr,nullptr,&result)&&!result.instant_recharge);
  check(f.w.elemental(f.source,f.target,plan,20,100,&visuals,nullptr,&result));
  check(result.instant_recharge==(health>500)&&f.health()==health-(rank==0?15:rank==12?51:75));
  check(std::count(visuals.begin(),visuals.end(),*particles::targeted_body_2022(f.target.actor,1,1068))==1);
 }
 for(bool npc:{false,true})for(bool higher:{false,true}){
  Fixture f(npc,higher?400:750,higher?450:600,higher?400:1000,higher?1000:600);ElementalOutcome result;
  check(f.w.elemental(f.source,f.target,*elemental(1068,12),20,100,nullptr,nullptr,&result)&&result.instant_recharge==higher);
 }
 {Fixture f(true,500,500.25);ElementalOutcome result;check(f.w.elemental(f.source,f.target,*elemental(1068,12),20,100,nullptr,nullptr,&result)&&result.instant_recharge&&f.health()==449.25);}
 for(bool npc:{false,true}){Fixture f(npc,10,20);ElementalOutcome result;
  check(f.w.elemental(f.source,f.target,*elemental(1068,12),20,100,nullptr,nullptr,&result)&&result.instant_recharge&&f.health()==0);}
 {Fixture f(true,500,510,1000,1000,false);ElementalOutcome result{true};check(!f.w.elemental(f.source,f.target,*elemental(1068,12),20,100,nullptr,nullptr,&result)&&!result.instant_recharge&&f.health()==510);}
 for(bool npc:{false,true})for(bool pvp:{false,true})for(unsigned mode=0;mode<9;++mode){
  Fixture f(npc,500,mode==1||mode==7?500:510);Slots bar{};bar[0]={1068,0};check(f.w.configure_adrenaline(f.source,bar,100));
  if(mode==7||mode==8)check(f.w.status_mutation(f.source,13,100,12,100,20100,true));
  Policy policy;policy.primary=4;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[1068]=1;attributes::Ranks ranks{};ranks[5]=12;
  casting::Hooks h;std::vector<Packet> packets;unsigned commits=0;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return f.w.resources(id);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float){return id==f.target.actor;};
  h.consume_energy_at=[&](auto cost,auto at){check(cost==5);return f.w.consume_energy(1,cost,f.source,at);};
  h.effect_snapshot=[&](auto at){return f.w.status_snapshot(f.source,at);};
  if(mode==6)h.elemental=[](auto,const auto&,auto)->casting::EffectCompletion{throw std::runtime_error("legacy elemental hook used for conditional cast");};
  if(mode!=6)h.conditional_elemental=[&](auto target,const auto& plan,auto at)->std::optional<casting::ConditionalElementalCompletion>{
   ElementalOutcome result;std::vector<Packet> visuals;if(!f.w.elemental(f.source,target,plan,20,at,&visuals,nullptr,&result))return {};
   ++commits;return casting::ConditionalElementalCompletion{result,std::move(visuals)};};
  casting::Runtime cast(1);check(cast.start(500,{1068,0,f.target.actor},bar,policy,ranks,true,h)==(mode==6?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==6){check(f.w.resources(1)->energy==50&&!commits);continue;}
  if(mode==2)check(f.w.heal_target(1,1,200,1248,true,1000));
  if(mode==3)check(bool(f.w.health_sacrifice(f.source,119,1000)));
  if(mode==4)cast.cancel(h,1000);if(mode==5)cast.interrupt(1000,h);
  cast.update(1500,h);const bool completed=mode<4||mode>=7,instant=mode==0||mode==3||mode==8;
  check(commits==unsigned(completed)&&f.health()==(completed?(mode==1||mode==7?449:459):510));
  check(f.w.resources(1)->energy==45);
  if(completed){
   const auto timer=casting::slot_event(228,1,{1068,0},instant?0:mode==7?7:10);
   check(std::find(packets.begin(),packets.end(),timer)!=packets.end());
   const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
   const auto target=std::find(packets.begin(),packets.end(),*particles::targeted_body_2022(f.target.actor,1,1068));check(finish<target);
   check(cast.start(2250,{1068,0,f.target.actor},bar,policy,ranks,true,h)==(instant?casting::Reason::accepted:casting::Reason::recharge));
  }
 }
 // Actual World completion provenance carries the instant-recharge exception
 // through Diversion, rather than merely hiding the ordinary timer in the UI.
 for(bool instant:{false,true}){Fixture f(false,500,instant?510:500);Slots bar{};bar[0]={1068,0};check(f.w.configure_adrenaline(f.source,bar,100));
  f.w.join(3,"Mesmer",30,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(3,{1000,1000,50,50},100);
  check(f.w.diversion(*f.w.effect_scope(3),f.source,*diversion(12),100));
  casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return f.w.resources(id);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[](auto id,float){return id==2;};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.conditional_elemental=[&](auto target,const auto& plan,auto at)->std::optional<casting::ConditionalElementalCompletion>{ElementalOutcome result;std::vector<Packet> visuals;
   if(!f.w.elemental(f.source,target,plan,20,at,&visuals,nullptr,&result))return {};return casting::ConditionalElementalCompletion{result,std::move(visuals)};};
  h.prepare_skill_use=[&](auto slot,auto event,auto at){return f.w.prepare_diversion_skill_use(f.source,slot,event,at);};
  h.finalize_skill_use=[&](auto event,auto at,auto base,auto existing,bool resets){return f.w.finalize_diversion_skill_use(f.source,event,at,base,existing,resets);};
  h.abort_skill_use=[&](auto event){return f.w.abort_diversion_skill_use(f.source,event);};
  h.cooldown_completion_source=[&](auto event){return f.w.cooldown_completion_stamp(f.source,event);};
  h.current_skillbar=[&]()->std::optional<Slots>{return bar;};h.skill_disabled_slot=[&](auto slot,auto at){return f.w.skill_disabled_slot(f.source,slot,at);};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[1068]=1;attributes::Ranks ranks{};ranks[5]=12;casting::Runtime cast(1);
  check(cast.start(500,{1068,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(1500,h);
  const auto disabled=f.w.skill_disabled_slot(f.source,{1068,0},1500);check(disabled&&(instant?*disabled<=1500:*disabled==58500));
  check(cast.start(2250,{1068,0,2},bar,policy,ranks,true,h)==(instant?casting::Reason::accepted:casting::Reason::recharge));
 }
 std::cout<<"PASS Bitter Chill current absolute precise Health before damage,strict equality,player/NPC/ranks,fractional target,missing facts/hooks,midcast changes,cancel/interrupt,conditional recharge and actual Diversion provenance\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
