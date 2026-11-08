#pragma once
#include "gw_aura_restoration.h"
#include "gw_i_will_survive.h"
#include "gw_i_am_unstoppable.h"
#include "gw_finish_him.h"
#include "gw_you_move_like_a_dwarf.h"
#include "gw_you_are_all_weaklings.h"
#include "gw_norn_attack_shouts.h"
#include "gw_for_great_justice.h"
#include "gw_to_limit.h"
#include "gw_endure_pain.h"
#include "gw_defy_pain.h"
#include "gw_watch_yourself.h"
#include "gw_shields_up.h"
#include "gw_dont_trip.h"
#include "gw_save_yourselves.h"
#include "gw_charge.h"
#include "gw_resource_gain.h"
#include "gw_skill_effect_plan.h"
#include "gw_effect_registry.h"
#include "gw_balanced_stance.h"
#include "gw_defensive_stance.h"
#include "gw_berserker_stance.h"
#include "gw_dwarven_battle_stance.h"
#include "gw_gladiator_defense_policy.h"
#include "gw_bonetti_defense_policy.h"
#include "gw_wary_stance.h"
#include "gw_whirling_defense_policy.h"
#include "gw_shield_stance.h"
#include "gw_shield_bash.h"
#include "gw_battle_rage.h"
#include "gw_bulls_charge.h"
#include "gw_frenzied_defense.h"
#include "gw_flurry.h"
#include "gw_flail.h"
#include "gw_escape.h"
#include "gw_warriors_cunning.h"
#include "gw_warriors_endurance.h"
#include "gw_lightning_reflexes.h"
#include "gw_distortion.h"
#include "gw_storm_chaser.h"
#include "gw_serpents_quickness.h"
#include "gw_tiger_stance.h"
#include "gw_hekets_rampage.h"
#include "gw_natural_stride.h"
#include "gw_deadly_paradox.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
namespace fw::gw::skills::timing {
struct Stance {unsigned skill,rank;std::uint64_t duration;};
// Quickening Zephyr charges 30% of the skill's base Energy at cast start.
// The documented exact-half rule rounds down: 5 -> +1, 15 -> +4, 25 -> +7.
// World must first prove a live in-range aura and a non-spirit caster.
inline std::optional<unsigned> quickening_zephyr_energy_cost(unsigned base,bool affected){
    if(base>10000)return {};
    return affected?base+unsigned((std::uint64_t(base)*3+4)/10):base;
}
inline bool instant_stance(const SkillRecord& skill){return skill.type==3&&skill.activation==0&&skill.aftercast==0;}
// The captured kind48 action is also used by the reviewed non-stance362.
// Do not infer instant handling for unrelated zero-activation skills/signets.
inline bool instant_action(const SkillRecord& skill){return instant_stance(skill)||(skill.id==363&&effects::shield_bash(0).has_value())||(skill.id==367&&effects::shields_up(0).has_value())||(skill.id==2216&&effects::dont_trip(0).has_value())||((skill.id==1954||skill.id==2097)&&effects::save_yourselves(skill.id,0).has_value())||(skill.id==364&&effects::charge(0).has_value())||(skill.id==318&&effects::defy_pain(0).has_value())||(skill.id==347&&effects::endure_pain(0).has_value())||((skill.id==348||skill.id==2858)&&effects::watch_yourself(0,skill.id==2858).has_value())||(skill.id==316&&effects::to_limit(0).has_value())||((skill.id==343||skill.id==2883)&&effects::for_great_justice(skill.id).has_value())||((skill.id==365||skill.id==1141)&&effects::resource_gain(skill.id,0).has_value())||(skill.id==368&&effects::i_will_survive(0).has_value())||(skill.id==2354&&effects::dodge_this(0).has_value())||(skill.id==2355&&effects::i_am_the_strongest(0).has_value())||(skill.id==2356&&effects::i_am_unstoppable(0).has_value())||(skill.id==2353&&effects::finish_him(0).has_value())||(skill.id==2358&&effects::you_move_like_a_dwarf(0).has_value())||(skill.id==2359&&effects::you_are_all_weaklings(0).has_value())||(skill.id==374&&effects::warriors_endurance(374,0).has_value())||(skill.id==362&&effects::warriors_cunning(0).has_value());}
inline std::optional<Stance> stance(unsigned skill,unsigned rank=0){
    if(skill==1727){const auto p=effects::natural_stride(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==1728){const auto p=effects::hekets_rampage(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==377){const auto p=effects::wary_stance(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==379){const auto p=effects::bulls_charge(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==317){const auto p=effects::battle_rage(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==1700){const auto p=effects::frenzied_defense(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==456){const auto p=effects::serpents_quickness(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==375){const auto p=effects::dwarven_battle_stance(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==380){const auto p=effects::bonetti_defense(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==450){const auto p=effects::whirling_defense(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==372){const auto p=effects::gladiator_defense(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==370){const auto p=effects::berserker_stance(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==995){const auto p=effects::tiger_stance(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==572){const auto p=effects::deadly_paradox(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==345){const auto p=effects::defensive_stance(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==378){const auto p=effects::shield_stance(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==455){const auto p=effects::storm_chaser(rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==11){const auto plan=effects::distortion(rank);return plan?std::optional<Stance>{{skill,rank,plan->duration}}:std::nullopt;}
    if(rank>20)return {};
    if(skill==453||skill==3141){const auto d=effects::lightning_reflexes(skill,rank);return d?std::optional<Stance>{{skill,rank,*d}}:std::nullopt;}
    if(skill==3002){const auto p=effects::warriors_endurance(skill,rank);return p?std::optional<Stance>{{skill,rank,p->duration}}:std::nullopt;}
    if(skill==448||skill==3060){const auto d=effects::escape(skill,rank);return d?std::optional<Stance>{{skill,rank,*d}}:std::nullopt;}
    if(skill==1404){const auto d=effects::flail(rank);return d?std::optional<Stance>{{1404,rank,*d}}:std::nullopt;}
    if(skill==344)return effects::flurry_plan()?std::optional<Stance>{{344,rank,5000}}:std::nullopt;
    if(skill==371){const auto p=effects::balanced_stance(rank);return p?std::optional<Stance>{{371,rank,p->duration}}:std::nullopt;}
    if(skill==349)return Stance{skill,rank,std::uint64_t(effects::interpolate(8,14,rank))*1000};
    if(skill==16)return Stance{skill,rank,std::uint64_t(effects::interpolate(1,38,rank))*1000};
    if(skill==17)return Stance{skill,rank,std::uint64_t(effects::interpolate(30,90,rank))*1000};
    if(skill==13)return Stance{skill,rank,std::uint64_t(effects::interpolate(5,20,rank))*1000};
    if(skill==15)return Stance{skill,rank,std::uint64_t(effects::interpolate(5,25,rank))*1000};
    if(skill==14)return Stance{skill,rank,std::uint64_t(effects::interpolate(5,25,rank))*1000};
    return {};
}
// Fast Casting: spell factor2^(-rank/15), signet factor1-.03*rank.
// The under2second exclusion uses the unmodified activation and profession.
// Millisecond scheduling retains the documented full-precision factors; the
// wiki table/tooltip's tenth-second presentation is not the server deadline.
inline std::optional<std::uint64_t> activation(const SkillRecord& skill,unsigned primary,unsigned fast,double modifier=1.){
    if(primary>10||fast>20||!std::isfinite(skill.activation)||skill.activation<0||skill.activation>90||!std::isfinite(modifier)||modifier<.75||modifier>2.5)return {};
    double seconds=skill.activation;
    if(primary==5&&fast&&(effects::is_spell(skill)||skill.type==7)&&(skill.profession==5||skill.activation>=2.f))
        seconds*=skill.type==7?1.-.03*fast:std::exp2(-double(fast)/15.);
    return static_cast<std::uint64_t>(std::llround(seconds*modifier*1000.));
}
// The original228 callback multiplies the final integer seconds by1000 and
// uses the same float seconds for its UI message. Apply all reviewed factors
// before one final nearest-second rounding, never round each factor in turn.
inline std::optional<unsigned> recharge(const SkillRecord& skill,unsigned primary,unsigned fast,bool pvp,
        const std::vector<effects::Entry>& active,std::uint64_t now,unsigned base,double intrinsic=1.,
        bool quickening_zephyr=false){
    if(primary>10||fast>20||base>86400||!std::isfinite(intrinsic)||intrinsic<=0||intrinsic>1)return {};
    double factor=1.;
    for(const auto& e:active){
        if(e.kind!=effects::Category::stance||e.applied>now||(e.expires&&now>=*e.expires)||e.rank>20)continue;
        if(e.skill==13&&effects::is_spell(skill))factor=std::min(factor,.67);
        if(e.skill==15&&skill.type==7)factor=std::min(factor,1.-double(effects::interpolate(10,40,e.rank))/100.);
        if(effects::serpents_quickness_reduces(skill,e,now))factor=std::min(factor,.67);
        if(effects::deadly_paradox_recharges(skill,e,now))factor=std::min(factor,.67);
    }
    // The caller must establish a live Quickening Zephyr spirit in range at
    // completion. Its Energy-cost effect is a separate cast-start decision.
    if(quickening_zephyr)factor=std::min(factor,.5);
    // Other recharge effects will share the ordinary50% cap. Fast Casting's
    // separate PvE Mesmer-spell factor explicitly lies outside that cap.
    // Apply a skill's own conditional factor before one final rounding. A
    // single reduction exceeding50% retains its documented cap exception.
    factor=std::max(std::min(.5,intrinsic),factor*intrinsic);
    if(primary==5&&fast&&!pvp&&skill.profession==5&&effects::is_spell(skill))factor*=1.-.03*fast;
    return static_cast<unsigned>(std::llround(double(base)*factor));
}
}

