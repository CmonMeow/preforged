#pragma once
#include "gw_effect_registry.h"
#include "gw_aura_restoration.h"
#include "gw_skillbar.h"
namespace fw::gw::world {class Instance;}
namespace fw::gw::skills::effects {
enum class EchoRestorationKind {observed_cooldown,independent_disable};
struct EchoPlan {
 unsigned skill=74;
 Time armed_duration=30000,copy_duration=30000,restore_cooldown=10000;
 bool spells_only=false;
 EchoRestorationKind restoration=EchoRestorationKind::observed_cooldown;
 auto operator<=>(const EchoPlan&)const=default;
};
inline std::optional<EchoPlan> echo(unsigned skill){
 const auto* row=find_skill(skill);
 if((skill!=74&&skill!=75)||!row||row->type!=6||row->attribute!=51||
    row->duration0!=(skill==74?30u:20u)||row->duration15!=row->duration0||
    row->recharge!=(skill==74?10u:20u))return {};
 return EchoPlan{skill,Time(row->duration0)*1000,Time(row->duration0)*1000,Time(row->recharge)*1000,skill==75,skill==75?EchoRestorationKind::independent_disable:EchoRestorationKind::observed_cooldown};
}
struct EchoUseProposal {
 Registry registry;
 Entry consumed;
 std::optional<unsigned> copied_skill;
 Time copy_duration=0,restore_cooldown=0;
 EchoRestorationKind restoration=EchoRestorationKind::observed_cooldown;
 Time at=0;
 skills::Slot source,used;
 std::uint64_t completion_event=0;
};
class EchoConsumption {
 friend class fw::gw::world::Instance;
 Scope owner_;Entry consumed_;skills::Slot source_,used_;Time at_;
 std::optional<unsigned> copied_;
 EchoConsumption(Scope owner,const EchoUseProposal& proposal):owner_(owner),consumed_(proposal.consumed),source_(proposal.source),used_(proposal.used),at_(proposal.at),copied_(proposal.copied_skill){}
public:
 Scope owner()const{return owner_;}const Entry& consumed()const{return consumed_;}
 skills::Slot source()const{return source_;}skills::Slot used()const{return used_;}
 Time at()const{return at_;}std::optional<unsigned> copied_skill()const{return copied_;}
};
// This observer accepts a server-confirmed completed use, not a client request.
// Cancel/interruption and other failure phases require separate evidence and
// integration. Slot replacement/allocation and independent restoration-disable
// publication belong to the shared temporary-slot transaction, not this plan.
inline std::optional<EchoUseProposal> prepare_echo_completed_use(const Registry& original,
 Scope owner,skills::Slot echo_slot,skills::Slot completed,Time now){
 const auto plan=echo(echo_slot.skill);const auto* used=find_skill(completed.skill);
 if(!plan||!used||!completed.skill||!used->type||!owner.instance||!owner.actor||!owner.generation||
    !original.matches(owner)||completed.skill==echo_slot.skill||
    (plan->skill==74&&completed.skill==3))return {}; // Reviewed self/Capture exclusions.
 const auto entry=original.newest_skill(owner,echo_slot.skill,now);
 if(!entry||entry->kind!=Category::enchantment||entry->cause!=owner.actor||!entry->instance||
    entry->applied>now||!entry->expires||now>=*entry->expires)return {};
 const bool copy=!plan->spells_only||is_spell(*used);
 if(copy&&now>UINT64_MAX-plan->copy_duration)return {};
 auto registry=original;
 if(!registry.remove(owner,entry->instance))return {};
 return EchoUseProposal{std::move(registry),*entry,copy?std::optional<unsigned>{completed.skill}:std::nullopt,
     copy?plan->copy_duration:0,copy?plan->restore_cooldown:0,plan->restoration,now,echo_slot,completed};
}
}
