#pragma once
// Pinned Ward176 lifetime, eligibility and attributed environmental block source.
#include "gw_block_attribution.h"
#include "gw_skill_effect_plan.h"
#include <cmath>
#include <limits>

namespace fw::gw::skills::effects {
struct WardAgainstMelee {
 Scope caster;unsigned effect=0,rank=0,plane=0;float x=0,y=0;
 Time created=0,expires=0;
 static constexpr float radius=322.f;
 static constexpr double block_probability=.5;
};
struct WardMeleeRecipient {
 Scope scope;unsigned plane=0;float x=0,y=0;
 bool authoritative_ally=false,spirit=false,living=false;
};
inline bool valid_ward_melee_scope(Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
inline std::optional<WardAgainstMelee> ward_against_melee(Scope caster,unsigned effect,
 unsigned rank,unsigned plane,float x,float y,Time now){
 const auto* row=find_skill(176);
 if(!row||row->type!=11||row->profession!=6||row->attribute!=9||row->special||row->energy_cost()!=15||row->health_sacrifice||
    row->activation!=1.f||row->aftercast!=.75f||row->recharge!=30||row->duration0!=5||row->duration15!=20||
    row->scale0!=50||row->scale15!=50||row->bonus0||row->bonus15||row->caster_body_resource!=315||row->target_body_resource!=2077||
    !valid_ward_melee_scope(caster)||!effect||rank>20||!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1000000||std::abs(y)>1000000)return {};
 const auto duration=Time(interpolate(5,20,rank))*1000;
 if(now>std::numeric_limits<Time>::max()-duration)return {};
 return WardAgainstMelee{caster,effect,rank,plane,x,y,now,now+duration};
}
struct WardMeleePlan {unsigned rank=0;Time duration=0;auto operator<=>(const WardMeleePlan&)const=default;};
inline std::optional<WardMeleePlan> ward_melee_plan(unsigned rank){
 const auto ward=ward_against_melee({1,1,1},1,rank,0,0,0,0);
 if(!ward)return {};return WardMeleePlan{rank,ward->expires};
}
inline bool valid_ward_against_melee(const WardAgainstMelee&ward){
 const auto canonical=ward_against_melee(ward.caster,ward.effect,ward.rank,ward.plane,ward.x,ward.y,ward.created);
 return canonical&&canonical->expires==ward.expires;
}
inline bool ward_melee_covers(const WardAgainstMelee&ward,const WardMeleeRecipient&recipient,Time now){
 //322 is the archived in-the-area convention. Same-plane admission is an
 // explicit private navigation barrier, not recovered retail vertical range.
 if(!valid_ward_against_melee(ward)||!valid_ward_melee_scope(recipient.scope)||ward.caster.instance!=recipient.scope.instance||
    ward.plane!=recipient.plane||!recipient.authoritative_ally||recipient.spirit||!recipient.living||
    !std::isfinite(recipient.x)||!std::isfinite(recipient.y)||now<ward.created||now>=ward.expires)return false;
 const double dx=double(recipient.x)-ward.x,dy=double(recipient.y)-ward.y;
 return dx*dx+dy*dy<=double(WardAgainstMelee::radius)*WardAgainstMelee::radius;
}
// Caller selects one authoritative active Ward176 source and owns its lifetime,
// ally classification and effect-ID namespace. No stack/order policy is inferred
// for overlapping same-skill wards. Integrate the returned source with existing
// block authorities so unblockable/Rigor and prior misses retain their meaning.
inline std::optional<BlockSource> ward_melee_block_source(const WardAgainstMelee&ward,
 const WardMeleeRecipient&recipient,combat::weapon::AttackDelivery delivery,Time now){
 if(delivery!=combat::weapon::AttackDelivery::melee||!ward_melee_covers(ward,recipient,now))return {};
 return BlockSource{176,ward.effect,WardAgainstMelee::block_probability,ward.caster};
}
}
