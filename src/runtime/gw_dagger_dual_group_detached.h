#pragma once
#include "gw_attack_outcomes.h"
#include "gw_dagger_chain.h"
#include "gw_dagger_identity_draft.h"
#include <array>

namespace fw::gw::combat::weapon {
// Group ownership is separate from attack-event ownership. World supplies each
// actual release and terminal receipt from its existing AttackOutcomes ledger.
// No synthetic second hit/qualification is generated when a participant retires.
class DaggerDualGroupDraft {
    AttackScope source_,target_;
    skills::effects::DaggerChain admitted_;
    unsigned skill_=0,ordinal_=0;
    std::uint64_t group_=0,at_=0,second_at_=0;
    std::array<std::uint64_t,2> events_{};
    std::array<std::optional<ResolvedAttack>,2> outcomes_{};
    bool stopped_=false;
    DaggerDualGroupDraft(AttackScope source,AttackScope target,skills::effects::DaggerChain admitted,
            unsigned skill,std::uint64_t group,std::uint64_t at):source_(source),target_(target),admitted_(admitted),skill_(skill),group_(group),at_(at){}
public:
    static std::optional<DaggerDualGroupDraft> begin(AttackScope source,AttackScope target,
            const skills::effects::DaggerChain& chain,unsigned skill,std::uint64_t group,std::uint64_t at){
        if(!source.instance||!source.actor||source.actor>65535||!source.generation||
           !target.actor||target.actor>65535||!target.generation||source.instance!=target.instance||
           source.actor==target.actor||!group||!dagger_dual_role(skill)||chain.current(target,at)!=2)return {};
        return DaggerDualGroupDraft(source,target,chain,skill,group,at);
    }
    bool matches(AttackScope source,AttackScope target,unsigned skill,std::uint64_t at)const{
        return !stopped_&&source_==source&&target_==target&&skill_==skill&&(ordinal_?second_at_:at_)==at;
    }
    bool select_second(std::uint64_t at=0){
        if(stopped_||ordinal_!=0||!events_[0]||!outcomes_[0]||events_[1])return false;
        if(at&&at<at_)return false;second_at_=at?at:at_;ordinal_=1;return true;
    }
    // Returns whether this is a second physical contact sharing the earlier
    // logical release. Event0/reuse/foreign scopes cannot claim either strike.
    std::optional<bool> release(AttackScope source,AttackScope target,unsigned skill,
            std::uint64_t event,std::uint64_t at){
        if(!matches(source,target,skill,at)||!event||events_[ordinal_]||
           (ordinal_==1&&(!outcomes_[0]||event<=events_[0])))return {};
        events_[ordinal_]=event;return ordinal_==1;
    }
    bool owns_event(std::uint64_t event)const{return event&&(events_[0]==event||events_[1]==event);}
    bool finish(const ResolvedAttack& outcome){
        if(stopped_||outcome.attacker!=source_||outcome.target!=target_||outcome.released!=(ordinal_?second_at_:at_)||outcome.at!=(ordinal_?second_at_:at_)||
           outcome.delivery!=AttackDelivery::melee||!valid_attack_outcome(outcome.outcome))return false;
        unsigned index=events_[0]==outcome.event?0:events_[1]==outcome.event?1:2;
        if(!outcome.event||index==2||outcomes_[index])return false;
        if(outcome.outcome==AttackOutcome::hit&&outcome.before_damage!=(index?second_at_:at_))return false;
        if(outcome.before_damage&&outcome.outcome!=AttackOutcome::hit&&outcome.outcome!=AttackOutcome::cancelled&&outcome.outcome!=AttackOutcome::target_retired)return false;
        outcomes_[index]=outcome;return true;
    }
    void stop(){stopped_=true;}
    unsigned ordinal()const{return ordinal_;}
    std::uint64_t group()const{return group_;}
    const auto& admitted_chain()const{return admitted_;}
    const auto& outcomes()const{return outcomes_;}
    bool complete()const{return outcomes_[0].has_value()&&outcomes_[1].has_value();}
};
}
