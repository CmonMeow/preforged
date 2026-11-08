#include "gw_ward_harm_exposure.h"
#include "gw_dolyak_signet.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ward Harm policy line "+std::to_string(at.line()));
}
combat::Packet bytes(std::string_view s){combat::Packet out;for(std::size_t i=0;i<s.size();i+=2)out.push_back(std::uint8_t(std::stoul(std::string(s.substr(i,2)),nullptr,16)));return out;}
double exposure(const std::vector<WardRegenerationBand>& bands){double result=0;for(const auto& b:bands)result+=(b.through-b.from)*b.pips;return result;}
int main()try{unsigned cases=0;
 const unsigned durations[]{5,6,6,7,8,8,9,10,10,11,12,12,13,14,14,15,16,16,17,18,18};
 const unsigned armor[]{12,13,14,14,15,16,17,18,18,19,20,21,22,22,23,24,25,26,26,27,28};
 const unsigned regen[]{1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,4,4};
 GroundAreaRecipient recipient{{1,30,1},2,0,0,0,true,false};
 for(unsigned skill:{239u,2806u})for(unsigned rank=0;rank<=20;++rank){
  const auto plan=ward_harm_plan(skill,rank);check(plan&&plan->duration==durations[rank]*1000&&plan->armor==armor[rank]&&
   plan->elemental_armor==armor[rank]&&plan->regeneration==regen[rank]&&plan->recharge==(skill==239?20000:30000));
  const auto ward=ward_against_harm({1,25,1},80,2,0,0,0,*plan,100);check(bool(ward));const std::array wards{*ward};
  const auto bonus=ward_harm_benefits(wards,recipient,100);check(bonus==WardHarmBenefits{armor[rank],armor[rank],regen[rank]});
  for(auto kind:{combat::Kind::earth,combat::Kind::fire,combat::Kind::cold,combat::Kind::lightning}){
   check(ward_harm_skill_armor(bonus,kind)==2*armor[rank]);check(ward_harm_skill_armor(bonus,kind,true)==0);
  }
  for(auto kind:{combat::Kind::piercing,combat::Kind::slashing,combat::Kind::holy})check(ward_harm_skill_armor(bonus,kind)==armor[rank]);
  check(ward_harm_skill_armor(bonus,combat::Kind::armor_ignoring)==0);
  check(stacked_skill_armor(20,*ward_harm_skill_armor(bonus,combat::Kind::fire))==std::max(25u,2*armor[rank]));
  check(ward_harm_benefits(wards,recipient,99)==WardHarmBenefits{}&&ward_harm_benefits(wards,recipient,ward->area.expires)==WardHarmBenefits{});
  auto r=recipient;r.x=322;check(ward_harm_benefits(wards,r,100)==bonus);r.x=std::nextafter(322.f,1000.f);check(ward_harm_benefits(wards,r,100)==WardHarmBenefits{});
  r=recipient;r.spirit=true;check(ward_harm_benefits(wards,r,100)==WardHarmBenefits{});r=recipient;r.living=false;check(ward_harm_benefits(wards,r,100)==WardHarmBenefits{});
  r=recipient;r.team=3;check(ward_harm_benefits(wards,r,100)==WardHarmBenefits{});r=recipient;r.plane=1;check(ward_harm_benefits(wards,r,100)==WardHarmBenefits{});
  r=recipient;r.scope.instance=2;check(ward_harm_benefits(wards,r,100)==WardHarmBenefits{});r=recipient;r.scope={1,25,2};check(ward_harm_benefits(wards,r,100)==bonus);
  auto forged=*ward;++forged.area.expires;check(!valid_ward_harm(forged));forged=*ward;++forged.area.radius;check(!valid_ward_harm(forged));
  auto modified=*plan;++modified.armor;check(!ward_against_harm({1,25,1},80,2,0,0,0,modified,100));++cases;
 }
 check(!ward_harm_plan(239,21)&&!ward_harm_plan(938,0));
 check(!ward_harm_skill_armor({UINT32_MAX,UINT32_MAX,1},combat::Kind::fire));
 check(ward_harm_skill_armor({},combat::Kind::fire)==0);
 const auto low=*ward_against_harm({1,25,1},80,2,0,0,0,*ward_harm_plan(239,0),100);
 const auto high=*ward_against_harm({1,25,1},81,2,0,0,0,*ward_harm_plan(2806,15),100);
 auto late=*ward_against_harm({1,25,1},82,2,0,0,0,*ward_harm_plan(239,0),14000);
 const std::array overlap{high,late};
 {const TimedGroundAreaLeg leg{recipient,0,0,0,20000};const auto bands=ward_harm_regeneration_bands(overlap,std::span(&leg,1));
  check(bands&&std::abs(exposure(*bands)-48900)<1e-8&&bands->size()==4);++cases;}
 {const std::array wards{low};const TimedGroundAreaLeg still{recipient,0,0,0,6000};auto bands=ward_harm_regeneration_bands(wards,std::span(&still,1));
  check(bands&&exposure(*bands)==5000);++cases;
  auto from=recipient;from.x=-600;const TimedGroundAreaLeg sparse{from,600,0,100,1300};bands=ward_harm_regeneration_bands(wards,std::span(&sparse,1));check(bands&&std::abs(exposure(*bands)-644)<1e-8);
  std::vector<TimedGroundAreaLeg> dense;for(unsigned i=0;i<12;++i){auto r=from;r.x=-600+float(i)*100;dense.push_back({r,r.x+100,0,double(100+i*100),double(200+i*100)});}
  auto sampled=ward_harm_regeneration_bands(wards,dense);check(sampled&&std::abs(exposure(*sampled)-exposure(*bands))<1e-8);++cases;
  auto clipped=sparse;clipped.from=4500;clipped.through=5700;bands=ward_harm_regeneration_bands(wards,std::span(&clipped,1));check(bands&&std::abs(exposure(*bands)-322)<1e-8);++cases;
  auto delayed=low;delayed=*ward_against_harm({1,25,1},80,2,0,0,0,*ward_harm_plan(239,0),600);const std::array delayed_wards{delayed};
  bands=ward_harm_regeneration_bands(delayed_wards,std::span(&sparse,1));check(bands&&std::abs(exposure(*bands)-422)<1e-8);++cases;
  auto tangent=sparse;tangent.recipient.y=tangent.end_y=322;bands=ward_harm_regeneration_bands(wards,std::span(&tangent,1));check(bands&&exposure(*bands)==0);++cases;
  tangent.recipient.y=tangent.end_y=std::nextafter(322.f,0.f);bands=ward_harm_regeneration_bands(wards,std::span(&tangent,1));check(bands&&exposure(*bands)>0);++cases;
  for(unsigned mode=0;mode<4;++mode){auto excluded=still;if(mode==0)excluded.recipient.spirit=true;if(mode==1)excluded.recipient.team=3;if(mode==2)excluded.recipient.plane=1;if(mode==3)excluded.recipient.living=false;
   bands=ward_harm_regeneration_bands(wards,std::span(&excluded,1));check(bands&&exposure(*bands)==0);++cases;}
  auto discontinuous=dense;discontinuous[1].recipient.x++;check(!ward_harm_regeneration_bands(wards,discontinuous));
  discontinuous=dense;++discontinuous[1].recipient.scope.generation;check(!ward_harm_regeneration_bands(wards,discontinuous));
  discontinuous=dense;discontinuous[1].from++;check(!ward_harm_regeneration_bands(wards,discontinuous));++cases;
  auto invalid=low;++invalid.area.expires;const std::array bad{invalid};check(!ward_harm_regeneration_bands(bad,std::span(&still,1)));++cases;
  std::vector<WardAgainstHarm> too_many(4097,low);check(!ward_harm_regeneration_bands(too_many,std::span(&still,1)));++cases;
 }
 {const std::array wards{low};auto r=recipient;
  std::vector<TimedGroundAreaLeg> phases{{r,0,0,0,2000}};r.plane=1;phases.push_back({r,0,0,2000,4000});
  auto bands=ward_harm_regeneration_bands(wards,phases);check(bands&&exposure(*bands)==1900);++cases;
  phases.clear();r=recipient;phases.push_back({r,600,0,1000,1000});r.x=600;phases.push_back({r,600,0,1000,2000});
  bands=ward_harm_regeneration_bands(wards,phases);check(bands&&exposure(*bands)==0);++cases;
  const TimedGroundAreaLeg still{recipient,0,0,0,6000};
  const auto ancient=*ward_against_harm({1,25,1},80,2,0,0,0,*ward_harm_plan(239,0),9007199254740980ull);
  const std::array late_wards{ancient};check(!ward_harm_regeneration_bands(late_wards,std::span(&still,1)));++cases;
  std::vector<WardAgainstHarm> many(600,low);std::vector<TimedGroundAreaLeg> long_route;
  for(unsigned i=0;i<600;++i)long_route.push_back({recipient,0,0,double(i*100),double((i+1)*100)});
  check(!ward_harm_regeneration_bands(many,long_route));check(low.area.expires==5100);++cases;
 }
 check(!stationary_ground_area({1,25,0},80,2,0,0,0,322,1000,100));
 check(!stationary_ground_area({1,25,1},0,2,0,0,0,322,1000,100));
 check(!stationary_ground_area({1,25,1},80,0,0,0,0,322,1000,100));
 check(!stationary_ground_area({1,25,1},80,2,65536,0,0,322,1000,100));
 check(!stationary_ground_area({1,25,1},80,2,0,0,0,0,1000,100));
 check(!stationary_ground_area({1,25,1},80,2,0,0,0,322,0,100));
 check(!stationary_ground_area({1,25,1},80,2,0,0,0,322,1000,UINT64_MAX-999));++cases;
 const float x=std::bit_cast<float>(0xc5b372a3u),y=std::bit_cast<float>(0x44e92324u);
 const auto wire=skills::particles::ward_harm_ground_2022(25,x,y,0);check(wire.size()==2);
 check(wire[0]==bytes("a080a372b3c52423e94400000000000064010000"));
 check(wire[1]==bytes("a080a372b3c52423e94400001900000084030000"));
 check(combat::detail::integer(21,25,420)==bytes("9e001500000019000000a4010000"));++cases;
 std::cout<<cases<<" Ward239/2806 all-rank policy, single-source armor/cap, shared area, exact exposure boundaries/dense-sparse and captured420/356/900 forms pass; World/Runtime consumers absent\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
