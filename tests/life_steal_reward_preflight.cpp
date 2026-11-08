#include "gw_life_steal_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Life-steal reward preflight line "+std::to_string(at.line())+" "+context);
}
struct Fixture {
 world::Instance w;Scope source,target;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool npc,unsigned version,std::uint64_t now,unsigned health=500,unsigned rank=0){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,1000};
   d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},now,id);
   w.set_vitals(id,{id==1?health:500u,1000,50,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
  const auto plan=*ether_renewal(version,rank);check(w.status_mutation(source,version,80,rank,now,now+plan.duration,true));box->drain();
 }
};
int main()try{unsigned cases=0,thresholds=0,deaths=0;
 for(bool npc:{false,true})for(unsigned version:{181u,2860u}){
  context="overflow npc="+std::to_string(npc)+" version="+std::to_string(version);
  constexpr auto now=UINT64_MAX-8000;Fixture f(npc,version,now);const auto plan=*life_steal(1076,0);
  check(f.w.before_spell_effect(f.source,1076,1,now));f.box->drain();
  bool accepted=false;std::vector<combat::Packet> visuals;
  try{accepted=f.w.complete_life_steal(f.source,f.target,plan,1,now,&visuals);}
  catch(const std::exception& e){std::cerr<<context<<" committed Health="<<f.w.resources(1)->health
   <<" Energy="<<f.w.resources(1)->energy<<" before exception: "<<e.what()<<'\n';throw;}
  // Renewal itself fits the clock, but its healing would cross half Health
  // and require a ten-second Bleeding which cannot be represented. Reject
  // before either reward or terminal, rather than throwing after payment.
  check(!accepted&&visuals.empty()&&f.w.resources(1)->health==500&&f.w.resources(1)->energy==50);
  check(f.box->drain().empty());
  // This is conditional preflight: a lower Health actor must still complete
  // at the same clock/event when reward cannot trigger the self-condition.
  f.w.set_vitals(1,{490,1000,50,100},100);f.box->drain();
  check(f.w.complete_life_steal(f.source,f.target,plan,1,now,&visuals));
  check(f.w.resources(1)->health==515&&f.w.resources(1)->energy==51);
  check(!f.w.has_condition(f.source,combat::conditions::Kind::bleeding,now));
  check(!f.w.complete_life_steal(f.source,f.target,plan,1,now,&visuals));++cases;
 }
 for(bool npc:{false,true})for(unsigned version:{181u,2860u})for(unsigned rank=0;rank<=20;++rank)
  for(unsigned enchantments:{1u,2u})for(bool reduced:{false,true})for(int side:{-1,0,1}){
   context="threshold npc="+std::to_string(npc)+" version="+std::to_string(version)+" rank="+std::to_string(rank)+
    " count="+std::to_string(enchantments)+" reduced="+std::to_string(reduced)+" side="+std::to_string(side);
   // Pinned healing5..20is linear at one point per attribute rank. Observe
   // both sides of the actual fractional threshold, not a rounded UI value.
   const double healing=(5u+rank)*enchantments*(reduced?.66:1.);
   const auto initial=static_cast<unsigned>(500-int(std::floor(healing))+side);
   Fixture f(npc,version,100,initial,rank);
   if(enchantments==2)check(f.w.status_mutation(f.source,245,81,0,100,5100,true));
   if(reduced)check(f.w.degeneration_hex(*f.w.effect_scope(2),f.source,*degeneration_hex(129,0),100));
   check(f.w.before_spell_effect(f.source,1076,1,200));const auto before=*f.w.resources(1);
   check(f.w.complete_life_steal(f.source,f.target,*life_steal(1076,0),1,200));
   const auto expected=double(initial)+healing;
   check(f.w.resources(1)->health==static_cast<unsigned>(std::ceil(expected+20)));
   check(f.w.resources(1)->energy==before.energy+enchantments*ether_renewal(version,rank)->energy_per_enchantment);
   check(f.w.has_condition(f.source,combat::conditions::Kind::bleeding,200)==(expected>500));
   ++thresholds;
  }
 for(bool npc:{false,true})for(unsigned version:{181u,2860u}){
  context="reward debt death npc="+std::to_string(npc)+" version="+std::to_string(version);
  Fixture f(npc,version,100,50);check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,60000));
  const auto epoch=*f.w.control_epoch(f.source);check(f.w.before_spell_effect(f.source,1076,1,100));
  std::vector<combat::Packet> visuals;check(f.w.complete_life_steal(f.source,f.target,*life_steal(1076,0),1,100,&visuals));
  check(f.w.resources(1)->health==0&&f.w.resources(1)->energy==50&&visuals.empty());
  check(*f.w.control_epoch(f.source)==epoch+1);
  const auto target_health=npc?f.w.npc_combat_snapshot(f.target)->health->current:double(f.w.resources(2)->health);
  check(target_health==500&&!f.w.complete_life_steal(f.source,f.target,*life_steal(1076,0),1,100));++deaths;
 }
 std::cout<<"PASS "<<cases<<" reward-dependent overflow/refusal/retry, "<<thresholds
  <<" rank/count/fractional-healing thresholds and "<<deaths<<" reward wound-debt deaths\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
