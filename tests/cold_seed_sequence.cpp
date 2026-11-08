#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace e=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("cold Seed line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{209u,2803u,1382u})for(int eligibility:{-1,0,1}){
  world::Instance w;
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=110;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};
  d.healing_allowed=eligibility<0?std::optional<bool>{}:std::optional(bool(eligibility));if(eligibility>=0)d.healing_evidence="fixture";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,1000,id==1?80u:20u,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),monk=*w.effect_scope(3);const auto seed=*e::healing_seed(12);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));check(w.complete_healing_seed(monk,target,seed,0,100));
  if(skill==1382)check(w.movement_hex(source,target,*e::movement_hex(210,12),200));
  const unsigned damage=skill==1382?e::freezing_gust(12)->damage:e::mind_freeze(skill,12)->damage;
  const unsigned hp=skill==1382?500:2*damage-seed.healing+1;w.set_vitals(2,{hp,1000,20,100},100);
  const bool ok=skill==1382?w.freezing_gust(source,target,*e::movement_hex(1382,12),20,1000):w.mind_freeze(source,target,*e::mind_freeze(skill,12),20,1000);
  check(ok==(eligibility>=0));
  if(!ok){check(w.resources(2)->health==hp&&w.resources(3)->health==500);continue;}
  const unsigned hits=skill==1382?1:2;
  check(w.resources(2)->health==hp-hits*damage+hits*seed.healing);check(w.resources(3)->health==500+hits*seed.healing);
  check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==500+(eligibility?hits*seed.healing:0));
  if(skill!=1382){const auto status=*w.status_snapshot(target,1000);check(std::any_of(status.begin(),status.end(),[&](const auto& entry){return entry.skill==skill;}));}
 }
 // One PvP Watch Yourself charge expires only on positive remaining damage.
 // Full307conversion preserves it for hit2; partial conversion consumes onhit1.
 for(int reversal:{-1,0,20}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,1000,id==1?80u:20u,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),monk=*w.effect_scope(3);const auto seed=*e::healing_seed(12);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));check(w.complete_healing_seed(monk,target,seed,0,100));
  check(w.stage_watch_yourself_recipient(monk,target,*e::watch_yourself(0,true),100));
  if(reversal>=0)check(w.status_mutation(target,307,801,unsigned(reversal),100,8100,true));
  box->drain();const auto freeze=*e::mind_freeze(209,12);std::vector<world::Packet> deferred;
  check(w.mind_freeze(source,target,freeze,20,1000,&deferred));
  const double first=freeze.damage*std::exp2(-5./40.);
  const double converted=reversal<0?0:std::min(first,double(e::reversal_fortune(unsigned(reversal))->maximum_healing));
  const double second=converted==first?first:double(freeze.damage);
  const unsigned triggers=converted==first?1:2;
  check(std::abs(double(w.resources(2)->health)-(500+converted-(first-converted)-second+triggers*seed.healing))<1.1);
  check(w.resources(3)->health==500+triggers*seed.healing&&w.watch_yourself_armor(target,1000)==0);
  const auto packets=box->drain();
  const auto damage=std::find(packets.begin(),packets.end(),combat::detail::integer(10,1,209));
  const auto healing=std::find(packets.begin(),packets.end(),skills::particles::health_change_2022(3,3,seed.healing,1000));
  check(damage!=packets.end()&&healing!=packets.end()&&damage<healing);
  check(std::find(deferred.begin(),deferred.end(),combat::detail::integer(10,1,209))==deferred.end());
 }
 std::cout<<"PASS cold sequence Seed feedback and atomic unknown-recipient refusal\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}
