#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_conditions.h"
#include "gw_hex_duration.h"
#include <set>
namespace fw::gw::skills::effects {
struct FeveredDreamsPlan {
 unsigned skill=55,rank=0;Time duration=10000,dazed_duration=1000;float range=1248,radius=322;
 auto operator<=>(const FeveredDreamsPlan&)const=default;
};
inline std::optional<FeveredDreamsPlan> fevered_dreams(unsigned rank,bool pvp=false){
 const auto id=pvp?3289u:55u;const auto* row=find_skill(id);
 if(rank>20||!row||row->type!=4||row->attribute!=1||!row->elite()||row->duration0!=(pvp?5u:10u)||row->duration15!=(pvp?15u:25u)||row->bonus0!=1||row->bonus15!=3||row->recharge!=(pvp?20u:10u))return {};
 return FeveredDreamsPlan{id,rank,Time(interpolate(pvp?5:10,pvp?15:25,rank))*1000,pvp?0:Time(interpolate(1,3,rank))*1000};
}
struct FeveredOwnership {Scope target,caster;unsigned effect=0;auto operator<=>(const FeveredOwnership&)const=default;};
inline bool valid_fevered_owner(const FeveredOwnership& o){return o.effect&&o.target.instance&&o.target.instance==o.caster.instance&&o.target.actor&&o.caster.actor&&o.target.actor<=65535&&o.caster.actor<=65535&&o.target.actor!=o.caster.actor&&o.target.generation&&o.caster.generation;}
struct FeveredApplication {Registry registry;FeveredOwnership owner;Entry entry;std::vector<Entry> removed;};
inline std::optional<FeveredApplication> prepare_fevered_dreams(const Registry& original,const Registry& caster_registry,Scope caster,Scope target,unsigned effect,const FeveredDreamsPlan& plan,Time now){
 const auto canonical=fevered_dreams(plan.rank,plan.skill==3289);const FeveredOwnership owner{target,caster,effect};
 if(!valid_fevered_owner(owner)||!canonical||*canonical!=plan||!original.matches(target)||!caster_registry.matches(caster)||original.lookup(target,effect))return {};
 const auto adjustment=hex_duration(plan.skill,plan.duration,caster,caster_registry.active(caster,now),now);
 if(!adjustment)return {};const auto duration=adjustment->for_hex(plan.skill,plan.duration,caster.actor);if(!duration)return {};
 auto registry=original;std::vector<Entry> removed;
 for(const auto& old:registry.active(target,now))if(old.skill==55||old.skill==3289){registry.remove(target,old.instance);removed.push_back(old);}
 const Entry entry{effect,caster.actor,plan.skill,plan.rank,Category::hex,now,now+*duration};
 auto applied=registry.apply(target,entry,Replace::same_skill);if(!applied.accepted)return {};
 removed.insert(removed.end(),applied.removed.begin(),applied.removed.end());
 return FeveredApplication{std::move(registry),owner,entry,std::move(removed)};
}
inline std::optional<Entry> owned_fevered_dreams(const Registry& registry,const FeveredOwnership& owner,Time now){
 if(!valid_fevered_owner(owner))return {};const auto e=registry.lookup(owner.target,owner.effect);
 if(!e||(e->skill!=55&&e->skill!=3289)||e->cause!=owner.caster.actor||e->kind!=Category::hex||e->applied>now||!e->expires||now>=*e->expires||!fevered_dreams(e->rank,e->skill==3289))return {};
 return e;
}
inline bool current_fevered_application(const FeveredApplication& app,const Registry& current,Scope caster,Scope target,Time now){
 if(app.owner.caster!=caster||app.owner.target!=target)return false;
 const auto e=owned_fevered_dreams(current,app.owner,now);
 return e&&e->skill==app.entry.skill&&e->rank==app.entry.rank&&e->applied==app.entry.applied&&e->expires==app.entry.expires;
}
// Caller must choose explicitly until a controlled renewal capture resolves
// whether generated Dazed copies its attempted or retained longer timer.
enum class FeveredDazedSpan {attempted_duration,retained_remaining};
struct FeveredConditionEvent {Scope target;std::uint64_t event=0;Time at=0;bool confirmed=false;};
inline bool valid_fevered_event(const FeveredConditionEvent& e){return e.target.instance&&e.target.actor&&e.target.actor<=65535&&e.target.generation&&e.event&&e.confirmed;}
struct FeveredPulse {combat::conditions::Kind kind;Time duration;unsigned original_condition_cause=0;bool generated_dazed=false;};
struct FeveredReaction {FeveredOwnership owner;Entry hex;FeveredConditionEvent event;combat::conditions::State target;std::vector<FeveredPulse> pulses;bool dazed_applied=false,dazed_renewed=false;};
// The inputs are committed condition states, not icon refresh packets. A
// multi-condition delta is treated as one atomic set; retail intra-set timing
// and propagation/Fragility ordering remain World integration concerns.
inline std::optional<FeveredReaction> fevered_condition_reaction(const Registry& registry,const FeveredOwnership& owner,
 const FeveredConditionEvent& event,const combat::conditions::State& before,const combat::conditions::State& after,
 FeveredDazedSpan dazed_span,bool hex_application=false){
 if(!valid_fevered_event(event)||event.target!=owner.target||(dazed_span!=FeveredDazedSpan::attempted_duration&&dazed_span!=FeveredDazedSpan::retained_remaining))return {};
 const auto entry=owned_fevered_dreams(registry,owner,event.at);if(!entry)return {};
 const auto plan=*fevered_dreams(entry->rank,entry->skill==3289);
 FeveredReaction result{owner,*entry,event,after,{},false,false};
 // Applying the hex does not copy conditions that were already on its target.
 if(!hex_application)for(unsigned i=0;i<10;++i){const auto kind=static_cast<combat::conditions::Kind>(i);
  if(!before.has(kind,event.at)&&after.has(kind,event.at)){const auto c=after.lookup(kind);result.pulses.push_back({kind,c->expires-event.at,c->cause,false});}
 }
 const bool trigger=hex_application?after.count(event.at)>=1:!result.pulses.empty()&&after.count(event.at)>=2;
 if(plan.dazed_duration&&trigger){
  result.dazed_renewed=after.has(combat::conditions::Kind::dazed,event.at);
  if(event.at>UINT64_MAX-plan.dazed_duration||!result.target.apply(combat::conditions::Kind::dazed,owner.caster.actor,event.at,plan.dazed_duration))return {};
  result.dazed_applied=true;
  const auto remaining=result.target.lookup(combat::conditions::Kind::dazed)->expires-event.at;
  const auto duration=dazed_span==FeveredDazedSpan::attempted_duration?plan.dazed_duration:remaining;
  // Deliberately retain a generated renewal pulse even though the generic
  // condition delta would not call that renewal a newly gained condition.
  result.pulses.push_back({combat::conditions::Kind::dazed,duration,owner.caster.actor,true});
 }
 return result;
}
struct FeveredContact {Scope owner;float distance=0;bool alive=true,foe=true;combat::conditions::State conditions;};
struct FeveredRecipient {Scope owner;combat::conditions::State before,after;};
inline std::optional<std::vector<FeveredRecipient>> fevered_recipients(const FeveredReaction& reaction,const std::vector<FeveredContact>& contacts){
 if(!valid_fevered_owner(reaction.owner)||!valid_fevered_event(reaction.event)||reaction.event.target!=reaction.owner.target)return {};
 const auto plan=fevered_dreams(reaction.hex.rank,reaction.hex.skill==3289);if(!plan||plan->skill!=reaction.hex.skill||reaction.hex.instance!=reaction.owner.effect||reaction.hex.cause!=reaction.owner.caster.actor||reaction.hex.kind!=Category::hex||reaction.hex.applied>reaction.event.at||!reaction.hex.expires||reaction.event.at>=*reaction.hex.expires)return {};
 for(const auto& pulse:reaction.pulses)if(static_cast<unsigned>(pulse.kind)>=10||!pulse.duration||pulse.duration>43200000||!pulse.original_condition_cause||reaction.event.at>UINT64_MAX-pulse.duration)return {};
 std::set<unsigned> seen;std::vector<FeveredRecipient> result;
 for(const auto& c:contacts){if(c.owner.instance!=reaction.owner.target.instance||!c.owner.actor||c.owner.actor>65535||!c.owner.generation||!seen.insert(c.owner.actor).second||!std::isfinite(c.distance)||c.distance<0)return {};
  if(c.owner.actor==reaction.owner.target.actor){if(c.owner!=reaction.owner.target)return {};continue;}
  if(c.owner.actor==reaction.owner.caster.actor){if(c.owner!=reaction.owner.caster)return {};continue;}
  if(!c.alive||!c.foe||c.distance>plan->radius||reaction.pulses.empty())continue;
  auto after=c.conditions;
  for(const auto& pulse:reaction.pulses)if(!after.apply(pulse.kind,reaction.owner.caster.actor,reaction.event.at,pulse.duration))return {};
  result.push_back({c.owner,c.conditions,std::move(after)});
 }
 return result;
}
// Copy alongside the World condition proposals. Keep across hex renewals;
// an old committed condition event cannot hit a new effect instance. Each
// actual propagation commit needs its own event ID, even in the same wave.
class FeveredEvents {
 Scope owner_;std::uint64_t last_=0;Time clock_=0;
public:
 explicit FeveredEvents(Scope owner):owner_(owner){}
 bool accept(const FeveredConditionEvent& e){if(!valid_fevered_event(e)||e.target!=owner_||e.event<=last_||e.at<clock_)return false;last_=e.event;clock_=e.at;return true;}
};
}
