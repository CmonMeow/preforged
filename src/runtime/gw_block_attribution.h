#pragma once
#include "gw_blocking.h"
#include <functional>
#include <set>
#include <utility>
namespace fw::gw::skills::effects {
// Empty owner denotes the recipient registry namespace; environmental sources
// carry the full caster lifetime so equal numeric UI IDs cannot alias.
struct BlockSource {unsigned skill=0,instance=0;double probability=0;Scope owner{};};
struct AttributedBlock {
 combat::weapon::AttackOutcome outcome;
 std::optional<BlockSource> source;
 unsigned draws=0;
};
// Partition a single uniform draw into each source's unconditional blocking
// mass. This preserves the existing aggregate block decision and one-draw
// contract while retaining which effect caused it. Source order is supplied
// explicitly; retail ordering between different effects remains unverified.
inline std::optional<AttributedBlock> resolve_attributed_block_draw(
 std::span<const BlockSource> sources,double draw){
 if(!std::isfinite(draw)||draw<0||draw>=1)return {};
 double remaining=1,threshold=0;
 std::set<std::pair<Scope,unsigned>> instances;
 std::optional<BlockSource> selected;
 for(const auto& source:sources){
  if(!source.skill||!source.instance||!std::isfinite(source.probability)||
     source.probability<0||source.probability>1||!instances.insert({source.owner,source.instance}).second)return {};
  threshold+=remaining*source.probability;
  remaining*=1-source.probability;
  if(!selected&&draw<threshold)selected=source;
 }
 return AttributedBlock{selected?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit,selected,1};
}

inline std::optional<std::vector<BlockSource>> reviewed_block_sources(std::span<const Entry> entries,Time now,std::optional<ShieldEquipment> equipment={},std::optional<unsigned> strength={},unsigned attack_skill=0,std::optional<Entry> owned_warding={}){
 std::vector<BlockSource> sources;
 const auto shield=shield_stance_protection(entries,now,equipment,strength);if(!shield)return {};
 if(shield->block)sources.push_back({378,shield->instance,shield->block});
 const auto wary=wary_would_block(entries,attack_skill,now);if(!wary)return {};
 const auto bash=shield_bash_protection(entries,now,equipment,attack_skill);if(!bash)return {};
 if(bash->instance)sources.push_back({363,bash->instance,1.});
 const auto frenzied=frenzied_defense_active(entries,now);if(!frenzied)return {};
 if(*frenzied){const Entry* chosen=nullptr;
  for(const auto& e:entries)if(e.skill==1700&&e.applied<=now&&e.expires&&now<*e.expires&&
      (!chosen||std::tie(e.applied,e.instance)>std::tie(chosen->applied,chosen->instance)))chosen=&e;
  if(chosen)sources.push_back({1700,chosen->instance,.75});
 }
 // One source per skill family, matching the aggregate stacking policy.
 for(unsigned family:{257u,258u,259u,453u,448u,11u,345u,372u,380u,450u,377u,1727u}){
  const Entry* selected=nullptr;
  for(const auto& entry:entries){
   const bool matches=family==257?entry.skill==257:family==258?entry.skill==258:family==259?entry.skill==259:family==453?(entry.skill==453||entry.skill==3141):family==11?entry.skill==11:family==345?entry.skill==345:family==372?entry.skill==372:family==380?entry.skill==380:family==450?entry.skill==450:family==377?entry.skill==377:family==1727?entry.skill==1727:(entry.skill==448||entry.skill==3060);
   if(!matches||!entry.instance||!entry.cause||entry.applied>now||!entry.expires||now>=*entry.expires||
      entry.kind!=((family==257||family==258||family==259)?Category::enchantment:Category::stance))continue;
   if((family==257&&!aegis(entry.rank))||(family==258&&!guardian_block(entry.rank))||(family==259&&!shield_deflection(entry.rank))||(family==453&&!lightning_reflexes(entry.skill,entry.rank))||
      (family==448&&!escape(entry.skill,entry.rank))||(family==11&&!distortion(entry.rank))||(family==345&&!defensive_stance(entry.rank))||(family==372&&!gladiator_defense(entry.rank))||(family==380&&!bonetti_defense(entry.rank))||(family==450&&!whirling_defense(entry.rank))||(family==377&&!wary_stance(entry.rank))||(family==1727&&!natural_stride_entry(entry,now)))return {};
   if(!selected||std::tie(entry.applied,entry.instance)>std::tie(selected->applied,selected->instance))selected=&entry;
  }
  if(selected&&(family!=377||*wary))sources.push_back({selected->skill,selected->instance,family==377?1.:(family==257||family==258||family==1727)?.5:.75});
 }
 if(owned_warding)sources.push_back({owned_warding->skill,owned_warding->instance,.5});
 return sources;
}
// The caller supplies an explicit source order. This does not establish retail
// ordering among different effects. Preserve attribution rather than collapsing
// every block into a probability and charging all reactive block sources.
inline std::optional<AttributedBlock> resolve_attributed_block(
 combat::weapon::AttackOutcome earlier,std::span<const BlockSource> sources,
 bool unblockable,std::span<const Entry> defender,Time now,
 const std::function<double(const BlockSource&)>& server_draw){
 std::set<std::pair<Scope,unsigned>> instances;
 for(const auto& source:sources){
  if(!source.skill||!source.instance||!std::isfinite(source.probability)||
     source.probability<0||source.probability>1||!instances.insert({source.owner,source.instance}).second)return {};
 }
 using Outcome=combat::weapon::AttackOutcome;
 AttributedBlock result{earlier,{},0};
 if(earlier!=Outcome::hit||unblockable||rigor_mortis_prevents_block(defender,now))return result;
 for(const auto& source:sources){
  if(source.probability==0)continue;
  if(!server_draw)return {};
  const auto draw=server_draw(source);++result.draws;
  if(!std::isfinite(draw)||draw<0||draw>=1)return {};
  if(draw<source.probability){result.outcome=Outcome::block;result.source=source;return result;}
 }
 return result;
}
}


