#include "gw_aftershock.h"
#include <stdexcept>
#include <cmath>
void check(bool value){if(!value)throw std::runtime_error("Aftershock regression failed");}
int main(){using namespace fw::gw;using namespace skills::effects;
 const auto plan=*caster_earth_aoe(174,12);ShockwaveTarget actor{{42,7,combat::Health{480,480},60,0,true},{}};
 const auto normal=aftershock_target(plan,actor,240,20,1,100,true,false);
 check(normal.accepted&&normal.hits.size()==1&&std::abs(normal.hits[0].damage-85.2)<1e-9);
 const auto knocked=aftershock_target(plan,actor,240,20,1,100,true,true);
 check(knocked.accepted&&knocked.hits.size()==2&&std::abs(knocked.hits[0].damage-85.2)<1e-9&&std::abs(knocked.hits[1].damage-56.4)<1e-9);
 auto protected_actor=actor;protected_actor.actor.incoming_reduction=5;
 const auto protected_hits=aftershock_target(plan,protected_actor,0,20,1,100,true,true);
 check(protected_hits.hits.size()==2&&std::abs(protected_hits.hits[0].damage-80.2)<1e-9&&std::abs(protected_hits.hits[1].damage-51.4)<1e-9);
 auto dying=actor;dying.actor.health->current=20;
 check(aftershock_target(plan,dying,0,20,1,100,true,true).hits.size()==1);
 check(actor.actor.health->current==480&&normal.applied_conditions.empty());
 check(aftershock_target(plan,actor,246,20,1,100,true,true).hits.empty());
 check(aftershock_target(plan,actor,240.01f,20,1,100,true,true).hits.empty());
 check(aftershock_target(plan,actor,0,20,1,100,false,true).hits.empty());
 auto unknown=actor;unknown.actor.earth_armor.reset();check(!aftershock_target(plan,unknown,0,20,1,100,true,false).accepted);
 auto forged=plan;forged.strikes[0].knocked_down_bonus=999;check(!aftershock_target(forged,actor,0,20,1,100,true,true).accepted);
 actor.actor.earth_armor=80;check(actor.conditions.apply(combat::conditions::Kind::cracked_armor,1,0,1000));
 check(std::abs(aftershock_target(plan,actor,0,20,1,100,true,false).hits[0].damage-85.2)<1e-9);
 check(aftershock_target(plan,actor,0,20,1,1000,true,false).hits[0].damage<85.2);
}
