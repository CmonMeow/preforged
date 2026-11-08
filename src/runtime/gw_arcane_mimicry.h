#pragma once
#include "gw_skillbar.h"
#include "gw_cooldown_bar_authority.h"
#include <optional>
namespace fw::gw::world {class Instance;}
namespace fw::gw::skills::effects {
struct ArcaneMimicryPlan {
    unsigned energy=15,recharge=60,copy_seconds=20,restoration_disable=60;
    float activation=2,aftercast=.75f;
    float range=1248;
};
inline std::optional<ArcaneMimicryPlan> arcane_mimicry(){
    const auto* row=find_skill(65);
    if(!row||row->type!=5||row->profession!=5||row->attribute!=51||row->elite()||row->energy_cost()!=15||
       row->activation!=2||row->aftercast!=.75f||row->recharge!=60||row->target!=4||row->equip_type!=1||
       row->duration0!=20||row->duration15!=20||row->caster_body_resource!=117||row->target_body_resource!=118)return {};
    return ArcaneMimicryPlan{};
}
enum class MimicrySelection {copy,no_elite,form,unsupported_elite,invalid_bar};
struct MimicryDonor {
    MimicrySelection status=MimicrySelection::no_elite;
    std::optional<Slot> slot;unsigned index=0;
    bool operator==(const MimicryDonor&)const=default;
};
// Trusted published target bar only. World must separately prove other-ally
// scope/liveness/range and retain this exact donor identity/revision. Selection
// grants neither skill ownership nor a copy ID, and consumes no cooldown.
inline MimicryDonor select_mimicry_donor(const Slots& bar){
    for(const auto slot:bar)if(!find_skill(slot.skill)||slot.copy>65535||(!slot.skill&&slot.copy))return {MimicrySelection::invalid_bar};
    for(unsigned index=0;index<bar.size();++index){
        const auto* row=find_skill(bar[index].skill);
        if(!row->elite())continue;
        if(row->type==26)return {MimicrySelection::form,bar[index],index};
        if(!row->type)return {MimicrySelection::unsupported_elite,bar[index],index};
        return {MimicrySelection::copy,bar[index],index};
    }
    return {};
}
// World-issued snapshot, not a client-provided grant. A session must revalidate
// it at the same completion frontier before allocating/publishing a copy.
class MimicryDonorLease {
    friend class fw::gw::world::Instance;
    Scope owner_,target_;Slot source_;CooldownBarSnapshot owner_bar_,target_bar_;
    MimicryDonor donor_;std::uint64_t at_;
    std::uint64_t owner_death_=0,target_death_=0;
    MimicryDonorLease(Scope owner,Scope target,Slot source,CooldownBarSnapshot owner_bar,
            CooldownBarSnapshot target_bar,MimicryDonor donor,std::uint64_t at,std::uint64_t owner_death,std::uint64_t target_death):
        owner_(owner),target_(target),source_(source),owner_bar_(owner_bar),target_bar_(target_bar),donor_(donor),at_(at),owner_death_(owner_death),target_death_(target_death){}
public:
    Scope owner()const{return owner_;}Scope target()const{return target_;}
    Slot source()const{return source_;}std::uint64_t at()const{return at_;}
    const MimicryDonor& donor()const{return donor_;}
    const CooldownBarSnapshot& owner_bar()const{return owner_bar_;}
    const CooldownBarSnapshot& target_bar()const{return target_bar_;}
    bool operator==(const MimicryDonorLease&)const=default;
};
class MimicryCopyReceipt {
    friend class fw::gw::world::Instance;
    MimicryDonorLease lease_;Slot replacement_;
    MimicryCopyReceipt(MimicryDonorLease lease,Slot replacement):lease_(lease),replacement_(replacement){}
public:
    const MimicryDonorLease& lease()const{return lease_;}Slot replacement()const{return replacement_;}
};
}
