#include "gw_storage.h"
#include <iostream>
int main()try{
 using namespace fw::gw;auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
 inventory::Item private_chest;private_chest.definition.id=42;private_chest.definition.file=544;private_chest.definition.type=7;private_chest.definition.quantity=1;private_chest.profession=6;private_chest.reviewed_armor=inventory::ReviewedArmor{60,0,0};
 private_chest.definition.modifiers={0x21e80a01};
 auto chest_wire=private_chest.definition;chest_wire.modifiers={0x21e80a01,0xa3c83c00,0x62c80500};
 check(inventory::item_packets(private_chest).front()==items::definition_2022(chest_wire),"private armor upgrade preserves native base armor and energy tooltip stats");
 check(private_chest.definition.modifiers==std::vector<std::uint32_t>{0x21e80a01},"tooltip projection does not mutate durable upgrade ownership");
 private_chest.definition.modifiers={0x21e80a01,0x62c80300};chest_wire.modifiers={0x21e80a01,0x62c80300,0xa3c83c00};
 check(inventory::item_packets(private_chest).front()==items::definition_2022(chest_wire),"explicit energy modifier is preserved without duplicate property");
 private_chest.definition.modifiers={0x21e80a01,0xa3c81900};
 check(inventory::item_packets(private_chest).front()==items::definition_2022(private_chest.definition),"captured explicit base armor definition remains unchanged");
 private_chest.reviewed_armor.reset();private_chest.definition.modifiers={0x21e80a01};
 check(inventory::item_packets(private_chest).front()==items::definition_2022(private_chest.definition),"unreviewed armor receives no fabricated baseline");
 auto wire=[](items::Packet p){p[1]|=0x80;std::string s;const char* h="0123456789abcdef";for(auto b:p){s+=h[b>>4];s+=h[b&15];}return s;};
 check(wire(items::inventory_2022(1))=="4381010000","retail inventory fixture");
 check(wire(items::bag_2022(1,1,0,2,20))=="3e810100010002001400000000","retail backpack fixture");
 check(wire(items::bag_2022(1,2,21,3,9))=="3e810100021503000900000000","retail equipped fixture");
 // Original352→345/313→318 callback probe is recorded separately in
 // item-container-2022.txt / build/reports/item-container-2022.json. It proves
 // pointer/backlink resolution and that a late352 does not repair early318.
 check(wire(items::bag_2022(1,1,1,4,5,42))=="3e81010001010400052a000000","installed-container318 matches original pinned serializer fixture");
 check(wire(items::location_2022(1,42,2,7,true))=="4a8101002a000000020007","retail move fixture");
 check(wire(items::quantity_2022(42,12))=="38812a0000000c000000","retail quantity fixture");
 check(wire(items::detach_2022(1,42))=="4c8101002a000000","retail detach fixture");
 check(wire(items::delete_2022(42))=="5c81000000002a000000","native definition deletion fixture");
 inventory::Packet unequip_wire;items::put(unequip_wire,77,2);items::put(unequip_wire,5,1);items::put(unequip_wire,1,2);items::put(unequip_wire,7,1);inventory::Request unequip_request;
 check(inventory::parse(unequip_wire,unequip_request)==world::FrameState::complete&&unequip_request.item==0&&unequip_request.equipment_slot==5&&unequip_request.bag==1&&unequip_request.slot==7,"native77 equipment selector is separate from owned item identity");
 auto equipment_fixture=inventory::baseline();inventory::Item worn;worn.definition.id=42;worn.definition.file=1;worn.definition.type=5;worn.bag=2;worn.slot=5;equipment_fixture.items.emplace(42,worn);equipment_fixture.next_item=43;
 check(inventory::owned_request_item(equipment_fixture,unequip_request)==42,"unequip resolves actual equipped item by slot");unequip_request.equipment_slot=4;check(!inventory::owned_request_item(equipment_fixture,unequip_request),"empty equipped selector cannot resolve unrelated item");unequip_request.equipment_slot=9;check(!inventory::owned_request_item(equipment_fixture,unequip_request),"out of range equipped selector denied");
 check(wire(storage::open_window(42))=="82802a0000000003000000","retail Xunlai opening fixture");
 auto p=inventory::baseline();inventory::Item item;item.definition.id=1;item.definition.file=1234;item.definition.type=11;item.definition.interaction=0x80000;item.definition.quantity=100;item.definition.encoded_name=u"fixture";item.definition.modifiers={0x12345678};item.bag=1;p.items.emplace(1,item);p.next_item=2;
 {
  // Synthetic definitions test admission, not captured equipment identities.
  auto packed=p;packed.bags.emplace(42,inventory::Bag{42,1,4,5,0});packed.items.at(1).customized=u"";
  const auto before=inventory::encode(packed);
  for(auto change:{inventory::Change::move,inventory::Change::split}){
   const auto denied=inventory::propose(packed,{change,1,1,42,0},{true,true});
   check(denied.status==inventory::Reason::slot&&denied.packets.empty()&&inventory::encode(denied.profile)==before,"equipment pack rejects materials without count identity or packet changes");
  }
  for(unsigned type:{2,4,5,7,12,13,15,16,19,22,24,26,27,32,35,36,44,45}){
   auto equipment=packed;equipment.items.at(1).definition.type=static_cast<std::uint8_t>(type);equipment.items.at(1).definition.quantity=1;equipment.items.at(1).definition.interaction=0;
   const auto moved=inventory::propose(equipment,{inventory::Change::move,1,0,42,0},{true,true});
   check(moved.accepted()&&moved.profile.items.at(1).bag==42&&moved.packets==inventory::Batch{items::location_2022(1,1,42,0,true)},"all documented equipment categories move into pack by native index rather than bag ID");
   check(inventory::decode(inventory::encode(moved.profile)).items.at(1).bag==42,"accepted equipment pack placement survives durable codec");
  }
  for(unsigned type:{0,3,8,9,10,11,17,18,20,21,29,30,31,33,34,255}){
   auto other=packed;other.items.at(1).definition.type=static_cast<std::uint8_t>(type);
   check(!inventory::propose(other,{inventory::Change::move,1,0,42,0},{true,true}).accepted(),"non-equipment and unknown types cannot enter equipment pack");
  }
  auto equipment=item;equipment.definition.id=2;equipment.definition.type=5;equipment.definition.quantity=1;equipment.definition.interaction=0;equipment.bag=42;equipment.slot=0;packed.items.emplace(2,equipment);packed.next_item=3;
  const auto swap_before=inventory::encode(packed);
  for(const auto request:{inventory::Request{inventory::Change::move,2,0,1,0},inventory::Request{inventory::Change::move,1,0,42,0}}){
   const auto denied=inventory::propose(packed,request,{true,true});
   check(denied.status==inventory::Reason::slot&&denied.packets.empty()&&inventory::encode(denied.profile)==swap_before,"either swap direction rejects non-equipment entering pack atomically");
  }
  check(inventory::propose(packed,{inventory::Change::move,2,0,1,1},{true,true}).accepted(),"pack equipment can move to empty ordinary slot");
  packed.items.at(1).definition.type=24;packed.items.at(1).definition.quantity=1;packed.items.at(1).definition.interaction=0;
  const auto swap=inventory::propose(packed,{inventory::Change::move,2,0,1,0},{true,true});
  check(swap.accepted()&&swap.profile.items.at(1).bag==42&&swap.profile.items.at(2).bag==1,"eligible equipment swaps in both directions");
  auto bank=storage::baseline();auto bank_item=item;bank_item.bag=100;bank.items.emplace(1,bank_item);bank.next_item=2;
  auto empty=inventory::baseline();empty.bags.emplace(42,inventory::Bag{42,1,4,5,0});
  const auto withdrawn=storage::transfer(empty,bank,{inventory::Change::move,1,0,42,0},{true,true,true});
  check(!withdrawn.accepted()&&withdrawn.packets.empty()&&inventory::encode(withdrawn.character)==inventory::encode(empty)&&inventory::encode(withdrawn.bank)==inventory::encode(bank),"storage withdrawal cannot bypass equipment pack admission");
  auto owner_item=packed;owner_item.items.at(1).customized=u"Pack Owner";
  const inventory::Request into_pack{inventory::Change::move,1,0,42,1};
  check(inventory::propose(owner_item,into_pack,{true,true,false,u"Pack Owner"}).accepted(),"matching character can pack customized equipment");
  for(const auto owner:{u"Other Character",u""}){
   const auto denied=inventory::propose(owner_item,into_pack,{true,true,false,owner});
   check(!denied.accepted()&&denied.packets.empty()&&inventory::encode(denied.profile)==inventory::encode(owner_item),"foreign or absent character cannot pack customized equipment");
  }
  auto unknown=owner_item;unknown.items.at(1).customized.reset();
  check(!inventory::propose(unknown,into_pack,{true,true,false,u"Pack Owner"}).accepted(),"unknown customization is not inferred uncustomized");
  check(inventory::propose(owner_item,{inventory::Change::move,1,0,1,2},{true,true,false,u"Other Character"}).accepted(),"foreign customized gear remains movable within ordinary bags");
  const auto denied_swap=inventory::propose(owner_item,{inventory::Change::move,2,0,1,0},{true,true,false,u"Other Character"});
  check(!denied_swap.accepted()&&denied_swap.packets.empty()&&inventory::encode(denied_swap.profile)==inventory::encode(owner_item),"reverse swap cannot admit foreign customized displaced gear");
  bank.items.at(1)=owner_item.items.at(1);bank.items.at(1).bag=100;
  const inventory::Request withdraw_gear{inventory::Change::move,1,0,42,0};
  check(storage::transfer(empty,bank,withdraw_gear,{true,true,true,u"Pack Owner"}).accepted(),"storage forwards actual character for customized pack withdrawal");
  const auto foreign=storage::transfer(empty,bank,withdraw_gear,{true,true,true,u"Other Character"});
  check(!foreign.accepted()&&foreign.packets.empty()&&inventory::encode(foreign.bank)==inventory::encode(bank)&&inventory::encode(foreign.character)==inventory::encode(empty),"alternate character cannot withdraw foreign customized equipment into pack");
 }
 auto destroyed=inventory::propose(p,{inventory::Change::destroy,1},{true,true});
 check(destroyed.accepted()&&destroyed.profile.items.empty()&&destroyed.packets.size()==2&&wire(destroyed.packets[0])==wire(items::detach_2022(1,1))&&wire(destroyed.packets[1])==wire(items::delete_2022(1)),"destroy detaches then deletes native definition after durable proposal");
 auto bytes=inventory::encode(p);check(inventory::encode(inventory::decode(bytes))==bytes,"codec preserves opaque definition");
 auto legacy=bytes;legacy.resize(legacy.size()-40);legacy[3]=1;const auto legacy_profile=inventory::decode(legacy);check(!legacy_profile.items.at(1).profession&&!legacy_profile.items.at(1).customized&&legacy_profile.material_quota==250,"legacy saves retain unknown metadata and baseline quota instead of invented upgrades");
 auto legacy2=bytes;legacy2.resize(legacy2.size()-39);legacy2[3]=2;check(inventory::decode(legacy2).weapons==weapon_sets::State{},"FWI2 without weapon sets remains readable");
 auto legacy3=bytes;legacy3.resize(legacy3.size()-6);legacy3[3]=3;check(inventory::decode(legacy3).gold==0&&inventory::decode(legacy3).material_quota==250,"FWI3 migrates absent gold/quota without inventing purchases");
 auto legacy4=bytes;legacy4.resize(legacy4.size()-2);legacy4[3]=4;check(inventory::decode(legacy4).material_quota==250&&inventory::decode(legacy4).items.at(1).definition.quantity==100,"FWI4 remains readable with default material quota");
 auto metadata_profile=p;metadata_profile.items.at(1).profession=6;metadata_profile.items.at(1).customized=u"Cmon Meow";const auto metadata_bytes=inventory::encode(metadata_profile);const auto metadata_restored=inventory::decode(metadata_bytes);check(metadata_restored.items.at(1).profession==6&&metadata_restored.items.at(1).customized==std::optional<std::u16string>(u"Cmon Meow"),"profession and customization survive save/reconnect");
 auto installed_profile=inventory::baseline();inventory::Item installed_item;installed_item.definition.id=1;installed_item.definition.file=1234;installed_item.definition.type=3;installed_item.definition.quantity=1;installed_item.definition.encoded_name=u"Captured container fixture";installed_item.profession=6;installed_item.customized=u"Fixture Owner";installed_profile.items.emplace(1,installed_item);installed_profile.bags.at(1).container=1;installed_profile.next_item=2;installed_profile.gold=789;
 const auto installed_blob=inventory::encode(installed_profile);const auto installed_saved=inventory::decode(installed_blob);
 check(installed_saved.items.at(1).bag==0&&installed_saved.items.at(1).slot==0&&installed_saved.bags.at(1).container==1&&installed_saved.gold==789,"FWI4 preserves actual installed item and gold without fake slot");
 const auto rejected_profile=[&](const auto& value,const char* reason){bool denied=false;try{inventory::validate(value);}catch(const std::invalid_argument&){denied=true;}check(denied,reason);};
 auto invalid_installed=installed_profile;invalid_installed.bags.at(1).container=0;rejected_profile(invalid_installed,"unreferenced unplaced item cannot persist");
 invalid_installed=installed_profile;invalid_installed.items.at(1).definition.type=11;rejected_profile(invalid_installed,"installed item must be native bag type3");
 invalid_installed=installed_profile;invalid_installed.items.at(1).definition.quantity=2;rejected_profile(invalid_installed,"installed item cannot be a stack");
 invalid_installed=installed_profile;invalid_installed.items.at(1).slot=1;rejected_profile(invalid_installed,"installed item has canonical zero slot");
 invalid_installed=installed_profile;invalid_installed.bags.emplace(3,inventory::Bag{3,1,1,10,1});rejected_profile(invalid_installed,"one owned container cannot back two bags");
 invalid_installed=installed_profile;invalid_installed.items.at(1).bag=1;rejected_profile(invalid_installed,"FWI4 installed reference cannot also occupy a carried slot");
 auto two_installed=installed_profile;auto second_container=installed_item;second_container.definition.id=2;two_installed.items.emplace(2,second_container);two_installed.bags.emplace(3,inventory::Bag{3,1,1,10,2});two_installed.next_item=3;
 check(inventory::decode(inventory::encode(two_installed)).items.size()==2,"distinct installed containers share zero placement without occupying one another's slots");
 auto installed_and_contents=installed_profile;auto contained_item=item;contained_item.definition.id=2;contained_item.bag=1;contained_item.slot=0;installed_and_contents.items.emplace(2,contained_item);installed_and_contents.next_item=3;
 const auto restored_contents=inventory::restore(installed_and_contents);
 check(std::count_if(restored_contents.begin(),restored_contents.end(),[](const auto& packet){return (packet[0]|(unsigned(packet[1])<<8))==317;})==1&&
   inventory::decode(inventory::encode(installed_and_contents)).items.at(2).slot==0,"installed bag item does not consume first real content slot");
 for(const auto change:{inventory::Change::move,inventory::Change::split,inventory::Change::destroy}){const auto denied=inventory::propose(installed_profile,{change,1,1,1,0},{true,true});check(!denied.accepted()&&denied.status==inventory::Reason::unsupported&&inventory::encode(denied.profile)==installed_blob,"ordinary mutation rejects installed container without exception or loss");}
 check(inventory::reconcile(installed_profile,1).empty(),"installed denial does not publish317/330 fake placement");
 check(!storage::transfer(installed_profile,storage::baseline(),{inventory::Change::move,1,0,100,0},{true,true,true}).accepted(),"installed container cannot be deposited");
 inventory::Request installed_unequip;installed_unequip.change=inventory::Change::unequip;installed_unequip.equipment_slot=0;check(!inventory::owned_request_item(installed_profile,installed_unequip),"unequip scan safely skips installed container");
 const auto installed_item_offset=13+installed_profile.bags.size()*9;
 auto old_installed_blob=installed_blob;old_installed_blob[installed_item_offset]=1;old_installed_blob[installed_item_offset+1]=0;old_installed_blob[installed_item_offset+2]=3;
 bool placed4_denied=false;try{inventory::decode(old_installed_blob);}catch(const std::invalid_argument&){placed4_denied=true;}check(placed4_denied,"FWI4 placed reference fails explicitly rather than silently shifting a real item");
 const auto installed_metadata_offset=installed_item_offset+5+items::definition_2022(installed_item.definition).size()-2;
 const auto installed_metadata_size=1+1+1+installed_item.customized->size()*2;
 for(unsigned version=1;version<=3;++version){auto old=old_installed_blob;old[3]=static_cast<std::uint8_t>(version);old.resize(old.size()-6);if(version<3)old.resize(old.size()-33);if(version==1)old.erase(old.begin()+installed_metadata_offset,old.begin()+installed_metadata_offset+installed_metadata_size);const auto migrated=inventory::decode(old);
   check(migrated.items.at(1).bag==0&&migrated.items.at(1).slot==0&&migrated.bags.at(1).container==1&&migrated.gold==0,"reviewed FWI1..3 installed relation migrates without minting gold");
   check(version==1?!migrated.items.at(1).profession:migrated.items.at(1).profession==6&&migrated.items.at(1).customized==installed_item.customized,"legacy container migration preserves available metadata");
   check(inventory::decode(inventory::encode(migrated)).items.at(1).bag==0,"legacy installed item upgrades to canonical durable FWI4");}
 auto malformed_legacy=installed_and_contents;malformed_legacy.items.at(1).bag=1;malformed_legacy.items.at(1).slot=0;bool migration_denied=false;try{inventory::migrate_legacy_containers(malformed_legacy);}catch(const std::invalid_argument&){migration_denied=true;}check(migration_denied&&malformed_legacy.items.at(1).bag==1,"migration checks duplicate legacy slots before removing placement and leaves malformed input untouched");
 const auto native_installed=inventory::restore(installed_profile,false),presented_installed=storage::restore_character(installed_profile,false);
 check(native_installed==presented_installed&&native_installed.size()==11,"actual installed containers share normal definition-before-bag restore path without placeholder");
 check(native_installed[0][0]==(323&255)&&native_installed[1][0]==(352&255)&&native_installed[2][0]==(345&255)&&native_installed[3][0]==(313&255)&&native_installed[4][0]==(318&255),"native container definition and metadata exist before318 backlink resolution");
 check(std::none_of(native_installed.begin(),native_installed.end(),[](const auto& packet){return (packet[0]|(unsigned(packet[1])<<8))==317;}),"installed container never receives content location317");
 auto publication=inventory::item_packets(metadata_restored.items.at(1));check(publication.size()==3&&publication[0][0]==96&&publication[1][0]==89&&publication[2][0]==57,"metadata publication follows definition before placement");
 auto staged_restore=storage::restore_character(metadata_restored,true,false);
 for(const auto& packet:staged_restore)check((packet[0]|(unsigned(packet[1])<<8))!=317,"pre-UI stage must not publish placements");
 const auto deferred=storage::restore_character_placements(metadata_restored);
 staged_restore.insert(staged_restore.end(),deferred.begin(),deferred.end());
 check(staged_restore==storage::restore_character(metadata_restored),"deferred restore publishes exactly the normal definitions/bags/placements/weapon sets once");
 const auto metadata_split=inventory::propose(metadata_restored,{inventory::Change::split,1,25,1,1},{true,true});check(metadata_split.accepted()&&metadata_split.profile.items.at(2).profession==6&&metadata_split.profile.items.at(2).customized==metadata_restored.items.at(1).customized&&metadata_split.packets.size()==5,"split preserves metadata and publishes definition then profession/customization before location");
 const auto metadata_deposit=storage::transfer(metadata_restored,storage::baseline(),{inventory::Change::split,1,25,100,0},{true,true,true});check(metadata_deposit.accepted()&&metadata_deposit.bank.items.at(2).customized==metadata_restored.items.at(1).customized&&inventory::decode(inventory::encode(metadata_deposit.bank)).items.at(2).profession==6,"atomic bank transfer retains complete persisted metadata");
 metadata_profile.items.at(1).customized=u"";check(inventory::decode(inventory::encode(metadata_profile)).items.at(1).customized==std::optional<std::u16string>(u""),"known uncustomized remains distinct from unknown");
 auto terminated=item.definition;terminated.modifiers.push_back(0xc0000000u);bool terminator_denied=false;try{items::definition_2022(terminated);}catch(const std::invalid_argument&){terminator_denied=true;}check(terminator_denied,"native appended modifier terminator cannot be serialized twice");
 for(std::size_t n=0;n<bytes.size();++n){bool failed=false;try{inventory::decode(std::span(bytes).first(n));}catch(const std::invalid_argument&){failed=true;}check(failed,"every truncated codec rejected");}
 inventory::Request r{inventory::Change::split,1,25,1,1};auto split=inventory::propose(p,r,{true,true});check(split.accepted()&&split.profile.items.at(1).definition.quantity==75&&split.profile.items.at(2).definition.quantity==25,"split conserves quantity and allocates identity");check(p.items.size()==1&&p.items.at(1).definition.quantity==100,"proposal does not mutate source");
 r={inventory::Change::move,2,0,1,0};auto merged=inventory::propose(split.profile,r,{true,true});check(merged.accepted()&&merged.profile.items.size()==1&&merged.profile.items.at(1).definition.quantity==100,"merge conservation");
 auto distinct_metadata=split.profile;distinct_metadata.items.at(1).customized=u"Cmon Meow";distinct_metadata.items.at(2).customized=u"Other Owner";const auto metadata_swap=inventory::propose(distinct_metadata,r,{true,true});check(metadata_swap.accepted()&&metadata_swap.profile.items.size()==2&&metadata_swap.profile.items.at(1).customized==distinct_metadata.items.at(1).customized&&metadata_swap.profile.items.at(2).customized==distinct_metadata.items.at(2).customized,"different customization never merges away item ownership metadata");
 check(merged.packets.size()==3&&wire(merged.packets[1])==wire(items::detach_2022(1,2))&&wire(merged.packets[2])==wire(items::delete_2022(2)),"complete merge retires source placement and definition without replaying occupied destination");
 auto incompatible=split.profile;incompatible.items.at(2).definition.file=4321;auto swapped=inventory::propose(incompatible,r,{true,true});check(swapped.accepted()&&swapped.profile.items.at(2).slot==0&&swapped.profile.items.at(1).slot==1,"swap placements");check(swapped.packets.size()==3&&(swapped.packets[0][0]|(unsigned(swapped.packets[0][1])<<8))==332,"swap detaches destination before placement");
 auto capped=split.profile;capped.items.at(1).definition.quantity=245;capped.items.at(2).definition.quantity=25;auto partial=inventory::propose(capped,r,{true,true});check(partial.profile.items.at(1).definition.quantity==250&&partial.profile.items.at(2).definition.quantity==20,"merge cap leaves remainder without loss");
 r={inventory::Change::use,1};check(!inventory::propose(p,r,{true,true}).accepted(),"unsupported use cannot consume item");
 r={inventory::Change::destroy,1};check(!inventory::propose(p,r,{true,false}).accepted(),"destroy permission required");check(inventory::propose(p,r,{true,true}).profile.items.empty(),"destroy removes owned placement");
 auto bank=storage::baseline();r={inventory::Change::split,1,25,100,0};check(!storage::transfer(p,bank,r,{false,true,true}).accepted(),"closed chest denies bank access");auto deposit=storage::transfer(p,bank,r,{true,true,true});check(deposit.accepted()&&deposit.bank.items.at(2).definition.quantity==25&&deposit.character.items.at(1).definition.quantity==75,"atomic deposit partitions ownership");
 r={inventory::Change::move,2,0,1,0};auto withdraw=storage::transfer(deposit.character,deposit.bank,r,{true,true,true});check(withdraw.accepted()&&withdraw.bank.items.empty()&&withdraw.character.items.at(1).definition.quantity==100,"withdraw merge preserves total");
 {
  check(inventory::mutation_allowed(true,false)&&!inventory::mutation_allowed(true,true)&&!inventory::mutation_allowed(false,false),"trade and scene gate all inventory mutation families");
  auto character=deposit.character,stored=deposit.bank;stored.items.at(2).definition.file=4321;
  const auto char_before=inventory::encode(character),bank_before=inventory::encode(stored);
  const inventory::Request swap_deposit{inventory::Change::move,1,0,100,0};
  auto denied=storage::transfer(character,stored,swap_deposit,{true,true,false});
  check(!denied.accepted()&&denied.packets.empty()&&inventory::encode(denied.character)==char_before&&inventory::encode(denied.bank)==bank_before,"deposit swap cannot withdraw bank item without permission");
  const inventory::Request swap_withdraw{inventory::Change::move,2,0,1,0};
  denied=storage::transfer(character,stored,swap_withdraw,{true,false,true});
  check(!denied.accepted()&&denied.packets.empty()&&inventory::encode(denied.character)==char_before&&inventory::encode(denied.bank)==bank_before,"withdraw swap cannot deposit carried item without permission");
  auto authorized=storage::transfer(character,stored,swap_deposit,{true,true,true});
  check(authorized.accepted()&&authorized.character.items.contains(2)&&authorized.bank.items.contains(1)&&authorized.character.items.at(2).definition.quantity==25&&authorized.bank.items.at(1).definition.quantity==75,"authorized swap preserves actual IDs quantities and both ownership domains");
  auto ordinary_deposit=storage::transfer(character,storage::baseline(),swap_deposit,{true,true,false});
  check(ordinary_deposit.accepted()&&ordinary_deposit.bank.items.contains(1),"deposit alone does not require withdrawal permission");
 }
 inventory::Packet request;items::put(request,112,2);items::put(request,1,4);items::put(request,100,2);items::put(request,3,1);inventory::Request parsed;check(inventory::parse(request,parsed)==world::FrameState::complete&&parsed.bag==100&&parsed.slot==3&&parsed.consumed==9,"native move parses bag ID and zero based slot");
 const auto presentation=storage::restore_character(inventory::baseline());check(presentation.size()==8&&presentation[0][0]==(352&255)&&presentation[1][0]==(318&255)&&presentation.back()[0]==(327&255),"regular bag definition precedes bags and weapon sets restore last");
 const auto container=storage::presentation_container(0,20);check(container.file==111926&&container.model==32&&container.interaction==0x20001000&&container.id==storage::container_first&&container.type==3&&container.modifiers==std::vector<std::uint32_t>{0x24481400},"baseline backpack uses rendered captured physical asset and exact capacity modifier");
 const auto unknown_backpack=storage::presentation_container(0,30);check(unknown_backpack.file==1&&unknown_backpack.interaction==4,"unproven backpack capacity does not borrow mismatched captured template");
 for(unsigned index=1;index<=4;++index){const auto c=storage::presentation_container(index);check(c.file==1&&c.interaction==4&&c.id==storage::container_first+index&&c.quantity==1,"uncaptured fallback containers retain bounded native no-asset icon path");}
 const auto value32=[](const inventory::Packet& b,unsigned at){unsigned n=0;for(unsigned i=0;i<4;++i)n|=unsigned(b.at(at+i))<<(8*i);return n;};
 check(value32(presentation[0],2)==storage::container_first&&value32(presentation[1],9)==storage::container_first,"baseline backpack references valid unplaced container");
 check(inventory::baseline().items.empty()&&inventory::baseline().bags.at(1).container==0,"presentation never fabricates owned durable possessions");
 auto reserved=p;auto reserved_item=reserved.items.begin()->second;reserved.items.clear();reserved_item.definition.id=storage::container_first;reserved.items.emplace(reserved_item.definition.id,reserved_item);reserved.next_item=storage::container_first+1;
 bool reserved_denied=false;try{storage::restore_character(reserved);}catch(const std::invalid_argument&){reserved_denied=true;}check(reserved_denied,"character presentation ID collision denied");
 auto future=p;future.next_item=storage::container_first;check(!storage::transfer(future,storage::baseline(),{inventory::Change::split,1,25,100,0},{true,true,true}).accepted(),"future split cannot allocate a presentation-only ID");
 check(!storage::transfer(p,storage::baseline(),{inventory::Change::move,storage::container_first,0,100,0},{true,true,true}).accepted(),"client cannot move presentation-only container");
 auto full_character=p;for(unsigned index=1;index<=4;++index)full_character.bags.emplace(2+index,inventory::Bag{static_cast<std::uint16_t>(2+index),1,static_cast<std::uint8_t>(index),20,0});
 auto full_bank=storage::baseline();for(unsigned index=8;index<=20;++index)full_bank.bags.emplace(100+index,inventory::Bag{static_cast<std::uint16_t>(100+index),4,static_cast<std::uint8_t>(index),20,0});
 const inventory::Request last_tab{inventory::Change::split,1,25,120,19};
 auto full_deposit=storage::transfer(full_character,full_bank,last_tab,{true,true,true});
 check(full_deposit.accepted()&&full_deposit.bank.items.at(2).bag==120&&full_deposit.bank.items.at(2).slot==19,"all regular bags and fourteen storage tabs permit last-tab deposit");
 check(full_deposit.bank.bags.at(120).index==20&&full_deposit.bank.bags.at(120).type==4,"storage transfer preserves actual indexes and types");
 check(inventory::encode(inventory::decode(inventory::encode(full_deposit.bank)))==inventory::encode(full_deposit.bank),"full storage survives persistence codec");
 check(!inventory::propose(full_deposit.bank,{inventory::Change::move,2,0,100,0},{true,false}).accepted(),"ordinary move permission cannot operate bank items");
 check(!storage::transfer(full_character,full_bank,last_tab,{false,true,true}).accepted(),"full storage retains chest authorization");
 auto weapons=inventory::baseline();auto weapon=item;weapon.definition.id=42;weapon.definition.type=5;weapon.definition.quantity=1;weapon.definition.interaction=0x200000;weapon.bag=1;weapon.slot=0;weapons.items.emplace(42,weapon);weapon.definition.id=43;weapon.definition.type=24;weapon.definition.interaction=0;weapon.slot=1;weapons.items.emplace(43,weapon);weapons.next_item=44;weapons.weapons.sets[0]={42,43};weapons.weapons.sets[2]={42,43};weapons.weapons.active=2;
 const auto weapon_restore=inventory::decode(inventory::encode(weapons));check(weapon_restore.weapons==weapons.weapons&&weapon_sets::effective(weapon_restore.weapons,inventory::weapon_lookup(weapon_restore)).offhand==0,"four sets persist shared IDs and two-handed effective suppression without losing offhand");
 const auto destroyed_weapon=inventory::propose(weapons,{inventory::Change::destroy,42},{true,true});check(destroyed_weapon.accepted()&&destroyed_weapon.profile.weapons.sets[0].weapon==0&&destroyed_weapon.profile.weapons.sets[2].weapon==0&&destroyed_weapon.profile.weapons.sets[2].offhand==43&&destroyed_weapon.packets.back()[0]==(327&255),"destroy clears every weapon reference and restores sets after deletion");
 const auto banked_weapon=storage::transfer(weapons,storage::baseline(),{inventory::Change::move,42,0,100,0},{true,true,true});check(banked_weapon.accepted()&&banked_weapon.character.weapons.sets[0].weapon==0&&banked_weapon.bank.items.contains(42)&&inventory::weapon_packets(banked_weapon.bank).empty(),"bank deposit retires character references without bank restoration resetting active character set");
 auto crowded_character=inventory::baseline();crowded_character.bags.at(1).capacity=64;
 for(unsigned index=1;index<=4;++index)crowded_character.bags.emplace(2+index,inventory::Bag{static_cast<std::uint16_t>(2+index),1,static_cast<std::uint8_t>(index),64,0});
 auto crowded_bank=storage::baseline();crowded_bank.bags.at(100).capacity=20;for(unsigned index=8;index<=20;++index)crowded_bank.bags.emplace(100+index,inventory::Bag{static_cast<std::uint16_t>(100+index),4,static_cast<std::uint8_t>(index),20,0});
 unsigned crowded_id=1;
 for(const auto& [bag_id,bag]:crowded_character.bags)for(unsigned slot=0;slot<bag.capacity;++slot){inventory::Item entry;entry.definition.id=crowded_id++;entry.definition.file=1234;entry.definition.type=11;entry.definition.quantity=1;entry.bag=bag_id;entry.slot=static_cast<std::uint8_t>(slot);crowded_character.items.emplace(entry.definition.id,entry);}
 for(const auto& [bag_id,bag]:crowded_bank.bags)if(bag.type==4)for(unsigned slot=0;slot<bag.capacity;++slot){if(bag_id==120&&slot==19)continue;inventory::Item entry;entry.definition.id=crowded_id++;entry.definition.file=1234;entry.definition.type=11;entry.definition.quantity=1;entry.bag=bag_id;entry.slot=static_cast<std::uint8_t>(slot);crowded_bank.items.emplace(entry.definition.id,entry);}
 crowded_character.next_item=crowded_bank.next_item=crowded_id;
 check(crowded_character.items.size()==329&&crowded_bank.items.size()==279,"physically bounded ownership domains exceed512 only when combined");
 const auto before_character=inventory::encode(crowded_character),before_bank=inventory::encode(crowded_bank);
 const auto crowded_transfer=storage::transfer(crowded_character,crowded_bank,{inventory::Change::move,1,0,120,19},{true,true,true});
 check(crowded_transfer.accepted()&&crowded_transfer.character.items.size()==328&&crowded_transfer.bank.items.size()==280,"608 combined items transfer without validation exception");
 check(inventory::encode(crowded_character)==before_character&&inventory::encode(crowded_bank)==before_bank,"large transfer preserves proposal input identities");
 check(inventory::decode(inventory::encode(crowded_transfer.bank)).items.at(1).bag==120,"large transfer final bank durable codec remains valid");
 crowded_character.items.at(1).definition.quantity=2;crowded_character.items.at(1).definition.interaction=0x80000;
 const auto crowded_split=storage::transfer(crowded_character,crowded_bank,{inventory::Change::split,1,1,120,19},{true,true,true});
 check(crowded_split.accepted()&&crowded_split.character.items.at(1).definition.quantity==1&&crowded_split.bank.items.at(crowded_id).definition.quantity==1&&crowded_split.bank.next_item==crowded_id+1,"combined split allocates bounded identity and validates domains separately");
 auto combined=crowded_character;for(auto [id,bag]:crowded_bank.bags)combined.bags.emplace(id,bag);for(auto [id,entry]:crowded_bank.items)combined.items.emplace(id,entry);
 bool codec_denied=false;try{inventory::encode(combined);}catch(const std::invalid_argument&){codec_denied=true;}check(codec_denied,"combined allowance cannot leak into persistentcodec");
 check(!inventory::propose(combined,{inventory::Change::move,1,0,120,19},{true,false,false},inventory::Bounds::combined_transfer).accepted(),"explicit combined scope requires trusted storage permission");
 // Material pane contents remain actual owned items, with element slots and
 // account quota independent from its41-entry pointer array capacity.
 auto material_bank=storage::baseline();check(material_bank.bags.at(100).capacity==25&&material_bank.bags.at(101).type==5&&material_bank.bags.at(101).index==5&&material_bank.bags.at(101).capacity==41&&material_bank.material_quota==250,"new storage baseline uses reviewed25-slot tab and default material pane");
 auto old_bank=material_bank;old_bank.bags.erase(101);old_bank.bags.at(100).capacity=20;old_bank.gold=54321;auto old_bank_bytes=inventory::encode(old_bank);old_bank_bytes.resize(old_bank_bytes.size()-2);old_bank_bytes[3]=4;
 const auto migrated_bank=inventory::decode(old_bank_bytes);check(migrated_bank.bags.size()==1&&migrated_bank.bags.at(100).capacity==20&&migrated_bank.gold==54321&&migrated_bank.material_quota==250,"legacy20-slot storage preserves its captured structure and gold without inferred purchases");
 auto material_character=inventory::baseline();inventory::Item diamond;diamond.definition.id=42;diamond.definition.file=1234;diamond.definition.type=11;diamond.definition.model=935;diamond.definition.quantity=250;diamond.definition.interaction=0x80000;diamond.definition.modifiers={0x25081800};diamond.bag=1;diamond.profession=6;diamond.customized=u"Exact fixture metadata";material_character.items.emplace(42,diamond);material_character.next_item=44;material_character.gold=12345;
 auto stored_diamond=diamond;stored_diamond.definition.id=43;stored_diamond.definition.quantity=2250;stored_diamond.bag=101;stored_diamond.slot=24;material_bank.items.emplace(43,stored_diamond);material_bank.next_item=44;material_bank.material_quota=2500;material_bank.gold=765432;
 const auto material_bytes=inventory::encode(material_bank),saved_material_character=inventory::encode(material_character);const auto decoded_material=inventory::decode(material_bytes);
 check(material_bytes[3]==5&&decoded_material.material_quota==2500&&decoded_material.gold==765432&&decoded_material.items.at(43).definition.quantity==2250&&decoded_material.items.at(43).slot==24&&decoded_material.items.at(43).customized==diamond.customized,"FWI5 restores explicit quota/count/element/exact metadata and bank gold");
 auto malformed_material=material_bank;malformed_material.items.at(43).slot=21;rejected_profile(malformed_material,"visual display21 cannot replace native Diamond element24");
 malformed_material=material_bank;malformed_material.items.at(43).definition.modifiers={0x25080000};rejected_profile(malformed_material,"material model/property disagreement cannot persist");
 malformed_material=material_bank;malformed_material.items.at(43).definition.quantity=2501;rejected_profile(malformed_material,"material quota applies to actual item count");
 malformed_material=material_bank;malformed_material.material_quota=251;rejected_profile(malformed_material,"unreviewed storage increments reject codec");
 malformed_material=material_bank;malformed_material.bags.at(101).index=6;rejected_profile(malformed_material,"GWCA public array6 is not native material index5");
 malformed_material=material_bank;malformed_material.bags.at(101).container=43;rejected_profile(malformed_material,"material pane has no installed bag-container relation");
 auto impossible_ordinary=material_character;impossible_ordinary.items.at(42).definition.quantity=251;rejected_profile(impossible_ordinary,"ordinary stack cannot persist above250");
 const auto material_deposit=storage::transfer(material_character,material_bank,{inventory::Change::move,42,0,101,24},{true,true,true});
 check(material_deposit.accepted()&&material_deposit.character.items.empty()&&material_deposit.bank.items.at(43).definition.quantity==2500&&material_deposit.bank.material_quota==2500&&material_deposit.character.gold==12345&&material_deposit.bank.gold==765432,"atomic material merge conserves all units and leaves currency/quota unchanged");
 check(material_deposit.packets.size()==3&&material_deposit.packets[0]==items::quantity_2022(43,2500)&&material_deposit.packets[1]==items::detach_2022(1,42)&&material_deposit.packets[2]==items::delete_2022(42),"complete material merge updates survivor then removes source placement/definition");
 check(inventory::encode(material_bank)==material_bytes&&inventory::encode(material_character)==saved_material_character,"failed CAS may discard material proposal without either input mutating");
 const auto wrong_material_slot=storage::transfer(material_character,material_bank,{inventory::Change::move,42,0,101,21},{true,true,true});check(!wrong_material_slot.accepted()&&wrong_material_slot.packets.empty()&&inventory::encode(wrong_material_slot.bank)==material_bytes,"wrong visual-slot request cannot move owned items");
 check(!storage::transfer(material_character,material_bank,{inventory::Change::move,42,0,101,24},{false,true,true}).accepted()&&!storage::transfer(material_character,material_bank,{inventory::Change::move,42,0,101,24},{true,false,true}).accepted(),"material transfers retain chest and direction authorization");
 const auto material_withdrawal=storage::transfer(material_deposit.character,material_deposit.bank,{inventory::Change::move,43,0,1,0},{true,true,true});
 check(material_withdrawal.accepted()&&material_withdrawal.character.items.at(44).definition.quantity==250&&material_withdrawal.character.items.at(44).profession==diamond.profession&&material_withdrawal.character.items.at(44).customized==diamond.customized&&material_withdrawal.bank.items.at(43).definition.quantity==2250&&material_withdrawal.bank.material_quota==2500,"whole-material withdrawal splits250 to ordinary carry and preserves2250 plus actual metadata");
 check(material_withdrawal.packets.size()==5&&material_withdrawal.packets[0]==items::quantity_2022(43,2250)&&material_withdrawal.packets[1][0]==(352&255)&&material_withdrawal.packets[2][0]==(345&255)&&material_withdrawal.packets[3][0]==(313&255)&&material_withdrawal.packets[4]==items::location_2022(1,44,1,0),"new withdrawn identity is defined with metadata before ordinary placement");
 const auto internal_material_move=storage::transfer(material_withdrawal.character,material_withdrawal.bank,{inventory::Change::move,43,0,100,24},{true,true,true});
 check(internal_material_move.accepted()&&internal_material_move.bank.items.at(45).definition.quantity==250&&internal_material_move.bank.items.at(45).bag==100&&internal_material_move.bank.items.at(45).slot==24&&internal_material_move.bank.items.at(43).definition.quantity==2000,"material withdrawal inside account bank respects ordinary cap and new25th slot");
 const auto internal_deposit=storage::transfer(internal_material_move.character,internal_material_move.bank,{inventory::Change::move,45,0,101,24},{true,true,true});check(internal_deposit.accepted()&&!internal_deposit.bank.items.contains(45)&&internal_deposit.bank.items.at(43).definition.quantity==2250,"bank-to-material merge is authorized and conserves exact units");
 check(!inventory::propose(material_bank,{inventory::Change::move,43,0,100,0},{true,false}).accepted(),"ordinary inventory permission does not authorize material storage");
 const auto material_restore=inventory::restore(material_bank);bool bag_seen=false;for(const auto& packet:material_restore){const auto opcode=packet[0]|(unsigned(packet[1])<<8);if(opcode==318)bag_seen=true;if(opcode==352)check(!bag_seen,"material definitions exist before first bag restore");if(opcode==317)check(bag_seen,"material locations follow actual bag definitions");}
 check(full_character.bags.size()+full_bank.bags.size()==21,"full carrying/equipped plus14tabs/material pane fits21 combined bags");
 auto provisioned=storage::provision_panes(full_deposit.bank,4);
 check(inventory::encode(provisioned)==inventory::encode(full_deposit.bank),"provisioning preserves purchased panes and existing items");
 auto original_bank=storage::baseline();original_bank.gold=12345;original_bank.material_quota=500;
 const auto four_panes=storage::provision_panes(original_bank,4);
 check(four_panes.bags.size()==5&&four_panes.gold==12345&&four_panes.material_quota==500&&four_panes.bags.at(101).type==5,"provisioning preserves funds and material pane");
 for(unsigned index=7;index<=10;++index)check(std::count_if(four_panes.bags.begin(),four_panes.bags.end(),[&](const auto& entry){return entry.second.type==4&&entry.second.index==index&&entry.second.capacity==25;})==1,"provisioned pane indexes unique and native sized");
 check(inventory::encode(storage::provision_panes(four_panes,4))==inventory::encode(four_panes),"pane provisioning idempotent");
 bool pane_bound=false;try{storage::provision_panes(original_bank,15);}catch(const std::invalid_argument&){pane_bound=true;}check(pane_bound,"pane entitlement bound enforced");
 std::cout<<"Native inventory codec, ownership, moves and bank transfer tests passed.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
