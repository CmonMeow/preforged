#pragma once
#include "gw_effect_registry.h"
#include "gw_skillbar.h"
#include <cmath>
namespace fw::gw::skills::effects {
struct DiversionPlan {
 unsigned rank=0;
 Time duration=6000,additional_recharge=10000;
 auto operator<=>(const DiversionPlan&)const=default;
};
inline std::optional<DiversionPlan> diversion(unsigned rank){
 const auto* row=find_skill(30);
 if(rank>20||!row||row->type!=4||row->attribute!=2||row->duration0!=6||row->duration15!=6||row->scale0!=10||row->scale15!=56)return {};
 return DiversionPlan{rank,6000,static_cast<Time>(std::lround(10.+46.*rank/15.))*1000};
}
struct DiversionUse {
 Scope owner;std::uint64_t event=0;skills::Slot slot{};
 // Supplied by authoritative skill resolution, not the client's request.
 bool skill_used=false,instant_recharge=false;
};
struct DiversionUseProposal {
 Registry registry;Entry consumed;skills::Slot slot;
 std::uint64_t event=0;Time additional_recharge=0;
};
// Caller stages this before the used skill removes hexes, but only after it
// has established a real skill-use event. Activation/cancellation alone is not
// enough. World must validate the original hex caster's lifetime separately.
inline std::optional<DiversionUseProposal> diversion_use(const Registry& original,
 const DiversionUse& use,Time now){
 if(!use.owner.instance||!use.owner.actor||use.owner.actor>65535||!use.owner.generation||
    !use.event||!use.slot.skill||!find_skill(use.slot.skill)||!use.skill_used)return {};
 const auto entry=original.newest_skill(use.owner,30,now);
 if(!entry||entry->kind!=Category::hex||!entry->instance||!entry->cause||entry->cause==use.owner.actor||
    entry->applied>now||!entry->expires||now>=*entry->expires)return {};
 const auto plan=diversion(entry->rank);if(!plan)return {};
 auto registry=original;if(!registry.remove(use.owner,entry->instance))return {};
 const bool resets=use.instant_recharge||use.slot.skill==22||use.slot.skill==1049;
 return DiversionUseProposal{std::move(registry),*entry,use.slot,use.event,resets?0:plan->additional_recharge};
}
// Receives the final base recharge from casting. Modifier ordering relative
// to Diversion remains an integration concern; this helper does not decide it.
inline std::optional<Time> diversion_recharge_deadline(Time now,Time base,Time additional,Time existing){
 if(base>86400000||additional>71000||base>UINT64_MAX-additional||now>UINT64_MAX-base-additional)return {};
 return std::max(existing,now+base+additional);
}
}
