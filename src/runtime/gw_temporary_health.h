#pragma once
#include "gw_to_limit.h"
#include "gw_endure_pain.h"
#include "gw_defy_pain.h"
#include "gw_stamina_signet.h"
#include "gw_deep_wound.h"
namespace fw::gw::skills::effects {
// Reviewed producers316,347,318/3204 and indefinite1411. The permanent capacity already includes the
// caller's base/equipment/morale calculation;skill bonuses never overwrite it.
class TemporaryHealth {
public:
    struct Source {unsigned effect,rank,bonus;Time begins,ends;};
private:
    Scope owner_;unsigned permanent_,last_effect_=0;Time clock_;
    std::optional<Source> limit_,pain_,defy_,stamina_;
    bool mutable_at(Time now)const{return owner_.instance&&now>=clock_&&(!deadline()||now<=*deadline());}
public:
    TemporaryHealth(Scope owner,unsigned permanent,Time now):owner_(owner),permanent_(permanent),clock_(now){
        if(!owner.instance||!owner.actor||owner.actor>65535||!owner.generation||!permanent||permanent>1000000)throw std::invalid_argument("invalid temporary Health authority");
    }
    Scope owner()const{return owner_;}Time cursor()const{return clock_;}
    unsigned permanent()const{return permanent_;}
    unsigned capacity()const{return permanent_+(limit_?limit_->bonus:0)+(pain_?pain_->bonus:0)+(defy_?defy_->bonus:0)+(stamina_?stamina_->bonus:0);}
    const std::optional<Source>& source()const{return limit_;}
    const std::optional<Source>& endure_pain_source()const{return pain_;}
    const std::optional<Source>& defy_pain_source()const{return defy_;}
    const std::optional<Source>& stamina_signet_source()const{return stamina_;}
    std::optional<Time> deadline()const{std::optional<Time> result;for(const auto* s:{&limit_,&pain_,&defy_})if(*s)result=result?std::min(*result,(*s)->ends):(*s)->ends;return result;}
    std::optional<unsigned> expiring_effect(Time now)const{if(limit_&&limit_->ends==now)return limit_->effect;if(pain_&&pain_->ends==now)return pain_->effect;if(defy_&&defy_->ends==now)return defy_->effect;return {};}

