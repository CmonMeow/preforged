#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location where=std::source_location::current()){
 if(!value)throw std::runtime_error("Whirlwind line "+std::to_string(where.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source;
 Fixture(unsigned unknown=0,std::optional<bool> susceptible=true,unsigned hp=1000){
  std::vector<world::Instance::Npc> population;
  for(unsigned id=100;id<=105;++id){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?166:id==101?167:id==102?252:id==103?253:100;n.plane=id==105?1:0;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
   d.relation=id==104?combat::npcs::Relation::protected_ally:combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{double(hp),1000};d.damageable=id!=104;d.conditions_allowed=false;
   if(id!=unknown){d.cold_armor=60;d.cold_armor_evidence="test";}
   d.knockdown_allowed=susceptible;d.knockdown_evidence=susceptible.has_value()?"test":"";
   n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);
  for(unsigned id=1;id<=6;++id){
   const float x=id==1?0:id==2?166:id==3?167:id==4?252:id==5?253:100;
   w.join(id,"Player",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==6?1:2);
   w.set_vitals(id,{hp,1000,100,100},100);
   check(w.elemental_armor(*w.effect_scope(id),60,id==unknown?std::nullopt:std::optional<int>{60}));
  }
  source=*w.effect_scope(1);box->drain();
 }
 skills::effects::Scope scope(unsigned id){return *w.effect_scope(id);}
 double health(unsigned id){return id>=100?w.npc_combat_snapshot(scope(id))->health->current:double(w.resources(id)->health);}
};
int main()try{
 using namespace skills;using Action=combat::npcs::control::Action;
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=*effects::elemental(163,rank);check(plan.kind==combat::Kind::cold&&plan.damage==15+4*rank&&plan.radius==166&&plan.caster_center);
 }
 check(!effects::elemental(163,21));
 {
  const auto packet=particles::whirlwind_target_2022(29,25);
  check(packet==world::Packet({159,0,20,0,0,0,29,0,0,0,25,0,0,0,32,1,0,0}));
 }
 for(bool expanded:{false,true})for(unsigned rank:{0u,12u,20u}){
  Fixture f;const auto plan=*effects::elemental(163,rank);std::vector<world::Packet> visuals;
  if(expanded)check(f.w.activate_overcast_energy(f.source,219,0,1,200));
  check(f.w.elemental(f.source,f.source,plan,20,1000,&visuals));
  for(unsigned id:{2u,3u,4u,5u,6u,100u,101u,102u,103u,104u,105u}){
   const bool hit=id==2||id==100||(expanded&&(id==3||id==4||id==101||id==102));
    if(f.health(id)!=1000-(hit?plan.damage:0))std::cerr<<"recipient "<<id<<" rank "<<rank<<" expanded "<<expanded<<" HP "<<f.health(id)<<'\n';
    check(f.health(id)==1000-(hit?plan.damage:0));check(!f.w.knocked_down(f.scope(id),1000));
  }
  check(visuals.size()==(expanded?6:2));
 }
 // An attack windup qualifies; a spell,completed attack or idle actor does not.
 for(unsigned mode=0;mode<4;++mode){
  Fixture f;const auto target=f.scope(100);
  if(mode){const auto token=f.w.begin_npc_action(target,mode==2?Action::spell:Action::attack,200,mode==3?500:2000);check(bool(token));
   if(mode==3)check(f.w.complete_npc_action(*token,500));}
  if(mode==1)check(f.w.attack_activity(f.scope(2),2000,200));
  check(f.w.elemental(f.source,f.source,*effects::elemental(163,12),20,1000));
  check(f.w.knocked_down(target,1000)==(mode==1));check(f.w.knocked_down(f.scope(2),1000)==(mode==1));
  if(mode==1)check(f.w.knocked_down(target,2999)&&!f.w.knocked_down(target,3000));
 }
 for(unsigned mode=0;mode<3;++mode){
  Fixture f(0,mode==0?std::optional<bool>{false}:mode==1?std::optional<bool>{}:std::optional<bool>{true},mode==2?10:1000);
  check(bool(f.w.begin_npc_action(f.scope(100),Action::attack,200,2000)));
  check(f.w.elemental(f.source,f.source,*effects::elemental(163,12),20,1000)==(mode!=1));
  check(!f.w.knocked_down(f.scope(100),1000));check(f.health(100)==(mode==0?937:mode==1?1000:0));
  if(mode==1)check(f.health(2)==1000);
 }
 for(unsigned unknown:{2u,100u}){
  Fixture f(unknown);std::vector<world::Packet> visuals{{1,2}};
  check(!f.w.elemental(f.source,f.source,*effects::elemental(163,12),20,1000,&visuals));
  check(visuals.empty()&&f.health(2)==1000&&f.health(100)==1000);
 }
 {
  Fixture f;check(f.w.activate_overcast_energy(f.source,219,0,1,200));
  // The current settled ceiling,not the mere existence of an old curve,
  // determines expansion at completion.
  check(f.w.elemental(f.source,f.source,*effects::elemental(163,12),20,100000));
  check(f.health(100)==937&&f.health(101)==1000&&f.health(102)==1000);
 }
 {
  Fixture f;auto forged=*effects::elemental(163,12);forged.radius=5000;
  check(!f.w.elemental(f.source,f.source,forged,20,1000));
  check(!f.w.elemental(f.source,f.scope(2),*effects::elemental(163,12),20,1000));
  auto stale=f.source;++stale.generation;check(!f.w.elemental(stale,stale,*effects::elemental(163,12),20,1000));
 }
 // Full dispatcher: self normalization,deadline,cost,scope,cancel and visual ownership.
 for(unsigned mode=0;mode<7;++mode){
  Fixture f(mode==6?100:0);casting::Hooks h;unsigned commits=0;bool stale=false;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_valid=[](unsigned id,float,bool self){return id==1&&self;};
  h.target_scope=[&](unsigned id){if(stale)return std::optional<effects::Scope>{};return f.w.effect_scope(id);};
  if(mode!=4)h.elemental=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{particles::whirlwind_target_2022(2,1)}};
   std::vector<world::Packet> visuals;const bool accepted=f.w.elemental(f.source,target,plan,20,at,&visuals);return {accepted,std::move(visuals)};
  };
  Slots bar{};bar[0]={163,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[163]=1;attributes::Ranks ranks{};ranks[8]=12;
  casting::Runtime cast(1);check(cast.start(1000,{163,0,100},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(f.w.resources(1)->energy==(mode==4?100:95));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1749,h);check(!commits);cast.update(1750,h);check(commits==unsigned(mode==0||mode>=5));
  check(f.health(100)==(mode==0?937:1000));
  const auto packets=f.box->drain();const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  const auto caster=*casting::success_body_effect(1,163);const auto visual=particles::whirlwind_target_2022(100,1);
  check(std::count(packets.begin(),packets.end(),visual)==unsigned(mode==0));
  if(mode==0)check(finish<std::find(packets.begin(),packets.end(),caster)&&std::find(packets.begin(),packets.end(),caster)<std::find(packets.begin(),packets.end(),visual));
  else check(finish==packets.end()&&std::find(packets.begin(),packets.end(),caster)==packets.end());
 }
 std::cout<<"PASS Whirlwind mixed recipients,Overcast boundaries,attack-state control,immunity,atomic rejection and real casts\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
