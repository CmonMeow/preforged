#include "../src/runtime/gw_companion_policy.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace fw::gw::ai::companion;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main()try{
 // Values below are test fixtures, not retail defaults.
 const Actor self{1,1,10},owner{1,1,20},a{1,1,30},b{1,1,40};
 Config cfg{100,50,200,300,200};
 Snapshot s{self,owner,true,20,{{a,true,true,true,10},{b,true,true,true,20}},Acquisition{a,Trigger::owner_attack,true},{}};
 Controller follower(Profile::friendly_follower);
 check(follower.tick(s,cfg).target==a,"follower assists owner");
 s.pending_action_target=a;s.owner_distance=201;
 auto d=follower.tick(s,cfg);check(d.intent==Intent::follow_owner&&d.cancel_pending_action&&!d.target,"recall cancels windup");
 s.owner_distance=90;check(follower.tick(s,cfg).intent==Intent::follow_owner,"follow hysteresis suppresses reacquire");
 s.owner_distance=50;check(follower.tick(s,cfg).target==a,"resume at stop threshold");
 for(auto profile:{Profile::minion,Profile::summoned_ally}){
  Controller c(profile);s.owner_distance=20;s.acquisition->target=a;check(c.tick(s,cfg).target==a,"summon acquires");
  s.owner_distance=10000;s.acquisition->target=b;check(c.tick(s,cfg).target==a,"summon keeps combat over follow/new owner target");
  s.targets[0].action_ready=false;d=c.tick(s,cfg);check(d.intent==Intent::pursue_target&&d.cancel_pending_action,"out of range pursues and cancels unreleased action");
  s.targets[0].alive=false;check(c.tick(s,cfg).target==b,"dead target allows new engagement");
  s.targets[0].alive=true;s.targets[0].action_ready=true;
 }
 Controller pet(Profile::ranger_pet);s.owner_distance=20;s.acquisition->target=a;
 check(pet.tick(s,cfg).target==a,"guard owner target");
 s.acquisition->target=b;d=pet.tick(s,cfg);check(d.target==b&&d.cancel_pending_action,"guard retarget cancels old windup");
 s.owner_distance=201;check(pet.tick(s,cfg).intent==Intent::follow_owner,"guard pet recalls from combat");
 s.owner_distance=20;check(pet.tick(s,cfg).target==b,"guard resumes near owner");
 check(pet.command({PetMode::heel,{}},s),"heel accepted");
 s.owner_distance=75;check(pet.tick(s,cfg).intent==Intent::follow_owner,"heel forces return inside normal start range");
 check(pet.command({PetMode::attack,a},s),"attack lock accepted");
 check(pet.tick(s,cfg).target==a,"attack ignores new owner target");
 s.targets[0].distance=301;d=pet.tick(s,cfg);check(pet.pet_mode()==PetMode::heel&&d.intent==Intent::follow_owner,"lock range exit restores heel");
 s.targets[0].distance=10;s.owner_distance=20;check(pet.command({PetMode::guard,{}},s),"guard accepted");
 check(pet.tick(s,cfg).target==b,"guard reengages");
 check(!pet.command({PetMode::attack,Actor{1,2,30}},s),"stale lifetime rejected");
 check(!pet.command({PetMode::attack,Actor{2,1,30}},s),"cross-instance rejected");
 s.self.generation=2;s.acquisition.reset();d=pet.tick(s,cfg);check(!d.target&&d.cancel_pending_action,"self generation clears action");
 s.owner.generation=2;check(!pet.tick(s,cfg).target,"owner generation clears lock");
 s.alive=false;check(pet.tick(s,cfg).reason==Reason::dead,"dead actor never acts");
 s.alive=true;s.owner_distance=20;Controller unknown(Profile::friendly_follower);
 check(unknown.tick(s,{}).intent==Intent::unsupported_configuration,"no invented thresholds");
 Config bad=cfg;bad.follow_stop=101;check(unknown.tick(s,bad).intent==Intent::unsupported_configuration,"invalid hysteresis rejected");
 bad=cfg;bad.follow_start=std::numeric_limits<double>::quiet_NaN();check(unknown.tick(s,bad).intent==Intent::unsupported_configuration,"NaN threshold rejected");
 s.owner_distance=std::numeric_limits<double>::infinity();check(unknown.tick(s,cfg).reason==Reason::missing_distance,"invalid geometry rejected");
 s.owner_distance=20;s.acquisition=Acquisition{a,Trigger::proximity,true};
 Controller guarded(Profile::ranger_pet);check(!guarded.tick(s,cfg).target,"guard does not acquire arbitrary proximity foe");
 Controller minion(Profile::minion);check(minion.tick(s,cfg).target==a,"minion certified proximity acquisition");
 s.targets[0].actor.generation=2;d=minion.tick(s,cfg);check(!d.target&&d.cancel_pending_action,"target reused ID cannot inherit engagement");
 s.targets[0].actor=a;s.acquisition->permitted=false;check(!minion.tick(s,cfg).target,"uncertified trigger cannot acquire");
 s.acquisition->permitted=true;s.targets.push_back(s.targets[0]);check(!minion.tick(s,cfg).target,"ambiguous duplicate actor rejected");
 s.targets.pop_back();s.acquisition->trigger=Trigger::owner_attack;s.pending_action_target=a;
 Controller rebinding(Profile::minion);check(rebinding.tick(s,cfg).target==a,"initial binding acquires");
 s.self.generation++;d=rebinding.tick(s,cfg);check(d.target==a&&d.cancel_pending_action,"new source lifetime cannot inherit same-target windup");
 Controller locked(Profile::ranger_pet);s.owner_distance=20;locked.tick(s,cfg);
 check(locked.command({PetMode::attack,a},s),"lock for missing config test");
 Config no_lock=cfg;no_lock.pet_attack_lock_range.reset();d=locked.tick(s,no_lock);
 check(d.intent==Intent::unsupported_configuration&&d.cancel_pending_action,"missing lock range fails closed");
 s.targets[0].alive=false;s.acquisition.reset();d=locked.tick(s,cfg);
 check(locked.pet_mode()==PetMode::guard&&!d.target,"dead lock restores original guard");
 std::cout<<"PASS companion profile, command, recall, lifetime and configuration regressions\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
