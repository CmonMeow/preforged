#pragma once
#include "gw_npc_action_control.h"

namespace fw::gw::combat::ai::failed_target {
using Scope=skills::effects::Scope;
enum class Status {unsupported,deferred,valid,failed};
enum class Reason {none,missing,replaced,unpublished,dead,plane,line_of_sight};
struct Observation {
    Scope selected;
    std::optional<Scope> current;
    bool published=false;
    std::optional<bool> alive,same_plane,line_of_sight;
    std::uint64_t clock=0;
};
struct Decision {Status status=Status::unsupported;Reason reason=Reason::none;};
// Deliberately bounded to Immolate, Flare and Ice Spear. Fireball has an explicit documented
// dead-primary exception. This models the existing server completion-time target
// validation, not a measured retail timing rule or an interruption.
inline Decision at_completion(const npcs::control::Token& token,const Observation& target,std::uint64_t now){
    const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
    if(token.kind!=npcs::control::Action::spell||!token.skill||
       (token.skill->skill!=191&&token.skill->skill!=194&&token.skill->skill!=214)||!valid(token.actor)||!valid(target.selected)||
       token.actor.instance!=target.selected.instance||!token.sequence||token.completes<=token.begins)return {};
    if(now<token.completes||now<target.clock)return {Status::deferred};
    if(!target.current)return {Status::failed,Reason::missing};
    if(!valid(*target.current))return {};
    if(*target.current!=target.selected)return {Status::failed,Reason::replaced};
    if(!target.published)return {Status::failed,Reason::unpublished};
    if(!target.alive)return {Status::deferred};
    if(!*target.alive)return {Status::failed,Reason::dead};
    if(!target.same_plane)return {Status::deferred};
    if(!*target.same_plane)return {Status::failed,Reason::plane};
    if(!target.line_of_sight)return {Status::deferred};
    if(!*target.line_of_sight)return {Status::failed,Reason::line_of_sight};
    return {Status::valid};
}
}
