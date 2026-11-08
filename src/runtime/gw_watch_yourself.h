#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct WatchYourselfPlan {
 unsigned skill=348,rank=0,armor=5,charges=10,adrenaline=80;Time duration=10000,recharge=4000;float range=1012;
 auto operator<=>(const WatchYourselfPlan&)const=default;
};
inline std::optional<WatchYourselfPlan> watch_yourself(unsigned rank,bool pvp=false){
 const unsigned skill=pvp?2858:348;const auto* r=find_skill(skill);
 if(rank>20||!r||r->profession!=1||r->attribute!=21||r->type!=15||r->energy_cost()!=0||r->adrenaline!=80||r->activation!=0||r->aftercast!=0||r->recharge!=4||r->scale0!=5||r->scale15!=25||r->bonus0!=(pvp?1u:10u)||r->bonus15!=(pvp?3u:10u)||r->duration0!=10||r->duration15!=10||r->caster_body_resource!=596||r->target_body_resource!=2077)return {};
 return WatchYourselfPlan{skill,rank,interpolate(5,25,rank),pvp?interpolate(1,3,rank):10u};
}
struct WatchYourselfOwned {
 Scope source,target;unsigned effect=0,skill=0,rank=0,remaining=0;Time begins=0,ends=0,last_time=0;std::uint64_t last_damage=0;
};
inline std::optional<WatchYourselfOwned> prepare_watch_yourself(const WatchYourselfPlan& plan,Scope source,Scope target,unsigned effect,Time now){
 const auto canonical=watch_yourself(plan.rank,plan.skill==2858);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=plan||!valid(source)||!valid(target)||source.instance!=target.instance||!effect||now>UINT64_MAX-plan.duration)return {};
 return WatchYourselfOwned{source,target,effect,plan.skill,plan.rank,plan.charges,now,now+plan.duration,now,0};
}
inline unsigned watch_yourself_armor(const Registry& registry,const WatchYourselfOwned& owned,Time now){
 const auto plan=watch_yourself(owned.rank,owned.skill==2858);const auto entry=registry.lookup(owned.target,owned.effect);
 if(!plan||plan->skill!=owned.skill||owned.source.instance!=owned.target.instance||!owned.source.actor||owned.source.actor>65535||!owned.source.generation||
  owned.ends<owned.begins||owned.ends-owned.begins!=plan->duration||!owned.remaining||owned.remaining>plan->charges||now<owned.begins||now>=owned.ends||!entry||
  entry->skill!=owned.skill||entry->rank!=owned.rank||entry->cause!=owned.source.actor||entry->kind!=Category::other||entry->applied!=owned.begins||entry->expires!=owned.ends)return 0;
 return plan->armor;
}
struct WatchYourselfDamage {Scope target;std::uint64_t event=0;Time at=0;bool accepted=false,armor_respecting=false;double applied=0;};
struct WatchYourselfConsumption {WatchYourselfOwned owned;bool consumed=false,ended=false;};
// Evaluate armor before the hit;consume only after real positive damage. The
// documented anomaly includes armor-respecting spells,not only weapon attacks.
// World must supply a unique per-recipient damage receipt,not an attack attempt.
inline std::optional<WatchYourselfConsumption> consume_watch_yourself(const Registry& registry,
 const WatchYourselfOwned& owned,const WatchYourselfDamage& damage){
 if(damage.target!=owned.target||!damage.event||damage.event<=owned.last_damage||damage.at<owned.last_time||
  !std::isfinite(damage.applied)||damage.applied<0||!watch_yourself_armor(registry,owned,damage.at))return {};
 WatchYourselfConsumption result{owned,false,false};result.owned.last_damage=damage.event;result.owned.last_time=damage.at;
 if(damage.accepted&&damage.armor_respecting&&damage.applied>0){result.consumed=true;--result.owned.remaining;result.ended=!result.owned.remaining;}
 return result;
}
}
