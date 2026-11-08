#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC Ice Spear regeneration line "+std::to_string(at.line()));
}
combat::npcs::LiveActor actor(double health=400){
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
 d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{health,500};d.damageable=true;d.conditions_allowed=true;
 return {d,{1,1,1,146,100}};
}
int main()try{
 const auto plan=*skills::effects::water_projectile(214,12);
 {auto n=actor();const auto life=n.lifetime();check(!n.apply_ice_spear_regeneration(life,plan,1,1000));
  check(n.observe_overcast(life,2,1000,"authored positive Overcast"));check(n.apply_ice_spear_regeneration(life,plan,1,1000));
  check(n.ice_spear_regeneration(life,1000)->expires==6000);n.advance_conditions(life,2000);check(n.actor(life)->health->current==406);
  check(n.apply_ice_spear_regeneration(life,*skills::effects::water_projectile(214,15),2,2000));
  check(!n.hex_effect(life,1)&&n.ice_spear_regeneration(life,2000)->instance==2);n.advance_conditions(life,8000);
  check(n.actor(life)->health->current==446&&!n.ice_spear_regeneration(life,8000));
 }
 {auto n=actor();const auto life=n.lifetime();check(n.observe_overcast(life,2,1000,"fixture")&&n.apply_ice_spear_regeneration(life,plan,1,1000));
  check(n.remove_hex_effect(life,1,2500));check(n.actor(life)->health->current==409);n.advance_conditions(life,7000);check(n.actor(life)->health->current==409);
 }
 {auto n=actor();const auto life=n.lifetime();check(n.observe_overcast(life,2,1000,"fixture")&&n.apply_ice_spear_regeneration(life,plan,1,1000));
  check(n.apply_condition(life,combat::conditions::Kind::poison,2,1000,2000));n.advance_conditions(life,6000);
  check(n.actor(life)->health->current==414); // -1 pip for2s, then +3 pips for3s; one delayed tick crosses sign
 }
 {auto n=actor(490);const auto life=n.lifetime();check(n.observe_overcast(life,1,1000,"fixture")&&n.apply_ice_spear_regeneration(life,plan,1,1000));
  n.advance_conditions(life,7000);check(n.actor(life)->health->current==500);
 }
 {auto n=actor(490);const auto life=n.lifetime();check(n.observe_overcast(life,1,1000,"fixture")&&n.apply_ice_spear_regeneration(life,plan,1,1000));
  check(n.apply_condition(life,combat::conditions::Kind::deep_wound,2,1000,10000));n.advance_conditions(life,6000);
  check(n.actor(life)->health->maximum==400&&n.actor(life)->health->current<=400);
 }
 {auto n=actor();const auto life=n.lifetime();check(n.observe_overcast(life,1,1000,"fixture")&&n.apply_ice_spear_regeneration(life,plan,1,1000));
  check(n.apply_damage(life,{combat::Kind::armor_ignoring,1000,20,194},1500).killed);n.advance_conditions(life,7000);
  check(n.actor(life)->health->current==0);n.refresh_hexes(life,7000);check(!n.hex_effect(life,1));
 }
 {auto n=actor(300);const auto life=n.lifetime();check(n.observe_overcast(life,1,1000,"fixture")&&n.apply_ice_spear_regeneration(life,plan,1,1000));
  check(n.clear_conditions(life,2000));const auto health=n.actor(life)->health->current;
  n.advance_conditions(life,3000);check(n.actor(life)->health->current>health);
 }
 std::cout<<"PASS NPC Ice Spear regeneration renewal,removal,expiry,sign crossing,modified cap and death\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
