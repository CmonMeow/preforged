#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include <cmath>
#include <set>
#include <span>
#include <vector>
namespace fw::gw::skills::effects {
// PvE only. The PvP variant2857 has different targeting and prevention rules.
struct AegisPlan {
 unsigned rank=0;Time duration=5000;float range=1012;double block=.5;
 bool operator==(const AegisPlan&)const=default;
};
inline std::optional<AegisPlan> aegis(unsigned rank){
 const auto* r=find_skill(257);
 if(rank>20||!r||r->profession!=3||r->attribute!=15||r->type!=6||r->special||
    r->energy_cost()!=10||r->adrenaline||r->health_sacrifice||r->activation!=2.f||
    r->aftercast!=.75f||r->recharge!=30||r->pvp_skill!=2857||r->target!=0||
    r->scale0!=50||r->scale15!=50||r->duration0!=5||r->duration15!=11||
    r->caster_body_resource!=469||r->target_body_resource!=2077)return {};
 return AegisPlan{rank,1000ull*interpolate(5,11,rank)};
}
struct AegisMember {Scope scope;double distance=0;bool living=false,same_party=false;};
// World supplies actual party membership, settled distance and liveness.
// Team allegiance alone cannot establish membership in this enchantment.
inline std::optional<std::vector<Scope>> aegis_members(const AegisPlan& plan,Scope source,
 std::span<const AegisMember> candidates){
 const auto canonical=aegis(plan.rank);
 const auto valid=[](Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;};
 if(!canonical||*canonical!=plan||!valid(source)||candidates.size()>4096)return {};
 std::set<unsigned> seen;std::vector<Scope> selected;bool has_source=false;
 for(const auto& c:candidates){
  if(!valid(c.scope)||c.scope.instance!=source.instance||!seen.insert(c.scope.actor).second||
     !std::isfinite(c.distance)||c.distance<0)return {};
  if(c.scope.actor==source.actor){if(c.scope!=source||c.distance!=0||!c.living||!c.same_party)return {};has_source=true;}
  if(c.living&&c.same_party&&c.distance<=plan.range)selected.push_back(c.scope);
 }
 if(!has_source)return {};return selected;
}
}
