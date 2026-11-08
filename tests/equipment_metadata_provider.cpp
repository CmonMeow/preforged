#include "gw_equipment_metadata_provider.h"
#include "gw_item_base_armor.h"
#include "gw_basic_armor_energy.h"
#include <iostream>
#include <stdexcept>
int main()try{
 using namespace fw::gw;auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
 auto inventory=inventory::baseline();inventory.next_item=2;items::Definition definition;definition.id=1;definition.file=1;definition.type=7;definition.model=123;
 inventory.items.emplace(1,inventory::Item{definition,1,0});
 check(!equipment::metadata::reviewed(inventory,definition),"absent capture metadata not interpreted as generic permission");
 auto& item=inventory.items.at(1);item.profession=6;item.customized=u"";
 auto record=equipment::metadata::reviewed(inventory,definition);check(record&&record->slot==2&&record->profession==6&&record->customized_character.empty(),"known metadata and native chest slot");
 item.profession=11;check(!equipment::metadata::reviewed(inventory,definition),"nativeunknownprofession11 cannot authorize equipment");item.profession=6;
 item.customized=u"\u4e2d\u6587";record=equipment::metadata::reviewed(inventory,definition);check(record&&record->customized_character=="\xe4\xb8\xad\xe6\x96\x87","customized owner UTF8 conversion preserves nonASCIIidentity");
 item.customized=std::u16string(1,char16_t(0xd800));check(!equipment::metadata::reviewed(inventory,definition),"invalid owner surrogate denied");item.customized=u"";
 auto forged=definition;forged.type=16;check(!equipment::metadata::reviewed(inventory,forged),"sameID forged type does not pass metadata provider");
 item.definition.type=22;item.definition.interaction=0x200000;record=equipment::metadata::reviewed(inventory,item.definition);check(record&&record->slot==0&&record->two_handed,"native twohandbit and main slot");
 item.definition.modifiers={0x27980909,0xa7a8160b,0x24b80300};
 record=equipment::metadata::reviewed(inventory,item.definition);check(record&&record->required_attribute==9&&record->required_rank==9,"native requirement exposed without equip denial");
 std::array<std::uint8_t,51> ranks{};ranks[9]=9;check(items::requirements::usable_weapon_damage(item.definition.modifiers,ranks)->maximum==22,"reviewed equipped weapon base stats available with met requirement");
 item.definition.modifiers.push_back(0x27980909);check(!equipment::metadata::reviewed(inventory,item.definition),"ambiguous requirement metadata denied");
 item.definition.type=7;item.definition.modifiers={0xa3c83c00};item.bag=2;item.slot=2;
 check(items::base_armor::owned(inventory,6)[0]==60,"owned exact captured Obsidian base armor");
 item.bag=1;item.slot=0;check(!items::base_armor::owned(inventory,6)[0],"carried armor not worn");
 item.bag=2;item.slot=2;item.definition.modifiers={0xa3c83c00,0x62c80500};check(energy::owned_basic_armor(inventory,6).capacity==5,"actual captured torsoenergy honored");
 item.definition.modifiers={0xa3c83c00};check(energy::owned_basic_armor(inventory,6).capacity==0,"raw explicit armor no invented professionenergy");
 item.reviewed_armor=inventory::ReviewedArmor{60,0,0};item.definition.modifiers={0x21e80a01};
 check(energy::owned_basic_armor(inventory,6).capacity==5,"upgraded private baseline energy agrees with projected tooltip");
 item.definition.modifiers.push_back(0x62c80300);
 check(energy::owned_basic_armor(inventory,6).capacity==3,"explicit energy overrides private baseline identically on server and client");
 item.reviewed_armor.reset();item.definition.modifiers={0x62c80300};
 check(energy::owned_basic_armor(inventory,6).capacity==3,"captured explicit energy does not require a separate base armor property");
 item.reviewed_armor=inventory::ReviewedArmor{60,0,0};item.definition.modifiers={0xa3c81900,0xa3c83c00,0x62c80300};
 check(energy::owned_basic_armor(inventory,6).capacity==3,"ambiguous armor rating does not erase an independently valid explicit energy property");
 const std::array<std::uint32_t,2> repeatedarmor{0xa3c83c00,0xa3c85000};check(!items::base_armor::decode(repeatedarmor),"ambiguous base armor denied");
 std::cout<<"Authoritative captured equipment metadata provider tests passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
