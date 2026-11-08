#pragma once
#include "gw_temporary_hex_admission.h"
#include <type_traits>
namespace fw::gw::skills::effects {
struct TemporarySkillSlotChange {unsigned index;skills::Slot before,after;};
class TemporarySkillSession {
 Scope owner_;skills::Slots base_,bar_;TemporaryHexSlots copies_;
 Time clock_=0;std::uint64_t revision_=1,base_epoch_=1;
 bool owns(Scope owner)const{return owner==owner_&&owner.instance&&owner.actor&&owner.actor<=65535&&owner.generation;}
 static bool same(const Entry& a,const Entry& b){return a.instance==b.instance&&a.cause==b.cause&&a.skill==b.skill&&a.rank==b.rank&&a.kind==b.kind&&a.applied==b.applied&&a.expires==b.expires;}
public:
 class PreparedMimicry {
  friend class TemporarySkillSession;
  Scope owner_;std::uint64_t revision_;MimicryDonorLease lease_;
  skills::Slots bar_;TemporarySkillSlots copies_;unsigned index_;
  PreparedMimicry(Scope owner,std::uint64_t revision,MimicryDonorLease lease,skills::Slots bar,TemporarySkillSlots copies,unsigned index):
   owner_(owner),revision_(revision),lease_(lease),bar_(bar),copies_(std::move(copies)),index_(index){}
 public:
  const auto& lease()const{return lease_;}const auto& bar()const{return bar_;}
  skills::Slot replacement()const{return bar_[index_];}
 };
 class PreparedCopy {
  friend class TemporarySkillSession;
  Scope owner_;std::uint64_t revision_;InspiredHexOffer offer_;
  skills::Slots bar_;TemporaryHexSlots copies_;unsigned index_;
  PreparedCopy(Scope owner,std::uint64_t revision,const InspiredHexOffer& offer,skills::Slots bar,TemporaryHexSlots copies,unsigned index):
   owner_(owner),revision_(revision),offer_(offer),bar_(bar),copies_(std::move(copies)),index_(index){}
 public:
  const InspiredHexOffer& offer()const{return offer_;}
  unsigned index()const{return index_;}
  skills::Slot replacement()const{return bar_[index_];}
  const skills::Slots& bar()const{return bar_;}
 };
 class PreparedEcho {
  friend class TemporarySkillSession;
  Scope owner_;std::uint64_t revision_;EchoUseProposal proposal_;
  skills::Slots bar_;TemporarySkillSlots copies_;unsigned index_;
  PreparedEcho(Scope owner,std::uint64_t revision,const EchoUseProposal& proposal,skills::Slots bar,TemporarySkillSlots copies,unsigned index):
   owner_(owner),revision_(revision),proposal_(proposal),bar_(bar),copies_(std::move(copies)),index_(index){}
 public:
  const EchoUseProposal& proposal()const{return proposal_;}
  unsigned index()const{return index_;}const skills::Slots& bar()const{return bar_;}
 };
 class PreparedEchoBatch {
  friend class TemporarySkillSession;
  Scope owner_;std::uint64_t revision_;std::vector<EchoUseProposal> proposals_;
  skills::Slots bar_;TemporarySkillSlots copies_;
  PreparedEchoBatch(Scope owner,std::uint64_t revision,std::vector<EchoUseProposal> proposals,skills::Slots bar,TemporarySkillSlots copies):
   owner_(owner),revision_(revision),proposals_(std::move(proposals)),bar_(bar),copies_(std::move(copies)){}
 public:
  const auto& proposals()const{return proposals_;}const auto& bar()const{return bar_;}
 };
 struct CastAdmission {std::uint64_t base_epoch;Time started;TemporaryHexAdmission cast;};
 explicit TemporarySkillSession(Scope owner,skills::Slots base):owner_(owner),base_(base),bar_(base),copies_(owner){}
 Scope owner()const{return owner_;}
 const skills::Slots& bar()const{return bar_;}
 const skills::Slots& base()const{return base_;}
 bool ready(const PreparedMimicry& prepared)const{return owns(prepared.owner_)&&prepared.revision_==revision_&&revision_!=UINT64_MAX;}
 std::optional<PreparedMimicry> prepare_mimicry(const MimicryDonorLease& lease,unsigned index,skills::Slot replacement)const{
  if(!owns(lease.owner())||lease.at()<clock_||revision_==UINT64_MAX)return {};
  auto bar=bar_;auto copies=copies_;
  if(!copies.install_mimicry(owner_,bar,index,lease.source(),replacement,lease.at(),lease))return {};
  return PreparedMimicry(owner_,revision_,lease,bar,std::move(copies),index);
 }
 bool commit_mimicry(PreparedMimicry&& prepared,const MimicryCopyReceipt& receipt)noexcept{
  static_assert(std::is_nothrow_move_assignable_v<TemporarySkillSlots>);
  if(!ready(prepared)||receipt.lease()!=prepared.lease_||receipt.replacement()!=prepared.replacement())return false;
  bar_=prepared.bar_;copies_=std::move(prepared.copies_);clock_=prepared.lease_.at();++revision_;return true;
 }
 bool ready(const PreparedCopy& prepared)const{return owns(prepared.owner_)&&prepared.revision_==revision_&&revision_!=UINT64_MAX;}
 bool ready(const PreparedEcho& prepared)const{return owns(prepared.owner_)&&prepared.revision_==revision_&&revision_!=UINT64_MAX;}
 std::optional<PreparedEcho> prepare_echo(const EchoUseProposal& proposal,unsigned index,skills::Slot replacement)const{
  if(!proposal.registry.matches(owner_)||proposal.at<clock_||revision_==UINT64_MAX||index>=bar_.size()||bar_[index]!=proposal.source)return {};
  auto bar=bar_;auto copies=copies_;
  if(proposal.copied_skill){
   if(!copies.install_echo(owner_,bar,index,proposal.source,replacement,proposal.at,proposal))return {};
  }else if(proposal.consumed.skill!=75||proposal.source.skill!=75||replacement!=proposal.source||proposal.copy_duration||proposal.restore_cooldown)return {};
  return PreparedEcho(owner_,revision_,proposal,bar,std::move(copies),index);
 }
 bool commit_echo(PreparedEcho&& prepared,const EchoConsumption& receipt)noexcept{
  static_assert(std::is_nothrow_move_assignable_v<TemporarySkillSlots>);
  if(!ready(prepared)||receipt.owner()!=prepared.owner_||receipt.at()!=prepared.proposal_.at||
     receipt.source()!=prepared.proposal_.source||receipt.used()!=prepared.proposal_.used||
     receipt.copied_skill()!=prepared.proposal_.copied_skill||!same(receipt.consumed(),prepared.proposal_.consumed))return false;
  bar_=prepared.bar_;copies_=std::move(prepared.copies_);clock_=prepared.proposal_.at;++revision_;return true;
 }
 std::optional<PreparedEchoBatch> prepare_echo_batch(const std::vector<EchoUseProposal>& proposals,
     const std::vector<skills::Slot>& replacements)const{
  if(proposals.empty()||proposals.size()>2||proposals.size()!=replacements.size()||revision_==UINT64_MAX)return {};
  auto bar=bar_;auto copies=copies_;std::set<unsigned> indices;
  for(unsigned n=0;n<proposals.size();++n){const auto& proposal=proposals[n];
   if(!proposal.registry.matches(owner_)||proposal.at<clock_||proposal.at!=proposals.front().at||
      proposal.used!=proposals.front().used||proposal.completion_event!=proposals.front().completion_event)return {};
   const auto found=std::find(bar_.begin(),bar_.end(),proposal.source);if(found==bar_.end())return {};
   const auto index=unsigned(found-bar_.begin());if(!indices.insert(index).second)return {};
   if(proposal.copied_skill){
    if(!copies.install_echo(owner_,bar,index,proposal.source,replacements[n],proposal.at,proposal))return {};
   }else if(proposal.source.skill!=75||proposal.consumed.skill!=75||replacements[n]!=proposal.source||proposal.copy_duration||proposal.restore_cooldown)return {};
  }
  return PreparedEchoBatch(owner_,revision_,proposals,bar,std::move(copies));
 }
 bool ready(const PreparedEchoBatch& prepared)const{return owns(prepared.owner_)&&prepared.revision_==revision_&&revision_!=UINT64_MAX;}
 bool commit_echo_batch(PreparedEchoBatch&& prepared,const std::vector<EchoConsumption>& receipts)noexcept{
  if(!ready(prepared)||receipts.size()!=prepared.proposals_.size())return false;
  for(unsigned n=0;n<receipts.size();++n){const auto& receipt=receipts[n];const auto& proposal=prepared.proposals_[n];
   if(receipt.owner()!=owner_||receipt.at()!=proposal.at||receipt.source()!=proposal.source||receipt.used()!=proposal.used||
      receipt.copied_skill()!=proposal.copied_skill||!same(receipt.consumed(),proposal.consumed))return false;
  }
  bar_=prepared.bar_;copies_=std::move(prepared.copies_);clock_=prepared.proposals_.front().at;++revision_;return true;
 }
 std::optional<PreparedCopy> prepare(const InspiredHexOffer& offer,unsigned index,skills::Slot original,skills::Slot replacement)const{
  if(!owns(offer.caster)||offer.at<clock_||revision_==UINT64_MAX||!offer.hex||
     offer.plan.skill!=original.skill||offer.copy.kind!=HexCopyKind::player||offer.copy.actual!=replacement.skill||
     offer.hex->skill!=replacement.skill)return {};
  auto bar=bar_;auto copies=copies_;
  if(!copies.install(owner_,bar,index,original,replacement,offer.at))return {};
  return PreparedCopy(owner_,revision_,offer,bar,std::move(copies),index);
 }
 bool commit(PreparedCopy&& prepared,const InspiredHexWorldResult& result)noexcept{
  static_assert(std::is_nothrow_move_assignable_v<TemporaryHexSlots>);
  if(!owns(prepared.owner_)||prepared.revision_!=revision_||revision_==UINT64_MAX||!result.removed||
     result.caster!=prepared.offer_.caster||result.ally!=prepared.offer_.ally||result.at!=prepared.offer_.at||
     result.source_skill!=prepared.offer_.plan.skill||!same(*result.removed,*prepared.offer_.hex)||
     result.copy.kind!=prepared.offer_.copy.kind||result.copy.actual!=prepared.offer_.copy.actual||result.copy.base!=prepared.offer_.copy.base)return false;
  bar_=prepared.bar_;copies_=std::move(prepared.copies_);clock_=prepared.offer_.at;++revision_;return true;
 }
 std::optional<std::vector<TemporaryHexRestoration>> expire(Scope owner,Time now){
  if(!owns(owner)||now<clock_||revision_==UINT64_MAX)return {};
  const auto changes=copies_.expire(owner,bar_,now);if(!changes)return {};
  if(now!=clock_||!changes->empty())++revision_;clock_=now;return changes;
 }
 std::optional<std::vector<TemporarySkillSlotChange>> replace_base(Scope owner,const skills::Slots& base,const skills::Policy& policy,Time now){
  if(!owns(owner)||now<clock_||revision_==UINT64_MAX||base_epoch_==UINT64_MAX||skills::validate(base,policy)!=skills::Reason::accepted)return {};
  TemporaryHexSlots copies(owner);std::vector<TemporarySkillSlotChange> changes;
  for(unsigned i=0;i<bar_.size();++i)if(bar_[i]!=base[i])changes.push_back({i,bar_[i],base[i]});
  copies_=std::move(copies);base_=base;bar_=base;clock_=now;++revision_;++base_epoch_;return changes;
 }
 std::optional<CastAdmission> admit(Scope owner,const skills::Policy& policy,unsigned index,skills::Slot requested,Time now)const{
  if(!owns(owner)||now<clock_)return {};const auto admission=admit_temporary_hex_cast(copies_,owner,bar_,policy,index,requested,now);
  return admission?std::optional(CastAdmission{base_epoch_,now,*admission}):std::nullopt;
 }
 bool valid(const CastAdmission& admission,const skills::Policy& policy,Time now)const{
  return now>=clock_&&now>=admission.started&&admission.base_epoch==base_epoch_&&owns(admission.cast.owner)&&
      valid_temporary_hex_admission(admission.cast,copies_,bar_,policy,now);
 }
};
}
