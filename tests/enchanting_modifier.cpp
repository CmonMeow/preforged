#include "gw_weapon_passives.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Enchanting modifier line "+std::to_string(at.line()));
}
int main()try{
 using items::weapon_passives::enchanting_percent;
 check(enchanting_percent({})==0);
 for(unsigned percent=0;percent<256;++percent){const std::array<std::uint32_t,1> words{0x22b80000u|percent};
  const auto result=enchanting_percent(words);
  if(percent>=10&&percent<=20)check(result==percent);else check(!result);
 }
 for(auto raw:{0x22b80114u,0x22b00014u,0x62b80014u,0xa2b80014u}){
  const std::array words{raw};check(!enchanting_percent(words));
 }
 const std::array<std::uint32_t,4> valid{0x25300420u,0x24080000u,0x62c80a00u,0x22b80014u};
 check(enchanting_percent(valid)==20);
 const std::array<std::uint32_t,2> duplicate{0x22b80014u,0x22b8000fu};check(!enchanting_percent(duplicate));
 const std::array<std::uint32_t,2> same{0x22b80014u,0x22b80014u};check(!enchanting_percent(same));
 check(enchanting_percent(std::span(valid).first(3))==0);
 check(items::weapon_passives::energy(valid)==10); // Existing Energy decoder remains independent.
 std::cout<<"PASS enchanting native suffix range,flags,duplicate rejection and independent Energy fields\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
