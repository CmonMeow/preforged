#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mixed weapon armor line "+std::to_string(at.line())+context);
}
int main()try{
 const std::array<int,5> locations{70,60,50,40,30};
 for(unsigned mode=0;mode<5;++mode)for(unsigned bucket=0;bucket<8;++bucket){context=" mode="+std::to_string(mode)+" bucket="+std::to_string(bucket);world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Warrior",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.physical_armor(target,{})&&w.physical_armor_locations(target,mode==4?std::optional<std::array<int,5>>{}:std::optional{locations}));
  unsigned draws=0;w.configure_armor_location_draws([&]{++draws;return bucket;});w.configure_attack_miss_draws([]{return .5;});
  combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,mode==2?2u:mode>=3?5u:27u};check(w.ordinary_weapon(source,weapon,20));
  unsigned bonus=0;
  if(mode==0){check(w.ordinary_attack(source,2,100)&&draws==0);w.tick(700);}
  else {const auto plan=*combat::weapon::attack_skill(mode==1?322:mode==2?330:mode==3?393:394,mode>=3?0:12,0);bonus=mode==1?34:mode==2?10:0;check(w.melee_attack(source,mode==2?source:target,weapon,plan,100));if(mode>=3){check(draws==0);w.tick(200);}}
  const int armor=bucket<3?70:bucket<5?60:bucket==5?50:bucket==6?40:30;
  const auto expected=mode==4?975u:unsigned(std::ceil(1000-20*std::exp2((60.-armor)/40.)-bonus));
  check(draws==(mode==4?0u:1u)&&w.resources(2)->health==expected);
  if(mode==0){w.tick(700);check(draws==(mode==4?0u:1u)&&w.resources(2)->health==expected);}
 }
 for(unsigned mode=0;mode<4;++mode){context=" rejection/fallback="+std::to_string(mode);world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);unsigned draws=0;w.configure_armor_location_draws([&]{++draws;return 8u;});
  if(mode<2)check(w.physical_armor_locations(target,mode==4?std::optional<std::array<int,5>>{}:std::optional{locations}));if(mode==3)check(w.physical_armor(target,60));
  combat::weapon::Owned weapon{12,1,{20,20,mode==1?5u:2u},12,false,false,27};check(w.ordinary_weapon(source,weapon,20));
  const bool accepted=w.melee_attack(source,target,weapon,*combat::weapon::attack_skill(322,12,0),100);
  check(accepted==(mode==3));check(draws==(mode==0?1u:0u));check(w.resources(2)->health==(mode==3?946u:1000u));
 }
 std::cout<<"PASS mixed physical armor all8weighted buckets,ordinary/melee/Cyclone contacts,no admission/replay draws,scalar fallback,unknown/elemental rejection and invalid draw\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
