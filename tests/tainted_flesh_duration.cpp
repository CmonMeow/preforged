#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills;
using namespace skills::effects;
using combat::conditions::Kind;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Tainted Flesh duration line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;inventory::Profile gear;Scope caster,target,foe;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned kind,unsigned percent){
  if(kind==2){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{1000,1000};
   d.damageable=d.conditions_allowed=true;d.disease_immune=false;d.disease_immunity_evidence="authored fixture";
   d.fleshy=true;d.fleshy_evidence="authored fixture";
   n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*10),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
  caster=*w.effect_scope(1);target=*w.effect_scope(kind==0?1:kind==1?2:100);foe=*w.effect_scope(3);
  gear.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;
  item.definition.id=50;item.definition.type=26;item.definition.quantity=1;item.bag=1;
  if(percent)item.definition.modifiers={0x22b80000|percent};
  gear.items.emplace(50,item);gear.weapons.sets[0].weapon=50;
 }
 std::optional<EnchantmentDuration> quote(unsigned rank,std::uint64_t now){
  const auto equipment=items::weapon_passives::equipped_enchanting(gear,1);
  const auto active=w.status_snapshot(caster,now);const auto plan=tainted_flesh(rank);
  return equipment&&active&&plan?enchanting_duration(113,plan->duration,caster,*equipment,*active,now):std::nullopt;
 }
};
int main()try{unsigned direct=0,paid=0;
 for(unsigned kind=0;kind<3;++kind)for(unsigned rank=0;rank<=20;++rank)for(unsigned percent:{0u,10u,20u}){
  Fixture f(kind,percent);const auto plan=*tainted_flesh(rank);const auto quote=f.quote(rank,1000);check(bool(quote));
  const auto duration=*quote->for_enchantment(113,plan.duration,f.caster,1000);
  const auto seconds=plan.duration/1000;
  check(duration==(seconds+(percent?std::max(std::uint64_t(1),(seconds*percent+50)/100):0))*1000);
  check(f.w.apply_condition(f.foe,f.target,Kind::disease,100,100000));
  check(f.w.has_condition(f.target,Kind::disease,100));
  std::vector<combat::Packet> visuals;
  check(!f.w.tainted_flesh(f.caster,f.target,plan,1001,&visuals,&*quote));
  check(visuals.empty()&&f.w.status_snapshot(f.target,100)->empty());
  auto wrong=f.caster;++wrong.generation;
  check(!f.w.tainted_flesh(wrong,f.target,plan,1000,&visuals,&*quote));
  check(f.w.tainted_flesh(f.caster,f.target,plan,1000,&visuals,&*quote));
  check(visuals.size()==1&&!f.w.has_condition(f.target,Kind::disease,1000));
  const auto entries=*f.w.status_snapshot(f.target,1000);
  check(entries.size()==1&&entries.front().skill==113&&entries.front().expires==1000+duration);
  if(percent){f.w.tick(1000+plan.duration);check(f.w.status_snapshot(f.target,1000+plan.duration)->size()==1);}
  f.w.tick(1000+duration-100);check(f.w.status_snapshot(f.target,1000+duration-100)->size()==1);
  f.w.tick(1000+duration);check(f.w.status_snapshot(f.target,1000+duration)->empty());
  check(f.w.apply_condition(f.foe,f.target,Kind::disease,1000+duration,10000));
  check(f.w.has_condition(f.target,Kind::disease,1000+duration));++direct;
 }
 for(unsigned kind=0;kind<3;++kind)for(unsigned mode=0;mode<6;++mode){
  Fixture f(kind,20);casting::Runtime cast(1);casting::Hooks h;unsigned calls=0,checks=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& packets){f.box->enqueue(packets);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.consume_energy_at=[&](auto cost,auto at){return f.w.consume_energy(1,cost,f.caster,at);};
  h.tainted_flesh_target_valid=[&](auto id,const auto& plan,auto at){return f.w.valid_tainted_flesh_target(f.caster,id,plan,at);};
  h.enchantment_duration_ready=[&](auto skill,auto base,auto at){++checks;check(skill==113&&base==tainted_flesh(12)->duration);return bool(f.quote(12,at));};
  h.tainted_flesh=[&](auto target,const auto& plan,auto at){++calls;const auto duration=f.quote(plan.rank,at);std::vector<combat::Packet> visuals;
   const bool ok=duration&&f.w.tainted_flesh(f.caster,target,plan,at,&visuals,&*duration);return casting::EffectCompletion{ok,std::move(visuals)};};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[113]=1;
  Slots bar{};bar[0]={113,0};attributes::Ranks ranks{};ranks[5]=12;
  if(mode==1)f.gear.items.at(50).definition.modifiers={0x22b800ff};
  check(cast.start(500,{113,0,f.target.actor},bar,policy,ranks,true,h)==(mode==1?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==1){check(calls==0&&f.w.resources(1)->energy==100);++paid;continue;}
  if(mode==2)h.enchantment_duration_ready={};
  if(mode==3)f.gear.items.at(50).definition.modifiers={0x22b800ff};
  if(mode==4)f.gear.items.at(50).definition.modifiers={0x22b8000a};
  if(mode==5)cast.cancel(h);
  cast.update(1500,h);check(calls==unsigned(mode==0||mode==4));
  const auto entries=*f.w.status_snapshot(f.target,1500);
  if(calls){const auto current=f.quote(12,1500);check(bool(current));const auto expected=*current->for_enchantment(113,tainted_flesh(12)->duration,f.caster,1500);
   check(entries.size()==1&&entries.front().expires==1500+expected);check(checks==2);
  }else check(entries.empty());++paid;
 }
 std::cout<<"PASS "<<direct<<" Tainted Flesh player/self/NPC rank/equipment/disease/expiry cases and "<<paid<<" paid admission/completion/equipment-change cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
