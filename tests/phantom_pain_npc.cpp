#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Phantom Pain NPC line "+std::to_string(at.line()));
}
combat::npcs::LiveActor npc(double health=480,bool conditions=true){
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=100;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{health,480};
    d.damageable=true;d.conditions_allowed=conditions;return {d,{1,1,1,146,100}};
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){
    const auto plan=*skills::effects::wound_spell(44,rank);
    auto coarse=npc(),fine=npc();const auto life=coarse.lifetime();
    check(coarse.apply_phantom_pain(life,plan,1,7,100).has_value());
    check(fine.apply_phantom_pain(life,plan,1,7,100).has_value());
    check(fine.advance_conditions(life,10099).has_value());
    check(!fine.has_condition(life,combat::conditions::Kind::deep_wound,10099));
    check(fine.advance_conditions(life,10100).has_value());
    check(fine.has_condition(life,combat::conditions::Kind::deep_wound,10100));
    check(fine.actor(life)->health->maximum==384);
    check(!fine.hex_registry_snapshot(life,10100)->lookup(fine.hex_scope(),7));
    const auto end=10100+plan.wound_duration;
    check(fine.advance_conditions(life,end+100).has_value());
    check(coarse.advance_conditions(life,end+100).has_value());
    check(!coarse.has_condition(life,combat::conditions::Kind::deep_wound,end+100));
    check(coarse.actor(life)->health->maximum==480);
    check(coarse.actor(life)->health->current==fine.actor(life)->health->current);
    auto removed=npc();check(removed.apply_phantom_pain(life,plan,1,7,100).has_value());
    check(removed.remove_hex_effect(life,7,1100));
    check(removed.has_condition(life,combat::conditions::Kind::deep_wound,1100));
    check(!removed.remove_hex_effect(life,7,1100));
    auto renewed=npc();check(renewed.apply_phantom_pain(life,plan,1,7,100).has_value());
    check(renewed.apply_phantom_pain(life,plan,1,8,1100).has_value());
    check(!renewed.has_condition(life,combat::conditions::Kind::deep_wound,1100));
    check(renewed.advance_conditions(life,10100).has_value());
    check(!renewed.has_condition(life,combat::conditions::Kind::deep_wound,10100));
    check(renewed.advance_conditions(life,11100).has_value());
    check(renewed.has_condition(life,combat::conditions::Kind::deep_wound,11100));
 }
 const auto plan=*skills::effects::wound_spell(44,12);
 auto dead=npc(1);check(dead.apply_phantom_pain(dead.lifetime(),plan,1,7,100).has_value());
 check(dead.advance_conditions(dead.lifetime(),10100)->killed);
 check(!dead.has_condition(dead.lifetime(),combat::conditions::Kind::deep_wound,10100));
 auto immune=npc(480,false);check(immune.apply_phantom_pain(immune.lifetime(),plan,1,7,100).has_value());
 check(immune.advance_conditions(immune.lifetime(),10100).has_value());
 check(!immune.has_condition(immune.lifetime(),combat::conditions::Kind::deep_wound,10100));
 auto stale=npc();auto wrong=stale.lifetime();++wrong.actor;
 check(!stale.apply_phantom_pain(wrong,plan,1,7,100));
 check(!stale.apply_phantom_pain(stale.lifetime(),plan,1,7,UINT64_MAX-10000));
 {
  using namespace skills::effects;
  const Scope caster{1,1,1};
  const std::vector<Entry> stance{{9,1,14,12,Category::stance,100,60100}};
  const auto adjusted=hex_duration(44,plan.hex_duration,caster,stance,100);check(adjusted.has_value());
  auto extended=npc();check(extended.apply_phantom_pain(extended.lifetime(),plan,1,7,100,*adjusted).has_value());
  check(extended.advance_conditions(extended.lifetime(),10100).has_value());
  check(!extended.has_condition(extended.lifetime(),combat::conditions::Kind::deep_wound,10100));
  check(extended.advance_conditions(extended.lifetime(),13500).has_value());
  check(extended.has_condition(extended.lifetime(),combat::conditions::Kind::deep_wound,13500));
  const auto unrelated=hex_duration(31,10000,caster,{},100);
  auto refused=npc();check(!refused.apply_phantom_pain(refused.lifetime(),plan,1,7,100,*unrelated));
 }
 std::cout<<"Phantom Pain NPC lifecycle passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
