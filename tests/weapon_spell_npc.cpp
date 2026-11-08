#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("weapon spell NPC line "+std::to_string(at.line()));}
combat::npcs::LiveActor actor(){combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=true;d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="fixture";d.spawn_health=combat::Health{500,1000};d.physical_armor=60;d.physical_armor_evidence="fixture";d.lightning_armor=60;d.lightning_armor_evidence="fixture";return combat::npcs::LiveActor(d,{1,1,3,146,100});}
int main()try{
 {auto npc=actor();const auto life=npc.lifetime();check(npc.apply_weapon_spell(life,{1,1,2},*weapon_spell(793,0,8,0),7,100));
  npc.refresh_hexes(life,4100);check(npc.actor(life)->health->current==512);check(!npc.weapon_spell_snapshot(life,4100));
  const auto ended=npc.retire_weapon_spell(life,4100);check(ended&&ended->instance==7&&!npc.retire_weapon_spell(life,4100));}
 {auto npc=actor();const auto life=npc.lifetime();check(npc.apply_weapon_spell(life,{1,1,2},*weapon_spell(787,0,8,0),7,100));
  check(npc.final_physical_armor(life,100)==60);check(npc.apply_condition(life,combat::conditions::Kind::weakness,1,100,1000));
  check(npc.final_physical_armor(life,100)==84);npc.advance_conditions(life,2100);check(npc.actor(life)->health->current==502&&npc.final_physical_armor(life,2100)==60);}
 {auto npc=actor();const auto life=npc.lifetime();check(npc.apply_weapon_spell(life,{1,1,2},*weapon_spell(793,0,8,0),7,100));
  const auto hit=npc.apply_damage(life,{combat::Kind::armor_ignoring,1000,20,219},100);check(hit.killed);
  const auto ended=npc.retire_weapon_spell(life,100);check(ended&&!npc.owned_effect_snapshot(life)->lookup(npc.hex_scope(),7));check(!(npc.actor(life)->effects&0x8000));}
 {auto npc=actor();const auto life=npc.lifetime();check(npc.apply_weapon_spell(life,{1,1,2},*weapon_spell(2893,0,8,0),7,100));
  check(npc.end_weapon_spell_on_attack(life,1000)&&!npc.weapon_spell_snapshot(life,1000));check(std::abs(npc.actor(life)->health->current-503.6)<1e-9);}
 {auto npc=actor();const auto life=npc.lifetime();check(npc.apply_weapon_spell(life,{1,1,2},*weapon_spell(787,0,8,20),7,100));
  check(bool(npc.apply_lightning_surge_hex(life,0,1,8,200)));const auto registry=npc.owned_effect_snapshot(life);check(bool(registry));
  const auto ending=EndingHexContext::capture(*registry,npc.hex_scope(),8,3200);check(bool(ending));
  npc.refresh_hexes(life,3200);check(!npc.newest_hex(life,3200));auto ordinary=npc;
  const auto hit=npc.apply_damage(life,{combat::Kind::lightning,100,20,205},3200,&*ending);
  const auto plain=ordinary.apply_damage(life,{combat::Kind::lightning,100,20,205},3200);
  check(hit.reason==combat::Reason::accepted&&plain.reason==combat::Reason::accepted);
  check(std::abs(hit.applied-100*std::exp2(-24./40))<1e-9&&std::abs(plain.applied-100)<1e-9);
  check(npc.final_caster_armor(life,4,3200)==60);}
 for(bool retained:{false,true}){auto npc=actor();const auto life=npc.lifetime();
  check(npc.apply_weapon_spell(life,{1,1,2},*weapon_spell(787,0,8,20),7,100));
  check(npc.apply_lightning_surge_hex(life,0,1,8,200).has_value()&&npc.apply_reversal_fortune(life,12,100,9,200));
  const auto registry=*npc.owned_effect_snapshot(life);const auto ending=EndingHexContext::capture(registry,npc.hex_scope(),8,3200);check(ending.has_value());
  npc.refresh_hexes(life,3200);const auto receipt=npc.apply_reversal_damage(life,{combat::Kind::lightning,100,20,205},3200,true,retained?&*ending:nullptr);
  check(receipt.has_value());const double raw=retained?100*std::exp2(-24./40):100.;
  check(std::abs(receipt->quote.reduced_amount-raw)<1e-9);
 }
 std::cout<<"weapon spell NPC scoped lifecycle passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
