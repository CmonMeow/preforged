#pragma once
#include "gw_npc_action_control.h"
#include "gw_npc_activated_skill.h"
#include "gw_aura_restoration.h"
#include "gw_heal_effects.h"
#include <cmath>
#include <functional>
#include <string>

namespace fw::gw::combat::ai {
struct SpellTarget {
    skills::effects::Scope scope;
    float distance=0;
    bool alive=false,hostile=false,same_plane=false,line_of_sight=false;
    // Unknown observations must not be treated as a particular weapon/mode.
    std::optional<bool> martial_weapon,martial_primary,hard_mode;
    // A known non-martial target need not wield a spellcasting weapon (for
    // example, it may be unarmed). Keep the positive observation separate.
    std::optional<bool> spellcasting_weapon;
};
enum class SpellTargetClass {reviewed,self,ally};
struct ReviewedSpell {skills::Slot slot;float range=0;std::string evidence;SpellTargetClass targeting=SpellTargetClass::reviewed;
    activated::Kind kind=activated::Kind::spell;};
inline bool spell_target(unsigned skill,const SpellTarget& target){
    // NPC effect scopes and wire helpers require actor IDs in 1..65535.
    // Reject invalid targets before admission/payment using the same domain.
    if(!target.scope.instance||!target.scope.actor||target.scope.actor>65535||!target.scope.generation||!target.alive||
       !target.same_plane||!target.line_of_sight||!std::isfinite(target.distance)||target.distance<0)return false;
    if(skill==1392)return !target.hostile;
    if(!target.hostile)return false;
    // Foe behavior oldid2656731. These are named skill rules, not a universal
    // guess about every hex or every blind-inflicting skill.
    if(skill==28)return target.spellcasting_weapon&&*target.spellcasting_weapon&&
        !(target.martial_weapon&&*target.martial_weapon); // Backfire
    // Eruption is explicitly weapon-gated in both modes. Unlike Blinding
    // Flash, Hard mode does not add a martial-primary requirement here.
    if(skill==167)return target.martial_weapon&&*target.martial_weapon;
    if(skill==220){ // Blinding Flash
        if(!target.martial_weapon||!*target.martial_weapon||!target.hard_mode)return false;
        return !*target.hard_mode||(target.martial_primary&&*target.martial_primary);
    }
    return true;
}
class Spellbook {
    std::vector<ReviewedSpell> spells_;
    std::map<std::pair<unsigned,unsigned>,std::uint64_t> ready_;
    std::optional<npcs::control::Token> pending_;
    std::optional<skills::effects::Scope> owner_;
    std::uint64_t last_sequence_=0;
    std::uint64_t clock_=0,aftercast_=0;
public:
    // Observed by the host under the same lock as admission. An unknown
    // external timer rejects that candidate; it does not fabricate readiness.
    using Availability=std::function<std::optional<std::uint64_t>(skills::Slot,std::uint64_t)>;
    explicit Spellbook(std::vector<ReviewedSpell> spells,std::optional<skills::effects::Scope> owner={}):spells_(std::move(spells)),owner_(owner){
        if(owner&&(!owner->instance||!owner->actor||owner->actor>65535||!owner->generation))throw std::invalid_argument("invalid NPC spellbook owner");
        if(spells_.size()>8)throw std::invalid_argument("NPC spellbook too large");
        std::set<std::pair<unsigned,unsigned>> seen;
        for(const auto& spell:spells_){const auto* row=skills::find_skill(spell.slot.skill);
            if(!row||!row->id||activated::kind(spell.slot.skill)!=spell.kind||spell.slot.copy>65535||
               (spell.kind==activated::Kind::skill&&spell.targeting!=SpellTargetClass::reviewed)||
               row->adrenaline||row->health_sacrifice||((row->special&1)&&row->overcast)||row->energy_cost()>25||
               !std::isfinite(row->activation)||row->activation<=0||row->activation>60||
               !std::isfinite(row->aftercast)||row->aftercast<0||row->aftercast>60||
               (spell.targeting!=SpellTargetClass::reviewed&&spell.targeting!=SpellTargetClass::self&&spell.targeting!=SpellTargetClass::ally)||
               (spell.targeting==SpellTargetClass::ally&&(!owner_||
                   (!skills::effects::direct_heal(spell.slot.skill)&&spell.slot.skill!=1526&&spell.slot.skill!=307)))||
               (spell.targeting==SpellTargetClass::self&&
                   ((spell.slot.skill!=180&&spell.slot.skill!=181&&spell.slot.skill!=2860)||!owner_||spell.range!=0))||
               ((spell.slot.skill==180||spell.slot.skill==181||spell.slot.skill==2860)&&spell.targeting!=SpellTargetClass::self)||
               !std::isfinite(spell.range)||(spell.targeting!=SpellTargetClass::self&&spell.range<=0)||spell.range>1248||spell.evidence.empty()||spell.evidence.size()>1024||
               !seen.emplace(spell.slot.skill,spell.slot.copy).second)throw std::invalid_argument("unsupported NPC reviewed spell");
        }
    }
    bool empty()const{return spells_.empty();}
    bool action_ready(std::uint64_t now)const{return !pending_&&now>=clock_&&now>=aftercast_;}
    // Aftercast_delay.html oldid2633746: movement, like auto-attacks, cannot
    // resume during aftercast. Clamp the interval, not just its final time:
    // a coarse scheduler tick must not spend the blocked part as movement.
    std::uint64_t movement_begin(std::uint64_t from,std::uint64_t until)const{
        if(until<=from||until<clock_||pending_)return until;
        return std::min(until,std::max(from,aftercast_));
    }
    bool eligible(skills::Slot slot,const SpellTarget& target,std::optional<unsigned> energy,
        std::uint64_t now,const Availability& external={})const{
        if(!energy||!action_ready(now))return false;
        const auto spell=std::find_if(spells_.begin(),spells_.end(),[&](const auto& entry){return entry.slot==slot;});
        if(spell==spells_.end())return false;
        const auto* row=skills::find_skill(slot.skill);const auto ready=ready_.find({slot.skill,slot.copy});
        const bool self=spell->targeting==SpellTargetClass::self;
        const auto heal=skills::effects::direct_heal(row->id);
        const bool ally=spell->targeting==SpellTargetClass::ally;
        // Selection must obey the owner's authority before considering content
        // order. LiveActor enforces these same boundaries at admission.
        if(owner_&&(target.scope.instance!=owner_->instance||
            (target.scope==*owner_&&!self&&!(ally&&heal&&heal->self)&&row->id!=1392)))return false;
        const bool target_allowed=self?owner_&&target.scope==*owner_&&target.alive&&!target.hostile&&target.same_plane&&
            target.line_of_sight&&target.distance==0:ally?(heal||row->id==1526||row->id==307)&&owner_&&target.scope.instance==owner_->instance&&
            target.scope.actor&&target.scope.actor<=65535&&target.scope.generation&&target.alive&&!target.hostile&&
            target.same_plane&&target.line_of_sight&&std::isfinite(target.distance)&&target.distance>=0&&
            ((heal&&heal->self)||target.scope!=*owner_):spell_target(row->id,target);
        if((ready!=ready_.end()&&now<ready->second)||*energy<row->energy_cost()||
            target.distance>spell->range||!target_allowed)return false;
        if(external){const auto until=external(slot,now);if(!until||now<*until)return false;}
        return true;
    }
    std::optional<skills::Slot> choose(const SpellTarget& target,std::optional<unsigned> energy,
        std::uint64_t now,const Availability& external={})const{
        // Content order is explicit host preference. Captures do not establish
        // a universal retail skill-priority score.
        for(const auto& spell:spells_)if(eligible(spell.slot,target,energy,now,external))return spell.slot;
        return {};
    }
    bool begin(const npcs::control::Token& token){
        // Admission must preserve the same lifecycle as selection. In
        // particular, cancelling a paid cast does not make its token reusable,
        // and a caller cannot bypass aftercast/base recharge by calling begin
        // directly. LiveActor still owns payment and control authorization.
        if(!action_ready(token.begins)||!token.skill||
           (token.kind!=npcs::control::Action::spell&&token.kind!=npcs::control::Action::skill)||
           !token.actor.instance||!token.actor.actor||token.actor.actor>65535||!token.actor.generation||
           (owner_&&*owner_!=token.actor)||!token.sequence||token.sequence<=last_sequence_||
           token.epoch==UINT64_MAX||token.completes<=token.begins||token.completes-token.begins>60000)return false;
        if(std::none_of(spells_.begin(),spells_.end(),[&](const auto& spell){return spell.slot==*token.skill&&
            (spell.kind==activated::Kind::spell?token.kind==npcs::control::Action::spell:token.kind==npcs::control::Action::skill);}))return false;
        const auto ready=ready_.find({token.skill->skill,token.skill->copy});
        if(ready!=ready_.end()&&token.begins<ready->second)return false;
        clock_=token.begins;owner_=token.actor;last_sequence_=token.sequence;pending_=token;return true;
    }
    bool tracks(const npcs::control::Token& token)const{return pending_&&*pending_==token;}
    bool pending()const{return pending_.has_value();}
    std::optional<std::uint64_t> recharge_until(skills::Slot slot,std::uint64_t now)const{
        if(now<clock_||std::none_of(spells_.begin(),spells_.end(),[&](const auto& spell){return spell.slot==slot;}))return {};
        const auto found=ready_.find({slot.skill,slot.copy});return found==ready_.end()?0:found->second;
    }
    // Completion/prevention begins recharge, not activation. Explicit voluntary
    // cancellation spends the already-paid Energy but starts no base recharge.
    void finish(std::uint64_t now,bool recharge,bool aftercast){
        if(!pending_||now<clock_)return;
        const auto* row=skills::find_skill(pending_->skill->skill);const auto key=std::pair{unsigned(row->id),pending_->skill->copy};
        if(recharge){const auto duration=std::uint64_t(row->recharge)*1000;
            ready_[key]=now<=UINT64_MAX-duration?now+duration:UINT64_MAX;}
        if(aftercast){const auto duration=static_cast<std::uint64_t>(std::ceil(row->aftercast*1000));
            aftercast_=now<=UINT64_MAX-duration?now+duration:UINT64_MAX;}
        clock_=now;pending_.reset();
    }
};
}
