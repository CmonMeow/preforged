#include "gw_controlled_spirits.h"
#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <source_location>
#include <string>
using namespace fw::gw::skills::effects;
using namespace fw::gw::skills::effects::spirits;
namespace combat=fw::gw::combat;
void check(bool yes,std::source_location at=std::source_location::current()){if(!yes)throw std::runtime_error("controlled spirit invariant line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto plan=signet_plan(rank);check(plan&&plan->lifetime==60000&&plan->count==3&&plan->nominal_attack_period==2000);}
 check(!signet_plan(21));
 {
  struct Flight{float sx,sy,dx,dy,seconds;};
  // Fixed packet/world joins from the hash-pinned map-34 six-spirit audit.
  const std::array flights{
   Flight{-22083.582031f,-234.540833f,-21252.947265625f,463.67718505859375f,.6798931956291199f},
   Flight{-22121.621094f,-226.636414f,-21720.2265625f,437.7799072265625f,.4863732159137726f},
   Flight{-22158.228516f,-239.646729f,-21714.328125f,433.07208251953125f,.5049977898597717f},
   Flight{-22628.130859f,-1905.205933f,-22173.408203125f,-2783.85400390625f,.6198877096176147f},
   Flight{-22603.613281f,-1935.342896f,-22170.341796875f,-2789.291748046875f,.5999853014945984f},
   Flight{-22567.005859f,-1948.353149f,-22170.919921875f,-2788.681884765625f,.5820786952972412f},
  };
  for(const auto& f:flights){const auto seconds=signet_attack_flight_seconds(f.sx,f.sy,f.dx,f.dy);
   check(seconds&&std::abs(*seconds-f.seconds)<.00002f);}
  check(!signet_attack_flight_seconds(0,0,0,0));
  check(!signet_attack_flight_seconds(0,0,50000,0));
  check(!signet_attack_flight_seconds(std::numeric_limits<float>::quiet_NaN(),0,1,0));
 }
 {const auto packet=attack_start_2022(42,43,3153);
  check(packet==combat::Packet({159,0,50,0,0,0,42,0,0,0,43,0,0,0,81,12,0,0}));
  for(const auto [source,target,resource]:std::array<std::array<unsigned,3>,4>{{
       {{0,43,3153}},{{42,42,3153}},{{42,43,1239}},{{65536,43,3153}}}}){
   bool rejected=false;try{(void)attack_start_2022(source,target,resource);}
   catch(const std::invalid_argument&){rejected=true;}check(rejected);
  }
 }
 {const auto x=std::bit_cast<float>(3332770277u),y=std::bit_cast<float>(1139267246u);
  const auto seconds=std::bit_cast<float>(1059982715u);
  const auto launch=attack_projectile_2022(69,x,y,0,seconds,1);
  check(launch==combat::Packet({0xa3,0x80,0x45,0,0,0,0xe5,0x09,0xa6,0xc6,
      0xae,0xd6,0xe7,0x43,0,0,0x7b,0x0d,0x2e,0x3f,2,0,0,0,1,0,0,0,0}));
  check(attack_arrival_2022(69,1)==combat::Packet({0xa6,0x80,0x45,0,0,0,1,0,0,0,7,0,0,0}));
  bool rejected=false;try{(void)attack_projectile_2022(69,x,y,0,0,1);}
  catch(const std::invalid_argument&){rejected=true;}check(rejected);
  rejected=false;try{(void)attack_arrival_2022(69,0);}
  catch(const std::invalid_argument&){rejected=true;}check(rejected);
 }
 {const auto plan=*signet_plan(20);
  combat::Actor armored{22,1,combat::Health{100,100},400,0,true};
  armored.physical_armor=400;
  const auto hit=attack_target(plan,3153,armored,true);
  check(hit.hit&&hit.hit->damage==plan.attack_damage&&
      hit.target.health->current==100-plan.attack_damage&&hit.hit->kind==combat::Kind::armor_ignoring&&hit.hit->skill==3153);
  armored.incoming_reduction=5;
  const auto protected_hit=attack_target(plan,3154,armored,true);
  check(protected_hit.hit&&protected_hit.hit->damage==plan.attack_damage-5&&
      protected_hit.target.health->current==105-plan.attack_damage);
  check(!attack_target(plan,3155,armored,false).hit);
  check(!attack_target(plan,1239,armored,true).hit);
  auto forged=plan;++forged.attack_damage;check(!attack_target(forged,3153,armored,true).hit);
  armored.health->current=0;check(!attack_target(plan,3153,armored,true).hit);
 }
 for(unsigned rank=0;rank<=20;++rank){const auto plan=summon_plan(rank);check(plan&&plan->healing==interpolate(60,100,rank));}
 check(!summon_plan(21));
 spirits::Registry registry;const Scope owner{9,10,3},other{9,11,1},spirit{9,69,7};
 const Record record{spirit,owner,4280,12,20,61000,426,3153};
 check(registry.add(record,1000));check(!registry.add(record,1000));
 check(!registry.add({{8,70,1},owner,4281,12,20,61000,426,3154},1000));
 check(!registry.add({owner,owner,4281,12,20,61000,426,3154},1000));
 check(!registry.add({{9,70,1},other,4281,12,20,1000,426,3154},1000));
 check(registry.find(spirit,61000)==std::nullopt);
 check(registry.owned(owner,1001).size()==1&&registry.owned(other,1001).empty());
 check(registry.retire_owner({9,10,4})==0&&registry.size()==1);
 unsigned expired=0;registry.advance(60999,[&](auto){++expired;});check(!expired);
 registry.advance(61000,[&](const Record& r){check(r==record);++expired;});
 check(expired==1&&registry.size()==0);
 check(registry.add({{9,69,8},owner,4281,12,20,80000,426,3154},61000));
 check(!registry.find(spirit,61000));check(registry.retire_owner(owner)==1);
 const Record a{{9,70,1},owner,4280,12,20,121000,426,3153};
 const Record b{{9,71,1},owner,4281,12,20,121000,426,3154};
 const Record c{{9,72,1},owner,4282,12,20,121000,426,3155};
 check(registry.add_group(std::array{a,b,c},61000)&&registry.size()==3);
 check(!registry.add_group(std::array{a,b},61000)&&registry.size()==3);
 auto foreign=c;foreign.spirit={9,73,1};foreign.owner=other;
 check(!registry.add_group(std::array{a,foreign},61000)&&registry.size()==3);
 check(registry.owned(owner,61000).size()==3);
 check(registry.retire_owner(other)==0&&registry.retire_owner(owner)==3);
 std::cout<<"PASS controlled spirit rank, ownership, generation, expiry; detached from World\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
