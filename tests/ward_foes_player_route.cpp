#include "gw_ward_foes_player_route.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
void check(bool value){if(!value)throw std::runtime_error("Ward177 player route");}
bool near(double a,double b){return std::abs(a-b)<.001;}
int main()try{
 const auto ward=*ward_against_foes({1,1,1},1,0,0,0,0,100);
 const std::array wards{ward};
 const WardFoeRecipient player{{1,2,1},0,-500,0,true,false,true};
 const std::array route{WardFoeRouteLeg{-500,0,500,0,0}};
 MovementEffects effects;
 auto click=ward_foes_player_route_travel(player,wards,route,100,8100,effects);
 check(click&&click->reached&&near(click->elapsed_ms,1644./288.*1000.));
 // Forward keyboard and click obey the same spatial and temporal slowdown.
 const auto forward=ward_foes_player_route_travel(player,wards,route,100,8100,effects,1);
 check(forward&&near(forward->elapsed_ms,click->elapsed_ms));
 const auto backward=ward_foes_player_route_travel(player,wards,route,100,8100,effects,4);
 check(backward&&!backward->reached&&backward->distance<1000);
 // Movement floor composes with the environmental factor before clamping.
 effects.apply({2,2,210,0,Category::hex,100,8100});
 const WardFoeRecipient inside{{1,2,1},0,0,0,true,false,true};
 const std::array short_route{WardFoeRouteLeg{0,0,100,0,0}};
 const auto snared=ward_foes_player_route_travel(inside,wards,short_route,100,1100,effects);
 check(snared&&!snared->reached&&near(snared->distance,288.*.34f));
 check(!ward_foes_player_route_travel(player,wards,route,100,200,effects,9));
 check(!ward_foes_player_route_travel(player,wards,route,200,100,effects));
 std::cout<<"Ward177 player route passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
