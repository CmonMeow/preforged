#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Condition Seed cohort line "+std::to_string(at.line()));
}
int main()try{
 unsigned cases=0;
 for(bool deferred:{false,true})for(unsigned renewal:{0u,181u,2860u})for(int allowed:{-1,0,1})for(bool shield:{false,true})for(int reversal:{-1,0,20}){
  if(!deferred&&renewal)continue;
  ++cases;world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=240;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.spawn_health={500,1000};d.damageable=true;
  d.relation=combat::npcs::Relation::allied_to_players;
  if(allowed>=0){d.healing_allowed=bool(allowed);d.healing_evidence="synthetic condition cohort";}
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  std::map<unsigned,std::shared_ptr<world::Outbox>> boxes;
  for(unsigned id:{1u,2u,3u,4u}){
   auto peer=id==1?box:std::make_shared<world::Outbox>();boxes.emplace(id,peer);
   w.join(id,"Actor",id==1?0:id==2?100:id==3?110:260,0,peer,0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==3?1u:500u,1000,id==1?50u:100u,100},100);
   check(w.observe_condition_equipment(*w.effect_scope(id),equipment::condition_equipment(inventory::baseline(),0),100));
  }
  const auto source=*w.effect_scope(1),a=*w.effect_scope(2),b=*w.effect_scope(3),monk=*w.effect_scope(4),npc=*w.effect_scope(100);
  if(renewal){const auto plan=*effects::ether_renewal(renewal,0);check(w.status_mutation(source,renewal,80,0,100,100+plan.duration,true));}
  for(auto target:{a,b}){
   check(w.degeneration_hex(source,target,*effects::degeneration_hex(2998,12),100));
   check(w.complete_healing_seed(monk,target,*effects::healing_seed(12),0,100));
   if(shield)check(w.status_mutation(target,1399,800,12,100,6100,true));
  }
  if(reversal>=0)check(w.status_mutation(b,307,900,reversal,100,8100,true));
  for(const auto& [id,peer]:boxes)peer->drain();
  if(renewal)check(w.before_spell_effect(source,118,1,200));
  std::vector<world::Packet> visuals;std::function<void()> publication;
  const auto plan=*effects::condition_spell(118,12);
  const auto result=deferred?w.complete_condition_spell(source,a,plan,200,visuals,publication,renewal?std::optional<std::uint64_t>{1}:std::nullopt):w.condition_spell_equipped(source,a,plan,200);
  const auto packets=box->drain();
  const bool rewarded=deferred&&renewal&&allowed>=0;
  check(w.resources(1)->health==500+(rewarded?5:0)&&w.resources(1)->energy==50+(rewarded?1:0));
  if(deferred&&!rewarded){check(packets.empty());for(const auto& [id,peer]:boxes)check(peer->drain().empty());}
  if(allowed<0){
   check(!result&&packets.empty()&&visuals.empty()&&!publication&&w.resources(2)->health==500&&w.resources(3)->health==1);
   check(!w.has_condition(a,combat::conditions::Kind::weakness,200)&&!w.has_condition(b,combat::conditions::Kind::weakness,200));continue;
  }
  check(result);const unsigned loss=effects::fragility(2998,12)->damage-(shield?5:0),heal=effects::healing_seed(12)->healing;
  const unsigned converted=reversal<0?0:std::min(loss,effects::reversal_fortune(reversal)->maximum_healing);const bool second=loss>converted;
  check(w.resources(2)->health==500-loss+heal+(second?heal:0));
  check(w.resources(3)->health==1+heal+converted-(loss-converted)+(second?heal:0));
  check(w.resources(4)->health==500+heal+(second?heal:0));
  check(w.npc_combat_snapshot(npc)->health->current==500+(allowed?heal+(second?heal:0):0));
  for(auto target:{a,b,monk})check(w.has_condition(target,combat::conditions::Kind::weakness,200));
  if(deferred){
   check(bool(publication));publication();check(!box->drain().empty());
   for(const auto& [id,peer]:boxes)peer->drain();
   publication();for(const auto& [id,peer]:boxes)check(peer->drain().empty());
  }
 }
 check(cases==72);std::cout<<"72immediate/deferred condition/Fragility/Seed/Renewal cohort cases,publication and replay guards pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
