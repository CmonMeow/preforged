#pragma once
#include "gw_keystone_signet.h"
namespace fw::gw::skills::effects {
struct KeystoneApplication {
 Registry registry;KeystoneCharges charges;Entry entry;Applied change;
};
inline std::optional<KeystoneEffect> owned_keystone(const Registry& registry,
 const KeystoneCharges& charges,Scope owner,Time now){
 const auto effect=charges.snapshot();if(!valid_keystone_scope(owner)||!registry.matches(owner)||!effect||
    effect->owner!=owner||now<effect->applied||now>=effect->expires)return {};
 const auto entry=registry.lookup(owner,effect->effect);
 if(!entry||entry->skill!=63||entry->kind!=Category::other||entry->cause!=owner.actor||
    entry->rank!=effect->rank||entry->applied!=effect->applied||entry->expires!=effect->expires)return {};
 return effect;
}
// Transaction-local state only. The caller must commit this together with
// World disable and Runtime recharge resets before publishing body112/timers.
inline std::optional<KeystoneApplication> prepare_keystone_application(
 const Registry& original,const KeystoneCharges& current,Scope owner,
 unsigned effect,std::uint64_t application,const KeystonePlan& plan,Time now){
 const auto canonical=keystone_signet(plan.rank);
 if(!valid_keystone_scope(owner)||!effect||!canonical||*canonical!=plan||
    !original.matches(owner)||original.lookup(owner,effect)||now>UINT64_MAX-plan.duration)return {};
 auto charges=current;if(!charges.apply(owner,plan,effect,application,now))return {};
 Entry entry{effect,owner.actor,63,plan.rank,Category::other,now,now+plan.duration};
 auto registry=original;auto change=registry.apply(owner,entry,Replace::same_skill);
 if(!change.accepted)return {};
 return KeystoneApplication{std::move(registry),std::move(charges),entry,std::move(change)};
}
}
