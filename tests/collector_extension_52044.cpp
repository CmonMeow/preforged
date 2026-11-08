#include "gw_collector_panel.h"
#include "gw_presearing_collectors.h"
#include "gw_equipment_metadata_provider.h"
#include "gw_basic_armor_energy.h"
#include <iostream>
int main()try{
 namespace c=fw::gw::collectors;namespace n=c::native;namespace inv=fw::gw::inventory;
 const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 const auto offers=c::presearing_offers();check(offers.size()==24,"staged catalog was not promoted");
 for(unsigned i=0;i<24;++i)check(offers[i].key==i+1,"existing offer key renumbered");
 for(const auto& [key,name,cost,token,type]:{std::tuple{23u,"Hatcher",5u,10499u,7u},std::tuple{24u,"Varis",3u,10474u,19u}}){
  const auto& offer=offers[key-1];check(offer.collector==name&&offer.map==166&&offer.profession==6&&offer.cost==cost&&
   offer.trophy.encoded_name==std::u16string{static_cast<char16_t>(token)}&&offer.reward.definition.type==type&&
   offer.reward.reviewed_armor==inv::ReviewedArmor{7,0,0},"new offer lost captured identity/wiki armor");
  auto owned=inv::baseline();inv::Item trophy;trophy.definition=offer.trophy;trophy.definition.id=42;
  trophy.definition.quantity=cost;trophy.bag=1;owned.items.emplace(42,trophy);owned.next_item=43;
  n::MenuIds ids;n::Binding menu;const n::Observation observed{10003,166,6,99,7,10,500,true};
  const auto panel=n::open_panel(menu,ids,offers,observed,std::string(name)+" [Collector]");
  check(panel&&panel->menu_items.size()==2&&menu.offers==std::vector<std::pair<unsigned,unsigned>>{{60006,key}},"native cost-first panel mapping incorrect");
  n::Request request{72,2,0,0,{{42,0}},{{60006,0}}};
  const auto plan=n::authorize(menu,request,offers,owned,observed);check(bool(plan),"selected full-cost trophy rejected");
  const auto result=n::exchange_selected(*plan,owned,166,6,10,u"Tester");
  check(result&&!result->profile.items.contains(42)&&result->profile.items.size()==1,"selected trophy/reward replacement failed");
  const auto& reward=result->profile.items.at(result->reward);
  check(reward.profession==6&&reward.customized==u"Tester"&&reward.reviewed_armor==inv::ReviewedArmor{7,0,0}&&
        reward.definition.file==offer.reward.definition.file&&reward.definition.modifiers==offer.reward.definition.modifiers,
        "collector armor identity/customization/defense metadata lost");
  const auto carried=fw::gw::energy::owned_basic_armor(result->profile,6);
  check(carried.capacity==0&&carried.pips==0,"carried reward granted equipped benefits");
  fw::gw::equipment::proposal::Policy equip_policy;
  equip_policy.allowed=true;equip_policy.primary=6;equip_policy.character="Tester";
  equip_policy.reviewed=[&](const fw::gw::items::Definition& definition){return fw::gw::equipment::metadata::reviewed(result->profile,definition);};
  inv::Request equip;equip.change=inv::Change::equip;equip.item=result->reward;
  const auto equipped=fw::gw::equipment::proposal::propose(result->profile,equip,equip_policy);
  check(equipped.accepted(),"captured collector reward rejected by real equipment authority");
  const auto restored=inv::decode(inv::encode(equipped.profile));
  const auto bonus=fw::gw::energy::owned_basic_armor(restored,6);
  check(bonus.capacity==(key==23?5u:0u)&&bonus.pips==(key==24?1u:0u),"equipped native Energy modifier not applied after persistence");
  std::array<int,5> expected{};expected[key==23?0:1]=7;
  check(fw::gw::energy::physical_armor_locations(restored,6)==expected&&
        fw::gw::energy::elemental_armor_locations(restored,6)==expected,
        "equipped collector AR7 not supplied to actual physical/elemental hit locations");
  const auto slot=static_cast<std::uint8_t>(key==23?2:3);
  equip_policy.reviewed=[&](const fw::gw::items::Definition& definition){return fw::gw::equipment::metadata::reviewed(restored,definition);};
  inv::Request unequip;unequip.change=inv::Change::unequip;unequip.equipment_slot=slot;unequip.bag=1;unequip.slot=0;
  const auto removed=fw::gw::equipment::proposal::propose(restored,unequip,equip_policy);
  check(removed.accepted()&&fw::gw::energy::owned_basic_armor(removed.profile,6).capacity==0&&
        fw::gw::energy::owned_basic_armor(removed.profile,6).pips==0&&
        fw::gw::energy::physical_armor_locations(removed.profile,6)==std::array<int,5>{},"unequip retained collector armor/resource benefits");
  auto other=observed;other.profession=1;
  check(!n::open_panel(menu,ids,offers,other,name),"uncaptured profession offer invented");
 }
 // Original trusted profession0 weapon rewards must also carry known-empty
 // customization. This stamps newly issued recipes, never unknown inventory.
 {const auto& offer=offers[2];auto owned=inv::baseline();
  c::Proposal pending{owned,{},0};const auto issued=c::issue_reward(owned,offer,u"Tester",std::move(pending));
  check(issued.has_value(),"original collector offhand issuance failed");
  auto restored=inv::decode(inv::encode(issued->profile));
  const auto& item=restored.items.at(issued->reward);
  check(item.profession==0&&item.customized==u""&&fw::gw::equipment::metadata::reviewed(restored,item.definition).has_value(),"trusted original collector offhand lacks equipment metadata");
  fw::gw::equipment::proposal::Policy policy;policy.allowed=true;policy.primary=6;policy.character="Tester";
  policy.reviewed=[&](const fw::gw::items::Definition& d){return fw::gw::equipment::metadata::reviewed(restored,d);};
  inv::Request equip;equip.change=inv::Change::equip;equip.item=issued->reward;
  check(fw::gw::equipment::proposal::propose(restored,equip,policy).accepted(),"original collector offhand cannot equip");
  auto unknown=offer;unknown.reward.profession.reset();
  const auto unreviewed=c::issue_reward(owned,unknown,u"Tester",c::Proposal{owned,{},0});
  check(unreviewed&&!unreviewed->profile.items.at(unreviewed->reward).customized,"unknown reward metadata inferred");
 }
 std::cout<<"PASS52044 Hatcher/Varis native panels, stable keys, selected exchange and reviewed armor\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
