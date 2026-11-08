#pragma once
#include "gw_elemental_damage.h"
#include "gw_water_projectiles.h"
#include <variant>

namespace fw::gw::combat::ai::offensive {
using Scope=skills::effects::Scope;
// Host-reviewed live observation, not a skillbar inference. Unknown, stale
// generation or future observations never authorize the no-Overcast branch.
struct OvercastObservation {
    Scope source;unsigned amount=0;std::uint64_t at=0;std::string evidence;
};
inline bool known_zero(const std::optional<OvercastObservation>& observed,Scope source,std::uint64_t now){
    return observed&&source.instance&&source.actor&&source.generation&&observed->source==source&&
        observed->amount==0&&observed->at<=now&&!observed->evidence.empty();
}

// Immutable reviewed variants. Ice Spear is deliberately not reclassified as
// an ordinary fire-family ElementalPlan: its conditional self enchantment is a
// distinct completion effect and requires separate Overcast authority.
class Plan {
    std::variant<skills::effects::ElementalPlan,skills::effects::WaterProjectilePlan> value_;
public:
    Plan()=default;
    Plan(const skills::effects::ElementalPlan& p):value_(p){
        if((p.skill!=183&&p.skill!=186&&p.skill!=188&&p.skill!=191&&p.skill!=194)||!skills::effects::valid_elemental(p))
            throw std::invalid_argument("unsupported NPC offensive elemental plan");
    }
    Plan(const skills::effects::WaterProjectilePlan& p):value_(p){
        if(p.skill!=214||!skills::effects::valid_water_projectile(p))
            throw std::invalid_argument("unsupported NPC offensive water plan");
    }
    unsigned skill()const{return std::visit([](const auto& p){return p.skill;},value_);}
    unsigned rank()const{return std::visit([](const auto& p){return p.rank;},value_);}
    // AI engagement distance for an untargeted PBAoE is its effect radius,
    // not the generic spell-range field retained in the canonical effect plan.
    float range()const{return caster_center()?radius():std::visit([](const auto& p){return p.range;},value_);}
    const skills::effects::ElementalPlan* elemental()const{return std::get_if<skills::effects::ElementalPlan>(&value_);}
    const skills::effects::WaterProjectilePlan* ice_spear()const{return std::get_if<skills::effects::WaterProjectilePlan>(&value_);}
    bool valid()const{return ice_spear()?skill()==214&&skills::effects::valid_water_projectile(*ice_spear()):
        (skill()==183||skill()==186||skill()==188||skill()==191||skill()==194)&&skills::effects::valid_elemental(*elemental());}
    bool caster_center()const{return elemental()&&elemental()->caster_center;}
    float radius()const{return elemental()?elemental()->radius:0;}
    bool projectile()const{return ice_spear()||elemental()->projectile;}
    combat::Kind kind()const{return ice_spear()?combat::Kind::cold:elemental()->kind;}
    bool operator==(const Plan& other)const{
        if(value_.index()!=other.value_.index())return false;
        if(elemental())return *elemental()==*other.elemental();
        // Constructors accept only complete canonical water plans.
        return skill()==other.skill()&&rank()==other.rank();
    }
};

inline skills::effects::ElementalResult target(const Plan& plan,Scope source,Scope destination,
    combat::Actor actor,const combat::conditions::State& conditions,unsigned level,std::uint64_t now,const combat::DamageProjection& projection={}){
    if(!plan.valid())return {};
    if(plan.elemental())return skills::effects::elemental_target(*plan.elemental(),actor,conditions,level,source.actor,now,false,1,true,{},projection);
    const auto result=skills::effects::water_projectile_target({source,destination,*plan.ice_spear(),false,now},actor,conditions,level,false,{},projection);
    if(!result.accepted)return {};
    return {true,result.actor,conditions,{result.hit},{},0};
}
}
