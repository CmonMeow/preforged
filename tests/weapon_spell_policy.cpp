#include "gw_weapon_spells.h"
#include "gw_block_attribution.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;
void check(bool b){if(!b)throw std::runtime_error("weapon spell policy");}
int main()try{
 check(weapon_spell(787,12,8,12)->duration==15000);
 check(weapon_spell(793,12,8,12)->duration==10000);
 check(weapon_spell(794,12,8,12)->duration==12000);
 check(weapon_spell(787,20,8,20)->duration==27000);
 check(weapon_spell_duration(3,4)==3000&&weapon_spell_duration(3,5)==4000);
 check(weapon_spell_duration(8,14)==12000&&weapon_spell_duration(10,9)==14000); // textual rule;archived table conflicts.
 check(!weapon_spell(787,21,8,0)&&!weapon_spell(787,0,2,1)&&!weapon_spell(787,0,0,0));
 check(weapon_spell(2893,0,8,0)->ends_on_attack&&!weapon_spell(793,0,8,0)->ends_on_attack);
 const Scope source{1,1,1},target{1,2,1};Registry registry(target);combat::conditions::State conditions;
 const auto plan=*weapon_spell(787,0,8,0);const Entry entry{1,1,787,0,Category::weapon_spell,100,3100};
 check(registry.apply(target,entry,Replace::category).accepted);OwnedWeaponSpell spell{source,target,entry,plan};
 check(spell.active(registry,target,100)&&!spell.active(registry,Scope{1,2,2},100));
 check(weapon_spell_pips(spell,registry,target,conditions,100)==0);
 check(conditions.apply(combat::conditions::Kind::poison,1,100,1000));
 check(weapon_spell_pips(spell,registry,target,conditions,1099)==1&&weapon_spell_armor(spell,registry,target,conditions,1099)==24);
 check(weapon_spell_pips(spell,registry,target,conditions,1100)==0&&weapon_spell_armor(spell,registry,target,conditions,1100)==0);
 check(registry.apply(target,{3,1,205,0,Category::hex,100,1100},Replace::same_skill).accepted);
 const auto ending=EndingHexContext::capture(registry,target,3,1100);check(bool(ending));registry.remove(target,3);
 check(weapon_spell_armor(spell,registry,target,conditions,1100,&*ending)==24);
 check(!spell.active(registry,target,3100)&&weapon_spell_armor(spell,registry,target,conditions,3100,&*ending)==0);
 // Caster liveness is deliberately absent:recipient owns a committed effect.
 const auto ward=*weapon_spell(793,0,8,0);const Entry replacement{2,3,793,0,Category::weapon_spell,1000,4000};
 check(registry.apply(target,replacement,Replace::category).accepted&&!spell.active(registry,target,1000));
 OwnedWeaponSpell second{{1,3,9},target,replacement,ward};check(second.active(registry,target,1000));
 check(weapon_spell_pips(second,registry,target,conditions,2000)==2);
 check(warding_block_entry(second,registry,target,2000).has_value());
 auto bad=second;bad.entry.expires=4001;check(!warding_block_entry(bad,registry,target,2000));
 bad=second;bad.source.actor=4;check(!warding_block_entry(bad,registry,target,2000));
 bad=second;bad.plan.spawning=20;check(!warding_block_entry(bad,registry,target,2000));
 check(!warding_block_entry(second,registry,Scope{1,2,2},2000));
 const auto entries=registry.active(target,2000);
 check(reviewed_block_probability(entries,false,2000)==0.); // raw metadata alone never blocks.
 const auto owned=warding_block_entry(second,registry,target,2000);
 check(reviewed_block_probability(entries,false,2000,{},{},0,owned)==.5);
 const auto block_sources=reviewed_block_sources(entries,2000,{},{},0,owned);
 check(block_sources&&block_sources->size()==1&&block_sources->front().skill==793);
 registry.clear(target);check(!warding_block_entry(second,registry,target,2000));
 std::cout<<"weapon spell policy passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