    bool observe(Time now){if(!mutable_at(now))return false;clock_=now;return true;}
    bool apply_to_limit(const ToLimitPlan& plan,unsigned effect,Time now){
        const auto canonical=to_limit(plan.rank);
        if(!canonical||*canonical!=plan||!mutable_at(now)||(deadline()&&now==*deadline())||!effect||effect<=last_effect_||now>UINT64_MAX-plan.duration||capacity()-(limit_?limit_->bonus:0)>1000000-plan.health_bonus)return false;
        limit_=Source{effect,plan.rank,plan.health_bonus,now,now+plan.duration};last_effect_=effect;clock_=now;return true;
    }
    bool apply_endure_pain(const EndurePainPlan& plan,unsigned effect,Time now){
        const auto canonical=endure_pain(plan.rank);
        if(!canonical||*canonical!=plan||!mutable_at(now)||(deadline()&&now==*deadline())||!effect||effect<=last_effect_||
           now>UINT64_MAX-plan.duration||capacity()-(pain_?pain_->bonus:0)>1000000-plan.health_bonus)return false;
        pain_=Source{effect,plan.rank,plan.health_bonus,now,now+plan.duration};last_effect_=effect;clock_=now;return true;
    }
    bool apply_defy_pain(const DefyPainPlan& plan,unsigned effect,Time now){
        const auto canonical=defy_pain(plan.rank,plan.skill==3204);
        if(!canonical||*canonical!=plan||!mutable_at(now)||(deadline()&&now==*deadline())||!effect||effect<=last_effect_||
           now>UINT64_MAX-plan.duration||capacity()-(defy_?defy_->bonus:0)>1000000-plan.health_bonus)return false;
        defy_=Source{effect,plan.rank,plan.health_bonus,now,now+plan.duration};last_effect_=effect;clock_=now;return true;
    }
    bool apply_stamina_signet(const StaminaSignetPlan& plan,unsigned effect,Time now){
        const auto canonical=stamina_signet(plan.rank);
        if(!canonical||*canonical!=plan||!mutable_at(now)||
           (deadline()&&now==*deadline())||!effect||effect<=last_effect_||
           capacity()-(stamina_?stamina_->bonus:0)>1000000-plan.health_bonus)return false;
        stamina_=Source{effect,plan.rank,plan.health_bonus,now,UINT64_MAX};last_effect_=effect;clock_=now;return true;
    }
    bool remove(unsigned effect,Time now){
        if(!mutable_at(now))return false;
        if(limit_&&limit_->effect==effect)limit_.reset();
        else if(pain_&&pain_->effect==effect)pain_.reset();else if(defy_&&defy_->effect==effect)defy_.reset();
        else if(stamina_&&stamina_->effect==effect)stamina_.reset();else return false;
        clock_=now;return true;
    }
    bool resize_permanent(unsigned capacity,Time now){
        if(!capacity||capacity>1000000||!mutable_at(now)||(deadline()&&now==*deadline())||capacity>1000000-(this->capacity()-permanent_))return false;
        permanent_=capacity;clock_=now;return true;
    }
};
struct TemporaryHealthSnapshot {Scope owner;Time at;double current;combat::deep_wound::State capacity;};
enum class HealthCapacityChange {temporary_source,permanent_capacity};
struct TemporaryHealthTransition {TemporaryHealth ledger;TemporaryHealthSnapshot health;std::optional<unsigned> removed;};
inline std::optional<TemporaryHealthSnapshot> temporary_health_transition(const TemporaryHealth& before,const TemporaryHealth& after,
        TemporaryHealthSnapshot snapshot,bool wounded,HealthCapacityChange change){
    auto& state=snapshot.capacity;
    if(snapshot.owner!=before.owner()||snapshot.owner!=after.owner()||snapshot.at<before.cursor()||snapshot.at!=after.cursor()||
       state.base!=before.capacity()||state.reduction!=(state.active?std::min(100u,state.base/5):0u)||
       !std::isfinite(snapshot.current)||snapshot.current<0||snapshot.current>state.maximum()||!std::isfinite(state.debt)||state.debt<0||(!snapshot.current&&state.debt))return {};
    if(change==HealthCapacityChange::permanent_capacity){state.capacity(snapshot.current,after.capacity(),wounded);return snapshot;}
    const auto next=after.capacity(),loss=wounded?std::min(100u,next/5):0;
    if(snapshot.current>0){
        // Temporary loss preserves latent Health instead of discarding it at
        // the living display floor. This is capacity change,not healing/damage.
        const double latent=snapshot.current-state.debt+state.reduction+double(next)-state.base-loss;
        snapshot.current=std::clamp(latent,1.,double(next-loss));state.debt=std::max(0.,snapshot.current-latent);
    }else state.debt=0; // A capacity change never resurrects a corpse.
    state.base=next;state.reduction=loss;state.active=wounded;return snapshot;
}
inline std::optional<TemporaryHealthTransition> prepare_to_limit_health(const TemporaryHealth& before,TemporaryHealthSnapshot snapshot,
        const ToLimitPlan& plan,unsigned effect,bool wounded){
    auto next=before;if(!next.apply_to_limit(plan,effect,snapshot.at))return {};
    auto result=temporary_health_transition(before,next,snapshot,wounded,HealthCapacityChange::temporary_source);if(!result)return {};
    return TemporaryHealthTransition{std::move(next),*result,before.source()?std::optional(before.source()->effect):std::nullopt};
}
inline std::optional<TemporaryHealthTransition> prepare_endure_pain_health(const TemporaryHealth& before,TemporaryHealthSnapshot snapshot,
        const EndurePainPlan& plan,unsigned effect,bool wounded){
    auto next=before;if(!next.apply_endure_pain(plan,effect,snapshot.at))return {};
    auto result=temporary_health_transition(before,next,snapshot,wounded,HealthCapacityChange::temporary_source);if(!result)return {};
    return TemporaryHealthTransition{std::move(next),*result,before.endure_pain_source()?std::optional(before.endure_pain_source()->effect):std::nullopt};
}
inline std::optional<TemporaryHealthTransition> prepare_defy_pain_health(const TemporaryHealth& before,TemporaryHealthSnapshot snapshot,
        const DefyPainPlan& plan,unsigned effect,bool wounded){
    auto next=before;if(!next.apply_defy_pain(plan,effect,snapshot.at))return {};
    auto result=temporary_health_transition(before,next,snapshot,wounded,HealthCapacityChange::temporary_source);if(!result)return {};
    return TemporaryHealthTransition{std::move(next),*result,before.defy_pain_source()?std::optional(before.defy_pain_source()->effect):std::nullopt};
}
inline std::optional<TemporaryHealthTransition> prepare_stamina_signet_health(const TemporaryHealth& before,TemporaryHealthSnapshot snapshot,
        const StaminaSignetPlan& plan,unsigned effect,bool wounded){
    auto next=before;if(!next.apply_stamina_signet(plan,effect,snapshot.at))return {};
    auto result=temporary_health_transition(before,next,snapshot,wounded,HealthCapacityChange::temporary_source);if(!result)return {};
    return TemporaryHealthTransition{std::move(next),*result,before.stamina_signet_source()?std::optional(before.stamina_signet_source()->effect):std::nullopt};
}
inline std::optional<TemporaryHealthTransition> prepare_temporary_health_end(const TemporaryHealth& before,TemporaryHealthSnapshot snapshot,unsigned effect,bool wounded){
    auto next=before;if(!next.remove(effect,snapshot.at))return {};
    auto result=temporary_health_transition(before,next,snapshot,wounded,HealthCapacityChange::temporary_source);if(!result)return {};
    return TemporaryHealthTransition{std::move(next),*result,effect};
}
inline std::optional<TemporaryHealthTransition> prepare_temporary_health_expiry(const TemporaryHealth& before,TemporaryHealthSnapshot snapshot,bool wounded){
    const auto effect=before.expiring_effect(snapshot.at);if(!effect)return {};
    return prepare_temporary_health_end(before,snapshot,*effect,wounded);
}
inline std::optional<TemporaryHealthTransition> prepare_temporary_health_condition(const TemporaryHealth& before,TemporaryHealthSnapshot snapshot,bool wounded){
    auto next=before;if(!next.observe(snapshot.at))return {};
    auto result=temporary_health_transition(before,next,snapshot,wounded,HealthCapacityChange::temporary_source);if(!result)return {};
    return TemporaryHealthTransition{std::move(next),*result,{}};
}
inline std::optional<TemporaryHealthTransition> prepare_permanent_health_resize(const TemporaryHealth& before,TemporaryHealthSnapshot snapshot,unsigned capacity,bool wounded){
    auto next=before;if(!next.resize_permanent(capacity,snapshot.at))return {};
    auto result=temporary_health_transition(before,next,snapshot,wounded,HealthCapacityChange::permanent_capacity);if(!result)return {};
    return TemporaryHealthTransition{std::move(next),*result,{}};
}
}
