#include "../src/runtime/gw_weapon_intent.h"
#include "../src/runtime/gw_frenzy.h"
#include "../src/runtime/gw_presearing_items.h"
#include <iostream>
using namespace fw::gw;using namespace combat::weapon;
void check(bool v){if(!v)throw std::runtime_error("weapon intent regression");}
int main()try{
 {
  const auto starter=inventory::presearing::creation_weapon(inventory::baseline(),2,u"Ranger Test");check(starter.has_value());
  Policy ranger;ranger.primary=2;ranger.character="Ranger Test";ranger.caster_weapon=true;ranger.effective[25]=8;
  const auto bow=owned_weapon(*starter,7,ranger);
  check(bow&&bow->item_type==5&&!bow->caster_weapon&&bow->mastery==8&&bow->customized&&bow->damage.minimum==3&&bow->damage.maximum==5);
  auto altered=*starter;altered.items.begin()->second.definition.modifiers[0]^=1;check(!owned_weapon(altered,7,ranger));
  altered=*starter;altered.items.begin()->second.definition.file=9381;check(!owned_weapon(altered,7,ranger));
  ranger.character="Other Ranger";check(!owned_weapon(*starter,7,ranger));
 }
 namespace effects=skills::effects;
 const auto frenzy=effects::frenzy(346);check(frenzy&&frenzy->duration==8000&&frenzy->energy==5&&frenzy->recharge==4000);check(!effects::frenzy(1));
 effects::Scope owner{1,1,1};effects::Registry statuses(owner);
 check(statuses.apply(owner,{1,1,346,0,effects::Category::stance,100,8100},effects::Replace::category).accepted);
 check(!effects::frenzy_active(statuses.active(owner,99),99));
 check(effects::frenzy_active(statuses.active(owner,100),100));
 check(effects::frenzy_active(statuses.active(owner,8099),8099));
 check(!effects::frenzy_active(statuses.active(owner,8100),8100));
 const auto accelerated=effects::frenzy_timing({500,1750,1200},true);check(accelerated&&accelerated->windup_ms==335&&accelerated->period_ms==1173&&accelerated->range==1200);
 check(effects::frenzy_timing({500,1750,1200},false)->period_ms==1750);
 check(!effects::frenzy_timing({UINT64_MAX,UINT64_MAX,1200},true));
 check(!effects::frenzy_timing({500,499,1200},false));
 check(!effects::frenzy_timing({0,1000,std::numeric_limits<double>::quiet_NaN()},true));
 check(effects::frenzy_timing({0,1,128},true)->period_ms==1);
 check(effects::frenzy_strike_damage(25,true)==50);check(effects::frenzy_strike_damage(25,false)==25);
 check(!effects::frenzy_strike_damage(-1,true));check(!effects::frenzy_strike_damage(600000,true));
 check(!effects::frenzy_strike_damage(std::numeric_limits<double>::infinity(),true));
 auto p=inventory::baseline();inventory::Item item;item.definition.id=1;item.definition.file=1;item.definition.type=26;item.definition.modifiers={0xa7a8160b,0x24b80b00,0x27980909};item.bag=2;item.slot=0;item.profession=6;item.customized=u"";p.items.emplace(1,item);p.next_item=2;p.weapons.sets[0].weapon=1;
 Policy policy;policy.primary=6;policy.character="Owner";policy.caster_weapon=true;policy.effective[9]=9;
 const auto weapon=owned_weapon(p,4,policy);check(weapon&&weapon->damage.maximum==22);
 // Duration-only upgrades do not change the base strike or silently enable
 // an unsupported combat suffix. All documented Enchanting values work.
 for(unsigned percent=10;percent<=20;++percent){
  auto upgraded=p;upgraded.items.at(1).definition.modifiers.push_back(0x22b80000u|percent);
  const auto strike=owned_weapon(upgraded,4,policy);
  check(strike&&strike->item==weapon->item&&strike->revision==weapon->revision&&
    strike->damage.minimum==weapon->damage.minimum&&strike->damage.maximum==weapon->damage.maximum&&
    strike->mastery==weapon->mastery&&strike->customized==weapon->customized);
  check(items::weapon_passives::equipped_enchanting(upgraded,4)->percent()==percent);
  upgraded.items.at(1).definition.modifiers.push_back(0x22b80014);check(!owned_weapon(upgraded,4,policy));
 }
 for(auto raw:{0x22b80000u,0x22b80009u,0x22b80015u,0x22b800ffu,0x22b80114u,0x22b90014u,0x62b80014u,0xa2b80014u,0xe2b80014u}){
  auto malformed=p;malformed.items.at(1).definition.modifiers.push_back(raw);check(!owned_weapon(malformed,4,policy));
 }
 p.items.at(1).definition.modifiers.insert(p.items.at(1).definition.modifiers.end(),{0x24080210,0x25300420,0x62c80a00});check(owned_weapon(p,4,policy).has_value());check(items::weapon_passives::worn_energy(p)==10);
 p.items.at(1).definition.modifiers.push_back(0x26980003);check(!owned_weapon(p,4,policy));p.items.at(1).definition.modifiers.pop_back();
 policy.effective[9]=8;check(!owned_weapon(p,4,policy));policy.effective[9]=9;
 p.items.at(1).customized=u"Someone Else";check(!owned_weapon(p,4,policy));p.items.at(1).customized=u"";
 for(const auto type:{2u,15u,27u}){auto melee=p;auto& entry=melee.items.at(1);entry.definition.type=static_cast<uint8_t>(type);entry.profession=1;const unsigned attr=type==2?18:type==15?19:20;entry.definition.modifiers={0xa7a81c0f,0x24b80000|(type==15?0u:2u)<<8,0x27980009|(attr<<8)};Policy warrior;warrior.primary=1;warrior.character="Owner";warrior.caster_weapon=true;warrior.effective[attr]=12;const auto owned=owned_weapon(melee,5,warrior);check(owned&&!owned->caster_weapon&&owned->item_type==type&&owned->mastery==12);for(unsigned percent=10;percent<=20;++percent){auto enchanting=melee;enchanting.items.at(1).definition.modifiers.push_back(0x22b80000u|percent);const auto accepted=owned_weapon(enchanting,5,warrior);check(accepted&&accepted->damage.minimum==owned->damage.minimum&&accepted->damage.maximum==owned->damage.maximum&&accepted->mastery==12);}for(unsigned raw:{3u,4u,5u,11u}){auto elemental=melee;elemental.items.at(1).definition.modifiers[1]=0x24b80000|(raw<<8);const auto converted=owned_weapon(elemental,5,warrior);check(converted&&!converted->caster_weapon&&converted->damage.raw_damage_type==raw&&converted->mastery==12);}auto chaos=melee;chaos.items.at(1).definition.modifiers[1]=0x24b80600;check(!owned_weapon(chaos,5,warrior));warrior.effective[attr]=8;check(!owned_weapon(melee,5,warrior));}
 Scope a{1,1,1},b{1,2,1};Snapshot state{a,b,1,4,true,true,true,true,100};Intent intent;
 check(intent.start(a,b,*weapon,{500,1500,1200},state,100));check(!intent.tick(state,599));check(intent.tick(state,600).has_value());check(!intent.tick(state,601));
 check(intent.tick(state,10000).has_value());check(!intent.tick(state,10001));state.revision=5;check(!intent.tick(state,11500)&&!intent.active());
 state.revision=4;check(intent.start(a,b,*weapon,{0,1500,1200},state,12000));state.target.generation=2;check(!intent.tick(state,12000)&&!intent.active());state.target=b;
 check(intent.start(a,b,*weapon,{500,1750,1200},state,20000));
 check(intent.retime(*accelerated,20250)); // Half the original windup remains.
 check(!intent.tick(state,20417));check(intent.tick(state,20418).has_value());
 check(intent.retime({500,1750,1200},20418));
 check(!intent.tick(state,22167));check(intent.tick(state,22168).has_value());
 check(!intent.retime({UINT64_MAX,UINT64_MAX,1200},22168));
 check(!intent.retime({500,1750,1200},22167));
 check(intent.start(a,b,*weapon,{500,1750,1200},state,23000));
 check(!intent.retime(*accelerated,23501));check(intent.tick(state,23501).has_value());
 check(!intent.tick(state,23501)); // No duplicate release after an overdue retime.
 check(intent.start(a,b,*weapon,{0,1500,1200},state,UINT64_MAX));
 check(intent.retime({0,1500,1200},UINT64_MAX));check(intent.tick(state,UINT64_MAX).has_value()&&!intent.active());
 check(intent.start(a,b,*weapon,{0,1500,1200},state,13000));state.hostile=false;check(!intent.tick(state,13000));state.hostile=true;
 check(intent.start(a,b,*weapon,{0,1500,1200},state,14000));intent.cancel(Cancel::explicit_input);check(!intent.tick(state,14000));
 std::cout<<"PASS owned metadata, requirement, timed repeat, stall, equipment revision and target lifetime\n";
}catch(const std::exception&e){std::cerr<<e.what();return 1;}
