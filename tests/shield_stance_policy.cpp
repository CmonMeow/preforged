#include "gw_shield_stance.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Shield Stance policy line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){
  const auto p=shield_stance(rank);check(p&&p->duration==1000ull*interpolate(1,6,rank)&&p->block==.75);
 }
 check(!shield_stance(21));
 inventory::Profile profile;profile.bags.emplace(1,inventory::Bag{1,1,1,20});
 const auto item=[&](unsigned id,unsigned type,unsigned interaction=0){inventory::Item i;i.definition.id=id;i.definition.type=type;i.definition.quantity=1;i.definition.interaction=interaction;i.bag=1;profile.items[id]=i;};
 item(10,27);item(11,24);item(12,12);item(13,5,0x200000);
 profile.weapons.sets[0]={10,11};profile.weapons.sets[1]={13,11};profile.weapons.sets[2]={10,12};
 check(shield_equipment(profile,1)==ShieldEquipment{1,11});
 profile.weapons.active=1;check(shield_equipment(profile,2)==ShieldEquipment{2,0});
 check(profile.weapons.sets[1].offhand==11);
 profile.weapons.active=2;check(shield_equipment(profile,3)==ShieldEquipment{3,0});
 profile.weapons.active=3;check(shield_equipment(profile,4)==ShieldEquipment{4,0});
 check(!shield_equipment(profile,0));
 profile.weapons.active=0;profile.items[11].bag=99;check(!shield_equipment(profile,5));
 profile.items[11].bag=1;profile.bags[1].type=3;check(!shield_equipment(profile,5));
 profile.bags[1].type=1;profile.items[11].definition.quantity=2;check(!shield_equipment(profile,5));
 profile.items[11].definition.quantity=1;profile.weapons.active=4;check(!shield_equipment(profile,5));profile.weapons.active=0;
 std::vector<Entry> entries{{100,1,378,12,Category::stance,1000,6000}};
 for(unsigned strength=0;strength<=20;++strength){const auto p=shield_stance_protection(entries,1000,shield_equipment(profile,6),strength);check(p&&p->instance==100&&p->block==.75&&p->reduction==std::min(15u,2*strength));}
 check(!shield_stance_protection(entries,1000,{},12));check(!shield_stance_protection(entries,1000,ShieldEquipment{6,11},{}));
 check(!shield_stance_protection(entries,1000,ShieldEquipment{6,11},21));
 check(shield_stance_protection(entries,1000,ShieldEquipment{6,0},12)==ShieldStanceProtection{});
 for(auto at:{999ull,6000ull,7000ull})check(shield_stance_protection(entries,at,{}, {})==ShieldStanceProtection{});
 entries.push_back({101,1,378,0,Category::stance,2000,3000});
 check(shield_stance_protection(entries,2000,ShieldEquipment{7,11},8)->instance==101);
 entries.back().rank=21;check(!shield_stance_protection(entries,2000,ShieldEquipment{7,11},8));
 entries.back().expires=2000;check(shield_stance_protection(entries,2000,ShieldEquipment{7,11},8)->instance==100);
 std::cout<<"PASS Shield Stance native-row policy, duration/rank caps, current shield ownership/two-handed suppression, Strength reduction, unknown facts and effect boundaries\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
