#include "gw_ward_foes_route.h"
#include "gw_movement_effects.h"
#include <array>
#include <iostream>
#include <source_location>
#include <string>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Ward177 route line "+std::to_string(at.line()));}
bool near(double a,double b,double epsilon=1e-7){return std::abs(a-b)<epsilon;}
int main()try{
 MovementEffects composed;composed.apply({1,2,210,0,Category::hex,100,8100});
 check(near(composed.at(100),.34f)&&near(composed.at_with_external(100,.5f),.34f));
 composed.apply({2,2,1043,0,Category::stance,100,3100});
 check(near(composed.at(100),.51f)&&near(composed.at_with_external(100,.5f),.34f));
 const Scope caster{1,1,1};const auto make=[&](float x=0,unsigned effect=1){return *ward_against_foes(caster,effect,0,0,x,0,100);};
 const auto ward=make();const std::array fields{ward};
 WardFoeRecipient recipient{{1,2,2},0,-500,0,true,false,true};
 const std::array straight{WardFoeRouteLeg{-500,0,500,0,0}};
 const WardFoeRouteSpeed standard=[](double,float factor)->std::optional<double>{return 1000.*factor;};
 auto travel=[&](auto candidate,auto wards,auto route,double from,double through,auto boundaries,const WardFoeRouteSpeed& speed=WardFoeRouteSpeed{}){
  return ward_foes_route_travel(candidate,wards,route,from,through,boundaries,speed?speed:standard);};
 const std::array<double,0> none{};
 auto result=travel(recipient,fields,straight,100,1744,none);check(result&&result->reached&&near(result->distance,1000)&&near(result->elapsed_ms,1644)&&near(result->x,500));
 result=travel(recipient,fields,straight,100,1000,none);check(result&&!result->reached&&near(result->distance,539)&&near(result->x,39));
 // Sparse/dense calls produce the same path length. The caller carries the
 // endpoint forward; no actor state or residual timer is hidden in the helper.
 double travelled=0;auto moving=recipient;
 for(double from=100;from<1744;){const auto through=std::min(1744.,from+100);
  const std::array remainder{WardFoeRouteLeg{moving.x,moving.y,500,0,0}};
  const auto slice=travel(moving,fields,remainder,from,through,none);check(bool(slice));
  travelled+=slice->distance;moving.x=float(slice->x);moving.y=float(slice->y);from=through;}
 check(near(travelled,1000)&&near(moving.x,500));
 // Union of circles, not a second multiplicative same-skill snare.
 for(bool duplicate:{false,true}){const std::array overlap{ward,make(duplicate?0:400,2)};
  const std::array path{WardFoeRouteLeg{-500,0,duplicate?500.f:1000.f,0,0}};
  result=travel(recipient,overlap,path,100,5000,none);check(result&&result->reached&&near(result->elapsed_ms,duplicate?1644:2544));}
 {const std::array disjoint{ward,make(1000,2)};const std::array path{WardFoeRouteLeg{-500,0,1500,0,0}};
  result=travel(recipient,disjoint,path,100,5000,none);check(result&&result->reached&&near(result->elapsed_ms,3288));}
 // Corners and explicit plane changes belong to the supplied route.
 {const std::array corner{WardFoeRouteLeg{-500,0,0,0,0},WardFoeRouteLeg{0,0,0,500,0}};
  result=travel(recipient,fields,corner,100,2000,none);check(result&&result->reached&&near(result->elapsed_ms,1644)&&near(result->y,500));
  auto other_plane=corner;other_plane[1].plane=1;result=travel(recipient,fields,other_plane,100,2000,none);
  check(result&&result->reached&&result->plane==1&&near(result->elapsed_ms,1322));}
 {auto tangent=recipient;tangent.y=322;const std::array path{WardFoeRouteLeg{-500,322,500,322,0}};
  result=travel(tangent,fields,path,100,2000,none);check(result&&result->reached&&near(result->elapsed_ms,1000));}
 // Creation and expiry may happen before entry or in the middle of a leg.
 {auto inside=recipient;inside.x=0;const std::array path{WardFoeRouteLeg{0,0,1000,0,0}};
  result=travel(inside,fields,path,0,500,none);check(result&&!result->reached&&near(result->distance,300));
  result=travel(inside,fields,path,8000,8500,none);check(result&&!result->reached&&near(result->distance,450));
  result=travel(inside,fields,path,8100,9100,none);check(result&&result->reached&&near(result->distance,1000));
  result=travel(recipient,fields,straight,8000,9000,none);check(result&&result->reached&&near(result->elapsed_ms,1000));}
 // Equal-time expiry/creation does not create a speed gap in the union.
 {auto replacement=*ward_against_foes(caster,2,0,0,0,0,8100);const std::array joined{ward,replacement};
  auto inside=recipient;inside.x=0;const std::array path{WardFoeRouteLeg{0,0,1000,0,0}};
  result=travel(inside,joined,path,8000,8500,none);check(result&&!result->reached&&near(result->distance,250));}
 // The speed callback owns time-varying modifiers, caps/floors and pauses.
 {auto inside=recipient;inside.x=0;const std::array path{WardFoeRouteLeg{0,0,1000,0,0}};const std::array<double,4> pauses{600,400,600,400};
  const WardFoeRouteSpeed pause=[](double at,float factor)->std::optional<double>{return at>=400&&at<600?0.:1000.*factor;};
  result=travel(inside,fields,path,100,2000,pauses,pause);check(result&&result->reached&&near(result->elapsed_ms,1522));
  const WardFoeRouteSpeed strongest_floor=[](double,float factor)->std::optional<double>{return 1000.*std::clamp(.34*factor,.34,1.34);};
  const std::array short_path{WardFoeRouteLeg{0,0,340,0,0}};
  result=travel(inside,fields,short_path,100,1200,none,strongest_floor);check(result&&result->reached&&near(result->elapsed_ms,1000));
  const WardFoeRouteSpeed haste_cap=[](double,float factor)->std::optional<double>{return 1000.*std::clamp(2.*factor,.5,1.34);};
  result=travel(inside,fields,path,100,2000,none,haste_cap);check(result&&result->reached&&near(result->elapsed_ms,322.+678./1.34));
 }
 for(unsigned mode=0;mode<5;++mode){auto excluded=recipient;
  if(mode==0)excluded.authoritative_foe=false;if(mode==1)excluded.spirit=true;if(mode==2)excluded.living=false;
  if(mode==3)excluded.scope.instance=2;
  auto wrong_plane=ward;if(mode==4)wrong_plane.plane=1;const std::array fields_out{wrong_plane};
  result=travel(excluded,fields_out,straight,100,1100,none);check(result&&result->reached&&near(result->elapsed_ms,1000));}
 // Deterministic fail-closed contract, including malformed input and rates.
 check(!ward_foes_route_travel(recipient,fields,straight,100,200,none,{}));
 for(double bad:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),1000001.}){
  const WardFoeRouteSpeed invalid=[&](double,float)->std::optional<double>{return bad;};check(!travel(recipient,fields,straight,100,200,none,invalid));}
 const WardFoeRouteSpeed unknown=[](double,float)->std::optional<double>{return {};};check(!travel(recipient,fields,straight,100,200,none,unknown));
 auto invalid_scope=recipient;invalid_scope.scope.generation=0;check(!travel(invalid_scope,fields,straight,100,200,none));
 auto invalid_plane=straight;invalid_plane[0].plane=1;check(!travel(recipient,fields,invalid_plane,100,200,none));
 auto invalid_point=straight;invalid_point[0].end_x=std::numeric_limits<float>::infinity();check(!travel(recipient,fields,invalid_point,100,200,none));
 auto disconnected=straight;disconnected[0].x=-499;check(!travel(recipient,fields,disconnected,100,200,none));
 auto forged=ward;++forged.expires;const std::array forged_fields{forged};check(!travel(recipient,forged_fields,straight,100,200,none));
 check(!travel(recipient,fields,straight,200,100,none));
 const std::array<double,1> invalid_boundary{std::numeric_limits<double>::quiet_NaN()};check(!travel(recipient,fields,straight,100,200,invalid_boundary));
 const std::vector<WardAgainstFoes> excessive(4097,ward);check(!travel(recipient,excessive,straight,100,200,none));
 const WardFoeRouteSpeed stopped=[](double,float)->std::optional<double>{return 0.;};
 result=travel(recipient,fields,straight,100,200,none,stopped);check(result&&!result->reached&&result->distance==0&&result->elapsed_ms==100);
 result=travel(recipient,fields,straight,100,100,none);check(result&&!result->reached&&result->distance==0);
 std::cout<<"Ward177 detached route union, segment/plane edges, creation/expiry, sparse/dense and callback composition passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
