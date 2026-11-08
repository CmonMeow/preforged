// DETACHED real Runtime -> reusable adapter -> World paid completion -> actual
// attack/holy return/Seed NPC cohort -> owned upkeep end. Normal248 unbound.
#include "gw_retribution_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::casting;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Retribution adapter World draft line "+std::to_string(at.line()));
}
static world::Instance::Npc body(unsigned id,bool hostile,float x,unsigned hp=500){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.primary=3;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
 d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
 d.spawn_health={double(hp),1000};d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=4;
 d.packed_allegiance=hostile?0x5000001u:0x3000001u;d.damageable=d.conditions_allowed=true;d.physical_armor=60;
 d.healing_allowed=true;d.healing_evidence="authored integration recipient";
 d.holy_traits={false,false};d.light_sensitive_evidence=d.summoned_evidence="authored explicit holy facts";
 n.combat_definition=d;return n;
}
int main()try{
 // mode0 healthy,1 cancel,2 stale target generation,3 lethal admitted Renewal.
 for(bool npc_recipient:{false,true})for(unsigned mode=0;mode<4;++mode)for(bool renewal:{false,true}){
  if((mode==3&&!renewal)||(mode==2&&npc_recipient))continue;
  auto w=std::make_unique<world::Instance>();auto box=std::make_shared<world::Outbox>();
  w->configure_npcs({body(100,false,60,100),body(101,true,0),body(104,false,50)});
  w->configure_attack_critical_draws([]{return .999999;});w->configure_attack_miss_draws([]{return .5;});
  for(unsigned id:{1u,2u,3u,4u}){w->join(id,"Actor",id==1?1000.f:id==2?60.f:id==3?0.f:100.f,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id<=2?1:2);
   w->set_vitals(id,{id==1?(mode==3?10u:100u):id==2?100u:500u,1000,id==1?40u:100u,100},100);}
  w->public_professions(1,3,6);const unsigned recipient=npc_recipient?100u:2u;
  const auto source=*w->effect_scope(1),target=*w->effect_scope(recipient),attacker=*w->effect_scope(3),monk=*w->effect_scope(4);
  check(w->ordinary_weapon(source,std::nullopt,20));
  // Monk/Elementalist has real Divine Favor and Energy Storage0; do not
  // invent two primary professions to exercise the completion reward.
  if(renewal){const auto r=*effects::ether_renewal(181,0);check(w->status_mutation(source,181,80,0,100,100+r.duration,true));}
  if(mode==3)check(w->apply_condition(attacker,source,combat::conditions::Kind::deep_wound,100,10000));
  const auto owner=[&]{return w->effect_scope(1);};
  auto completion=make_retribution_completion(*w,owner);
  Hooks h;unsigned calls=0,admissions=0,payments=0;std::uint64_t admitted=0;vitals::Resources before{};std::function<void()> copied;
  h.resources=[&](unsigned id){return w->resources(id);};h.target_scope=[&](unsigned id){return w->effect_scope(id);};
  h.events=[&](const auto& events){box->enqueue(events);};
  h.consume_energy_at=[&](unsigned cost,std::uint64_t now){check(cost==10);++payments;const auto current=owner();return current&&w->consume_energy(1,cost,*current,now);};
  h.retribution_target_valid=make_retribution_target_valid(*w,owner);
  h.before_spell_effect=[&](unsigned skill,std::uint64_t event,std::uint64_t now){check(skill==248);++admissions;admitted=event;
   w->tick(now);const auto current=owner();if(!current)return false;const bool ok=w->before_spell_effect(*current,skill,event,now);before=*w->resources(1);box->drain();return ok;};
  h.retribution_completed=[&](auto scope,const auto& plan,unsigned divine,std::uint64_t event,std::uint64_t now){
   ++calls;check(event==admitted&&divine==12);auto result=completion(scope,plan,divine,event,now);copied=result.publication;
   check(box->drain().empty());if(result.accepted)check(result.completion_prefix_published==(mode==3));return result;};
  Policy policy;policy.primary=3;policy.secondary=6;policy.campaigns=255;policy.learned[248]=1;Slots bar{};bar[0]={248,0};attributes::Ranks ranks{};ranks[14]=ranks[16]=12;
  Runtime cast(1);check(!Runtime::effect_supported(248));box->drain();check(cast.start(100,{248,0,recipient},bar,policy,ranks,true,h)==casting::Reason::accepted);
  check(payments==1&&w->resources(1)->energy==30);box->drain();
  if(mode==1)cast.cancel(h,1000);
  if(mode==2){w->leave(2);w->join(2,"Reused target",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w->set_vitals(2,{100,1000,100,100},100);check(*w->effect_scope(2)!=target);box->drain();}
  cast.update(2099,h);check(calls==0);cast.update(2100,h);
  if(mode==1||mode==2){check(!calls&&!admissions&&w->maintained_enchantment_count(source,2100)==0);continue;}
  check(calls==1&&admissions==1&&bool(copied));
  const auto packets=box->drain();const auto visual=*particles::targeted_body_2022(recipient,1,248);
  check(std::count(packets.begin(),packets.end(),terminal(58,1))==1&&std::count(packets.begin(),packets.end(),visual)==1);
  check(std::find(packets.begin(),packets.end(),terminal(58,1))<std::find(packets.begin(),packets.end(),visual));
  copied();check(box->drain().empty());cast.update(2101,h);check(calls==1);
  if(mode==3){check(w->resources(1)->health==0&&(npc_recipient?w->npc_combat_snapshot(target)->health->current:double(w->resources(2)->health))==100&&w->maintained_enchantment_count(source,2100)==0);continue;}
  check(w->resources(1)->health==before.health+(renewal?5u:0u));
  check(w->resources(1)->energy==before.energy+(renewal?effects::ether_renewal(181,0)->energy_per_enchantment:0u));
  check((npc_recipient?w->npc_combat_snapshot(target)->health->current:double(w->resources(2)->health))==138&&w->maintained_enchantment_count(source,2100)==1);
  check(!w->complete_retribution(source,target,*effects::retribution(12),12,2100,admitted).accepted);
  const auto active=w->status_snapshot(target,2100);check(active&&active->size()==1&&active->front().skill==248);
  const auto lease=active->front();const auto wire=effects::mending_wire(source,target,lease);check(bool(wire));
  check(std::count(packets.begin(),packets.end(),wire->upkeep)==1&&std::count(packets.begin(),packets.end(),wire->effect)==1);
  check(std::find(packets.begin(),packets.end(),visual)<std::find(packets.begin(),packets.end(),wire->effect));
  if(npc_recipient){
   // Real reviewed hostile NPC delivery onto the paid NPC recipient. Do not
   // fabricate a player Seed caster for that hostile attacker.
   const auto npc_attacker=*w->effect_scope(101);
   const combat::ai::ordinary::Reviewed weapon{20,20,12,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"authored paid NPC contact"};
   check(w->configure_npc_opposed_ordinary(npc_attacker,weapon,2200));
   const auto token=w->begin_npc_opposed_ordinary(npc_attacker,target,2200);check(bool(token));w->tick(token->completes);
   check(w->npc_combat_snapshot(target)->health->current==118&&w->npc_combat_snapshot(npc_attacker)->health->current==494);
  }else{
   // Real player return triggers Seed and reaches actual adjacent NPC allies.
   const auto seed=*effects::healing_seed(12);check(w->complete_healing_seed(monk,attacker,seed,0,2100));check(w->player_holy_equipment(attacker,0));
   combat::weapon::Owned sword{12,1,{20,20,2},12,false,false,27};check(w->ordinary_weapon(attacker,sword,20)&&w->physical_armor(target,60));
   check(w->melee_attack(attacker,target,sword,*combat::weapon::attack_skill(322,0),2200));
   check(w->resources(2)->health==108&&w->resources(3)->health==516&&w->resources(4)->health==525);
   check(w->npc_combat_snapshot(*w->effect_scope(104))->health->current==525);
  }
  check(!w->retribution_damage_failures()&&!w->retribution_pending_damage_contacts());
  const auto end=effects::mending_end_wire(source,target,lease);check(bool(end));box->drain();
  check(w->cancel_maintained_enchantment(source,lease.instance,3200));check(w->maintained_enchantment_count(source,3200)==0);
  check(w->status_snapshot(target,3200)->empty());const auto ended=box->drain();
  check(std::count(ended.begin(),ended.end(),end->upkeep)==1&&std::count(ended.begin(),ended.end(),end->effect)==1);
  check(!w->cancel_maintained_enchantment(source,lease.instance,3200)&&box->drain().empty());
 }
 std::cout<<"PASS detached real paid adapter cost/DF/Renewal/cancel/stale/lethal publication/attack/NPC Seed/end\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
