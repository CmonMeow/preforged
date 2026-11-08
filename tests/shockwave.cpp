#include "gw_shockwave.h"
#include <stdexcept>
#include <cmath>
void check(bool b){if(!b)throw std::runtime_error("Shockwave regression failed");}
int main(){using namespace fw::gw;using namespace skills::effects;const auto plan=*caster_earth_aoe(937,12);ShockwaveTarget t{{42,7,combat::Health{480,480},80,0,true},{}};
 const auto close=shockwave_target(plan,t,100,20,1,100,true);check(close.accepted&&close.hits.size()==3&&close.applied_conditions.size()==3);check(std::abs(close.hits[0].damage-51/std::sqrt(2.))<1e-9&&close.hits[1].damage==close.hits[0].damage&&close.hits[2].damage==51);check(close.after.conditions.has(combat::conditions::Kind::blind,8099)&&!close.after.conditions.has(combat::conditions::Kind::blind,8100));check(t.actor.health->current==480&&!t.conditions.count(100));
 auto mid=shockwave_target(plan,t,200,20,1,100,true);check(mid.accepted&&mid.hits.size()==2&&mid.applied_conditions.size()==2);auto edge=shockwave_target(plan,t,300,20,1,100,true);check(edge.accepted&&edge.hits.size()==1&&edge.applied_conditions.size()==1);check(shockwave_target(plan,t,323,20,1,100,true).hits.empty());check(shockwave_target(plan,t,100,20,1,100,false).hits.empty());
 auto vulnerable=t;vulnerable.actor.incoming_damage_multiplier=2;
 const auto amplified=shockwave_target(plan,vulnerable,100,20,1,100,true);
 check(amplified.accepted&&amplified.hits.size()==3);
 for(unsigned i=0;i<3;++i)check(std::abs(amplified.hits[i].damage-2*close.hits[i].damage)<1e-9);
 check(std::abs(amplified.after.actor.health->current-(480-2*(480-close.after.actor.health->current)))<1e-9);
 auto unknown=t;unknown.actor.earth_armor.reset();check(!shockwave_target(plan,unknown,100,20,1,100,true).accepted);unknown=t;unknown.actor.health.reset();check(!shockwave_target(plan,unknown,100,20,1,100,true).accepted);auto lethal=t;lethal.actor.health->current=10;auto dead=shockwave_target(plan,lethal,100,20,1,100,true);check(dead.accepted&&dead.hits.size()==1&&dead.hits[0].killed&&dead.applied_conditions.empty());auto forged=plan;forged.strikes[0].earth_damage=10000;check(!shockwave_target(forged,t,100,20,1,100,true).accepted);
 // A reaction after preflight adds Burning and extends Weakness.
 auto changed=t.conditions;check(changed.apply(combat::conditions::Kind::burning,2,100,20000));
 check(changed.apply(combat::conditions::Kind::weakness,2,100,30000));
 const auto merged=merge_earth_conditions(plan,close,changed,1,100);check(bool(merged));
 check(merged->has(combat::conditions::Kind::burning,20099));
 check(merged->lookup(combat::conditions::Kind::weakness)->expires==30100);
 check(merged->has(combat::conditions::Kind::blind,100));
 auto duplicate=close;duplicate.applied_conditions.push_back(duplicate.applied_conditions.front());
 check(!merge_earth_conditions(plan,duplicate,changed,1,100));
}
