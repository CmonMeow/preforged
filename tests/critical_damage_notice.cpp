#include "gw_weapon_hit.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat;
void check(bool b){if(!b)throw std::runtime_error("Critical damage notice");}
unsigned word(const Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
int main()try{
 for(double damage:{2.,600.}){Actor actor{30,1,Health{480,480},{},0,true};auto hit=*weapon::apply_resolved(actor,damage,true);hit.skill=321;
  const auto normal=damage_packets_2022(25,hit),critical=damage_packets_2022(25,hit,true);
  check(critical.size()==normal.size()&&critical[0]==normal[0]&&critical[2]==normal[2]);
  check(word(critical[1],2)==17&&word(normal[1],2)==16);
  auto normalized=critical;normalized[1][2]=16;check(normalized==normal);
  check(word(critical[0],10)==321&&word(critical[1],6)==30&&word(critical[1],10)==25);
  check(word(critical[1],14)==std::bit_cast<unsigned>(float(-damage/480.)));
  check(word(critical[2],10)==std::bit_cast<unsigned>(float(hit.after.current/480.)));
  if(damage==2.)check(critical[1]==Packet{162,0,17,0,0,0,30,0,0,0,25,0,0,0,0x89,0x88,0x88,0xbb});
  else check(critical.size()==4&&word(critical[2],10)==0);
  hit.kind=Kind::armor_ignoring;bool rejected=false;try{(void)damage_packets_2022(25,hit,true);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 }
 std::cout<<"PASS critical17 captured bytes,attribution,raw overkill,absolute Health,ordinary unchanged and armor-ignore rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
