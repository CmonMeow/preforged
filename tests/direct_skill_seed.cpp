#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace e=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("direct Seed line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{2103u,2193u})for(int eligible:{-1,0,1})for(int reversal:{-1,20})for(bool active:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();world::Instance::Npc n;n.id=n.definition.id=100;n.x=110;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.healing_allowed=eligible<0?std::optional<bool>{}:std::optional(bool(eligible));if(eligible>=0)d.healing_evidence="fixture";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?0:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,1000,id==1&&!active&&skill==2193?0u:100u,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),monk=*w.effect_scope(3);const auto seed=*e::healing_seed(12);
  check(w.party_action(2,158,3,100)&&w.party_action(3,154,2,100));check(w.complete_healing_seed(monk,target,seed,0,100));
  if(skill==2103&&active)check(w.degeneration_hex(source,target,*e::degeneration_hex(31,0),100));
  if(reversal>=0)check(w.status_mutation(target,307,801,unsigned(reversal),100,8100,true));
  box->drain();std::vector<world::Packet> deferred;
  const auto before=w.resources(2)->health;
  const unsigned damage=!active?0:skill==2103?e::necrosis(5)->damage:130;
  const double converted=reversal<0?0:std::min(double(damage),double(e::reversal_fortune(unsigned(reversal))->maximum_healing));
  const bool trigger=damage>converted;
  const bool accepted=eligible>=0||!trigger;
  const bool ok=skill==2103?w.necrosis(source,target,*e::necrosis(5),20,100,&deferred):w.energy_blast(source,target,*e::energy_blast(12),20,100,&deferred);
  check(ok==accepted);
  check(w.resources(2)->health==(accepted?before+converted-(damage-converted)+(trigger?seed.healing:0):before));
  check(w.resources(3)->health==500+(accepted&&trigger?seed.healing:0));
  check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==500+(accepted&&trigger&&eligible==1?seed.healing:0));
  const auto packets=box->drain();const auto hit=combat::detail::integer(10,1,skill),heal=skills::particles::health_change_2022(3,3,seed.healing,1000);
  if(accepted&&trigger){const auto a=std::find(packets.begin(),packets.end(),hit),b=std::find(packets.begin(),packets.end(),heal);check(a!=packets.end()&&b!=packets.end()&&a<b);}
  if(!accepted)check(std::find(packets.begin(),packets.end(),hit)==packets.end()&&deferred.empty());
  check(std::find(deferred.begin(),deferred.end(),hit)==deferred.end());
 }
 std::cout<<"PASS direct skill Seed eligibility, conversion, no-effect and causal publication\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<'\n';return 1;}
