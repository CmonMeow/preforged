#pragma once
#include "gw_holy_damage.h"
#include "gw_effect_registry.h"
#include <set>
namespace fw::gw::skills::effects {
struct SignetJudgmentPlan {
 unsigned skill=294,rank=0,damage=15,knockdown_ms=2000,energy=0;
 float range=1248,radius=166;
 Time activation=1000,aftercast=750,recharge=20000;
 auto operator<=>(const SignetJudgmentPlan&)const=default;
};
inline std::optional<SignetJudgmentPlan> signet_judgment(unsigned rank,bool pvp=false){
 const auto holy=holy_damage(pvp?2887:294,rank);if(!holy)return {};
 SignetJudgmentPlan plan;plan.skill=holy->skill;plan.rank=rank;plan.damage=holy->damage;plan.range=holy->range;return plan;
}
struct SignetJudgmentRecipient {
 Scope owner;combat::Actor actor;HolyTraits traits;double distance=0;unsigned plane=0;
 std::optional<bool> knockdown_allowed;bool already_down=false;unsigned holy_bonus=0;combat::DamageProjection projection;
};
struct SignetJudgmentHit {Scope target;combat::Hit hit;unsigned knockdown_ms=0;};
// Copy-only proposal for a World-selected hostile crowd at completion time.
// World must prove allegiance,positions/LOS,source level and every scope again
// before commit. This does not pay a cast or publish particles/Health/control.
inline std::optional<std::vector<SignetJudgmentHit>> prepare_signet_judgment(
        const SignetJudgmentPlan& plan,Scope caster,Scope selected,unsigned level,
        double selected_range,unsigned plane,const std::vector<SignetJudgmentRecipient>& inputs){
 const auto canonical=signet_judgment(plan.rank,plan.skill==2887);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=plan||!valid(caster)||!valid(selected)||caster.instance!=selected.instance||caster.actor==selected.actor||
    !std::isfinite(selected_range)||selected_range<0||selected_range>plan.range||inputs.size()>4096)return {};
 std::vector<SignetJudgmentHit> result;std::set<unsigned> actors;bool found_primary=false;
 for(const auto& input:inputs){
  if(!valid(input.owner)||input.owner.instance!=caster.instance||input.owner.actor==caster.actor||!actors.insert(input.owner.actor).second||
     input.actor.id!=input.owner.actor||input.actor.generation!=input.owner.generation||!std::isfinite(input.distance)||input.distance<0)return {};
  const bool primary=input.owner==selected;
  if(primary&&(input.distance!=0||input.plane!=plane))return {};
  if(input.distance>plan.radius||input.plane!=plane)continue;
  if(!input.actor.health||!combat::valid(*input.actor.health))return {};
  if(input.actor.health->current<=0){if(primary)return {};continue;}
  auto damage=holy_strike(*holy_damage(plan.skill,plan.rank),input.traits,level);if(!damage)return {};damage->amount+=input.holy_bonus;if(!combat::valid(*damage))return {};
  auto actor=input.actor;auto projection=input.projection;const auto step=combat::project_damage(actor,*damage,projection);if(!step)return {};const auto& hit=step->incoming;
  unsigned knockdown=0;
  if(primary){found_primary=true;
   if(actor.health->current>0&&!input.already_down){if(!input.knockdown_allowed)return {};if(*input.knockdown_allowed)knockdown=plan.knockdown_ms;}
  }
  result.push_back({input.owner,hit,knockdown});
 }
 return found_primary?std::optional(std::move(result)):std::nullopt;
}
}
