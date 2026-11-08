#include "gw_wound_spell_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Phantom completion line "+std::to_string(at.line())+" "+context);
}
struct Fixture {
 world::Instance w;bool npc;Scope source,target;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool n,unsigned renewal):npc(n){
  if(n){world::Instance::Npc a;a.id=a.definition.id=100;a.x=100;a.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{480,480};
   d.damageable=d.conditions_allowed=true;a.combat_definition=d;w.configure_npcs({a});}
  for(unsigned id:{1u,2u}){w.join(id,"Phantom",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{id==1?500u:480u,id==1?1000u:480u,50,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(n?100:2);
  if(renewal){const auto plan=*ether_renewal(renewal,0);check(w.status_mutation(source,renewal,80,0,100,100+plan.duration,true));}
  box->drain();
 }
 auto completion(){return casting::make_wound_spell_completion(w,[this]{return w.effect_scope(1);},20);}
 bool hex(std::uint64_t at){const auto r=w.status_snapshot(target,at);return r&&std::any_of(r->begin(),r->end(),[](const auto& e){return e.skill==44;});}
};
int main()try{unsigned paid=0,direct=0;
 for(bool npc:{false,true})for(unsigned renewal:{0u,181u,2860u})for(unsigned mode=0;mode<9;++mode){
  context="npc="+std::to_string(npc)+" renewal="+std::to_string(renewal)+" mode="+std::to_string(mode);
  Fixture f(npc,renewal);casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0;std::uint64_t event=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.events=[&](const auto& packets){f.box->enqueue(packets);};
  h.consume_energy_at=[&](auto amount,auto at){return f.w.consume_energy(1,amount,f.source,at);};
  h.target_valid=[](auto,float,bool){return true;};h.projectile_target_valid=[](auto,float){return true;};
  h.before_spell_effect=[&](auto skill,auto serial,auto at){f.w.tick(at);event=serial;return f.w.before_spell_effect(f.source,skill,serial,at);};
  auto native=f.completion();h.wound_spell_completed=[&](auto target,const auto& plan,auto serial,auto at){++calls;check(serial&&serial==event);return native(target,plan,serial,at);};
  h.wound_spell=[](auto,const auto&,auto)->bool{throw std::runtime_error("admitted Phantom downgraded");};
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[44]=1;Slots bar{};bar[0]={44,0};attributes::Ranks ranks{};
  if(mode==8)h.before_spell_effect={};
  check(runtime.start(100,{44,0,f.target.actor},bar,policy,ranks,true,h)==(mode==8?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(101,h);if(mode==3)h.wound_spell_completed={};
  if(mode==4)h.target_scope={};if(mode==5)h.projectile_target_valid={};if(mode==6)h.before_spell_effect={};
  if(mode==7)h.target_scope=[&](auto id){auto s=f.w.effect_scope(id);if(s&&id==f.target.actor)++s->generation;return s;};
  const auto at=100+std::uint64_t(std::llround(find_skill(44)->activation*1000));f.box->drain();runtime.update(at,h);
  const auto packets=f.box->drain();runtime.update(at,h);check(calls==unsigned(mode==0));
  check(f.hex(at)==(mode==0));check(f.w.resources(1)->health==500+(mode==0&&renewal?5:0));
  if(mode==0){
   const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));check(finish!=packets.end());
   const auto effect=std::find_if(packets.begin(),packets.end(),[&](const auto& p){return p.size()>=10&&p[0]==159&&p[2]==20&&p[6]==f.target.actor;});
   if(npc)check(effect!=packets.end()&&finish<effect);
   if(renewal){const auto reward=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162&&p[2]==55&&p[6]==1;});check(reward!=packets.end()&&reward<finish);}
   check(!native(f.target,*wound_spell(44,0),event,at).accepted);
  }++paid;
 }
 for(bool npc:{false,true})for(unsigned renewal:{0u,181u,2860u})for(unsigned rank=0;rank<=20;++rank){
  Fixture f(npc,renewal);check(f.w.before_spell_effect(f.source,44,1,200));
  auto result=f.completion()(f.target,*wound_spell(44,rank),1,200);check(result.accepted&&result.publication&&f.hex(200));
  result.publication();check(f.w.resources(1)->health==500+(renewal?5:0));
  check(f.w.resources(1)->energy==50+(renewal?1:0));
  check(!f.completion()(f.target,*wound_spell(44,rank),1,200).accepted);++direct;
 }
 for(bool npc:{false,true}){
  Fixture f(npc,181);check(f.w.before_spell_effect(f.source,44,1,200));
  check(!f.completion()(f.target,*wound_spell(44,12),2,200).accepted&&!f.hex(200));
  auto stale=f.target;++stale.generation;
  check(!f.completion()(stale,*wound_spell(44,12),1,200).accepted&&!f.hex(200));
  check(f.w.resources(1)->health==500&&f.w.resources(1)->energy==50);
  auto good=f.completion()(f.target,*wound_spell(44,12),1,200);check(good.accepted&&f.hex(200));good.publication();
 }
 for(bool npc:{false,true}){
  Fixture f(npc,181);f.w.set_vitals(1,{50,1000,50,100},100);
  check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,60000));
  check(f.w.before_spell_effect(f.source,44,1,200));
  auto result=f.completion()(f.target,*wound_spell(44,12),1,200);
  check(result.accepted&&f.w.resources(1)->health==0&&!f.hex(200));if(result.publication)result.publication();
 }
 {
  Fixture f(false,181);check(f.w.hex_breaker(f.target,*hex_breaker(12),20,100));
  check(f.w.before_spell_effect(f.source,44,1,200));
  auto result=f.completion()(f.target,*wound_spell(44,12),1,200);
  check(result.accepted&&!f.hex(200)&&f.w.resources(1)->health<505);result.publication();
 }
 std::cout<<paid<<" paid Phantom routing/cancel/lifetime cases and "<<direct<<" admitted rank/Renewal cases passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
