#pragma once
#include "gw_npc_skill_ai.h"
#include "gw_elemental_damage.h"
#include "gw_projectile_flight.h"
#include "gw_npc_elemental_wire.h"
#include "gw_effect_capacity.h"
#include "gw_npc_offensive_plan.h"

namespace fw::gw::combat::ai::elemental {
using Scope=skills::effects::Scope;
using Plan=offensive::Plan;
using Token=npcs::control::Token;
inline bool scope_valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
struct Reviewed {skills::Slot slot;Plan plan;std::string evidence;};
struct Pose {
    Scope scope;float x=0,y=0;unsigned plane=0;bool alive=false;
    std::uint64_t clock=0; // maximum authoritative Health/Energy/movement clock
    bool operator==(const Pose&)const=default;
};
inline bool pose_valid(const Pose& p){return scope_valid(p.scope)&&std::isfinite(p.x)&&std::isfinite(p.y);}
// Fail.html oldid2625489 explicitly lists Fireball among spells that still
// activate when their selected foe dies. This is not a generic AoE rule.
inline bool dead_primary_anchor(const Plan& p){return p.skill()==186&&p.valid();}
struct Reservation {
    std::uint64_t id=0,at=0;skills::Slot slot;Scope target;Plan plan;
    bool operator==(const Reservation&)const=default;
};
struct Payment {
    unsigned before=0,after=0,maximum=0;
    std::uint64_t at=0,action_sequence=0;
};
struct PaidCast {Reservation use;Token token;bool operator==(const PaidCast&)const=default;};
struct Release {
    PaidCast cast;float aim_x=0,aim_y=0;unsigned plane=0;
    std::uint64_t released=0,arrives=0;
    bool operator==(const Release&)const=default;
};
enum class ReleaseStatus {invalid,deferred,ready,stopped};
struct ReleaseProposal {
    ReleaseStatus status=ReleaseStatus::invalid;PaidCast cast;
    std::uint64_t at=0;std::optional<Release> release;
    bool operator==(const ReleaseProposal&)const=default;
};
struct Recipient {Pose pose;combat::Actor actor;combat::conditions::State conditions;bool hostile=false;combat::DamageProjection projection;};
struct Hit {Scope target;skills::effects::ElementalResult result;};
enum class ImpactStatus {invalid,deferred,stale,miss,unsupported,hit};
struct ImpactProposal {
    ImpactStatus status=ImpactStatus::invalid;Release release;
    std::uint64_t at=0;std::vector<Hit> hits;
};

// This ledger owns no Energy pool, recharge clock or action control. LiveActor
// owns those authorities. Reserve BEFORE payment, then accept the real paid
// token under the same World lock; finish/impact proposals commit under that
// lock with the actual action/protection/condition/Health transactions.
class Ledger {
    Scope source_;unsigned level_;projectiles::Policy flight_;std::size_t capacity_;
    std::vector<Reviewed> book_;
    std::uint64_t nonce_=0,clock_=0,last_sequence_=0;
    std::optional<Reservation> reserved_;
    std::optional<PaidCast> pending_;
    std::optional<ReleaseProposal> prepared_;
    std::map<std::uint64_t,Release> releases_;
public:
    Ledger(Scope source,unsigned level,std::vector<Reviewed> book,projectiles::Policy flight,std::size_t capacity):
        source_(source),level_(level),flight_(flight),capacity_(capacity),book_(std::move(book)){
        if(!scope_valid(source)||!level||level>255||!flight.valid()||!capacity||capacity>64||book_.empty()||book_.size()>8)
            throw std::invalid_argument("invalid NPC elemental ledger policy");
        for(const auto& spell:book_){
            if(spell.plan.skill()!=spell.slot.skill||!spell.plan.valid())throw std::invalid_argument("unsupported NPC elemental effect plan");
        }
        // Validate exact copies, native costs and reviewed provenance using the
        // same configuration rules as LiveActor, without duplicating readiness.
        (void)Spellbook(reviewed_spells());
    }
    std::vector<ReviewedSpell> reviewed_spells()const{
        std::vector<ReviewedSpell> spells;for(const auto& s:book_)spells.push_back({s.slot,s.plan.range(),s.evidence});return spells;
    }
    std::optional<Plan> plan(skills::Slot slot)const{
        const auto s=std::find_if(book_.begin(),book_.end(),[&](const auto& entry){return entry.slot==slot;});
        return s==book_.end()?std::nullopt:std::optional(s->plan);
    }
    const std::optional<PaidCast>& pending()const{return pending_;}
    std::size_t in_flight()const{return releases_.size();}
    // Selection must observe the same bounded ledger admission as reserve.
    // This says nothing about completion of an already accepted paid cast.
    bool admission_ready(std::uint64_t now)const{
        return !reserved_&&!pending_&&releases_.size()<capacity_&&now>=clock_&&nonce_!=UINT64_MAX;
    }
    std::optional<std::uint64_t> next_arrival()const{
        std::optional<std::uint64_t> next;
        for(const auto&[id,shot]:releases_)if(!next||shot.arrives<*next)next=shot.arrives;
        return next;
    }
    std::vector<std::uint64_t> released_ids()const{
        std::vector<std::uint64_t> ids;for(const auto&[id,shot]:releases_)ids.push_back(id);return ids;
    }
    std::optional<Reservation> reserve(skills::Slot slot,const SpellTarget& target,std::uint64_t now){
        const auto selected=plan(slot);
        if(!selected||!admission_ready(now)||
           !scope_valid(target.scope)||target.scope.instance!=source_.instance||target.scope==source_||
           !spell_target(slot.skill,target)||target.distance>selected->range())return {};
        clock_=now;reserved_=Reservation{++nonce_,now,slot,target.scope,*selected};return reserved_;
    }
    bool abort(const Reservation& reservation){
        if(!reserved_||*reserved_!=reservation)return false;reserved_.reset();return true;
    }
    bool accept(const Reservation& reservation,const Token& token,const Payment& payment){
        const auto* row=skills::find_skill(reservation.slot.skill);
        if(!reserved_||*reserved_!=reservation||pending_||!row||token.actor!=source_||!token.sequence||
           token.sequence<=last_sequence_||token.kind!=npcs::control::Action::spell||token.skill!=reservation.slot||
           token.begins!=reservation.at||token.completes<=token.begins||payment.at!=token.begins||
           payment.action_sequence!=token.sequence||!payment.maximum||payment.maximum>1000000||payment.before>payment.maximum||
           payment.after>payment.before||payment.before-payment.after!=row->energy_cost())return false;
        pending_=PaidCast{reservation,token};last_sequence_=token.sequence;reserved_.reset();prepared_.reset();return true;
    }
    bool cancel(const Token& token,std::uint64_t now){
        if(!pending_||pending_->token!=token||now<clock_||now<token.begins)return false;
        pending_.reset();prepared_.reset();clock_=now;return true; // no Energy refund; LiveActor owns recharge
    }
    ReleaseProposal prepare_release(const Token& token,const Pose& source,const Pose& target,
        std::uint64_t control_epoch,bool line_of_sight,std::uint64_t now,float target_vx=0,float target_vy=0){
        if(!pending_||pending_->token!=token||now<clock_)return {};
        prepared_.reset();
        const bool centered=pending_->use.plan.caster_center();
        if(now<token.completes||now<source.clock||(!centered&&now<target.clock))return {ReleaseStatus::deferred,*pending_,now,{}};
        ReleaseProposal proposal{ReleaseStatus::stopped,*pending_,now,{}};
        if(pose_valid(source)&&source.scope==source_&&source.alive&&control_epoch==token.epoch&&
           (centered||(pose_valid(target)&&target.scope==pending_->use.target&&
           (target.alive||dead_primary_anchor(pending_->use.plan))&&source.plane==target.plane&&line_of_sight))){
            projectiles::Aim aim{centered?source.x:target.x,centered?source.y:target.y,0};
            const auto& effect=pending_->use.plan;const auto flight=effect.ice_spear()?effect.ice_spear()->flight:flight_;
            if(effect.projectile()){const auto predicted=projectiles::intercept(flight,source.x,source.y,target.x,target.y,target_vx,target_vy);
                if(!predicted)return {};aim=*predicted;}
            if(effect.ice_spear()&&(!aim.flight_ms||aim.flight_ms>30000))return {}; // native projectile duration preflight
            if(now>UINT64_MAX-aim.flight_ms)return {};
            proposal.status=ReleaseStatus::ready;proposal.release=Release{*pending_,aim.x,aim.y,source.plane,now,now+aim.flight_ms};
        }
        prepared_=proposal;return proposal;
    }
    // Call only after authoritative action completion and before-effect
    // reactions. A lethal Backfire reaction consumes this cast but queues none.
    bool commit_release(const ReleaseProposal& proposal,bool reaction_survived){
        if(!prepared_||*prepared_!=proposal||!pending_||proposal.cast!=*pending_||proposal.at<clock_||
           (proposal.status!=ReleaseStatus::ready&&proposal.status!=ReleaseStatus::stopped))return false;
        if(proposal.status==ReleaseStatus::ready&&reaction_survived){
            if(!proposal.release||releases_.size()>=capacity_||releases_.contains(proposal.cast.use.id))return false;
            releases_.emplace(proposal.cast.use.id,*proposal.release);
        }
        clock_=proposal.at;pending_.reset();prepared_.reset();return true;
    }
    ImpactProposal prepare_impact(std::uint64_t id,Scope current_source,const std::vector<Recipient>& contacts,std::uint64_t now)const{
        const auto found=releases_.find(id);if(found==releases_.end())return {};
        ImpactProposal out{ImpactStatus::deferred,found->second,now,{}};const auto& shot=found->second;
        if(now<shot.arrives)return out;
        if(current_source!=source_){out.status=ImpactStatus::stale;return out;}
        const auto& effect=shot.cast.use.plan;
        // Untargeted Flame Burst belongs to the caster's release point; losing
        // the foe that motivated its activation does not erase other contacts.
        if(!effect.caster_center()){
            const auto primary=std::find_if(contacts.begin(),contacts.end(),[&](const auto& c){return c.pose.scope==shot.cast.use.target;});
            if(primary==contacts.end()){out.status=ImpactStatus::stale;return out;}
            if(now<primary->pose.clock)return out;
            const bool corpse=dead_primary_anchor(effect)&&!primary->pose.alive&&primary->actor.health&&
                combat::valid(*primary->actor.health)&&primary->actor.health->current==0;
            if(!pose_valid(primary->pose)||(!primary->pose.alive&&!corpse)||!primary->hostile||primary->pose.plane!=shot.plane||
               primary->actor.id!=primary->pose.scope.actor||primary->actor.generation!=primary->pose.scope.generation){out.status=ImpactStatus::stale;return out;}
            const auto flight=effect.ice_spear()?effect.ice_spear()->flight:flight_;
            if(effect.projectile()&&std::hypot(primary->pose.x-shot.aim_x,primary->pose.y-shot.aim_y)>flight.hit_radius){out.status=ImpactStatus::miss;return out;}
        }
        std::set<Scope> seen;
        for(const auto& c:contacts){
            if(!c.hostile||!c.pose.alive||c.pose.scope.instance!=source_.instance||c.pose.scope==source_||c.pose.plane!=shot.plane)continue;
            if(effect.radius()?std::hypot(c.pose.x-shot.aim_x,c.pose.y-shot.aim_y)>effect.radius():c.pose.scope!=shot.cast.use.target)continue;
            if(now<c.pose.clock){out.hits.clear();return out;} // no partial Health proposal at a future recipient clock
            if(!pose_valid(c.pose)||!seen.insert(c.pose.scope).second||c.actor.id!=c.pose.scope.actor||c.actor.generation!=c.pose.scope.generation){out.status=ImpactStatus::unsupported;out.hits.clear();return out;}
            auto result=offensive::target(effect,source_,c.pose.scope,c.actor,c.conditions,level_,now,c.projection);
            if(!result.accepted){out.status=ImpactStatus::unsupported;out.hits.clear();return out;}
            out.hits.push_back({c.pose.scope,std::move(result)});
        }
        out.status=ImpactStatus::hit;return out;
    }
    // World commits recipient proposals and consumes this release in one lock.
    // Deferred/unknown states retain it; stale,miss and accepted hit are terminal.
    bool commit_impact(const ImpactProposal& proposal){
        const auto found=releases_.find(proposal.release.cast.use.id);
        if(found==releases_.end()||found->second!=proposal.release||proposal.at<found->second.arrives||
           (proposal.status!=ImpactStatus::hit&&proposal.status!=ImpactStatus::miss&&proposal.status!=ImpactStatus::stale))return false;
        releases_.erase(found);return true;
    }
};
}
