#pragma once
#include "gw_creature_traits.h"
#include "gw_conditions.h"
#include "gw_effect_registry.h"
#include <cmath>
#include <string>
#include <map>
#include <set>
namespace fw::gw::combat::conditions {
struct DiseaseContactActor {
 skills::effects::Scope scope;double x=0,y=0;unsigned plane=0;bool alive=false;
 std::optional<std::string> type;std::optional<bool> fleshy,immune;
};
enum class DiseaseContact {eligible,no_contact,unknown,invalid};
// Contact eligibility only. It neither schedules a spread nor assigns its
// duration, strain identity, reinfection rule or credit to a new recipient.
inline DiseaseContact disease_contact(const DiseaseContactActor& source,const DiseaseContactActor& recipient,
 const Entry& disease,std::uint64_t now){
 const auto valid=[](const DiseaseContactActor& a){return a.scope.instance&&a.scope.actor&&a.scope.actor<=65535&&a.scope.generation&&
  std::isfinite(a.x)&&std::isfinite(a.y)&&(!a.type||creatures::valid_type(*a.type));};
 if(!valid(source)||!valid(recipient)||!disease.cause||disease.cause>65535||disease.expires<=disease.applied)return DiseaseContact::invalid;
 if(source.scope==recipient.scope||source.scope.instance!=recipient.scope.instance||!source.alive||!recipient.alive||
    source.plane!=recipient.plane||std::hypot(source.x-recipient.x,source.y-recipient.y)>166||
    now<disease.applied||now>=disease.expires)return DiseaseContact::no_contact;
 if((source.type&&recipient.type&&*source.type!=*recipient.type)||
    (source.fleshy&&!*source.fleshy)||(recipient.fleshy&&!*recipient.fleshy)||(recipient.immune&&*recipient.immune))return DiseaseContact::no_contact;
 if(!source.type||!recipient.type||!source.fleshy||!recipient.fleshy||!recipient.immune)return DiseaseContact::unknown;
 return DiseaseContact::eligible;
}
// A proposal only: World must commit through its condition reaction path.
// Preserve absolute expiry across contact chains. Existing equal/longer Disease
// is not reapplied, so repeated contacts cannot reorder covers or trigger hits.
struct DiseaseTransfer {
 DiseaseContact contact=DiseaseContact::invalid;
 bool changed=false;
 State state;
};
inline DiseaseTransfer disease_transfer(const DiseaseContactActor& source,const DiseaseContactActor& recipient,
 const Entry& disease,const State& before,std::uint64_t now){
 DiseaseTransfer result{disease_contact(source,recipient,disease,now),false,before};
 if(result.contact!=DiseaseContact::eligible)return result;
 const auto existing=before.lookup(Kind::disease);
 if(before.has(Kind::disease,now)&&existing->expires>=disease.expires)return result;
 // Keep original condition cause as proposal metadata; lifetime-bound World
 // damage credit is a separate responsibility, not inferred from actor number.
 if(!result.state.apply(Kind::disease,disease.cause,now,disease.expires-now)){
  result.contact=DiseaseContact::invalid;return result;
 }
 result.changed=true;return result;
}

struct DiseasePopulationActor {DiseaseContactActor actor;State conditions;};
struct DiseaseTransmission {
 skills::effects::Scope source,recipient;
 Entry infection;
 State after;
};
struct DiseaseContactBatch {
 std::vector<DiseaseTransmission> transmissions;
 std::size_t unknown_contacts=0;
};
// Evaluate one immutable World snapshot. Newly infected recipients become
// sources on a subsequent snapshot, never midway through this traversal.
// Longest expiry wins; equal-expiry sources are ordered by lifetime scope.
// This is deterministic private-server scheduling, not a retail cadence claim.
inline std::optional<DiseaseContactBatch> disease_contacts(
 const std::vector<DiseasePopulationActor>& population,std::uint64_t now){
 std::set<std::pair<std::uint64_t,std::uint32_t>> actors;
 for(const auto& entry:population){
  const auto& a=entry.actor;
  if(!a.scope.instance||!a.scope.actor||a.scope.actor>65535||!a.scope.generation||
     !std::isfinite(a.x)||!std::isfinite(a.y)||(a.type&&!creatures::valid_type(*a.type))||
     !actors.emplace(a.scope.instance,a.scope.actor).second)return std::nullopt;
 }
 DiseaseContactBatch result;
 std::map<skills::effects::Scope,DiseaseTransmission> chosen;
 for(const auto& source:population){
  if(!source.conditions.has(Kind::disease,now))continue;
  const auto infection=*source.conditions.lookup(Kind::disease);
  for(const auto& recipient:population){
   const auto proposal=disease_transfer(source.actor,recipient.actor,infection,recipient.conditions,now);
   if(proposal.contact==DiseaseContact::invalid)return std::nullopt;
   if(proposal.contact==DiseaseContact::unknown){++result.unknown_contacts;continue;}
   if(!proposal.changed)continue;
   const auto existing=chosen.find(recipient.actor.scope);
   if(existing!=chosen.end()&&(existing->second.infection.expires>infection.expires||
      (existing->second.infection.expires==infection.expires&&existing->second.source<source.actor.scope)))continue;
   chosen.insert_or_assign(recipient.actor.scope,DiseaseTransmission{
    source.actor.scope,recipient.actor.scope,infection,proposal.state});
  }
 }
 for(auto& [scope,transmission]:chosen)result.transmissions.push_back(std::move(transmission));
 return result;
}

}
