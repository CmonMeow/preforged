#pragma once
#include "gw_movement_effects.h"
#include <limits>

namespace fw::gw::combat::npcs::control {
using Time=std::uint64_t;
using Scope=skills::effects::Scope;
enum class Action {attack,spell,skill};
struct Token {
    Scope actor;std::uint64_t epoch=0,sequence=0;Action kind=Action::attack;
    Time begins=0,completes=0;
    std::optional<skills::Slot> skill;
    bool operator==(const Token&)const=default;
};
enum class Knockdown {accepted,unknown,immune,already_down,invalid};
// Authoritative windups only. A cadence or a client animation is not an action.
// The owning world lock covers scope validation, state mutation and publication.
class State {
    struct Pause {Time begins,ends;};
    Scope actor_;std::uint64_t epoch_=0,sequence_=0;
    Time clock_=0,consumed_=0;
    std::optional<Token> action_;
    std::vector<Pause> pauses_;
    struct MovementPause {Time begins=0;std::optional<Time> ends;};
    std::vector<MovementPause> spell_movement_pauses_;
public:
    explicit State(Scope actor):actor_(actor){
        if(!actor.instance||!actor.actor||!actor.generation)throw std::invalid_argument("invalid NPC control scope");
    }
    std::uint64_t epoch()const{return epoch_;}
    bool exhausted()const{return epoch_==UINT64_MAX;}
    void invalidate(){action_.reset();if(!exhausted())++epoch_;}
    bool down(Time now)const{return std::any_of(pauses_.begin(),pauses_.end(),[&](const auto& p){return p.begins<=now&&now<p.ends;});}
    std::optional<Token> active(Time now)const{
        if(!exhausted()&&action_&&action_->epoch==epoch_&&action_->begins<=now&&now<action_->completes&&!down(now))return action_;
        return {};
    }
    bool attacking(Time now)const{return action_&&action_->kind==Action::attack&&action_->begins<=now&&now<action_->completes&&!down(now);}
    std::optional<Token> begin(Scope actor,Action kind,Time now,Time completes,std::optional<skills::Slot> skill={}){
        if(actor!=actor_||exhausted()||now<clock_||now<consumed_||completes<=now||completes-now>60000||down(now)||action_||
           (kind!=Action::attack&&kind!=Action::spell&&kind!=Action::skill)||sequence_==UINT64_MAX||
           (kind==Action::skill&&!skill))return {};
        if(skill){
            const auto* row=skills::find_skill(skill->skill);
            if(!row||!skill->skill||skill->copy>65535||
               (kind==Action::attack&&row->type!=14)||(kind==Action::skill&&row->type!=10))return {};
        }
        clock_=now;action_=Token{actor_,epoch_,++sequence_,kind,now,completes,skill};return action_;
    }
    bool complete(const Token& token,Time now){
        if(token.actor!=actor_||exhausted()||!action_||*action_!=token||token.epoch!=epoch_||now<clock_||now<consumed_||now<token.completes||down(now))return false;
        clock_=now;action_.reset();return true;
    }
    bool cancel(Scope actor,Time now){
        if(actor!=actor_||now<clock_||now<consumed_||epoch_==UINT64_MAX)return false;
        clock_=now;++epoch_;action_.reset();return true;
    }
    // Only a typed LiveActor revival with confirmed dead/current lifetime may
    // call this. Preserve action sequence and advance replay epoch, while old
    // corpse knockdown/aftercast intervals cannot pause the revived actor.
    bool reset_after_death(Scope actor,Time now){
        if(actor!=actor_||now<clock_||now<consumed_||epoch_==UINT64_MAX)return false;
        clock_=consumed_=now;++epoch_;action_.reset();pauses_.clear();spell_movement_pauses_.clear();return true;
    }
    Knockdown knock_down(Scope actor,std::optional<bool> allowed,Time now,Time duration){
        if(actor!=actor_||now<clock_||now<consumed_||!duration||duration>4000||now>UINT64_MAX-duration||epoch_==UINT64_MAX)return Knockdown::invalid;
        if(!allowed)return Knockdown::unknown;if(!*allowed)return Knockdown::immune;
        if(down(now))return Knockdown::already_down;
        if(pauses_.size()>=4096)return Knockdown::invalid;
        clock_=now;++epoch_;action_.reset();pauses_.push_back({now,now+duration});return Knockdown::accepted;
    }
    // Spellbook pauses are separate from action tokens: completion ends the
    // windup but retains aftercast history until its movement cursor consumes it.
    bool can_begin_movement_pause(Scope actor,Time now)const{
        return actor==actor_&&now>=clock_&&now>=consumed_&&spell_movement_pauses_.size()<4096&&
            (spell_movement_pauses_.empty()||spell_movement_pauses_.back().ends.has_value());
    }
    bool begin_movement_pause(Scope actor,Time now){
        if(!can_begin_movement_pause(actor,now))return false;
        spell_movement_pauses_.push_back({now,{}});return true;
    }
    bool end_movement_pause(Scope actor,Time now,Time aftercast=0){
        if(actor!=actor_||now<consumed_||spell_movement_pauses_.empty())return false;
        auto& pause=spell_movement_pauses_.back();if(pause.ends||now<pause.begins)return false;
        pause.ends=now<=UINT64_MAX-aftercast?now+aftercast:UINT64_MAX;return true;
    }
    std::optional<double> historical_movement_rate(const skills::effects::MovementEffects& movement,Time at,float external)const{
        if(at<consumed_||!std::isfinite(external)||external<=0)return {};
        if(down(at)||std::any_of(spell_movement_pauses_.begin(),spell_movement_pauses_.end(),[&](const auto& pause){
            return at>=pause.begins&&(!pause.ends||at<*pause.ends);}))return 0.;
        const auto rate=movement.at_with_external(at,external);
        if(!std::isfinite(rate)||rate<0)return {};return double(rate);
    }
    std::optional<std::vector<Time>> historical_movement_boundaries(const skills::effects::MovementEffects& movement,Time from,Time through)const{
        if(from<consumed_||from>through)return {};
        auto points=movement.boundaries(from,through);
        const auto append=[&](Time at){if(at>from&&at<=through)points.push_back(at);};
        for(const auto& pause:pauses_){append(pause.begins);append(pause.ends);}
        for(const auto& pause:spell_movement_pauses_){append(pause.begins);if(pause.ends)append(*pause.ends);}
        std::sort(points.begin(),points.end());points.erase(std::unique(points.begin(),points.end()),points.end());return points;
    }
    double movement_integral(const skills::effects::MovementEffects& movement,Time from,Time until)const{
        if(until<=from||from<consumed_)return 0;
        double result=0;auto cursor=from;
        for(const auto& p:pauses_){
            if(p.ends<=cursor)continue;if(p.begins>=until)break;
            if(p.begins>cursor)result+=movement.integral(cursor,std::min(until,p.begins));
            cursor=std::max(cursor,std::min(until,p.ends));if(cursor==until)break;
        }
        return result+movement.integral(cursor,until);
    }
    std::vector<Time> movement_boundaries(const skills::effects::MovementEffects& movement,Time from,Time until)const{
        auto result=movement.boundaries(from,until);
        for(const auto& pause:pauses_){
            if(pause.begins>from&&pause.begins<until)result.push_back(pause.begins);
            if(pause.ends>from&&pause.ends<until)result.push_back(pause.ends);
        }
        std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());return result;
    }
    void consume_movement(Time now){
        if(now<consumed_)return;consumed_=now;
        std::erase_if(pauses_,[&](const auto& p){return p.ends<=now;});
        std::erase_if(spell_movement_pauses_,[&](const auto& p){return p.ends&&*p.ends<=now;});
    }
};
}
