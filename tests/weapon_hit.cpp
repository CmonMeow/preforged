#include "gw_weapon_hit.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>
int main(int argc,char**argv)try{
 using namespace fw::gw::combat;auto check=[](bool ok){if(!ok)throw std::runtime_error("ordinary hit regression");};
 check(argc==3);Actor target;target.id=42;target.generation=9;target.damageable=true;target.health=Health{480,480};
 auto hit=weapon::apply_resolved(target,120,true);check(hit&&hit->skill==0&&hit->after.current==360&&hit->generation==9);
 auto packets=weapon::packets_2022(43,*hit);std::ifstream fixture(argv[1]);check(bool(fixture));std::string row;unsigned count=0;
 while(std::getline(fixture,row)){if(row.empty())continue;check(count<packets.size());auto framed=packets[count++];framed.at(1)|=0x80;std::ostringstream hex;hex<<std::hex;for(auto byte:framed){if(byte<16)hex<<'0';hex<<unsigned(byte);}check(hex.str()==row);}
 check(count==3);hit=weapon::apply_resolved(target,500,true);check(hit&&hit->killed&&hit->applied==360&&hit->damage==500&&target.effects==0x10);
 check(weapon::packets_2022(43,*hit).size()==4);check(!weapon::apply_resolved(target,1,true));
 target.health=Health{100,480};target.effects=0;check(!weapon::apply_resolved(target,1,false));
 check(!weapon::apply_resolved(target,std::numeric_limits<double>::quiet_NaN(),true));
 hit=weapon::apply_resolved(target,0,true);check(hit&&target.health->current==100&&weapon::packets_2022(43,*hit).empty());
 const std::vector<Packet> actions{weapon::start_2022(42,43),weapon::start_2022(43,42),weapon::finish_2022(42,false),weapon::finish_2022(42,true),weapon::stop_2022(42)};
 std::ifstream animation(argv[2]);check(bool(animation));count=0;
 while(std::getline(animation,row)){if(row.empty())continue;const auto space=row.find(' ');check(space!=std::string::npos&&count<actions.size());auto framed=actions[count++];framed.at(1)|=0x80;std::ostringstream hex;hex<<std::hex;for(auto byte:framed){if(byte<16)hex<<'0';hex<<unsigned(byte);}check(hex.str()==row.substr(space+1));}
 check(count==5);
 bool rejected=false;try{weapon::start_2022(42,42);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 std::cout<<"PASS native ordinary zero-attribution wire, fractional health, overkill/death, hostile and invalid guards\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
