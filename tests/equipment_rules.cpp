#include "gw_equipment_rules.h"
#include <iostream>
#include <stdexcept>
int main()try{
 using namespace fw::gw::equipment::rules;auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
 std::array<std::optional<ArmorPiece>,5> gear{};
 gear[0]=ArmorPiece{Slot::head,6,{RuneKind::attribute,9,3},Insignia::survivor};
 gear[1]=ArmorPiece{Slot::chest,6,{RuneKind::attribute,9,3},Insignia::radiant};
 gear[2]=ArmorPiece{Slot::hands,6,{RuneKind::vigor,0,3},Insignia::survivor};
 gear[3]=ArmorPiece{Slot::legs,6,{RuneKind::vigor,0,2},Insignia::survivor};
 gear[4]=ArmorPiece{Slot::feet,6,{RuneKind::attunement},Insignia::radiant};
 auto bonus=armor_bonuses(6,gear);check(bonus&&bonus->attributes[9]==3&&bonus->health==-80&&bonus->energy==6,"rune positive nonstack, negative stack, Vigor strongest and perpiece insignia");
 gear[1]->profession=3;check(!armor_bonuses(6,gear),"foreign profession armor denied");gear[1]->profession=6;gear[1]->rune={RuneKind::attribute,13,3};check(!armor_bonuses(6,gear),"secondary profession rune cannot be worn");
 gear={};gear[0]=ArmorPiece{Slot::head,6,{RuneKind::vitae}};gear[1]=ArmorPiece{Slot::chest,6,{RuneKind::vitae}};check(armor_bonuses(6,gear)->health==20,"Vitae stacks");
 gear={};gear[0]=ArmorPiece{Slot::head,1,{RuneKind::absorption,0,1}};gear[1]=ArmorPiece{Slot::chest,1,{RuneKind::absorption,0,3}};gear[2]=ArmorPiece{Slot::hands,1,{RuneKind::absorption,0,2}};
 check(armor_bonuses(1,gear)->physical_reduction==3&&armor_bonuses(1,gear)->health==0,"Absorption strongest global benefit and no health penalty");
 gear[0]->rune={RuneKind::absorption,0,4};check(!armor_bonuses(1,gear),"unreviewed absorption tier denied"); check(upgraded_capacity(Container::bag,5,HoldingRune::normal)==10&&upgraded_capacity(Container::bag,10,HoldingRune::superior)==15,"bag upgrade stages");
 check(!upgraded_capacity(Container::bag,10,HoldingRune::normal)&&!upgraded_capacity(Container::equipment_pack,5,HoldingRune::superior)&&!upgraded_capacity(Container::belt_pouch,5,HoldingRune::normal),"wrong upgrade cannot consume or change container");
 check(upgraded_capacity(Container::belt_pouch,5,HoldingRune::belt)==10&&!can_uninstall_container(1)&&can_uninstall_container(0),"belt rune and nonempty container restriction");
 std::cout<<"Reviewed equipment and container upgrade rules passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
