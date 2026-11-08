#pragma once
#include "gw_skill_catalog_generated.h"
#include "gw_conditions.h"
#include "gw_life_drain_hexes.h"
#include "gw_arcane_conundrum.h"
#include "gw_control_hexes.h"
#include "gw_fragility.h"
#include "gw_attack_trigger_hexes.h"
#include "gw_barbs.h"
#include "gw_mark_of_pain.h"
#include "gw_insidious_parasite.h"
#include "gw_rigor_mortis.h"
#include "gw_price_of_failure.h"
#include "gw_parasitic_bond.h"
#include "gw_mark_of_death.h"
#include "gw_putrid_bile.h"
#include <cmath>
#include <optional>
namespace fw::gw::skills::effects {
struct DegenerationHexPlan {
    unsigned skill,rank,pips;
    std::uint64_t duration,end_duration;
    float range=1248.f,radius=0;
    std::optional<combat::conditions::Kind> end_condition;
    bool life_drain=false;
    float break_range=0;
};
inline std::optional<DegenerationHexPlan> degeneration_hex(unsigned skill,unsigned rank=0,bool pvp=false){
    if(skill==2058){const auto plan=putrid_bile(rank);if(!plan)return {};return DegenerationHexPlan{2058,rank,plan->degeneration_pips,plan->duration_ms,0,plan->range};}
    if(skill==785){const auto plan=mark_of_death(rank);if(!plan)return {};return DegenerationHexPlan{785,rank,0,plan->duration,0,plan->range};}
    if(skill==99){const auto plan=parasitic_bond(rank);if(!plan)return {};return DegenerationHexPlan{99,rank,plan->pips,plan->duration,0};}
    if(skill==103){const auto plan=price_of_failure(rank);if(!plan)return {};return DegenerationHexPlan{103,rank,0,plan->duration,0};}
    if(skill==137){const auto plan=rigor_mortis(rank);if(!plan)return {};return DegenerationHexPlan{137,rank,0,plan->duration,0};}
    if(skill==123){const auto plan=insidious_parasite(rank);if(!plan)return {};return DegenerationHexPlan{123,rank,0,plan->duration,0};}
    if(skill==150){const auto plan=mark_of_pain(rank);if(!plan)return {};return DegenerationHexPlan{150,rank,0,plan->duration,0};}
    if(skill==101){const auto plan=barbs(rank);if(!plan)return {};return DegenerationHexPlan{101,rank,0,plan->duration,0};}
    if(const auto plan=attack_hex(skill,rank))return DegenerationHexPlan{plan->skill,rank,0,plan->duration,0,plan->range,plan->radius};
    if(skill==19&&pvp)skill=2998;
    if(const auto plan=fragility(skill,rank))return DegenerationHexPlan{plan->skill,rank,0,plan->duration,0,plan->range,plan->radius};
    if(skill==53&&pvp)skill=3183;
    if(const auto drain=life_drain_hex(skill,rank))return DegenerationHexPlan{skill,rank,drain->pips,drain->duration,0,drain->range,drain->radius,std::nullopt,true,drain->break_range};
    if(const auto control=control_hex(skill,rank))return DegenerationHexPlan{skill,rank,0,control->duration,0,control->range,control->radius};
    if(skill==36){const auto plan=arcane_conundrum(rank);if(!plan)return {};return DegenerationHexPlan{36,rank,0,plan->duration,0,plan->range,plan->radius};}
    if(rank>20||(skill!=31&&skill!=53&&skill!=3183&&skill!=859&&skill!=108&&skill!=129&&skill!=142&&skill!=2054))return {};
    const auto* row=find_skill(skill);if(!row||row->type!=4)return {};
    if(skill==129)return DegenerationHexPlan{skill,rank,0,static_cast<std::uint64_t>(std::lround(row->duration0+(double(row->duration15)-row->duration0)*rank/15.))*1000,0};
    if(skill==53||skill==3183)return DegenerationHexPlan{skill,rank,static_cast<unsigned>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.)),static_cast<std::uint64_t>(std::lround(row->duration0+(double(row->duration15)-row->duration0)*rank/15.))*1000,0};
    const auto pips=static_cast<unsigned>(std::lround(row->scale0+(double(row->scale15)-row->scale0)*rank/15.));
    // Durations are reviewed local wiki progressions; native scale fields
    // store degeneration here, not duration. Native bonuses store the end
    // condition duration for Shrinking Armor.
    const auto duration=skill==2054?10u:(skill==108||skill==142)?static_cast<unsigned>(std::lround(6.+24.*rank/15.)):static_cast<unsigned>(std::lround(2.+14.*rank/15.));
    const auto end=skill==2054?static_cast<unsigned>(std::lround(row->bonus0+(double(row->bonus15)-row->bonus0)*rank/15.)):0u;
    return DegenerationHexPlan{skill,rank,pips,duration*1000u,end*1000u,1248.f,(skill==108||skill==142)?252.f:0.f,skill==2054?std::optional(combat::conditions::Kind::cracked_armor):std::nullopt};
}
inline bool valid_degeneration_hex(const DegenerationHexPlan& p){
    const auto x=degeneration_hex(p.skill,p.rank);return x&&p.pips==x->pips&&p.duration==x->duration&&p.end_duration==x->end_duration&&p.range==x->range&&p.radius==x->radius&&p.end_condition==x->end_condition&&p.life_drain==x->life_drain&&p.break_range==x->break_range;
}
}
