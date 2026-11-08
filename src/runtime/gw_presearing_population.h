#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace fw::gw::population::presearing {
// Evidence: data/gw/npcs/presearing-eligibility-evidence.json. These rules apply
// only to the Lakeside starter placements, never identically named later NPCs.
struct Starter {unsigned profession;std::uint32_t quest;std::string_view name;};
inline constexpr std::array<Starter,6> starters{{
    {1,85,"Van the Warrior"},{2,86,"Artemis the Ranger"},{3,84,"Ciglo the Monk"},
    {4,82,"Verata the Necromancer"},{5,81,"Sebedoh the Mesmer"},{6,83,"Howland the Elementalist"}
}};
enum class TestStage {unknown,not_started,active,objective_complete,rewarded};
// Caller owns movement/arrival and supplies this stage only after a committed
// reward. Captures show trainer and Haversdan coexisting during the walk; a
// reward must not immediately delete the departing trainer. On fresh entry use
// settled. No fixed timer or inferred retail path is encoded here.
enum class Handoff {settled,walking};
struct Member {unsigned primary=0;TestStage test=TestStage::unknown;Handoff handoff=Handoff::settled;};
enum class PartyPolicy {require_explicit_authority,selected_member,union_members};
struct Authority {PartyPolicy policy=PartyPolicy::require_explicit_authority;std::size_t selected=0;};
enum class Decision {unmanaged,hidden,visible,unresolved};

inline const Starter* starter(std::string_view name){
    for(const auto& entry:starters)if(entry.name==name)return &entry;
    return nullptr;
}
inline Decision for_member(std::string_view name,const Member& member){
    const auto* entry=starter(name);
    if(!entry&&name!="Haversdan")return Decision::unmanaged;
    if(member.primary<1||member.primary>6||member.test==TestStage::unknown)return Decision::unresolved;
    if(name=="Haversdan")return member.test==TestStage::rewarded?Decision::visible:Decision::hidden;
    if(entry->profession!=member.primary)return Decision::hidden;
    return member.test!=TestStage::rewarded||member.handoff==Handoff::walking?
        Decision::visible:Decision::hidden;
}
inline Decision resolve(unsigned map,std::string_view name,std::span<const Member> members,Authority authority={}){
    if(map!=146||(!starter(name)&&name!="Haversdan"))return Decision::unmanaged;
    if(members.empty())return Decision::unresolved;
    if(authority.policy==PartyPolicy::selected_member){
        if(authority.selected>=members.size())return Decision::unresolved;
        return for_member(name,members[authority.selected]);
    }
    if(members.size()==1)return for_member(name,members.front());
    // Solo capture evidence does not establish leader precedence in a party.
    if(authority.policy==PartyPolicy::require_explicit_authority)return Decision::unresolved;
    bool unknown=false;
    for(const auto& member:members){
        const auto result=for_member(name,member);
        if(result==Decision::visible)return result;
        unknown|=result==Decision::unresolved;
    }
    return unknown?Decision::unresolved:Decision::hidden;
}
}
