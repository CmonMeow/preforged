#include "../src/runtime/gw_arthur_focus_armor.h"
#include "../src/runtime/gw_equipment_metadata_provider.h"
#include "../src/runtime/gw_weapon_passives.h"
#include "../src/runtime/gw_basic_armor_energy.h"
#include <iostream>
int main()try{
 namespace inv=fw::gw::inventory;namespace c=fw::gw::crafter;
 const auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
 unsigned rows=0;
 for(const auto& recipe:c::arthur_focus_candidates()){
  auto owned=inv::baseline();owned.gold=500;owned.next_item=100;const auto material=recipe.materials.front();
  const auto known=std::find_if(inv::materials::rows.begin(),inv::materials::rows.end(),[&](auto r){return r.element==material.element;});
  check(known!=inv::materials::rows.end(),"unknown material fixture");inv::Item stack;stack.definition.id=43;stack.definition.file=1234;stack.definition.type=11;stack.definition.quantity=material.units;stack.definition.model=known->model;stack.definition.interaction=0x80001;stack.definition.modifiers={0x25080000u|(material.element<<8)};stack.bag=1;stack.slot=0;owned.items.emplace(43,stack);
  c::Binding opening;fw::gw::collectors::native::MenuIds ids;const c::Observation at{10007,148,0,99,7,10,500,true};
  check(c::bind(opening,ids,at,"Arthur Ayala",owned,std::span(&recipe,1)),"focus recipe bind failed");
  c::Request request{75,3,50,0,{{43,1}},{{opening.recipes.front().reward.definition.id,1}}};const auto plan=c::authorize(opening,request,owned,at);check(bool(plan),"focus recipe admission failed");
  auto proposed=c::propose(*plan,owned,10);check(bool(proposed),"focus issuance failed");owned=inv::decode(inv::encode(proposed->profile));check(owned.items.size()==1,"material debit or reward count wrong");const auto focus=owned.items.begin()->first;
  check(fw::gw::items::arthur_focus::trusted(owned.items.at(focus)),"issued persisted metadata not trusted");
  const auto equip=[&](unsigned id){fw::gw::equipment::proposal::Policy policy;policy.allowed=true;policy.primary=6;policy.character="Tester";policy.reviewed=[&](const auto& d){return fw::gw::equipment::metadata::reviewed(owned,d);};inv::Request r;r.change=inv::Change::equip;r.item=id;const auto p=fw::gw::equipment::proposal::propose(owned,r,policy);check(p.accepted(),"actual strict equip rejected");owned=inv::decode(inv::encode(p.profile));};
  unsigned bodyArmor=0;const auto benefits=[&](unsigned armor,unsigned energy){check(fw::gw::items::arthur_focus::armor_bonus(owned)==armor,"effective intrinsic focus armor wrong");check(fw::gw::items::weapon_passives::worn_energy(owned)==energy,"effective focus Energy wrong");std::array<int,5> expected{};expected.fill(bodyArmor+armor);check(fw::gw::energy::physical_armor_locations(owned,6)==expected&&fw::gw::energy::elemental_armor_locations(owned,6)==expected,"shared focus armor locations wrong");check(fw::gw::energy::fully_modeled_physical_armor(owned,6)==static_cast<int>(bodyArmor+armor)&&fw::gw::energy::fully_modeled_elemental_armor(owned,6)==static_cast<int>(bodyArmor+armor),"shared uniform focus armor wrong");};
  equip(focus);benefits(5,3);unsigned sword=0,staff=0;
  for(unsigned type:{27u,26u}){inv::Item item;item.definition.id=owned.next_item++;item.definition.type=type;item.definition.file=1234;item.definition.quantity=1;item.definition.interaction=type==26?0x200000:0;item.profession=0;item.customized=u"";item.bag=1;item.slot=type==26?1:0;if(type==26)item.definition.modifiers={0x62c80a00};owned.items.emplace(item.definition.id,item);(type==26?staff:sword)=item.definition.id;}
  equip(sword);benefits(5,3);equip(staff);benefits(0,10);check(owned.items.at(focus).bag==2&&owned.items.at(focus).slot==1&&owned.weapons.sets[0].offhand==focus,"staff removed retained offhand");equip(sword);benefits(5,3);
  const auto original=owned;
  for(unsigned bad=0;bad<7;++bad){owned=original;auto& item=owned.items.at(focus);switch(bad){case 0:item.profession.reset();break;case 1:item.customized.reset();break;case 2:item.customized=u"Other";break;case 3:++item.definition.model;break;case 4:++item.definition.price;break;case 5:item.definition.modifiers={0x62c80300,0xa3c80600};break;case 6:item.definition.encoded_name.push_back(1);break;}check(!fw::gw::items::arthur_focus::trusted(item)&&!fw::gw::items::arthur_focus::armor_bonus(owned),"near-match granted intrinsic armor");}
  owned=original;owned.items.at(focus).bag=1;owned.items.at(focus).slot=5;benefits(0,0);
  owned=original;constexpr unsigned types[]{7,19,16,4,13};
  for(unsigned slot=2;slot<=6;++slot){inv::Item armor;armor.definition.id=owned.next_item++;armor.definition.file=1234;armor.definition.type=types[slot-2];armor.definition.quantity=1;armor.definition.modifiers={0xa3c83c00};armor.bag=2;armor.slot=slot;armor.profession=6;armor.customized=u"Tester";armor.reviewed_armor=inv::ReviewedArmor{60,0,0};owned.items.emplace(armor.definition.id,armor);}
  owned=inv::decode(inv::encode(owned));bodyArmor=60;benefits(5,3);equip(staff);benefits(0,10);equip(sword);benefits(5,3);++rows;
 }
 check(rows==9,"expected exact9 focus candidates");std::cout<<"PASS9 crafted focus issuance,persist,equip,staff suppression and metadata identity\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
