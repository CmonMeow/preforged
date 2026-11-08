#pragma once
#include "gw_effect_registry.h"
#include "gw_skill_effect_plan.h"
#include "gw_recall.h"
#include "gw_succor.h"
#include <map>
#include <algorithm>
#include <set>
namespace fw::gw::skills::effects {
struct BlessedSignetPlan {
 unsigned rank=0,per_enchantment=3,maximum=3;
 Time activation=2000,aftercast=750,recharge=10000;
 auto operator<=>(const BlessedSignetPlan&)const=default;
};
inline std::optional<BlessedSignetPlan> blessed_signet(unsigned rank){
 const auto* row=find_skill(297);
 if(rank>20||!row||row->profession!=3||row->attribute!=16||row->type!=7||row->special||row->energy_cost()||row->health_sacrifice||row->adrenaline||row->activation!=2.f||row->aftercast!=.75f||row->recharge!=10||row->scale0!=3||row->scale15!=24||row->bonus0!=3||row->bonus15!=3||row->caster_body_resource!=523||row->target_body_resource!=2077)return {};
 return BlessedSignetPlan{rank,3,interpolate(3,24,rank)};
}
// Server-owned upkeep ownership,not a count of buffs on the caster. The
// reviewed producers are Mending290 and self-owned Recall925. Recall's ally
// anchor is separate from this upkeep recipient. World end handlers must be
// installed before its new producer is exposed to normal casting.
class MaintainedEnchantments {
 struct Link {Scope caster,target;unsigned effect=0,skill=0;Time applied=0;std::uint64_t order=0;};
 std::map<std::pair<Scope,unsigned>,Link> links_;
 std::map<Scope,Time> clocks_;
 std::map<Scope,unsigned> last_effect_;
 bool chronological(Scope owner,Time now)const{const auto found=clocks_.find(owner);return found==clocks_.end()||now>=found->second;}
 std::uint64_t order_=0,revision_=0;
 static bool valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
 bool add_reviewed(Scope caster,Scope target,unsigned effect,const Registry& registry,Time now,unsigned skill){
  if(skill!=290&&skill!=925&&skill!=308&&skill!=248)return false;
  if(order_==UINT64_MAX||revision_==UINT64_MAX||(!chronological(caster,now)||!chronological(target,now))||!valid(caster)||!valid(target)||caster.instance!=target.instance||!effect||links_.size()>=4096)return false;
  if(const auto last=last_effect_.find(caster);last!=last_effect_.end()&&effect<=last->second)return false;
  const auto entry=registry.lookup(target,effect);
  if(!entry||entry->skill!=skill||entry->kind!=Category::enchantment||entry->cause!=caster.actor||entry->expires||entry->applied!=now||entry->rank>20)return false;
  if(skill==308&&(!succor()||caster.actor==target.actor||entry->rank))return false;
  if(skill==925&&(!recall_shadow()||caster!=target||entry->rank))return false;
  for(const auto& [key,link]:links_)if(link.caster==caster&&(link.effect==effect||(link.target==target&&link.skill==skill)))return false;
  const bool inserted=links_.emplace(std::pair{target,effect},Link{caster,target,effect,skill,entry->applied,order_+1}).second;
  if(inserted){last_effect_[caster]=effect;clocks_[caster]=now;++order_;++revision_;}return inserted;
 }
public:
 // Staged Retribution authority; World end/damage consumers must be installed
 // before a production cast producer can enroll this maintained effect.
 bool add_retribution(Scope caster,Scope target,unsigned effect,const Registry& registry,Time now){return add_reviewed(caster,target,effect,registry,now,248);}
 bool add_succor(Scope caster,Scope target,unsigned effect,const Registry& registry,Time now){return add_reviewed(caster,target,effect,registry,now,308);}
 bool add_mending(Scope caster,Scope target,unsigned effect,const Registry& registry,Time now){return add_reviewed(caster,target,effect,registry,now,290);}
 bool add_recall(const RecallAnchor& anchor,const Registry& registry,Time now){
  if(!recall_scope(anchor.caster)||!recall_scope(anchor.target)||anchor.caster.instance!=anchor.target.instance||
     anchor.caster.actor==anchor.target.actor||anchor.applied!=now)return false;
  return add_reviewed(anchor.caster,anchor.caster,anchor.effect,registry,now,925);
 }
 bool ended(Scope target,unsigned effect,Time now){
  const auto found=links_.find({target,effect});
  if(found==links_.end()||revision_==UINT64_MAX||!chronological(found->second.caster,now))return false;
  clocks_[found->second.caster]=now;links_.erase(found);++revision_;return true;
 }
 std::optional<std::size_t> retire(Scope actor,Time now){
  if(revision_==UINT64_MAX||!valid(actor)||!chronological(actor,now))return {};
  std::set<Scope> affected{actor};for(const auto& [key,link]:links_)if(link.caster==actor||link.target==actor)affected.insert(link.caster);
  for(const auto owner:affected)if(!chronological(owner,now))return {};
  std::size_t removed=0;for(auto it=links_.begin();it!=links_.end();)if(it->second.caster==actor||it->second.target==actor){it=links_.erase(it);++removed;}else ++it;
  for(const auto owner:affected)clocks_[owner]=now;++revision_;return removed;
 }
 // Caller must synchronize with actual target registries/removals under the
 // World lock. No time-based polling can infer an upkeep removal or death.
 std::optional<unsigned> count(Scope caster,Time now)const{
  if(!valid(caster)||!chronological(caster,now))return {};unsigned n=0;
  for(const auto& [key,link]:links_)if(link.caster==caster){if(now<link.applied)return {};++n;}
  return n;
 }
 struct Release {Scope target;unsigned effect=0;auto operator<=>(const Release&)const=default;};
 struct UpkeepProposal {
  Scope caster;Time at=0;std::uint64_t revision=0;int external_pips=0,pips=0;bool empty_energy=false;std::vector<Release> releases;
  auto operator<=>(const UpkeepProposal&)const=default;
 };
 std::optional<UpkeepProposal> prepare_upkeep(Scope caster,int external_pips,bool empty_energy,Time now)const{
  const auto owned=count(caster,now);if(!owned||external_pips< -4096||external_pips>4096)return {};
  UpkeepProposal proposal{caster,now,revision_,external_pips,external_pips-int(*owned),empty_energy,{}};
  std::vector<const Link*> newest;for(const auto& [key,link]:links_)if(link.caster==caster)newest.push_back(&link);
  std::sort(newest.begin(),newest.end(),[](const auto* a,const auto* b){return a->order>b->order;});
  for(const auto* link:newest){if(proposal.pips>= (empty_energy?0:-10))break;proposal.releases.push_back({link->target,link->effect});++proposal.pips;}
  proposal.pips=std::clamp(proposal.pips,-10,10);return proposal;
 }
 // Pure ownership commit only. World must first preflight matching recipient
 // status/regeneration removal and publish the same ordered end events.
 bool commit_upkeep(const UpkeepProposal& proposal){
  if(revision_==UINT64_MAX)return false;
  const auto canonical=prepare_upkeep(proposal.caster,proposal.external_pips,proposal.empty_energy,proposal.at);
  if(!canonical||*canonical!=proposal)return false;
  for(const auto& release:proposal.releases)links_.erase({release.target,release.effect});
  if(!proposal.releases.empty()){clocks_[proposal.caster]=proposal.at;++revision_;}return true;
 }
 std::optional<Scope> current_maintainer(Scope target,unsigned effect)const{
  const auto found=links_.find({target,effect});return found==links_.end()?std::nullopt:std::optional(found->second.caster);
 }
 std::optional<Scope> maintainer(Scope target,unsigned effect,Time now)const{
  const auto found=links_.find({target,effect});if(found==links_.end()||!chronological(found->second.caster,now))return {};return found->second.caster;
 }
 std::optional<Scope> maintained_target(Scope caster,unsigned effect,Time now)const{
  if(!valid(caster)||!effect||!chronological(caster,now))return {};
  for(const auto& [key,link]:links_)if(link.caster==caster&&link.effect==effect)return link.target;return {};
 }
 struct Owned {Scope caster,target;unsigned effect=0;};
 std::vector<Owned> pending()const{std::vector<Owned> result;for(const auto& [key,link]:links_)result.push_back({link.caster,link.target,link.effect});return result;}
 std::size_t size()const{return links_.size();}
};
inline std::optional<unsigned> blessed_signet_gain(const BlessedSignetPlan& plan,Scope caster,const MaintainedEnchantments& maintained,Time now){
 const auto canonical=blessed_signet(plan.rank);if(!canonical||*canonical!=plan)return {};
 const auto count=maintained.count(caster,now);if(!count)return {};
 return std::min(plan.maximum,*count*plan.per_enchantment);
}
}

