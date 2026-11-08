#include "gw_equipment_proposal.h"
#include "gw_equipment_assignments.h"
#include <iostream>
#include <stdexcept>
int main()try{
 using namespace fw::gw;using namespace equipment::proposal;
 auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
 auto p=inventory::baseline();p.next_item=4;items::Definition fresh;fresh.id=1;fresh.file=1;fresh.type=7;fresh.model=123;p.items.emplace(1,inventory::Item{fresh,1,3});fresh.id=2;p.items.emplace(2,inventory::Item{fresh,2,2});
 Policy policy;policy.allowed=true;policy.primary=6;policy.character="owned-character";policy.reviewed=[](const auto&)->std::optional<Reviewed>{return Reviewed{2,6,{},{},"",false};};
 inventory::Request equip;equip.change=inventory::Change::equip;equip.item=1;const auto original=inventory::encode(p);
 auto result=propose(p,equip,policy);check(result.accepted()&&result.profile.items.size()==2&&result.profile.next_item==4,"equip preserves identities and count");
 check(result.profile.items.at(1).bag==2&&result.profile.items.at(1).slot==2&&result.profile.items.at(2).bag==1&&result.profile.items.at(2).slot==3,"occupied equipment swaps into vacated source");
 check(result.packets.size()==3&&result.packets[0][0]==76&&result.packets[1][0]==74&&result.packets[2][0]==61,"native332 then330 then317 withoutdelete");
 check(inventory::encode(p)==original&&inventory::decode(inventory::encode(result.profile)).items.size()==2,"proposal immutable and durable roundtrip");
 inventory::Request unequip;unequip.change=inventory::Change::unequip;unequip.equipment_slot=2;unequip.bag=1;unequip.slot=4;auto removed=propose(result.profile,unequip,policy);
 check(removed.accepted()&&removed.profile.items.at(1).bag==1&&removed.profile.items.at(1).slot==4,"CMSG77 selector resolves equippedslot not itemID");
 {
  auto packed=p;packed.bags.emplace(42,inventory::Bag{42,1,4,5,0});
  auto pack_unequip=unequip;pack_unequip.bag=42;pack_unequip.slot=0;
  packed.items.at(2).customized=u"owned-character";
  check(propose(packed,pack_unequip,policy).accepted(),"owned customized armor can unequip into pack");
  packed.items.at(2).customized=u"";
  check(propose(packed,pack_unequip,policy).accepted(),"explicit uncustomized equipment can unequip into pack");
  for(const auto owner:{std::optional<std::u16string>{},std::optional<std::u16string>{u"other-character"}}){
   packed.items.at(2).customized=owner;const auto before=inventory::encode(packed);
   const auto denied=propose(packed,pack_unequip,policy);
   check(!denied.accepted()&&denied.packets.empty()&&inventory::encode(denied.profile)==before,"unknown or foreign equipped owner rejects pack unequip atomically");
   packed.items.at(1).bag=42;packed.items.at(1).slot=0;packed.items.at(1).customized=u"owned-character";
   const auto swap_before=inventory::encode(packed);const auto swap_denied=propose(packed,equip,policy);
   check(!swap_denied.accepted()&&swap_denied.packets.empty()&&inventory::encode(swap_denied.profile)==swap_before,"equip from pack cannot displace foreign or unknown owner into pack");
   packed.items.at(1).bag=1;packed.items.at(1).slot=3;
  }
  packed.items.at(1).bag=42;packed.items.at(1).slot=0;packed.items.at(2).customized=u"owned-character";
  const auto swapped=propose(packed,equip,policy);
  check(swapped.accepted()&&swapped.profile.items.at(2).bag==42&&swapped.profile.items.at(1).bag==2,"owned displaced equipment returns to pack vacated slot");
  auto unicode=policy;unicode.character="Pack \xc3\xa9";packed.items.at(2).customized=u"Pack \u00e9";
  check(propose(packed,equip,unicode).accepted(),"pack owner comparison uses actual UTF8 to UTF16 character conversion");
 }
 const auto changed=equipment::assignment_changes(42,equipment::assignments(p),equipment::assignments(result.profile));
 check(changed.size()==1&&changed.front()==equipment::assignment_2022(42,2,1),"occupied equipment publishes new owned wire ID once");
 const auto cleared=equipment::assignment_changes(42,equipment::assignments(result.profile),equipment::assignments(removed.profile));
 check(cleared.size()==1&&cleared.front()==equipment::assignment_2022(42,2,0),"unequip clears association instead of referencing carrying item");
 check(equipment::assignment_changes(42,equipment::assignments(p),equipment::assignments(p)).empty(),"unchanged or rejected equipment publishes no associations");
 const auto restored=equipment::restore_assignments(42,equipment::assignments(result.profile));
 check(restored.size()==1&&restored.front()==equipment::assignment_2022(42,2,1),"cosmetic rebuild restores equipped items only, excluding displaced backpack armor");
 check(equipment::restore_assignments(42,equipment::assignments(removed.profile)).empty(),"empty equipped bag does not restore carrying items");
 const auto equipped_visual=equipment::armor_visuals(result.profile);
 check(equipped_visual.size()==5&&!equipped_visual[0].composite&&equipped_visual[1].composite==1,
       "headless armor keeps chest in public chest slot");
 check(!equipment::armor_visuals(removed.profile)[1].composite,"reconnect appearance excludes removed backpack armor");
 auto recolored=result.profile;recolored.items.at(1).definition.dye=12;recolored.items.at(1).definition.unknown1=6;
 check(equipment::armor_visuals(recolored)[1].color==12&&equipment::armor_visuals(recolored)[1].packed_composite()==0x60001,
       "saved dye and render parameter drive reconnect appearance");
 CharacterProfile summary;summary.unknown1=8;summary.extension_count=37;summary.extended[2]=0xabcd;summary.extended[12]=0x1631;
 const auto projected=summary.with_equipment_2022(equipment::armor_visuals(recolored));
 check(projected.unknown1==8&&projected.last_map_id==summary.last_map_id&&projected.extended[2]==0xabcd&&projected.extended[12]==0x1631,
       "roster projection preserves version, identity appearance, origin and profession flags");
 check(projected.equipment_2022().size()==5&&projected.equipment_2022()[1].color==12,
       "roster projection serializes saved equipment in fixed slots");
 unequip.slot=3;check(propose(result.profile,unequip,policy).status==Reason::occupied,"unequip occupied destination denies without overwrite");
 policy.reviewed={};check(propose(p,equip,policy).status==Reason::metadata,"unknown metadata denies equip");
 policy.reviewed=[](const auto&)->std::optional<Reviewed>{return Reviewed{2,3,{},{},"",false};};check(propose(p,equip,policy).status==Reason::profession,"wrong profession denial");
 policy.reviewed=[](const auto&)->std::optional<Reviewed>{return Reviewed{2,6,{},{},"other-character",false};};check(propose(p,equip,policy).status==Reason::customization,"foreign customization denial");
 auto weapons=inventory::baseline();weapons.next_item=4;
 items::Definition sword; sword.id=1;sword.file=1;sword.type=2;sword.model=123;
 weapons.items.emplace(1,inventory::Item{sword,1,3});sword.id=2;weapons.items.emplace(2,inventory::Item{sword,2,0});
 weapons.weapons.sets[0].weapon=2;weapons.weapons.sets[2].weapon=2;
 policy.reviewed=[](const auto&)->std::optional<Reviewed>{return Reviewed{0,0,{},{},"",false};};
 equip.item=1;auto armed=propose(weapons,equip,policy);
 check(armed.accepted()&&armed.profile.weapons.sets[0].weapon==1&&armed.profile.weapons.sets[2].weapon==2,"equip updates active hand while preserving inactive possessions");
 check(armed.packets.size()==8&&armed.packets[3][0]==70&&armed.packets.back()[0]==71,"membership restoration follows occupied hand placement");
 auto durable=inventory::decode(inventory::encode(armed.profile));check(durable.weapons==armed.profile.weapons,"equipment memberships survive reconnect");
 unequip.equipment_slot=0;unequip.slot=4;auto disarmed=propose(armed.profile,unequip,policy);
 check(disarmed.accepted()&&!disarmed.profile.weapons.sets[0].weapon&&disarmed.profile.weapons.sets[2].weapon==2,"unequip clears only actual active hand");
 check(disarmed.packets.size()==6&&disarmed.packets.back()[0]==71,"unequip publishes memberships after location");
 auto twohand=weapons;twohand.items.at(1).definition.type=26;twohand.items.at(1).definition.interaction=0x200000;
 items::Definition shield=sword;shield.id=3;shield.type=24;twohand.next_item=4;twohand.items.emplace(3,inventory::Item{shield,2,1});twohand.weapons.sets[0].offhand=3;
 policy.reviewed=[](const auto&)->std::optional<Reviewed>{return Reviewed{0,0,{},{},"",true};};
 auto staff=propose(twohand,equip,policy);
 check(staff.accepted()&&staff.profile.items.at(3).bag==2&&staff.profile.items.at(3).slot==1&&staff.profile.weapons.sets[0].offhand==3,"two-handed equip preserves equipped and stored offhand");
 check(!weapon_sets::effective(staff.profile.weapons,inventory::weapon_lookup(staff.profile)).offhand,"two-handed suppresses only effective offhand");
 std::cout<<"Equipment proposal conservation, durable memberships and native swap tests passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
