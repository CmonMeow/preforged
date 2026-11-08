#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include "gw_conditions.h"
#include <compare>

namespace fw::gw::skills::effects {
struct DancingDaggersPlan {
 unsigned rank=0,projectiles=3,launch_model=854,impact_resource=855;
 double damage=5;
 float range=624;
 auto operator<=>(const DancingDaggersPlan&)const=default;
};
inline std::optional<DancingDaggersPlan> dancing_daggers(unsigned rank){
 const auto* row=find_skill(858);
 if(rank>20||!row||row->type!=5||row->attribute!=30||row->encoded_energy!=5||
    row->activation!=1.f||row->aftercast!=.75f||row->recharge!=5||
    row->scale0!=5||row->scale15!=35||row->bonus0!=1800||row->bonus15!=1800||
    row->caster_body_resource!=2077)return {};
 return DancingDaggersPlan{rank,3,854,855,damage_scale(5,35,rank),624};
}
struct DancingDaggerHit {bool accepted=false;combat::Actor actor;combat::Hit hit;};
// A single impact. The launcher must enforce range, collision, one hit per
// missile, target lifetime and the separate lead-attack chain transition.
inline DancingDaggerHit dancing_dagger_hit(const DancingDaggersPlan& plan,combat::Actor actor,
 const combat::conditions::State& conditions,unsigned level,std::uint64_t now){
 const auto expected=dancing_daggers(plan.rank);
 if(!expected||plan!=*expected||!actor.id||!actor.generation||!actor.earth_armor||!level||level>255)return {};
 actor.earth_armor=conditions.armor(*actor.earth_armor,now);
 const auto hit=combat::apply(actor,{combat::Kind::earth,plan.damage,level,858},true);
 if(hit.reason!=combat::Reason::accepted)return {};
 return {true,actor,hit};
}
}
