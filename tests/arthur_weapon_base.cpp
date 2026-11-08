// Exact4family production intent. No generic weapon fallback or conditional
// Health/Charr modifier execution is supplied by this base-only authority.
#include "gw_arthur_weapon_base.h"
#include "gw_weapon_intent.h"
#include "gw_weapon_attack.h"
#include <iostream>
int main()try{
 using namespace fw::gw;const auto check=[](bool v,const char* why){if(!v)throw std::runtime_error(why);};unsigned count=0;
 for(const auto& recipe:crafter::arthur_wiki_catalog()){
  if(recipe.reward.definition.type==12){check(!items::arthur_weapon::reviewed(recipe.reward.definition),"focus classified as primary weapon");continue;}
  const auto& definition=recipe.reward.definition;const auto base=items::arthur_weapon::reviewed(definition);check(bool(base),"known exactweapon rejected");
  std::array<std::uint8_t,51> effective{};
  if(base->requirement_attribute){check(!items::arthur_weapon::usable_base(definition,effective),"unmet attribute requirement bypassed");effective[*base->requirement_attribute]=3;}
  const auto damage=items::arthur_weapon::usable_base(definition,effective);check(damage&&damage->minimum==base->damage.minimum&&damage->maximum==base->damage.maximum,"base damage or met requirement wrong");
  combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=damage->minimum;strike.maximum=damage->maximum;strike.location_armor=60;strike.base_roll=.5;strike.miss_roll=.95;strike.item_type=definition.type;
  const auto hit=combat::weapon::resolve(strike);check(hit&&hit->damage==(damage->minimum+damage->maximum)/2.,"existing shared strike resolver cannot use trusted base");
  auto owned=inventory::baseline();auto item=recipe.reward;item.definition.id=43;item.bag=1;item.slot=0;owned.next_item=44;owned.items.emplace(43,item);
  equipment::proposal::Policy policy;policy.allowed=true;policy.primary=1;policy.character="Tester";policy.effective=effective;policy.reviewed=[&](const auto& d){return equipment::metadata::reviewed(owned,d);};inventory::Request equip;equip.change=inventory::Change::equip;equip.item=43;
  const auto equipped=equipment::proposal::propose(owned,equip,policy);check(equipped.accepted(),"strict equip rejected trusted crafted primary");owned=inventory::decode(inventory::encode(equipped.profile));
  combat::weapon::Policy combat_policy;combat_policy.primary=1;combat_policy.character="Tester";combat_policy.effective=effective;combat_policy.mastery_attribute=definition.type==2?18:definition.type==15?19:definition.type==27?20:25;
  const auto attack=combat::weapon::owned_weapon(owned,10,combat_policy);
  check(attack&&attack->damage.minimum==damage->minimum&&attack->damage.maximum==damage->maximum&&attack->damage.raw_damage_type==damage->raw_damage_type,"actual crafted owned-weapon base intent rejected");
  if(base->requirement_attribute){auto below=combat_policy;below.effective[*base->requirement_attribute]=2;check(!combat::weapon::owned_weapon(owned,10,below),"production weapon intent bypassed rank3 requirement");}
  const auto original=owned;owned.items.at(43).profession.reset();check(!combat::weapon::owned_weapon(owned,10,combat_policy),"unknown profession metadata authorized");owned=original;
  owned.items.at(43).customized.reset();check(!combat::weapon::owned_weapon(owned,10,combat_policy),"unknown owner metadata authorized");owned=original;
  owned.items.at(43).customized=u"Other";check(!combat::weapon::owned_weapon(owned,10,combat_policy),"foreign customization authorized");owned=original;

  for(unsigned bad=0;bad<4;++bad){auto forged=definition;switch(bad){case 0:++forged.model;break;case 1:++forged.price;break;case 2:forged.modifiers.push_back(0x24580001);break;case 3:forged.encoded_name.push_back(1);break;}check(!items::arthur_weapon::reviewed(forged),"near-match weapon gained base authority");forged.id=43;owned.items.at(43).definition=forged;check(!combat::weapon::owned_weapon(owned,10,combat_policy),"production near-match weapon gained base intent");owned=original;}
  if(definition.type==2)check(base->health_bonus==5&&!base->charr_damage_bonus,"Axe Health5 state lost");
  if(definition.type==5||definition.type==15)check(base->charr_damage_bonus==1,"unsupported Charr damage state lost");
  if(definition.type==27)check(base->rinblade_armor_bug&&!base->charr_damage_bonus,"Rinblade nonfunctional armor incorrectly granted");
  ++count;
 }
 check(count==4,"expected exact4 crafted weapon family");std::cout<<"PASS exactArthur4 base ranges/requirements, strict metadata and production base intent\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
