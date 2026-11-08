#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"

namespace fw::gw::skills::effects {
// Impact-only state. The World owns flight, movement evidence, lifetime checks
// and reactions between hits. Never cache a whole target Actor across strikes.
class LightningBoltImpact {
    unsigned rank_=0,remaining_=0;
    LightningBoltImpact(unsigned rank,unsigned strikes):rank_(rank),remaining_(strikes){}
public:
    static std::optional<LightningBoltImpact> begin(unsigned rank,std::optional<bool> moving){
        const auto* row=find_skill(1369);
        if(rank>20||!moving||!row||row->type!=5||row->profession!=6||row->attribute!=8||
           row->energy_cost()!=5||row->activation!=1.f||row->aftercast!=.75f||row->recharge!=8||
           row->scale0!=5||row->scale15!=50||row->caster_body_resource!=1180||row->target_body_resource!=2077)return {};
        return LightningBoltImpact(rank,*moving?2:1);
    }
    unsigned remaining()const{return remaining_;}
    combat::Hit strike(combat::Actor& current,unsigned level,bool hostile,const combat::DamageProjection& projection={}){
        if(!remaining_)return {};
        if(!projection||!hostile){
            const auto hit=combat::apply(current,{combat::Kind::lightning,damage_scale(5,50,rank_),level,1369,25},hostile);
            if(hit.reason==combat::Reason::accepted){--remaining_;if(hit.killed)remaining_=0;}
            return hit;
        }
        const auto step=combat::project_damage(current,{combat::Kind::lightning,damage_scale(5,50,rank_),level,1369,25},projection);
        if(!step)return {};
        --remaining_;if(current.health->current<=0)remaining_=0;
        return step->incoming;
    }
};
}
