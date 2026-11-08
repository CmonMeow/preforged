#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Watch NPC line "+std::to_string(at.line()));}
combat::npcs::Definition definition(){combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{10000,10000};d.damageable=d.conditions_allowed=true;d.earth_armor=d.physical_armor=d.fire_armor=d.cold_armor=d.lightning_armor=60;return d;}
int main()try{
 const Scope source{1,1,1};
 for(bool pvp:{false,true})for(unsigned rank=0;rank<=20;++rank){combat::npcs::LiveActor n(definition(),{1,1,1,146,100});const auto life=n.lifetime();const auto plan=*watch_yourself(rank,pvp);
  check(n.apply_watch_yourself(life,source,plan,1,100).has_value());
  check(n.final_physical_armor(life,100)==60+int(plan.armor));for(unsigned kind:{3u,4u,5u,11u})check(n.final_caster_armor(life,kind,100)==60+int(plan.armor));
  std::uint64_t sequence=0;const auto ignored=n.apply_damage(life,{combat::Kind::armor_ignoring,50,20,133},100);check(ignored.applied==50);
  const auto nocharge=n.consume_watch_yourself_damage(life,{n.hex_scope(),++sequence,100,true,false,ignored.applied});check(nocharge&&!nocharge->consumed&&nocharge->owned.remaining==plan.charges);
  for(unsigned hit=0;hit<plan.charges;++hit){const auto damage=n.apply_damage(life,{combat::Kind::earth,100,20,174},100);
   check(damage.reason==combat::Reason::accepted&&std::abs(damage.applied-100*std::exp2(-double(plan.armor)/40))<1e-7);
   const WatchYourselfDamage receipt{n.hex_scope(),++sequence,100,true,true,damage.applied};
   const auto step=n.consume_watch_yourself_damage(life,receipt);check(step&&step->consumed&&step->ended==(hit+1==plan.charges));check(!n.consume_watch_yourself_damage(life,receipt));}
  check(n.watch_yourself_armor(life,100)==0&&!n.hex_effect(life,1));const auto unbuffed=n.apply_damage(life,{combat::Kind::earth,100,20,174},100);check(std::abs(unbuffed.applied-100)<1e-7);
 }
 {combat::npcs::LiveActor n(definition(),{1,1,1,146,100});const auto life=n.lifetime();const auto plan=*watch_yourself(12);
  check(n.apply_watch_yourself(life,source,plan,1,100).has_value());auto preview=n;
  const WatchYourselfDamage receipt{n.hex_scope(),1,100,true,true,1};check(n.consume_watch_yourself_damage(life,receipt).has_value());
  check(preview.preserve_current_hexes(n,life,100)&&!preview.consume_watch_yourself_damage(life,receipt));
  const auto refreshed=n.apply_watch_yourself(life,source,*watch_yourself(0,true),2,200);check(refreshed&&refreshed->removed.size()==1&&!n.hex_effect(life,1));
  check(n.watch_yourself_armor(life,200)==5&&n.watch_yourself_armor(life,10199)==5&&n.watch_yourself_armor(life,10200)==0);
  check(n.remove_hex_effect(life,2,300)&&n.watch_yourself_armor(life,300)==0);
  check(n.apply_watch_yourself(life,source,plan,3,400).has_value());check(n.apply_damage(life,{combat::Kind::armor_ignoring,100000,20,133},400).killed&&n.watch_yourself_armor(life,400)==0);
 }
 {combat::npcs::LiveActor n(definition(),{1,1,1,146,100});const auto life=n.lifetime();auto stale=life;++stale.actor_generation;
  check(!n.apply_watch_yourself(stale,source,*watch_yourself(12),1,100));check(!n.apply_watch_yourself(life,{2,1,1},*watch_yourself(12),1,100));
 }
 for(unsigned rank:{0u,12u,20u}){combat::npcs::LiveActor n(definition(),{1,1,1,146,100});const auto life=n.lifetime();
  check(n.apply_single_degeneration_hex(life,*degeneration_hex(101,0),1,1,100).has_value());
  check(n.apply_convert_hexes(life,source,*convert_hexes(12),2,100).has_value());
  check(n.apply_watch_yourself(life,source,*watch_yourself(rank),3,100).has_value());
  const unsigned expected=rank==0?15:rank==12?25:32;check(n.final_earth_armor(life,100)==60+int(expected));
  const auto hit=n.apply_damage(life,{combat::Kind::earth,100,20,174},100);check(std::abs(hit.applied-100*std::exp2(-double(expected)/40))<1e-7);
 }
 std::cout<<"PASS42NPC Watch Yourself armor/damage/final-charge contexts,explicit committed receipts,ignore armor-ignoring,alias refresh,expiry/strip/death and preservation;World adapters pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

