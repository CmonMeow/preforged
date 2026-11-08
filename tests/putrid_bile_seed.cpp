#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Bile Seed line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;effects::Scope caster,dead,early,later,monk,ally,enemy;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(std::optional<bool> allowed,unsigned late_hp=40,bool seeds=true){
  std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?910:700;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.spawn_health={500,1000};d.damageable=true;
   d.relation=id==100?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
   if(id==100){d.healing_allowed=allowed;if(allowed.has_value())d.healing_evidence="synthetic Bile Seed";}
   else d.conditions_allowed=true;n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);
  for(unsigned id:{1u,2u,3u,4u,5u}){
   w.join(id,"Actor",id==1?0:id==2||id==3?600:id==4?760:920,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==4?late_hp:500u,1000,100,100},100);check(w.energy_capacity(id,6,0,100,{80,-2}));
  }
  caster=*w.effect_scope(1);dead=*w.effect_scope(2);early=*w.effect_scope(3);later=*w.effect_scope(4);monk=*w.effect_scope(5);
  ally=*w.effect_scope(100);enemy=*w.effect_scope(101);
  check(w.party_action(4,158,5,100)&&w.party_action(5,154,4,100));
  if(seeds){check(w.complete_healing_seed(monk,early,*effects::healing_seed(12),0,100));check(w.complete_healing_seed(monk,later,*effects::healing_seed(12),0,100));}
 }
 void bile(effects::Scope target,unsigned rank){check(w.degeneration_hex(caster,target,*effects::degeneration_hex(2058,rank),100,20));}
};
int main()try{
 const auto seed=*effects::healing_seed(12);unsigned cases=0;
 for(int allowed:{-1,0,1})for(bool shield:{false,true})for(int reversal:{-1,0,20}){
  ++cases;Fixture f(allowed<0?std::optional<bool>{}:std::optional(bool(allowed)));
  if(shield)for(auto target:{f.early,f.later})check(f.w.status_mutation(target,1399,800+target.actor,12,100,6100,true));
  if(reversal>=0)check(f.w.status_mutation(f.later,307,900,reversal,100,8100,true));
  f.bile(f.dead,12);f.box->drain();check(f.w.kill(f.dead,101));
  const unsigned raw=effects::putrid_bile(12)->death_damage,damage=raw-(shield?5:0);
  const unsigned convert=reversal<0?0:std::min(damage,effects::reversal_fortune(reversal)->maximum_healing);
  const bool late_seed=damage>convert,blocked=allowed<0&&late_seed;
  check(f.w.resources(2)->health==0);
  if(blocked){
   check(f.w.pending_putrid_deaths()==1&&f.w.resources(3)->health==500&&f.w.resources(4)->health==40);
   check(f.w.resources(5)->health==500&&f.w.npc_combat_snapshot(f.enemy)->health->current==500);
   f.w.tick(20000);check(f.w.pending_putrid_deaths()==1&&f.w.resources(3)->health==500);
   f.w.leave(1);check(!f.w.pending_putrid_deaths());continue;
  }
  check(!f.w.pending_putrid_deaths());
  check(f.w.resources(3)->health==500-damage+seed.healing+(late_seed?seed.healing:0));
  check(f.w.resources(4)->health==40+seed.healing+convert-(damage-convert)+(late_seed?seed.healing:0));
  check(f.w.resources(5)->health==500+(late_seed?seed.healing:0));
  check(f.w.npc_combat_snapshot(f.ally)->health->current==500+(late_seed&&allowed==1?seed.healing:0));
  check(f.w.npc_combat_snapshot(f.enemy)->health->current==500-raw);
  const auto packets=f.box->drain();check(!f.w.kill(f.dead,102));f.w.tick(102);check(!f.w.pending_putrid_deaths());
 }
 // A nested Bile death must be admitted before any first-explosion recipient
 // loses HP. On success, cumulative protection advances across both hits.
 for(int allowed:{-1,0,1}){
  ++cases;Fixture f(allowed<0?std::optional<bool>{}:std::optional(bool(allowed)),500,false);
  f.w.set_vitals(3,{10,1000,100,100},100);f.bile(f.dead,0);f.bile(f.early,0);
  check(f.w.complete_healing_seed(f.monk,f.later,seed,0,100));
  check(f.w.status_mutation(f.later,1399,800,12,100,6100,true));
  f.box->drain();check(f.w.kill(f.dead,101));
  if(allowed<0){check(f.w.resources(3)->health==10&&f.w.resources(4)->health==500&&f.w.pending_putrid_deaths()==1);continue;}
  check(f.w.resources(3)->health==0&&f.w.resources(4)->health==515&&f.w.resources(5)->health==550);
  check(f.w.npc_combat_snapshot(f.enemy)->health->current==450&&!f.w.pending_putrid_deaths());
  const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,1,2058))==5);
 }
 std::cout<<"PASS "<<cases<<" Bile Seed/protection/conversion, retained death and recursive admission cases\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}
