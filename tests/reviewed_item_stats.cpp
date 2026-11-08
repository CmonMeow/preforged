#include "gw_reviewed_item_stats.h"
#include "gw_armor_attribute_runes.h"
#include <array>
#include <iostream>
#include <stdexcept>
int main()try{
 using namespace fw::gw::items;auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
 auto raw=std::array<std::uint32_t,3>{0xa7a81c0f,0x24b80200,0x2798090a};auto records=modifiers::decode_wire(raw);
 auto damage=stats::reviewed_weapon_damage(records);check(damage&&damage->minimum==15&&damage->maximum==28&&damage->raw_damage_type==2,"source reviewed min low/max high and raw damage byte");
 const auto requirement=fw::gw::equipment::attribute_runes::requirement(raw);
 check(requirement&&requirement->attribute==9&&requirement->rank==10&&records[2].raw==raw[2],"native requirement decoded without changing raw capture");
 auto duplicate=records;duplicate.push_back(records[0]);check(!stats::reviewed_weapon_damage(duplicate),"duplicate base damage denied rather than selecting arbitrary record");
 check(!stats::reviewed_weapon_damage(modifiers::decode_wire(std::array<std::uint32_t,1>{0xa7a80f1c})),"reversed impossible damage denied");
 auto without_type=stats::reviewed_weapon_damage(modifiers::decode_wire(std::array<std::uint32_t,1>{0xa7a81c0f}));check(without_type&&!without_type->raw_damage_type,"absent damage type never defaulted");
 const std::array<std::uint32_t,2> starter{0x24b80200,0xa4880302};
 damage=stats::reviewed_weapon_damage(modifiers::decode_wire(starter));check(damage&&damage->minimum==2&&damage->maximum==3&&damage->raw_damage_type==2,"captured Starter Sword matches local wiki slashing2..3");
 check(!stats::reviewed_weapon_damage(modifiers::decode_wire(std::array<std::uint32_t,2>{0x24b80200,0xa4880a05})),"unreviewed A488 values not extrapolated");
 check(!stats::reviewed_weapon_damage(modifiers::decode_wire(std::array<std::uint32_t,3>{0x24b80200,0xa4880302,0xa7a80302})),"mixed base damage properties rejected");
 std::cout<<"Reviewed captured raw weapon damage tests passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
