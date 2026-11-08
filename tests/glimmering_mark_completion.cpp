#include "gw_glimmering_mark_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Glimmering completion line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;Scope source,target;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool npc,unsigned renewal){
  if(npc){world::Instance::Npc a;a.id=a.definition.id=100;a.x=100;a.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={480,480};d.damageable=d.conditions_allowed=true;
   a.combat_definition=d;w.configure_npcs({a});}
  for(unsigned id:{1u,2u}){w.join(id,"Mark",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{id==1?500u:480u,id==1?1000u:480u,50,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
  if(renewal){const auto plan=*ether_renewal(renewal,0);check(w.status_mutation(source,renewal,80,0,100,100+plan.duration,true));}
  box->drain();
 }
 auto completion(){return casting::make_glimmering_mark_completion(w,[this]{return w.effect_scope(1);},20);}
 bool mark(unsigned at){return w.glimmering_mark_snapshot(target,at).has_value();}
};
int main()try{unsigned cases=0;
 for(bool npc:{false,true})for(unsigned renewal:{0u,181u,2860u})for(unsigned rank=0;rank<=20;++rank){
  Fixture f(npc,renewal);check(f.w.before_spell_effect(f.source,227,1,200));f.box->drain();
  auto result=f.completion()(f.target,*glimmering_mark(rank),1,200);check(result.accepted&&result.publication&&f.mark(200));
  check(f.w.resources(1)->health==500+(renewal?5:0)&&f.w.resources(1)->energy==50+(renewal?1:0));
  const auto before=f.box->drain();const auto body=*particles::targeted_body_2022(f.target.actor,1,227);
  check(std::find(before.begin(),before.end(),body)==before.end());
  result.publication();const auto after=f.box->drain();check(std::count(after.begin(),after.end(),body)==1);
  result.publication();check(f.box->drain().empty());
  check(!f.completion()(f.target,*glimmering_mark(rank),1,200).accepted);++cases;
 }
 for(bool npc:{false,true}){
  Fixture f(npc,181);check(f.w.before_spell_effect(f.source,227,1,200));
  check(!f.completion()(f.target,*glimmering_mark(0),2,200).accepted&&!f.mark(200));
  auto stale=f.target;++stale.generation;check(!f.completion()(stale,*glimmering_mark(0),1,200).accepted&&!f.mark(200));
  check(f.w.resources(1)->health==500&&f.w.resources(1)->energy==50);
  auto good=f.completion()(f.target,*glimmering_mark(0),1,200);check(good.accepted&&f.mark(200));good.publication();++cases;
 }
 for(bool npc:{false,true}){
  Fixture f(npc,181);f.w.set_vitals(1,{50,1000,50,100},100);
  check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,60000));
  check(f.w.before_spell_effect(f.source,227,1,200));
  auto result=f.completion()(f.target,*glimmering_mark(0),1,200);
  check(result.accepted&&f.w.resources(1)->health==0&&!f.mark(200));if(result.publication)result.publication();++cases;
 }
 {Fixture f(false,181);check(f.w.hex_breaker(f.target,*hex_breaker(12),20,100));check(f.w.before_spell_effect(f.source,227,1,200));
  auto result=f.completion()(f.target,*glimmering_mark(0),1,200);check(result.accepted&&!f.mark(200)&&f.w.resources(1)->health<505);
  result.publication();check(!f.completion()(f.target,*glimmering_mark(0),1,200).accepted);++cases;}
 std::cout<<cases<<" admitted player/NPC rank, Renewal, deferred body400, replay, retry, reward-death and HexBreaker cases passed; Runtime227 remains disabled\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
