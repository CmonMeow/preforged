#pragma once
#include "gw_skill_catalog_generated.h"
#include <cmath>
#include <algorithm>
#include <optional>
namespace fw::gw::skills::effects {
struct PartyHealPlan {unsigned skill,rank,base_heal,caster_divine_heal;float range=5000.f;bool all_creatures=false;bool remove_burning=false;};
inline std::optional<PartyHealPlan> party_heal(unsigned skill,unsigned rank,unsigned divine){
    const auto* row=find_skill(skill);if(!row||rank>20||divine>20)return {};
    if(skill==2221){
        if(rank>10||row->campaign!=4||row->profession||row->attribute!=51||row->type!=5||
           row->energy_cost()!=10||row->activation!=.25f||row->aftercast!=.75f||row->recharge!=15||
           row->scale0!=50||row->scale15!=60||row->aoe_range!=5000.f||
           row->caster_body_resource!=1957||row->target_body_resource!=2077)return {};
        return PartyHealPlan{skill,rank,50+2*std::min(rank,5u),static_cast<unsigned>(std::lround(3.2*divine)),5000.f,false,true};
    }
    if(skill!=287&&skill!=3232&&skill!=1397&&skill!=279&&skill!=1117&&skill!=280&&skill!=1119)return {};
    // Light of Deliverance's scale endpoints101/101 are retained threshold
    // metadata, not healing. Reviewed pinned bonus endpoints are5/70.
    const auto low=skill==1397?row->bonus0:row->scale0,high=skill==1397?row->bonus15:row->scale15;
    return PartyHealPlan{skill,rank,static_cast<unsigned>(std::lround(low+(double(high)-low)*rank/15.)),static_cast<unsigned>(std::lround(3.2*divine)),(skill==280||skill==1119)?166.f:(skill==279||skill==1117)?1000.f:5000.f,skill==280||skill==1119};
}
inline unsigned recipient_heal(const PartyHealPlan& plan,bool caster){return plan.base_heal+(caster?plan.caster_divine_heal:0);}
}
