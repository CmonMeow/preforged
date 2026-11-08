#pragma once
// Detached proposal: pure admission facts, never a spawn/metadata inference API.
#include "gw_npc_ordinary_ai.h"
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <set>
#include <span>
namespace fw::gw::combat::ai::owned_ordinary {
using Scope=skills::effects::Scope;
enum class ObjectiveKind {kill_all,kill_any,loot};
struct Profile {
    unsigned damage=5,mastery=0,damage_type=0;
    std::uint64_t windup=750,period=2000;
    double reach=128,speed=220,acquire=1000,follow=1500,leash=2000,patrol_radius=80;
    std::uint64_t pause=1200,leg_timeout=6000;
    std::string evidence;
    bool operator==(const Profile&)const=default;
    bool valid()const{
        return ordinary::Reviewed{damage,damage,mastery,damage_type,false,ordinary::Delivery::melee,
            windup,period,reach,{100,32},evidence}.valid()&&
            std::isfinite(speed)&&speed>0&&speed<=440&&std::isfinite(acquire)&&acquire>0&&
            std::isfinite(follow)&&follow>=acquire&&follow<=10000&&std::isfinite(leash)&&leash>0&&leash<=10000&&
            std::isfinite(patrol_radius)&&patrol_radius>0&&patrol_radius<leash&&pause&&pause<=60000&&leg_timeout&&leg_timeout<=300000;
    }
};
struct Descriptor {
    unsigned quest=0,map=0,members=0,model=0,flags=0,level=0,maximum_health=0;
    ObjectiveKind kind=ObjectiveKind::kill_all;
    std::array<int,6> armor{};
    Profile profile;
    std::string capture_evidence;
    bool valid()const{return quest&&map&&members&&members<=64&&model&&level<=255&&maximum_health&&
        (kind==ObjectiveKind::kill_all||kind==ObjectiveKind::kill_any)&&profile.valid()&&!capture_evidence.empty();}
};
// Factory is an explicit reviewed quest allowlist. No ambient model auto-arm.
// Quest83 remains unsupported: its current three anchors are authored and
// neither exact captured membership nor this kill-objective policy applies.
inline std::optional<Descriptor> captured_tads(unsigned quest){
    if(quest!=81&&quest!=85)return {};
    Descriptor d;d.quest=quest;d.map=146;d.members=quest==81?4:3;d.model=82023;d.flags=8;
    d.level=0;d.maximum_health=10;d.kind=quest==81?ObjectiveKind::kill_any:ObjectiveKind::kill_all;
    d.profile.evidence=quest==81?"data/gw/quests/mesmer-test-development-policy.txt":"data/gw/quests/warrior-test-development-policy.txt";
    d.capture_evidence=quest==81?"docs/npc-quest81-retaliation-evidence-20261029.json":"docs/npc-quest85-83-enrollment-evidence-20261029.txt";
    return d;
}
struct Objective {
    Scope owner;
    unsigned quest=0,cycle=0,map=0;
    ObjectiveKind kind=ObjectiveKind::kill_all;
    std::set<Scope> assigned,remaining;
};
struct Member {
    Scope source,assigned_owner;
    unsigned assigned_quest=0,assigned_cycle=0,map=0;
    unsigned actor=0,definition=0,model=0,flags=0,level=0;
    std::optional<unsigned> maximum_health;
    std::optional<double> current_health;
    std::array<std::optional<int>,6> armor;
    bool hostile=false;
    std::uint64_t health_clock=0,controller_clock=0;
    // World must populate these from exact current controller authority, not
    // caller assertions. Existing controller profile is compared in full.
    bool controller_present=false,controller_owned=false;
    Scope controller_source,controller_owner;
    unsigned controller_quest=0,controller_cycle=0;
    std::optional<Profile> installed_profile;
};
enum class Refusal {none,descriptor,owner,objective,roster,identity,unknown_combat,dead,future,controller};
struct Admission {Refusal refusal=Refusal::none;std::size_t members=0;explicit operator bool()const{return refusal==Refusal::none;}};
inline Admission validate(const Descriptor& d,const Objective& o,Scope requested_owner,unsigned requested_cycle,
        std::span<const Member> facts,std::uint64_t now){
    if(!d.valid())return {Refusal::descriptor};
    if(!ordinary::valid_scope(requested_owner)||o.owner!=requested_owner||!requested_cycle)return {Refusal::owner};
    if(o.quest!=d.quest||o.map!=d.map||o.cycle!=requested_cycle||o.kind!=d.kind)return {Refusal::objective};
    if(o.assigned.size()!=d.members||o.remaining.empty()||facts.size()!=o.remaining.size())return {Refusal::roster};
    for(const auto s:o.assigned)if(!ordinary::valid_scope(s)||s.instance!=o.owner.instance||s==o.owner)return {Refusal::roster};
    for(const auto s:o.remaining)if(!o.assigned.contains(s))return {Refusal::roster};
    if(d.kind==ObjectiveKind::kill_any&&o.remaining!=o.assigned)return {Refusal::roster};
    std::set<Scope> seen;
    for(const auto& m:facts){
        if(!o.remaining.contains(m.source)||!seen.insert(m.source).second)return {Refusal::roster};
        if(m.assigned_owner!=o.owner||m.assigned_quest!=o.quest||m.assigned_cycle!=o.cycle||m.map!=d.map||
           m.actor!=m.source.actor||m.definition!=m.source.actor||m.model!=d.model||m.flags!=d.flags||m.level!=d.level)return {Refusal::identity};
        if(!m.maximum_health||*m.maximum_health!=d.maximum_health||!m.current_health||!std::isfinite(*m.current_health)||
           *m.current_health>*m.maximum_health||!m.hostile)return {Refusal::unknown_combat};
        for(unsigned i=0;i<6;++i)if(!m.armor[i]||*m.armor[i]!=d.armor[i])return {Refusal::unknown_combat};
        if(*m.current_health<=0)return {Refusal::dead};
        if(m.health_clock>now||m.controller_clock>now)return {Refusal::future};
        if(m.controller_present&&(!m.controller_owned||m.controller_source!=m.source||m.controller_owner!=o.owner||
           m.controller_quest!=o.quest||m.controller_cycle!=o.cycle||!m.installed_profile||*m.installed_profile!=d.profile))return {Refusal::controller};
    }
    return {Refusal::none,facts.size()};
}
}

