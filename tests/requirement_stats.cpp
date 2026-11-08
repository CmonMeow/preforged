#include "../src/runtime/gw_requirement_stats.h"
#include <iostream>
using namespace fw::gw::items::requirements;
void check(bool v){if(!v)throw std::runtime_error("requirement stats regression");}
int main()try{
 std::array<std::uint8_t,51> rank{};const std::array<std::uint32_t,1> req{0x27980909};
 check(resolve(req,{16,{},Origin::unknown},rank)->shield_armor==8);
 check(resolve(req,{12,{},Origin::nonrandom},rank)->shield_armor==5);
 check(resolve(req,{12,{},Origin::random_drop},rank)->shield_armor==8);
 check(!resolve(req,{12,{},Origin::unknown},rank)->shield_armor);
 check(resolve(req,{{},12,Origin::random_drop},rank)->focus_energy==6);
 check(resolve(req,{{},12,Origin::nonrandom},rank)->focus_energy==3);
 check(!resolve(req,{{},12,Origin::unknown},rank)->focus_energy);
 rank[9]=9;check(resolve(req,{{},12,Origin::unknown},rank)->focus_energy==12);
 const std::array<std::uint32_t,3> weapon{0x27980909,0xa7a8160b,0x24b80300};
 check(usable_weapon_damage(weapon,rank)->minimum==11&&usable_weapon_damage(weapon,rank)->maximum==22);
 rank[9]=8;check(!usable_weapon_damage(weapon,rank));
 const std::array<std::uint32_t,2> duplicate{req[0],req[0]};check(!resolve(duplicate,{},rank));
 std::cout<<"PASS requirement equality, effective ranks, provenance fallbacks and unknown denial\n";
}catch(const std::exception&e){std::cerr<<e.what();return 1;}
