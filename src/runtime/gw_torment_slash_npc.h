#pragma once
#include "gw_skill_effect_plan.h"
#include "gw_combat.h"
#include <compare>
#include <optional>
#include <vector>

namespace fw::gw::skills::effects {
struct TormentSlashPlan {
    unsigned skill=1324,damage_per_hit=150,hits=2;
    auto operator<=>(const TormentSlashPlan&)const=default;
};
inline std::optional<TormentSlashPlan> torment_slash_npc(){
    const auto* row=find_skill(1324);
    if(!row||row->profession!=0||row->attribute!=51||row->type!=10||
       row->energy_cost()!=5||row->activation!=3.25f||
       row->aftercast!=.75f||row->recharge!=8||
       row->scale0!=150||row->scale15!=150||
       row->target_body_resource!=2077)return {};
    return TormentSlashPlan{};
}
inline std::optional<TormentSlashPlan> torment_slash_tendrils_npc(){
    const auto* row=find_skill(1880);
    if(!row||row->profession!=0||row->attribute!=51||row->type!=10||
       row->energy_cost()!=5||row->activation!=3.25f||
       row->aftercast!=.75f||row->recharge!=2||
       row->scale0!=200||row->scale15!=200||
       row->target_body_resource!=2077)return {};
    return TormentSlashPlan{1880,200,2};
}
// World must determine the authoritative forward hit region. A non-front
// actor receives no hit. Each accepted strike is independent for protection,
// damage attribution and lethal stop; this does not schedule an NPC cast.
inline std::optional<std::vector<combat::Hit>> torment_slash_target(
        const TormentSlashPlan& plan,combat::Actor& target,unsigned level,
        bool hostile,bool in_front){
    const auto reviewed=plan.skill==1324?torment_slash_npc():
        plan.skill==1880?torment_slash_tendrils_npc():std::nullopt;
    if(reviewed!=plan||!level||level>255||!hostile||
       !target.health||!combat::valid(*target.health)||
       target.health->current<=0||!target.damageable)return {};
    std::vector<combat::Hit> hits;
    if(!in_front)return hits;
    for(unsigned i=0;i<plan.hits;++i){
        const auto hit=combat::apply(target,
            {combat::Kind::slashing,double(plan.damage_per_hit),level,plan.skill},true);
        if(hit.reason!=combat::Reason::accepted)return {};
        hits.push_back(hit);
        if(hit.killed)break;
    }
    return hits;
}
}
