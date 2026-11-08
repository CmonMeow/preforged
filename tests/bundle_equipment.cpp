#include "gw_bundle_equipment.h"
#include "gw_weapon_passives.h"
#include "gw_shield_stance.h"
#include "gw_basic_armor_energy.h"
#include "gw_enchantment_duration.h"
#include <iostream>
using namespace fw::gw;
void check(bool value){if(!value)throw std::runtime_error("bundle equipment regression");}
int main()try{
    auto owned=inventory::baseline();owned.next_item=5;owned.gold=123;
    for(unsigned id=1;id<=4;++id){
        inventory::Item item;item.definition.id=id;item.definition.file=1234;item.definition.quantity=1;
        item.definition.type=id==2?24:id==3?7:26;item.bag=id==4?1:2;item.slot=id==4?0:id-1;
        if(id==1)item.definition.modifiers={0x22b80014u,0x62c80a00u};
        if(id==2)item.definition.modifiers={0x62c80c00u};
        if(id==3){item.profession=6;item.reviewed_armor=inventory::ReviewedArmor{60,0,0};}
        if(id==4)item.definition.modifiers={0x22b8000au};
        owned.items.emplace(id,item);
    }
    owned.weapons.sets[0]={1,2};owned.weapons.sets[1]={4,2};inventory::validate(owned);
    const auto encoded=inventory::encode(owned);const auto sets=inventory::weapon_packets(owned);
    const auto before=energy::owned_basic_armor(owned,6);
    check(items::weapon_passives::worn_energy(owned)==22);
    check(items::weapon_passives::equipped_enchanting(owned,7)->percent()==20);
    check(skills::effects::shield_equipment(owned,7)->shield==2);
    const auto held=bundles::without_weapon_benefits(owned);inventory::validate(held);
    check(!held.items.contains(1)&&!held.items.contains(2)&&held.items.contains(3)&&held.items.contains(4));
    check(items::weapon_passives::worn_energy(held)==0);
    check(items::weapon_passives::equipped_enchanting(held,7)->percent()==0);
    const skills::effects::Scope caster{1,1,1};
    const auto normal_duration=skills::effects::enchanting_duration(1392,10000,caster,
        *items::weapon_passives::equipped_enchanting(owned,7),{},1000);
    const auto held_duration=skills::effects::enchanting_duration(1392,10000,caster,
        *items::weapon_passives::equipped_enchanting(held,7),{},1000);
    check(normal_duration&&normal_duration->for_enchantment(1392,10000,caster,1000)==12000);
    check(held_duration&&held_duration->for_enchantment(1392,10000,caster,1000)==10000);
    check(skills::effects::shield_equipment(held,7)->shield==0);
    const auto after=energy::owned_basic_armor(held,6);
    check(before.capacity==after.capacity&&before.pips==after.pips);
    check(energy::fully_modeled_physical_armor(owned,6)==energy::fully_modeled_physical_armor(held,6));
    check(held.gold==owned.gold&&held.next_item==owned.next_item);
    check(inventory::encode(owned)==encoded&&inventory::weapon_packets(owned)==sets);
    check(items::weapon_passives::equipped_enchanting(owned,7)->percent()==20);
    auto bad=owned;bad.weapons.sets[3].weapon=99;bool rejected=false;
    try{(void)bundles::without_weapon_benefits(bad);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected);std::cout<<"Bundle equipment view passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
