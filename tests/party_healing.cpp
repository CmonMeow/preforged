#include "gw_party_healing.h"
#include "gw_combat.h"
#include <fstream>
#include <string>
#include <iostream>
#include <stdexcept>
int main()try{
 using namespace fw::gw::skills::effects;auto check=[](bool b,const char* m){if(!b)throw std::runtime_error(m);};
 auto hp=party_heal(287,12,10),pvp=party_heal(3232,12,10),lod=party_heal(1397,12,10);
 check(hp&&hp->base_heal==66&&hp->range==5000,"Heal Party pinned scales and party range");
 check(recipient_heal(*hp,false)==66&&recipient_heal(*hp,true)==98,"Divine Favor affects caster only");
 check(pvp&&pvp->base_heal==66,"PvP Heal Party shared semantics independent timing");
 check(lod&&lod->base_heal==57&&recipient_heal(*lod,true)==89,"Light of Deliverance uses genuine heal field not101 threshold");
 check(!party_heal(835,12,10)&&!party_heal(2871,12,10)&&!party_heal(287,21,10),"wrongID and incomplete PvP disabling effect not falsely supported");
 for(unsigned id:{279u,1117u,280u,1119u}){auto plan=*party_heal(id,12,12);check(plan.base_heal==(id==279||id==1117?51u:150u)&&plan.range==(id==279||id==1117?1000.f:166.f)&&plan.all_creatures==(id==280||id==1119),"Exact area versus party recipients and skill attribute scales");check(recipient_heal(plan,true)==plan.base_heal+38&&recipient_heal(plan,false)==plan.base_heal,"Untargeted spells give caster-only Divine Favor");}
 std::ifstream fixture(FIXTURE_FILE);check(bool(fixture),"native heal fixture present");
 const auto compare=[&](fw::gw::combat::Packet packet){std::string line;check(bool(std::getline(fixture,line)),"native heal fixture line");std::vector<unsigned char> expected;for(std::size_t at=0;at<line.size();at+=2)expected.push_back(static_cast<unsigned char>(std::stoul(line.substr(at,2),nullptr,16)));packet[1]|=0x80;check(packet==expected,"native161 health serializer exact");};
 for(unsigned id:{279u,1117u,280u,1119u}){auto plan=*party_heal(id,12,12);compare(fw::gw::combat::detail::fraction(1,{double(100+recipient_heal(plan,true)),480}));compare(fw::gw::combat::detail::fraction(2,{double(100+recipient_heal(plan,false)),480}));}compare(fw::gw::combat::detail::fraction(1,{183,480}));
 std::cout<<"Reviewed party healing plans passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
