#include "gw_well_field.h"
#include <array>
#include <stdexcept>
#include <iostream>
using namespace fw::gw::skills::effects;
template<class T> void check(const T& value){if(!static_cast<bool>(value))throw std::runtime_error("Well field policy failed");}
int main(){
    const Scope caster{42,25,3},corpse{42,70,8};
    for(unsigned skill=91;skill<=94;++skill)for(unsigned rank:{0u,15u,20u}){
        const auto plan=well_field_plan(skill,rank);check(plan);
        const auto claim=prepare_well_corpse_claim(plan->cast,caster,
            {corpse,11,0,100,200,true,true,false},true,true,1000);check(claim);
        const auto field=create_well_field(*claim,7);check(field);
        check(field->expires-field->begins==plan->duration);
        check(plan->duration==1000ull*static_cast<unsigned>(well_anchor(rank,
            skill==93?10:8,skill==93?26:18,skill==93?30:20)));
        const auto ally=well_field_effect(*field,0,100,200,7,1000);
        const auto foe=well_field_effect(*field,0,100,200,8,1000);
        if(skill==91)check(ally.health_pips>0&&ally.energy_pips==2&&!foe.health_pips);
        if(skill==92)check(ally.health_pips>0&&ally.energy_pips==0&&!foe.health_pips);
        if(skill==93)check(foe.health_pips<0&&!ally.health_pips);
        if(skill==94)check(foe.strip_enchantments&&foe.block_enchantments&&!ally.block_enchantments);
        check(!well_field_effect(*field,1,100,200,7,1000).health_pips);
        check(!well_field_effect(*field,0,100,200,7,field->expires).health_pips);
        check(!well_field_effect(*field,0,1000,200,7,1000).health_pips);
    }
    check(well_field_plan(94,4)->low_rank_failure_percent==50);
    check(well_field_plan(94,5)->low_rank_failure_percent==0);
    check(!well_field_plan(95,0));
    const auto plan=*well_field_plan(92,15);
    const auto claim=*prepare_well_corpse_claim(plan.cast,caster,
        {corpse,11,0,0,0,true,true,false},true,true,1300);
    const auto field=*create_well_field(claim,7);
    const WellRouteLeg crossing{0,-500,0,500,0,1000,2000};
    const auto interval=well_route_interval(field,crossing,7);check(interval);
    check(std::abs(interval->enter-.3)<1e-9);
    check(std::abs(interval->leave-.822)<1e-9);
    check(!well_route_interval(field,crossing,8));
    check(!well_route_interval(field,{1,-500,0,500,0,1000,2000},7));
    check(!well_route_interval(field,{0,500,500,600,600,1000,2000},7));
    check(!well_route_interval(field,{0,-500,322,500,322,1000,2000},7));
    const auto stationary=well_route_interval(field,{0,0,0,0,0,1000,2000},7);
    check(stationary&&std::abs(stationary->enter-.3)<1e-9&&
          std::abs(stationary->leave-1.)<1e-9);
    check(!well_route_interval(field,{0,0,0,0,0,0,1000},7));
    auto overlap=field;overlap.x=200;overlap.plan.health_pips=3;
    const std::array fields{field,overlap};
    const auto segments=well_route_segments(fields,crossing,7);
    check(segments.size()==4);
    check(std::abs(segments[0].enter)<1e-9&&std::abs(segments[0].leave-.3)<1e-9&&segments[0].effect.health_pips==0);
    check(segments[1].effect.health_pips==field.plan.health_pips);
    check(segments[2].effect.health_pips==field.plan.health_pips+3);
    check(segments[3].effect.health_pips==3);
    check(segments.back().leave==1);
    const auto power_plan=*well_field_plan(91,15);
    const auto power_claim=*prepare_well_corpse_claim(power_plan.cast,caster,
        {corpse,11,0,0,0,true,true,false},true,true,1300);
    const auto power=*create_well_field(power_claim,7);
    const auto power_segments=well_route_segments(std::span<const WellField>(&power,1),crossing,7);
    check(power_segments.size()==3&&power_segments[1].effect.energy_pips==2);
    const auto profane_plan=*well_field_plan(94,15);
    const auto profane_claim=*prepare_well_corpse_claim(profane_plan.cast,caster,
        {corpse,11,0,0,0,true,true,false},true,true,1300);
    const auto profane=*create_well_field(profane_claim,7);
    const auto profane_segments=well_route_segments(std::span<const WellField>(&profane,1),crossing,8);
    check(profane_segments.size()==3&&profane_segments[1].effect.strip_enchantments&&
          profane_segments[1].effect.block_enchantments);
    std::cout<<"Well field rank anchors, coverage, teams and expiry passed\n";
}
