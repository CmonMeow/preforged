#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
namespace fw::gw::skills::effects {
struct RecallPlan {unsigned skill=82,rank=0,energy=10;Time duration=20000;};
inline std::optional<RecallPlan> mantra_recall(unsigned rank){
 const auto* row=find_skill(82);
 if(rank>20||!row||row->type!=6||row->attribute!=3||!row->elite()||row->scale0!=10||row->scale15!=25||row->duration0!=20||row->duration15!=20)return {};
 return RecallPlan{82,rank,interpolate(10,25,rank),20000};
}
enum class RecallEnd {expiration,removal,renewal};
struct RecallReward {Scope owner;unsigned effect=0,energy=0;Time at=0;};
struct RecallEndProposal {Registry registry;Entry ended;std::optional<RecallReward> reward;};
// Timer/removal owner supplies the explicit cause. Renewal is not an end
// reward. Use the authoritative stored expiry (possibly duration-modified),
// not the late polling time. World must commit once and settle Energy at that
// event time, including debt/capacity and current actor lifetime/alive state.
inline std::optional<RecallEndProposal> prepare_recall_end(const Registry& original,Scope owner,
 unsigned effect,RecallEnd reason,Time now){
 if(!original.matches(owner)||!owner.actor||owner.actor>65535||
    (reason!=RecallEnd::expiration&&reason!=RecallEnd::removal&&reason!=RecallEnd::renewal))return {};
 const auto entry=original.lookup(owner,effect);
 if(!entry||entry->skill!=82||entry->kind!=Category::enchantment||entry->cause!=owner.actor||!entry->expires||now<entry->applied)return {};
 const auto plan=mantra_recall(entry->rank);if(!plan)return {};
 if(reason==RecallEnd::expiration?now<*entry->expires:now>=*entry->expires)return {};
 auto registry=original;if(!registry.remove(owner,effect))return {};
 std::optional<RecallReward> reward;
 if(reason!=RecallEnd::renewal)reward=RecallReward{owner,effect,plan->energy,reason==RecallEnd::expiration?*entry->expires:now};
 return RecallEndProposal{std::move(registry),*entry,reward};
}
}
