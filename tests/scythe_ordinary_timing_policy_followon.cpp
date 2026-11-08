// NEW unregistered repeat-period and owned deadline policy regression.
#include "gw_ordinary_weapon_timing.h"
#include "gw_faintheartedness.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace combat::weapon;
void check(bool ok){if(!ok)throw std::runtime_error("Scythe ordinary period policy");}
int main(){
 const Owned scythe{12,1,{20,20,2},12,false,false,35};const auto policy=ordinary_weapon_timing_policy(scythe);
 check(policy.basis==OrdinaryWeaponTimingPolicy::Basis::scythe&&policy.base.period_ms==1500&&policy.base.windup_ms==500&&policy.base.range==128&&!policy.hammer);
 auto hammer=scythe;hammer.item_type=15;const auto hammer_policy=ordinary_weapon_timing_policy(hammer);check(hammer_policy.base.period_ms==1750&&hammer_policy.hammer);
 auto sword=scythe;sword.item_type=27;check(ordinary_weapon_timing_policy(sword).base.period_ms==1330);
 auto bow=scythe;bow.item_type=5;const auto recurve=ordinary_weapon_timing_policy(bow);check(recurve.base.period_ms==2475&&recurve.base.range==1200&&!recurve.hammer);
 auto staff=scythe;staff.item_type=26;staff.caster_weapon=true;const auto caster=ordinary_weapon_timing_policy(staff);check(caster.base.period_ms==1750&&caster.base.range==1200&&!caster.hammer);
 const skills::effects::Scope source{1,1,1},target{1,2,1};const Snapshot facts{source,target,12,1,true,true,true,true,80};
 for(unsigned mask=0;mask<4;++mask){std::vector<skills::effects::Entry> effects;
  if(mask&1)effects.push_back({1,source.actor,346,0,skills::effects::Category::stance,100,8100});
  if(mask&2)effects.push_back({2,target.actor,135,12,skills::effects::Category::hex,100,13100});
  const auto timing=skills::effects::reviewed_attack_timing(policy.base,effects,1000,false,policy.hammer);check(bool(timing));
  const auto expected_period=mask==0?1500u:mask==1?1005u:mask==2?2250u:1508u;
  const auto expected_windup=mask==0?500u:mask==1?335u:mask==2?750u:503u;
  check(timing->period_ms==expected_period&&timing->windup_ms==expected_windup);
  Intent action;check(action.start(source,target,scythe,*timing,facts,1000));const auto first=1000+expected_windup;check(action.release_deadline()==first);
  check(!action.tick(facts,first-1));check(action.tick(facts,first).has_value());check(action.release_deadline()==first+expected_period);
  check(!action.tick(facts,first+expected_period-1));check(action.tick(facts,first+expected_period).has_value());
  action.cancel(Cancel::explicit_input);check(!action.release_deadline());
 }
 // A shared policy must not classify Scythe as Hammer merely to get another
 // base interval: Dwarven Battle Stance remains Hammer-only haste.
 const auto dwarven_plan=skills::effects::dwarven_battle_stance(12);check(bool(dwarven_plan));
 const std::vector<skills::effects::Entry> dwarven{{1,source.actor,375,12,skills::effects::Category::stance,100,100+dwarven_plan->duration}};
 const auto not_hammer=skills::effects::reviewed_attack_timing(policy.base,dwarven,1000,false,policy.hammer);
 const auto is_hammer=skills::effects::reviewed_attack_timing(hammer_policy.base,dwarven,1000,false,hammer_policy.hammer);
 check(not_hammer&&not_hammer->period_ms==1500&&is_hammer&&is_hammer->period_ms==1173);
 std::cout<<"PASS Scythe1500period/unchanged500windup, IAS/slow, same owned exact deadlines, Hammer isolation\n";
}
