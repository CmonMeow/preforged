#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Whirling World line "+std::to_string(at.line()));}
int main()try{
 for(bool npc:{false,true})for(bool reversal:{false,true})for(unsigned equipment:{0u,3u}){
  world::Instance w;w.configure_attack_miss_draws([]{return .5;});
  if(npc){
   world::Instance::Npc n;n.id=n.definition.id=100;n.x=80;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};
   d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="authored Whirling overlap fixture";
   n.combat_definition=d;w.configure_npcs({n});
  }else{
   w.join(3,"Adjacent foe",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
   w.set_vitals(3,{500,500,100,100},100);
  }
  for(unsigned id:{1u,2u}){
   w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{1000,1000,30,30},100);
  }
  struct {skills::effects::Scope source,target;combat::weapon::Owned weapon;} f{
   *w.effect_scope(1),*w.effect_scope(2),{12,1,{20,20,2},12,false,false,combat::weapon::attack_skill(399,12)->required_type}};
  check(w.energy_capacity(1,1,0,100,{10,-2}));check(w.physical_armor(f.target,60));
  check(w.ordinary_weapon(f.source,f.weapon,20));
  const auto adjacent=*w.effect_scope(npc?100:3);
  check(w.ordinary_weapon(f.target,{},20));w.configure_attack_block_draws([]{return .1;});
  check(w.status_mutation(f.target,450,123,12,100,18100,true));
  check(w.physical_absorption(f.source,equipment));
  check(w.status_mutation(f.source,1399,80,12,100,6100,true));
  if(npc)check(w.stage_npc_shield_absorption(adjacent,12,100));
  else check(w.status_mutation(adjacent,1399,80,12,100,6100,true));
  if(reversal){
   check(w.degeneration_hex(f.target,f.source,*skills::effects::degeneration_hex(101,12),100));
   check(w.degeneration_hex(f.target,f.source,*skills::effects::degeneration_hex(150,12),100));
   check(w.status_mutation(f.source,307,81,12,100,8100,true));
   if(npc)check(w.stage_npc_reversal(adjacent,12,100));
   else check(w.status_mutation(adjacent,307,81,12,100,8100,true));
  }
  const auto adjacent_health=[&](){return npc?w.npc_combat_snapshot(adjacent)->health->current:double(w.resources(3)->health);};
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),300));w.tick(400);
  const unsigned first=reversal?1000:995+equipment;
  check(w.resources(1)->health==first&&adjacent_health()==(reversal?500:495));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),1500));w.tick(1600);
  check(w.resources(1)->health==first&&adjacent_health()==(reversal?500:495));
  // Energy Burn adds no conditional Deep Wound to the doubly hexed shooter.
  const auto burn=*skills::effects::energy_drain(42,12);
  check(w.resources(1)->energy>=burn.amount);
  check(w.energy_drain(f.target,f.source,burn,20,1601));
  check(w.resources(1)->health==first-burn.amount*burn.damage_per_energy+15);
  const auto followup=*skills::effects::wound_spell(1052,12);
  if(npc){
   check(w.damage_npc(f.target,adjacent,{combat::Kind::armor_ignoring,50,20,42},1248,1601).reason==combat::Reason::accepted);
   check(adjacent_health()==(reversal?465:460));
  }else{
   check(w.wound_spell(f.target,adjacent,followup,20,1601));
   check(adjacent_health()==(reversal?500:495)-followup.damage+15);
  }
 }
 // An earlier recipient's Seed heals a later recipient before its base hit.
 // Fresh protection/Health must be resolved without restoring the old snapshot.
 {
  warrior_test::Fixture f(399,false);auto& w=f.world;
  w.set_vitals(1,{500,1000,30,30},100);
  w.join(3,"Seed caster",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.set_vitals(3,{400,1000,100,100},100);const auto monk=*w.effect_scope(3);
  check(w.party_action(3,158,1,100)&&w.party_action(1,154,3,100));
  const auto seed=*skills::effects::healing_seed(12);
  check(w.complete_healing_seed(monk,f.source,seed,0,100));
  check(w.status_mutation(f.source,1399,80,12,100,6100,true));
  check(w.status_mutation(monk,1399,80,12,100,6100,true));
  check(w.ordinary_weapon(f.target,{},20));w.configure_attack_block_draws([]{return .1;});
  check(w.status_mutation(f.target,450,123,12,100,18100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),300));w.tick(400);
  check(w.resources(1)->health==495+seed.healing);
  check(w.resources(3)->health==395+seed.healing);
 }
 for(unsigned rank:{0u,12u,20u}){
  warrior_test::Fixture f(399,false);auto& w=f.world;
  skills::Policy policy;policy.primary=2;policy.campaigns=0x1e;policy.learned[450]=1;
  skills::Slots bar{};bar[0]={450,0};attributes::Ranks ranks{};ranks[23]=rank;
  skills::casting::Hooks h;std::vector<world::Packet> packets;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
  h.resources=[&](unsigned id){return w.resources(id);};
  h.target_valid=[](unsigned id,float,bool self){return id==1&&self;};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};
  h.status_mutation=[&](auto skill,auto id,auto r,auto at,auto until,bool active){return w.status_mutation(f.source,skill,id,r,at,until,active);};
  h.consume_energy_at=[&](unsigned cost,auto at){return w.consume_energy(1,cost,f.source,at);};
  skills::casting::Runtime runtime(1);
  check(runtime.start(300,{450,0,1},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
  runtime.update(300,h);
  const auto entries=w.status_snapshot(f.source,300);check(entries&&std::any_of(entries->begin(),entries->end(),[](const auto& e){return e.skill==450;}));
  check(runtime.start(301,{450,0,1},bar,policy,ranks,true,h)==skills::casting::Reason::recharge);
  runtime.update(300+skills::effects::whirling_defense(rank)->duration,h);
  check(w.status_snapshot(f.source,300+skills::effects::whirling_defense(rank)->duration)->empty());
 }
 // Bow contact at60 units: source is adjacent to defender. Guardian must
 // still cause retaliation while Whirling is active (the documented bug).
 for(unsigned mode=0;mode<5;++mode){
  warrior_test::Fixture f(399,false);auto& w=f.world;
  check(w.ordinary_weapon(f.target,{},20));w.configure_attack_block_draws([]{return .1;});
  check(w.status_mutation(f.target,450,123,12,100,18100,true));
  if(mode==1)check(w.status_mutation(f.target,258,124,0,100,2100,true));
  if(mode==2)check(w.status_mutation(f.target,137,124,0,100,8100,true));
  if(mode==3)check(w.status_mutation(f.target,450,123,12,200,{},false));
  if(mode==4)check(w.physical_absorption(f.source,3));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),300));w.tick(400);
  check(w.resources(1)->health==(mode==2||mode==3?1000:mode==4?993:990));
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==1);
  if(mode!=2&&mode!=3)check(outcomes.front().outcome==combat::weapon::AttackOutcome::block&&f.health()==1000);
  w.tick(500);check(w.resources(1)->health==(mode==2||mode==3?1000:mode==4?993:990));
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;check(w.ordinary_weapon(f.target,{},20));
  w.configure_attack_block_draws([]{return .1;});check(w.status_mutation(f.target,450,123,12,100,18100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),300));
  check(w.resources(1)->health==1000&&f.health()==1000);
 }
 {warrior_test::Fixture f(399,false);auto& w=f.world;
  check(w.ordinary_weapon(f.target,{},20));w.configure_attack_block_draws([]{return .1;});
  w.set_vitals(1,{500,1000,30,30},100);
  w.join(3,"Dying Recipient",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,3);
  w.set_vitals(3,{1,1000,30,30},100);const auto later=*w.effect_scope(3);
  check(w.apply_parasitic_bond(f.source,later,0,100));
  check(w.apply_condition(f.source,later,combat::conditions::Kind::bleeding,100,5000));
  check(w.status_mutation(f.target,450,123,12,100,18100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(399,12),300));w.tick(400);
  // Advancing the later recipient ends its Bond and heals the earlier shooter
  // by30. Retaliation must start from that settled530, not overwrite it with490.
  check(w.resources(3)->health==0);check(w.resources(1)->health==520);
  w.tick(500);check(w.resources(1)->health==520);
 }
 std::cout<<"PASS Whirling World bow retaliation,other-source blocks,Rigor,removal,physical reduction,melee exclusion,settled linked healing\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


