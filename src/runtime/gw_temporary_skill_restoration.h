#pragma once
#include "gw_temporary_skill_slots.h"
#include "gw_cooldown_bar_authority.h"
namespace fw::gw::skills::effects {
struct CopyRestorationCooldown {
 CooldownStamp stamp;skills::Slot slot;Time until=0;std::uint64_t generation=0;
 EchoRestorationKind kind=EchoRestorationKind::observed_cooldown;
 bool operator==(const CopyRestorationCooldown&)const=default;
};
struct CopyRestorationPlan {CooldownBarAuthority authority;std::vector<CopyRestorationCooldown> cooldowns;};
inline std::optional<CopyRestorationPlan> prepare_copy_restoration(const CooldownBarAuthority& authority,
 Scope owner,const skills::Slots& before,const skills::Slots& after,
 const std::vector<TemporarySkillRestoration>& changes,Time now){
 const auto current=authority.snapshot(owner);
 if(!current||current->bar!=before||current->published.at>now||changes.size()>8)return {};
 auto expected=before;std::set<unsigned> indices;std::vector<CopyRestorationCooldown> timers;
 for(const auto& change:changes){
  const auto& grant=change.grant;
  if(grant.index>=before.size()||!indices.insert(grant.index).second||!grant.generation||grant.applied>=grant.expires||now<grant.expires)return {};
  if(!change.restored)continue;
  if(before[grant.index]!=grant.replacement||grant.original==grant.replacement)return {};
  expected[grant.index]=grant.original;
  if(!grant.restore_cooldown){if(!inspired_hex(grant.original.skill,0))return {};continue;}
  const auto rule=echo(grant.original.skill);const auto mimicry=arcane_mimicry();
  const auto duration=grant.original.skill==65&&mimicry?Time(mimicry->restoration_disable)*1000:rule?rule->restore_cooldown:0;
  const auto kind=grant.original.skill==65?EchoRestorationKind::independent_disable:rule?rule->restoration:EchoRestorationKind::observed_cooldown;
  if(!duration||grant.restore_cooldown!=duration||grant.restoration!=kind||
     grant.expires>UINT64_MAX-grant.restore_cooldown)return {};
  const auto until=grant.expires+grant.restore_cooldown;
  if(until>now)timers.push_back({{},grant.original,until,grant.generation,grant.restoration});
 }
 if(expected!=after)return {};
 auto next=authority;
 if(before!=after&&!next.publish(owner,after,now))return {};
 for(auto& timer:timers){const auto stamp=next.order(owner,now);if(!stamp)return {};timer.stamp=*stamp;}
 return CopyRestorationPlan{std::move(next),std::move(timers)};
}
template<class Events> inline std::optional<Time> copy_restoration_disabled(const Events& events,skills::Slot slot,Time now){
 Time until=0;
 for(const auto& event:events){
  if(event.stamp.at>now)return {};
  if(event.slot==slot&&event.kind==EchoRestorationKind::independent_disable&&event.until>now)until=std::max(until,event.until);
 }
 return until;
}
class CopyRestorationReceipt {
 friend class fw::gw::world::Instance;
 Scope owner_;std::vector<CopyRestorationCooldown> cooldowns_;
 CopyRestorationReceipt(Scope owner,std::vector<CopyRestorationCooldown> cooldowns):owner_(owner),cooldowns_(std::move(cooldowns)){}
public:
 Scope owner()const{return owner_;}
 const std::vector<CopyRestorationCooldown>& cooldowns()const{return cooldowns_;}
};
}
