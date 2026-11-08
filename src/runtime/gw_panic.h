#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <set>
namespace fw::gw::skills::effects {
struct PanicPlan {unsigned skill=52,rank=0;Time duration=1000;float range=1248,apply_radius=252,reaction_radius=252;};
inline std::optional<PanicPlan> panic(unsigned rank){
 const auto* row=find_skill(52);
 if(rank>20||!row||row->type!=4||row->attribute!=2||!row->elite()||row->duration0!=1||row->duration15!=10)return {};
 return PanicPlan{52,rank,Time(interpolate(1,10,rank))*1000};
}
struct PanicUse {Scope owner;unsigned skill=0;std::uint64_t event=0;Time began=0,used=0;bool success=false;};
inline bool valid_panic_use(const PanicUse& use){
 const auto* row=find_skill(use.skill);
 return use.owner.instance&&use.owner.actor&&use.owner.actor<=65535&&use.owner.generation&&use.event&&use.skill&&row&&row->type&&use.used>=use.began;
}
inline std::optional<PanicPlan> panic_reaction(const PanicUse& use,const Entry& hex){
 if(!valid_panic_use(use)||!use.success||hex.skill!=52||hex.kind!=Category::hex||!hex.instance||!hex.cause||hex.cause==use.owner.actor||
    hex.applied>use.used||!hex.expires||use.used>=*hex.expires)return {};
 return panic(hex.rank);
}
struct PanicContact {Scope owner;float distance=0;bool alive=true,foe=true,acting=false;};
struct PanicRecipient {Scope owner;bool interrupt_attempt=false;};
// 'foe' is relative to the original hex caster, not the triggering actor.
// This plans attempts; World must handle each recipient's protection. It does
// not establish ordering between the used skill's effect and the Panic trigger.
inline std::optional<std::vector<PanicRecipient>> panic_recipients(Scope trigger,const std::vector<PanicContact>& contacts){
 if(!trigger.instance||!trigger.actor||trigger.actor>65535||!trigger.generation)return {};
 std::set<std::uint32_t> seen;std::vector<PanicRecipient> result;
 for(const auto& contact:contacts){
  if(contact.owner.instance!=trigger.instance||!contact.owner.actor||contact.owner.actor>65535||!contact.owner.generation||
     !seen.insert(contact.owner.actor).second||!std::isfinite(contact.distance)||contact.distance<0)return {};
  if(contact.owner.actor==trigger.actor){if(contact.owner!=trigger)return {};continue;}
  if(contact.alive&&contact.foe&&contact.distance<=252)result.push_back({contact.owner,contact.acting});
 }
 return result;
}
// Repeated or backdated trusted use events cannot trigger a newly applied hex.
// World still must choose the authoritative pre/post-effect snapshot and tie
// the original hex caster generation to its current recipient allegiance.
class PanicEvents {
 Scope owner_;std::uint64_t last_=0;Time clock_=0;
public:
 struct Result {std::optional<PanicPlan> reaction;};
 explicit PanicEvents(Scope owner):owner_(owner){}
 std::optional<Result> observe(const PanicUse& use,std::optional<Entry> current){
  if(use.owner!=owner_||!valid_panic_use(use)||use.event<=last_||use.used<clock_)return {};
  const auto reaction=current?panic_reaction(use,*current):std::nullopt;
  last_=use.event;clock_=use.used;return Result{reaction};
 }
};
}
