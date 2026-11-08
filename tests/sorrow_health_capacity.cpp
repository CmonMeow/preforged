#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
using namespace fw::gw;
namespace effects=skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Sorrow capacity line "+std::to_string(at.line()));
}
static unsigned word(const world::Packet& p,unsigned at){unsigned v=0;for(unsigned i=0;i<4;++i)v|=unsigned(p[at+i])<<(i*8);return v;}
int main()try{
 unsigned cases=0;
 for(bool npc:{false,true})for(unsigned maximum:{100u,480u,1000u,2000u})for(unsigned health_mode:{0u,1u,2u})for(int rr:{-1,0,12,20})for(bool seed:{false,true}){
  if(npc&&seed)continue;
  const unsigned hp=health_mode==0?10u:health_mode==1?maximum/2:maximum;
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=50;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={double(hp),maximum};d.damageable=d.conditions_allowed=true;
   d.healing_allowed=true;d.healing_evidence="synthetic Sorrow wire-capacity fixture";n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0.f:id==2?50.f:100.f,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==2&&!npc?hp:500u,id==2&&!npc?maximum:1000u,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2),monk=*w.effect_scope(3);
  if(seed){check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));}
  if(rr>=0)check(npc?w.stage_npc_reversal(target,rr,100):w.status_mutation(target,307,900,rr,100,8100,true));
  box->drain();const auto result=w.complete_signet_sorrow(source,target,*effects::signet_sorrow(12),20,200);
  check(result.accepted&&result.visuals.empty()&&result.publication&&box->drain().empty());
  result.publication();const auto wire=box->drain();result.publication();check(box->drain().empty());
  unsigned capacity=0,capacity_count=0,first_capacity=unsigned(wire.size()),first_fraction=unsigned(wire.size());double shown=-1;
  for(unsigned i=0;i<wire.size();++i){const auto& p=wire[i];
   if(p.size()==14&&p[0]==158&&word(p,2)==42&&word(p,6)==target.actor){capacity=word(p,10);check(capacity==maximum);++capacity_count;first_capacity=std::min(first_capacity,i);}
   if(p.size()==14&&p[0]==161&&word(p,2)==34&&word(p,6)==target.actor){check(capacity==maximum);shown=capacity*double(std::bit_cast<float>(word(p,10)));first_fraction=std::min(first_fraction,i);}
  }
  const double actual=npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);
  check(capacity_count>=1&&first_capacity<first_fraction&&std::abs(shown-actual)<1.01);
  const auto damage=effects::signet_sorrow(12)->damage;const double converted=rr<0?0:std::min(damage,effects::reversal_fortune(rr)->maximum_healing);
  const auto healed=std::min(double(maximum),hp+converted),loss=std::min(healed,damage-converted);
  const auto wanted=healed-loss>0?std::min(double(maximum),healed-loss+(seed&&loss>0?25:0)):0.;check(std::abs(actual-wanted)<1.01);
  if(!npc)check(w.resources(3)->health==500+(seed&&loss>0?25u:0u));++cases;
 }
 check(cases==144);std::cout<<"PASS "<<cases<<" Sorrow authoritative-capacity/player/NPC/conversion/Seed/deferred-wire cases; no recorded480 visual replay\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
