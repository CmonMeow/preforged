#pragma once
#include "gw_effect_registry.h"
#include "gw_combat.h"
#include <map>
#include <optional>
#include <algorithm>
namespace fw::gw::combat::weapon {
using AttackScope=skills::effects::Scope;
enum class AttackOutcome {hit,miss,block,dodge,obstruction,target_retired,cancelled,replaced};
enum class AttackDelivery {unknown,melee,ranged};
inline bool valid_attack_delivery(AttackDelivery value){return static_cast<unsigned>(value)<=static_cast<unsigned>(AttackDelivery::ranged);}
// Supplied GWCA ItemType values:axe2,hammer15,sword27,daggers32,scythe35;
// bow5,wand22,staff26,spear36. Unknown/inconsistent facts stay unclassified.
inline AttackDelivery weapon_delivery(unsigned item_type,bool caster_weapon){
    if(item_type==22||item_type==26)return caster_weapon?AttackDelivery::ranged:AttackDelivery::unknown;
    if(caster_weapon)return AttackDelivery::unknown;
    if(item_type==2||item_type==15||item_type==27||item_type==32||item_type==35)return AttackDelivery::melee;
    if(item_type==5||item_type==36)return AttackDelivery::ranged;
    return AttackDelivery::unknown;
}
inline bool valid_attack_outcome(AttackOutcome outcome){return static_cast<unsigned>(outcome)<=static_cast<unsigned>(AttackOutcome::replaced);}
struct ResolvedAttack {
    AttackScope attacker,target;std::uint64_t event=0,released=0,at=0;AttackOutcome outcome=AttackOutcome::hit;
    std::optional<std::uint64_t> before_damage;
    AttackDelivery delivery=AttackDelivery::unknown;
    bool skill_failed=false; // ledger-owned failed prerequisite, never ordinary cancellation
    bool failed_hit()const{return skill_failed||outcome==AttackOutcome::miss||outcome==AttackOutcome::block||outcome==AttackOutcome::dodge||outcome==AttackOutcome::obstruction;}
};
// A hit has passed accuracy/recipient validation,but its Health damage has not
// been committed. This phase is distinct from a terminal outcome notification.
struct QualifiedHit {AttackScope attacker,target;std::uint64_t event=0,released=0,at=0;AttackDelivery delivery=AttackDelivery::unknown;};
class AttackOutcomes;
// Issued once by the owner of an already-qualified attack. Damage consumers
// cannot construct a receipt from arbitrary Health loss or an unrelated spell.
class CommittedAttackDamage {
 ResolvedAttack attack_;combat::Hit hit_;
 CommittedAttackDamage(ResolvedAttack attack,combat::Hit hit):attack_(attack),hit_(std::move(hit)){}
 friend class AttackOutcomes;
public:
 const ResolvedAttack& attack()const{return attack_;}
 const combat::Hit& hit()const{return hit_;}
};
// No further result at or below this frontier can be produced by this owner's
// ledger. The caller must finish synchronous consumers before using it to prune
// their receipts. A pending lower-ID projectile holds the frontier back even
// when newer projectiles have completed. Only the ledger can issue this token.
class AttackCompletionFrontier {
 AttackScope owner_;std::uint64_t through_=0,at_=0;
 AttackCompletionFrontier(AttackScope owner,std::uint64_t through,std::uint64_t at):owner_(owner),through_(through),at_(at){}
 friend class AttackOutcomes;
public:
 AttackScope owner()const{return owner_;}
 std::uint64_t through()const{return through_;}
 std::uint64_t at()const{return at_;}
};
// Reservation has an ID/budget but is NOT an actual made attack. Only the
// canonical ledger can promote this opaque capability after World preflight.
class AttackActionReservation {
 AttackScope owner_;std::uint64_t event_=0,at_=0;unsigned skill_=0;std::uint64_t source_epoch_=0;
 AttackActionReservation(AttackScope owner,std::uint64_t event,std::uint64_t at,unsigned skill,std::uint64_t epoch):owner_(owner),event_(event),at_(at),skill_(skill),source_epoch_(epoch){}
 friend class AttackOutcomes;
public:
 AttackScope owner()const{return owner_;}std::uint64_t event()const{return event_;}std::uint64_t at()const{return at_;}unsigned skill()const{return skill_;}std::uint64_t source_epoch()const{return source_epoch_;}
};
// Targetless actual attack identity, issued only by the SAME canonical ledger.
// This is not QualifiedHit: it carries no recipient and cannot commit damage.
class AttackMade {
 AttackScope owner_;std::uint64_t event_=0,at_=0;unsigned skill_=0;std::uint64_t source_epoch_=0;
 AttackMade(AttackScope owner,std::uint64_t event,std::uint64_t at,unsigned skill,std::uint64_t epoch):owner_(owner),event_(event),at_(at),skill_(skill),source_epoch_(epoch){}
 friend class AttackOutcomes;
public:
 AttackScope owner()const{return owner_;}std::uint64_t event()const{return event_;}std::uint64_t at()const{return at_;}unsigned skill()const{return skill_;}std::uint64_t source_epoch()const{return source_epoch_;}
};
// Release and outcome are separate:projectiles can complete out of order.
// Event IDs are assigned by the server per released recipient/projectile.
// Consume before the owner invokes any reactive damage callback. No Health,
// hit probability or client-supplied outcome is inferred by this ledger.
class AttackOutcomes {
    struct Pending {std::optional<AttackScope> target;std::uint64_t released;std::optional<std::uint64_t> qualified;AttackDelivery delivery;bool damage_committed=false;unsigned release_observers=0;unsigned action_skill=0;std::uint64_t source_epoch=0;bool action_made=false;unsigned declared_contacts=0,next_contact=0;std::uint64_t parent_action=0;unsigned contact_ordinal=0;};
    AttackScope owner_;std::map<std::uint64_t,Pending> pending_;
    std::uint64_t last_event_=0,clock_=0;bool retired_=false;
    static bool valid(AttackScope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
public:
    explicit AttackOutcomes(AttackScope owner):owner_(owner){}
    bool release(AttackScope caller,AttackScope target,std::uint64_t event,std::uint64_t at,AttackDelivery delivery=AttackDelivery::unknown){
        if(!valid_attack_delivery(delivery)||retired_||caller!=owner_||!valid(owner_)||!valid(target)||target.instance!=owner_.instance||target==owner_||
           target.actor==owner_.actor||!event||event<=last_event_||at<clock_||pending_.size()>=4096)return false;
        const auto added=pending_.emplace(event,Pending{target,at,{},delivery}).second;if(!added)return false;
        last_event_=event;clock_=at;return true;
    }
    // Trusted producer calls only AFTER owned cast/action admission. Same
    // pending budget, ID ordering, scope lifetime, clock and frontier as contacts.
private:
    std::optional<AttackActionReservation> reserve_action_impl(AttackScope caller,std::uint64_t event,std::uint64_t at,unsigned skill,std::uint64_t source_epoch=0){
        if(retired_||caller!=owner_||!valid(owner_)||!event||event<=last_event_||at<clock_||pending_.size()>=4096)return {};
        if(!pending_.emplace(event,Pending{std::nullopt,at,{},AttackDelivery::unknown,false,0,skill,source_epoch}).second)return {};
        last_event_=event;clock_=at;return AttackActionReservation(owner_,event,at,skill,source_epoch);
    }
public:
    std::optional<AttackActionReservation> reserve_action(AttackScope caller,std::uint64_t event,std::uint64_t at,unsigned skill,std::uint64_t source_epoch=0){
        if(!skill)return {};return reserve_action_impl(caller,event,at,skill,source_epoch);
    }
    std::optional<AttackActionReservation> reserve_ordinary_action(AttackScope caller,std::uint64_t event,std::uint64_t at,std::uint64_t source_epoch=0){
        return reserve_action_impl(caller,event,at,0,source_epoch);
    }
    std::optional<AttackMade> make_action(AttackScope caller,const AttackActionReservation& action){
        if(retired_||caller!=owner_||action.owner()!=owner_)return {};
        const auto found=pending_.find(action.event());
        if(found==pending_.end()||found->second.target||found->second.action_made||found->second.released!=action.at()||found->second.action_skill!=action.skill()||found->second.source_epoch!=action.source_epoch())return {};
        found->second.action_made=true;return AttackMade(owner_,action.event(),action.at(),action.skill(),action.source_epoch());
    }
    bool cancel_action(AttackScope caller,const AttackActionReservation& action,std::uint64_t at){
        if(retired_||caller!=owner_||action.owner()!=owner_||at<clock_)return false;
        const auto found=pending_.find(action.event());
        if(found==pending_.end()||found->second.target||found->second.action_made||found->second.released!=action.at()||found->second.action_skill!=action.skill()||found->second.source_epoch!=action.source_epoch())return false;
        pending_.erase(found);clock_=at;return true;
    }
    std::optional<bool> action_observed(AttackScope caller,const AttackMade& action,unsigned observer)const{
        if(retired_||caller!=owner_||action.owner()!=owner_||!observer||(observer&(observer-1)))return {};
        const auto found=pending_.find(action.event());
        if(found==pending_.end()||found->second.target||!found->second.action_made||found->second.released!=action.at()||found->second.action_skill!=action.skill()||found->second.source_epoch!=action.source_epoch())return {};
        return (found->second.release_observers&observer)!=0;
    }
    std::optional<bool> observe_action_once(AttackScope caller,const AttackMade& action,unsigned observer){
        const auto seen=action_observed(caller,action,observer);if(!seen)return {};
        pending_.find(action.event())->second.release_observers|=observer;return !*seen;
    }
    // All source-only observers complete SYNCHRONOUSLY before this close. No
    // recipient outcome is manufactured or sent through physical-hit consumers.
    // A private made identity may finish after an earlier source observer
    // terminates execution. Presence validates identity; bit1 need not be set.
    // Shared ordinary action/physical-contact ownership. Trusted World producer
    // declares selected count AFTER policy/admission; no RNG or timing inferred.
    bool declare_ordinary_contacts(AttackScope caller,const AttackMade& action,unsigned count,std::uint64_t source_epoch){
        if(action.skill()!=0||action.source_epoch()!=source_epoch||count<1||count>3||!action_observed(caller,action,1))return false;
        auto& pending=pending_.at(action.event());if(pending.declared_contacts)return false;
        pending.declared_contacts=count;return true;
    }
    bool release_action_contact(AttackScope caller,const AttackMade& action,AttackScope target,
            std::uint64_t event,std::uint64_t at,unsigned ordinal,std::uint64_t source_epoch){
        if(action.source_epoch()!=source_epoch||!action_observed(caller,action,1)||at<action.at())return false;
        auto& parent=pending_.at(action.event());
        if(!parent.declared_contacts||ordinal!=parent.next_contact||ordinal>=parent.declared_contacts)return false;
        if(!release(caller,target,event,at,AttackDelivery::melee))return false;
        auto& contact=pending_.at(event);contact.parent_action=action.event();contact.contact_ordinal=ordinal;
        ++parent.next_contact;return true;
    }
    std::optional<unsigned> action_contact_ordinal(AttackScope caller,const AttackMade& action,
            AttackScope target,std::uint64_t event)const{
        if(!action_observed(caller,action,1))return {};
        const auto found=pending_.find(event);
        if(found==pending_.end()||found->second.target!=target||found->second.parent_action!=action.event())return {};
        return found->second.contact_ordinal;
    }
    bool finish_action(AttackScope caller,const AttackMade& action,std::uint64_t at){
        if(!action_observed(caller,action,1)||at<clock_||at<action.at())return false;
        pending_.erase(action.event());clock_=at;return true;
    }
    // Read-only duplicate check; callers can skip side-effecting readiness
    // when this exact retained canonical release already ran its observer.
    std::optional<bool> release_observed(AttackScope caller,AttackScope target,std::uint64_t event,std::uint64_t at,unsigned observer)const{
        if(caller!=owner_||retired_||!observer||(observer&(observer-1)))return {};
        const auto found=pending_.find(event);
        if(found==pending_.end()||found->second.target!=target||at<found->second.released)return {};
        return (found->second.release_observers&observer)!=0;
    }
    // True is the first observation; false is an idempotent repeat. Only a
    // current retained server release may be observed, never a fabricated ID.
    std::optional<bool> observe_release_once(AttackScope caller,AttackScope target,std::uint64_t event,std::uint64_t at,unsigned observer){
        if(caller!=owner_||retired_||!observer||(observer&(observer-1)))return {};
        const auto found=pending_.find(event);
        if(found==pending_.end()||found->second.target!=target||at<found->second.released)return {};
        const bool first=(found->second.release_observers&observer)==0;
        found->second.release_observers|=observer;return first;
    }
    std::optional<QualifiedHit> qualify_hit(AttackScope caller,AttackScope target,std::uint64_t event,std::uint64_t at){
        if(retired_||caller!=owner_||at<clock_)return {};
        const auto found=pending_.find(event);
        if(found==pending_.end()||found->second.target!=target||found->second.qualified||at<found->second.released)return {};
        found->second.qualified=at;clock_=at;
        return QualifiedHit{owner_,target,event,found->second.released,at,found->second.delivery};
    }
    std::optional<CommittedAttackDamage> commit_damage(AttackScope caller,AttackScope target,std::uint64_t event,const combat::Hit& hit,std::uint64_t at){
        if(retired_||caller!=owner_||at<clock_)return {};
        const auto found=pending_.find(event);
        if(found==pending_.end()||found->second.target!=target||found->second.damage_committed||found->second.qualified!=at||
           hit.reason!=combat::Reason::accepted||hit.target!=target.actor||hit.generation!=target.generation||
           !std::isfinite(hit.damage)||hit.damage<0||hit.damage>16000000||
           hit.applied!=std::min(hit.damage,hit.before.current)||!combat::valid_damage_health(hit))return {};
        found->second.damage_committed=true;clock_=at;
        return CommittedAttackDamage({owner_,target,event,found->second.released,at,AttackOutcome::hit,at,found->second.delivery},hit);
    }
    std::optional<ResolvedAttack> finish(AttackScope caller,AttackScope target,std::uint64_t event,AttackOutcome outcome,std::uint64_t at,bool skill_failed=false){
        if(retired_||caller!=owner_||at<clock_||!valid_attack_outcome(outcome))return {};
        const auto found=pending_.find(event);
        if(found==pending_.end()||found->second.target!=target||at<found->second.released)return {};
        if(skill_failed&&(outcome!=AttackOutcome::cancelled||found->second.qualified||found->second.damage_committed))return {};
        if(found->second.damage_committed&&outcome!=AttackOutcome::hit)return {};
        // A validated hit cannot later become an accuracy failure. Retirement
        // or cancellation may still happen during a pre-damage reaction.
        if(found->second.qualified&&outcome!=AttackOutcome::hit&&outcome!=AttackOutcome::cancelled&&outcome!=AttackOutcome::target_retired)return {};
        const ResolvedAttack result{owner_,target,event,found->second.released,at,outcome,found->second.qualified,found->second.delivery,skill_failed};pending_.erase(found);clock_=at;return result;
    }
    void discard_target(AttackScope target){std::erase_if(pending_,[&](const auto& p){return p.second.target==target;});}
    bool retire(AttackScope caller){if(caller!=owner_||retired_)return false;pending_.clear();retired_=true;return true;}
    std::size_t size()const{return pending_.size();}
    std::uint64_t clock()const{return clock_;}
    std::optional<AttackCompletionFrontier> completion_frontier(AttackScope caller)const{
        if(caller!=owner_||!valid(owner_))return {};
        const auto through=pending_.empty()?last_event_:std::min(last_event_,pending_.begin()->first-1);
        return AttackCompletionFrontier(owner_,through,clock_);
    }
};
}
