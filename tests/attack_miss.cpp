#include "gw_weapon_attack_skills.h"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::weapon;
void check(bool ok){if(!ok)throw std::runtime_error("attack miss policy regression");}
int main(){
 check(combined_miss_chance({})==0);
 const std::array pair{.5,.25},blind{.9,.25},certain{1.,.25},none{0.,0.};
 check(combined_miss_chance(pair)==.625);
 check(std::abs(*combined_miss_chance(blind)-.925)<1e-14);
 check(combined_miss_chance(certain)==1&&combined_miss_chance(none)==0);
 // Exhaustive equally spaced quantiles exercise the entire distribution,
 // including exact thresholds;this is not a stochastic/flaky probability test.
 unsigned plain=0,combined=0,blinded=0;
 for(unsigned n=0;n<10000;++n){const double roll=double(n)/10000;
  plain+=*attack_misses(roll,false,.25);combined+=*attack_misses(roll,true,.25);blinded+=*attack_misses(roll,true);
 }
 check(plain==2500&&combined==9250&&blinded==9000);
 check(attack_misses(.25,false,.25)==false&&attack_misses(.249999,false,.25)==true);
 for(double value:std::array<double,4>{-1.,1.,INFINITY,NAN})check(!attack_misses(value,false));
 for(double value:std::array<double,4>{-1.,1.1,INFINITY,NAN})check(!attack_misses(.5,false,value));
 const std::array<double,2> bad{1.,NAN};check(!combined_miss_chance(bad)); // Do not stop validation at guaranteed miss.
 Strike strike;strike.level=20;strike.mastery=12;strike.minimum=strike.maximum=20;strike.location_armor=60;strike.additional_miss_chance=.25;
 const auto fixed=*attack_skill(394,12);
 for(bool is_blind:{false,true}){
  strike.blind=is_blind;const double threshold=is_blind?.925:.25;
  strike.miss_roll=threshold-.000001;
  check(resolve(strike)->missed&&resolve_attack_skill(strike,fixed,false)->missed);
  strike.miss_roll=threshold;
  check(!resolve(strike)->missed&&!resolve_attack_skill(strike,fixed,false)->missed);
 }
 strike.additional_miss_chance=NAN;
 check(!resolve(strike)&&!resolve_attack_skill(strike,fixed,false));
 std::cout<<"PASS multiplicative miss probabilities,complete quantile counts and invalid-draw rejection\n";
}
