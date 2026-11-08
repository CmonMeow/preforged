#include "gw_life_steal_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Life-steal Renewal line "+std::to_string(at.line())+" "+context);
}
struct Fixture {
 world::Instance w;Scope source,target;bool npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool is_npc,unsigned version,unsigned target_health=500):npc(is_npc){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(target_health),1000};
   d.damageable=d.conditions_allowed=true;d.fleshy=true;d.fleshy_evidence="synthetic Renewal recipient";
   n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{id==1?500u:target_health,1000,50,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
  if(version){const auto plan=*ether_renewal(version,0);check(w.status_mutation(source,version,80,0,100,100+plan.duration,true));}
  box->drain();
 }
 double health(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 auto completion(){return casting::make_admitted_life_steal_completion(w,[this]{return w.effect_scope(1);});}
};
int main()try{unsigned direct=0,paid=0,touches=0,edges=0;
 for(unsigned skill:{153u,902u,1076u,2189u})for(bool npc:{false,true})for(unsigned version:{0u,181u,2860u})for(unsigned rank:{0u,12u,20u}){
  context="direct skill="+std::to_string(skill)+" npc="+std::to_string(npc)+" version="+std::to_string(version)+" rank="+std::to_string(rank);
  Fixture f(npc,version);check(f.w.before_spell_effect(f.source,skill,1,200));
  const auto plan=*life_steal(skill,rank);auto complete=f.completion();const auto result=complete(f.target,plan,1,200);check(result.accepted);
  check(f.w.resources(1)->health==500+(version?5:0)+plan.amount&&f.health()==500-plan.amount);
  check(f.w.resources(1)->energy==50+(version?1:0));
  check(f.w.has_condition(f.source,combat::conditions::Kind::bleeding,200)==bool(version&&skill==1076));
  f.box->drain();const auto repeated=complete(f.target,plan,1,200);
  check(!repeated.accepted&&repeated.visuals.empty()&&f.box->drain().empty());++direct;
 }
 for(unsigned skill:{153u,902u,1076u,2189u})for(bool npc:{false,true})for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<11;++mode){
  context="paid skill="+std::to_string(skill)+" npc="+std::to_string(npc)+" version="+std::to_string(version)+" mode="+std::to_string(mode);
  Fixture f(npc,version);casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0,before_health=0,before_energy=0;std::uint64_t admitted=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.events=[&](const auto& packets){f.box->enqueue(packets);};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_valid=[](auto,float,bool){return true;};h.projectile_target_valid=[](auto,float){return true;};
  h.before_spell_effect=[&](auto used,auto event,auto at){f.w.tick(at);admitted=event;
   before_health=f.w.resources(1)->health;before_energy=f.w.resources(1)->energy;
   return f.w.before_spell_effect(f.source,used,event,at);};
  auto native=f.completion();h.life_steal_spell_completed=[&](auto target,const auto& plan,auto event,auto at){
   check(event==admitted&&event);++calls;return native(target,plan,event,at);};
  h.complete_life_steal=[](auto,const auto&,auto)->casting::EffectCompletion{throw std::runtime_error("admitted completion downgraded");};
  h.life_steal=[](auto,const auto&,auto)->bool{throw std::runtime_error("admitted completion used plain callback");};
  const bool normal=mode==0||mode==10;
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;policy.pvp_match=mode==10;
  Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
  if(mode==9)h.before_spell_effect={};
  check(runtime.start(100,{skill,0,f.target.actor},bar,policy,ranks,true,h)==(mode==9?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(101,h);if(mode==3)h.life_steal_spell_completed={};
  if(mode==4)h.before_spell_effect={};if(mode==5)h.target_scope={};if(mode==6)h.projectile_target_valid={};
  if(mode==7){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},200,1);f.w.set_vitals(1,{500,1000,40,100},100);}
  if(mode==8){if(npc){h.target_scope=[&](auto id){auto scope=f.w.effect_scope(id);if(scope&&id==f.target.actor)++scope->generation;return scope;};}
   else{f.w.leave(2);f.w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},200,2);f.w.set_vitals(2,{500,1000,50,100},100);}}
  const auto at=100+std::uint64_t(std::llround(find_skill(skill)->activation*1000));f.box->drain();runtime.update(at,h);const auto packets=f.box->drain();runtime.update(at+100,h);
  check(calls==unsigned(normal));if(mode==9)check(f.w.resources(1)->energy==50);
  if(normal){const auto plan=*life_steal(skill,0);check(f.w.resources(1)->health==before_health+(version?5:0)+plan.amount);
   check(f.w.resources(1)->energy==before_energy+(version?1:0)&&f.health()==500-plan.amount);
   const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish!=packets.end());
   if(const auto visual=particles::life_steal_target_2022(f.target.actor,skill,1)){
    const auto body=std::find(packets.begin(),packets.end(),*visual);check(finish<body&&body!=packets.end()&&std::count(packets.begin(),packets.end(),*visual)==1);}
   if(version){const auto reward=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[0]==162&&p[2]==55;});check(reward<finish);}
   check(f.w.has_condition(f.source,combat::conditions::Kind::bleeding,at)==bool(version&&skill==1076));
  }else check(f.w.resources(1)->health==500);
  ++paid;
 }
 for(unsigned skill:{153u,902u,1076u,2189u})for(bool npc:{false,true})for(unsigned version:{0u,181u,2860u}){
  context="legacy admission skill="+std::to_string(skill);Fixture f(npc,version);casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.events=[&](const auto& packets){f.box->enqueue(packets);};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};h.target_valid=[](auto,float,bool){return true;};h.projectile_target_valid=[](auto,float){return true;};
  h.before_spell_effect=[&](auto used,auto event,auto at){return f.w.before_spell_effect(f.source,used,event,at);};
  auto legacy=casting::make_life_steal_completion(f.w,[&]{return std::optional{f.source};});
  h.complete_life_steal=[&](auto target,const auto& plan,auto at){++calls;return legacy(target,plan,at);};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
  check(runtime.start(100,{skill,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  h.life_steal_spell_completed=[](auto,const auto&,auto,auto)->casting::EffectCompletion{throw std::runtime_error("legacy admission silently upgraded");};
  runtime.update(100+std::uint64_t(std::llround(find_skill(skill)->activation*1000)),h);
  check(calls==1&&f.w.resources(1)->health==500+life_steal(skill,0)->amount);++edges;
 }
 for(unsigned skill:{156u,1077u})for(bool npc:{false,true})for(unsigned version:{0u,181u,2860u}){
  context="nonspell skill="+std::to_string(skill);Fixture f(npc,version);casting::Runtime runtime(1);casting::Hooks h;unsigned admissions=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.events=[&](const auto& packets){f.box->enqueue(packets);};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};h.target_valid=[](auto,float,bool){return true;};h.projectile_target_valid=[](auto,float){return true;};
  h.before_spell_effect=[&](auto,auto,auto){++admissions;return true;};h.complete_life_steal=casting::make_life_steal_completion(f.w,[&]{return std::optional{f.source};});
  h.life_steal_spell_completed=[](auto,const auto&,auto,auto)->casting::EffectCompletion{throw std::runtime_error("touch entered spell terminal");};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};attributes::Ranks ranks{};
  check(runtime.start(100,{skill,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);runtime.update(850,h);
  check(admissions==0&&f.w.resources(1)->health==500+life_steal(skill,0)->amount);
  check(!f.w.complete_life_steal(f.source,f.target,*life_steal(skill,0),1,850));++touches;
 }
 for(unsigned skill:{153u,902u,1076u,2189u})for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  context="refusal/retry skill="+std::to_string(skill)+" npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  Fixture f(npc,181);check(f.w.before_spell_effect(f.source,skill,1,200));auto plan=*life_steal(skill,0);auto target=f.target,owner=f.source;
  if(mode==0)++plan.amount;if(mode==1)++target.generation;if(mode==2)++owner.generation;
  if(mode==3)f.w.tick(300);if(mode==4)plan={110,0,10,322};
  std::vector<combat::Packet> visuals;f.box->drain();
  check(!f.w.complete_life_steal(owner,target,plan,mode==5?2:1,200,&visuals)&&visuals.empty());
  check(f.w.resources(1)->health==500&&f.w.resources(1)->energy==50&&f.health()==500);
  check(f.w.complete_life_steal(f.source,f.target,*life_steal(skill,0),1,mode==3?300:200,&visuals));++edges;
 }
 for(unsigned skill:{153u,902u,1076u,2189u})for(bool npc:{false,true})for(unsigned version:{181u,2860u}){
  context="expired skill="+std::to_string(skill);Fixture f(npc,version);
  const auto at=100+ether_renewal(version,0)->duration;f.w.tick(at);
  check(f.w.before_spell_effect(f.source,skill,1,at));const auto before=*f.w.resources(1);auto complete=f.completion();
  check(complete(f.target,*life_steal(skill,0),1,at).accepted);
  check(f.w.resources(1)->health==before.health+life_steal(skill,0)->amount&&f.w.resources(1)->energy==before.energy);
  check(!f.w.has_condition(f.source,combat::conditions::Kind::bleeding,at));++edges;
 }
 for(bool npc:{false,true})for(unsigned version:{181u,2860u}){
  context="conditional Energy";Fixture f(npc,version);const auto plan=*life_steal(2189,12);
  check(f.w.apply_condition(*f.w.effect_scope(2),f.source,combat::conditions::Kind::blind,100,10000));
  check(f.w.before_spell_effect(f.source,2189,1,200)&&f.completion()(f.target,plan,1,200).accepted);
  check(f.w.resources(1)->energy==51+plan.energy_if_condition);++edges;
 }
 for(unsigned skill:{153u,902u,1076u,2189u})for(unsigned version:{181u,2860u}){
  context="due recipient death skill="+std::to_string(skill);Fixture f(true,version,4);
  check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::poison,100,10000));
  check(f.w.before_spell_effect(f.source,skill,1,1100));auto complete=f.completion();
  const auto before=*f.w.resources(1);check(!complete(f.target,*life_steal(skill,0),1,1100).accepted);
  check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);
  check(f.w.npc_combat_snapshot(f.target)->health->current==0&&f.w.take_npc_deaths(f.source).size()==1);
  // The trusted World refusal did not consume the admitted terminal. Runtime
  // separately pins its target scope and never retargets a pending cast.
  check(complete(*f.w.effect_scope(2),*life_steal(skill,0),1,1100).accepted);
  check(f.w.resources(1)->health==before.health+10&&f.w.resources(1)->energy==before.energy+1);++edges;
 }
 for(bool npc:{false,true})for(unsigned version:{181u,2860u}){
  context="lethal Blood Drinker reaction";Fixture f(npc,version);f.w.set_vitals(1,{2,3,50,100},100);
  check(f.w.degeneration_hex(*f.w.effect_scope(2),f.source,*degeneration_hex(19,20),100));
  check(f.w.before_spell_effect(f.source,1076,1,200));const auto result=f.completion()(f.target,*life_steal(1076,0),1,200);
  check(result.accepted&&result.visuals.empty()&&f.w.resources(1)->health==0&&f.health()==500);
  check(!f.completion()(f.target,*life_steal(1076,0),1,200).accepted);++edges;
 }
 std::cout<<"PASS "<<direct<<" direct, "<<paid<<" paid control/visual, "<<touches<<" nonspell and "<<edges<<" refusal/expiry/reaction cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
