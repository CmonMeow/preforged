#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Zealous Benediction line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills;
 const auto effect=effects::direct_heal(1687);
 check(effect&&effect->self&&effect->range==1248&&effect->divine_multiplier==1&&!effect->below_half_bonus);
 for(unsigned rank=0;rank<=20;++rank){
  const unsigned expected=effects::heal_scale(30,180,rank);
  check(effects::direct_heal_amount(1687,rank,0,499,1000)==expected);
  check(effects::direct_heal_amount(1687,rank,0,500,1000)==expected);
 }
 check(!effects::direct_heal_amount(1687,21,0,499,1000));
 for(unsigned health:{499u,500u,501u}){
  warrior_test::Fixture f(320,false);auto& w=f.world;
  w.set_vitals(2,{health,1000,10,30},100);
  check(w.zealous_benediction(f.target,f.target,12,0,100));
  check(w.resources(2)->health==health+150);
  check(w.resources(2)->energy==(health<500?17u:10u));
  check(!w.zealous_benediction(f.source,f.target,12,0,100)); // opposing team
  check(!w.zealous_benediction(f.target,{f.target.instance,2,f.target.generation+1},12,0,100));
 }
 {
  world::Instance w;auto first=std::make_shared<world::Outbox>(),second=std::make_shared<world::Outbox>();
  w.join(1,"Monk",0,0,first,0,{}, {},100,1);
  w.join(2,"Ally",60,0,second,0,{}, {},100,1);
  w.set_vitals(1,{900,1000,5,30},100);w.set_vitals(2,{499,1000,10,30},100);
  check(w.zealous_benediction(*w.effect_scope(1),*w.effect_scope(2),12,0,100));
  check(w.resources(2)->health==649&&w.resources(2)->energy==10);
  check(w.resources(1)->energy==12); // Return belongs to the caster.
 }
 {
  warrior_test::Fixture f(320,false);auto& w=f.world;
  w.set_vitals(2,{499,1000,25,30},100);
  unsigned completed=0;std::vector<world::Packet> wire;casting::Hooks hooks;
  hooks.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};
  hooks.resources=[&](auto id){return w.resources(id);};
  hooks.consume_energy_at=[&](auto cost,auto at){check(cost==10);return w.consume_energy(2,cost,f.target,at);};
  hooks.target_valid=[](auto id,float range,bool self){return id==2&&range==1248&&self;};
  hooks.zealous_target_valid=[&](auto id,float range,auto at){return w.valid_shatter_ally(f.target,id,range,at);};
  hooks.target_scope=[&](auto id){return w.effect_scope(id);};
  hooks.heal_scoped=[](auto,auto,auto,auto,auto){throw std::runtime_error("generic heal bypassed specialized reward");return false;};
  hooks.zealous_benediction=[&](auto target,auto rank,auto divine,auto at){++completed;return w.zealous_benediction(f.target,target,rank,divine,at);};
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[1687]=1;
  Slots bar{};bar[0]={1687,0};attributes::Ranks ranks{};ranks[15]=12;
  casting::Runtime cast(2);check(casting::Runtime::effect_supported(1687));
  check(cast.start(1000,{1687,0,2},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
  cast.update(1749,hooks);check(completed==0);
  cast.update(1750,hooks);check(completed==1&&w.resources(2)->health>=649);
  check(std::count(wire.begin(),wire.end(),*skills::particles::target_body_2022(2,1687))==1);
  check(cast.start(1800,{1687,0,2},bar,policy,ranks,true,hooks)==casting::Reason::busy);
  check(cast.start(2500,{1687,0,2},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
 }
 {
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=60;npc.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition definition;definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
  definition.relation=combat::npcs::Relation::allied_to_players;definition.spawn_health={499,1000};
  definition.damageable=true;definition.healing_allowed=true;definition.healing_evidence="test";
  definition.physical_armor=60;definition.physical_armor_evidence="test";npc.combat_definition=definition;
  w.configure_npcs({npc});w.join(1,"Monk",0,0,box,0,{}, {},100,1);w.set_vitals(1,{900,1000,25,30},100);
  const auto caster=*w.effect_scope(1),ally=*w.effect_scope(100);
  check(w.valid_shatter_ally(caster,100,1248,1000));
  casting::Hooks hooks;std::vector<world::Packet> wire;
  hooks.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};
  hooks.resources=[&](auto id){return w.resources(id);};
  hooks.consume_energy_at=[&](auto cost,auto at){return w.consume_energy(1,cost,caster,at);};
  hooks.target_valid=[](auto,float,bool){return false;}; // Generic player-only path must not admit the NPC.
  hooks.zealous_target_valid=[&](auto id,float range,auto at){return w.valid_shatter_ally(caster,id,range,at);};
  hooks.target_scope=[&](auto id){return w.effect_scope(id);};
  hooks.heal_scoped=[](auto,auto,auto,auto,auto){return false;};
  hooks.zealous_benediction=[&](auto target,auto rank,auto divine,auto at){return w.zealous_benediction(caster,target,rank,divine,at);};
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[1687]=1;
  Slots bar{};bar[0]={1687,0};attributes::Ranks ranks{};ranks[15]=12;
  casting::Runtime cast(1);
  check(cast.start(1000,{1687,0,100},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
  cast.update(1750,hooks);
  check(w.npc_combat_snapshot(ally)->health->current==649);
  check(w.resources(1)->energy>=22); // Paid 10, then returned 7 (plus elapsed regeneration).
  check(std::count(wire.begin(),wire.end(),*skills::particles::target_body_2022(100,1687))==1);
 }
 std::cout<<"PASS Zealous Benediction locked pre-heal threshold, heal, Energy reward, cast timing and body effect\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
