#include "gw_npc_combat_catalog.h"
#include "gw_chilblains_npc.h"
#include "gw_watchful_healing_npc.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Watchful NPC line "+std::to_string(at.line()));
}
combat::npcs::LiveActor actor(double health=300,combat::npcs::Relation relation=combat::npcs::Relation::allied_to_players,std::optional<int> armor={}){
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
 d.relation=relation;d.spawn_health=combat::Health{health,500};d.damageable=true;d.conditions_allowed=true;
 d.cold_armor=armor;
 return {d,{1,1,1,146,100}};
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){
  const auto plan=*skills::effects::watchful_healing(rank);
  auto n=actor();const auto life=n.lifetime();
  auto stale=life;++stale.actor_generation;
  check(!n.apply_watchful_healing(stale,plan,9,1,1000));
  auto forged=plan;++forged.gain;check(!n.apply_watchful_healing(life,forged,9,1,1000));
  check(!n.apply_watchful_healing(life,plan,0,1,1000));
  check(n.apply_watchful_healing(life,plan,9,1,1000));
  check(!n.apply_watchful_healing(life,plan,9,1,1000));
  n.advance_conditions(life,2000);
  const auto regenerated=300.+2.*plan.pips;
  check(n.actor(life)->health->current==regenerated);
  check(!n.remove_reviewed_enchantment(life,1,1500));
  check(n.remove_reviewed_enchantment(life,1,2000));
  const auto gained=std::min(500.,regenerated+plan.gain);
  check(n.actor(life)->health->current==gained);
  auto stale_life=life;++stale_life.actor_generation;
  check(n.take_enchantment_ends(stale_life).empty());
  const auto ends=n.take_enchantment_ends(life);check(ends.size()==1&&n.take_enchantment_ends(life).empty());
  const skills::effects::Scope caster{1,9,3},target=n.hex_scope();
  const skills::effects::WatchfulOwnership owner{caster,target,1000};
  const auto packets=skills::effects::watchful_npc_end_packets(ends.front(),target,owner,caster);
  check(packets&&packets->size()==3&&(*packets)[1]==skills::particles::health_change_2022(100,9,gained-regenerated,500));
  check(packets->back()[0]==67);
  auto reused=caster;++reused.generation;
  check(skills::effects::watchful_npc_end_packets(ends.front(),target,owner,reused)->size()==2);
  check(skills::effects::watchful_npc_end_packets(ends.front(),target,owner,{})->size()==2);
  auto corrupt=ends.front();corrupt.gained=plan.gain+1;
  check(!skills::effects::watchful_npc_end_packets(corrupt,target,owner,caster));
  check(!n.remove_reviewed_enchantment(life,1,2000));
  n.advance_conditions(life,12000);check(n.actor(life)->health->current==gained);
 }
 const auto plan=*skills::effects::watchful_healing(12);
 {auto n=actor();const auto life=n.lifetime();check(n.apply_watchful_healing(life,plan,9,1,1000));
  n.refresh_hexes(life,12000);check(n.actor(life)->health->current==300.+20.*plan.pips);
  check(!n.hex_effect(life,1));
 }
 {auto n=actor();const auto life=n.lifetime();check(n.apply_watchful_healing(life,plan,9,1,1000));
  n.advance_conditions(life,12000);check(n.actor(life)->health->current==300.+20.*plan.pips);
  check(n.remove_reviewed_enchantment(life,1,12000));check(n.actor(life)->health->current==300.+20.*plan.pips);
 }
 {auto n=actor();const auto life=n.lifetime();check(n.apply_watchful_healing(life,plan,9,1,1000));
  check(n.apply_watchful_healing(life,plan,10,2,2000));check(!n.hex_effect(life,1));
  check(n.actor(life)->health->current==300.+2.*plan.pips);
  n.advance_conditions(life,13000);check(n.actor(life)->health->current==300.+22.*plan.pips);
 }
 {auto n=actor(490);const auto life=n.lifetime();check(n.apply_watchful_healing(life,plan,9,1,1000));
  check(n.remove_reviewed_enchantment(life,1,1000));check(n.actor(life)->health->current==500);
  const auto ends=n.take_enchantment_ends(life);check(ends.size()==1&&ends.front().gained==10);
  const skills::effects::Scope source{1,9,1};
  const auto packets=skills::effects::watchful_npc_end_packets(ends.front(),n.hex_scope(),{source,n.hex_scope(),1000},source);
  check(packets&&packets->size()==3&&(*packets)[1]==skills::particles::health_change_2022(100,9,10,500));
 }
 {auto n=actor();const auto life=n.lifetime();check(n.apply_watchful_healing(life,plan,9,1,1000));
  check(n.apply_condition(life,combat::conditions::Kind::deep_wound,2,1000,20000));
  const auto before=n.actor(life)->health->current;
  check(n.remove_reviewed_enchantment(life,1,1000));
  check(n.actor(life)->health->maximum==400);
  check(n.actor(life)->health->current==std::min(400.,before+plan.gain));
 }
 {auto n=actor(300,combat::npcs::Relation::hostile_to_players);const auto life=n.lifetime();check(n.apply_watchful_healing(life,plan,9,1,1000));
  check(n.apply_damage(life,{combat::Kind::armor_ignoring,1000,20,194},1500).killed);
  check(n.remove_reviewed_enchantment(life,1,1500));check(n.actor(life)->health->current==0);
  const auto ends=n.take_enchantment_ends(life);check(ends.size()==1&&ends.front().gained==0);
  const skills::effects::Scope source{1,9,1};
  check(skills::effects::watchful_npc_end_packets(ends.front(),n.hex_scope(),{source,n.hex_scope(),1000},source)->size()==2);
 }
 {auto n=actor(300,combat::npcs::Relation::hostile_to_players);const auto life=n.lifetime();
  check(n.apply_watchful_healing(life,plan,9,1,1000));
  // Unknown armor must reject the whole proposal without
  // stripping or granting Health on the input actor.
  const std::vector<skills::effects::ChilblainsNpcRecipient> inputs{{n,life,0}};
  check(!skills::effects::prepare_chilblains_npcs(*skills::effects::chilblains(12),{1,1,1},20,{},inputs,
   skills::effects::ChilblainsOrder::damage_then_remove,1000));
  check(n.hex_effect(life,1)&&n.actor(life)->health->current==300);
 }
 {auto n=actor(300,combat::npcs::Relation::hostile_to_players,60);const auto life=n.lifetime();
  check(n.apply_watchful_healing(life,plan,9,1,1000));
  const std::vector<skills::effects::ChilblainsNpcRecipient> inputs{{n,life,0}};
  const auto chil=*skills::effects::chilblains(12);
  const auto result=skills::effects::prepare_chilblains_npcs(chil,{1,1,1},20,{},inputs,
   skills::effects::ChilblainsOrder::damage_then_remove,1000);
  check(result&&result->recipients.size()==1);
  const auto& changed=result->recipients.front();
  check(changed.removed.size()==1&&changed.removed.front().skill==1392);
  check(std::abs(changed.actor.actor(life)->health->current-(300.-chil.damage+plan.gain))<1e-9);
  auto committed=changed.actor;const auto ends=committed.take_enchantment_ends(life);
  check(ends.size()==1&&ends.front().gained==plan.gain&&ends.front().at==1000);
  check(n.take_enchantment_ends(life).empty()); // Copy-only proposal did not leak publication into its input.
  check(!skills::effects::prepare_chilblains_npcs(chil,{1,1,1},20,{},inputs,
   skills::effects::ChilblainsOrder::remove_then_damage,1000));
  check(n.hex_effect(life,1)&&n.actor(life)->health->current==300);
 }
 std::cout<<"PASS Watchful NPC ranks,guards,regeneration,early gain,expiry,renewal,cap,wound,death\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
