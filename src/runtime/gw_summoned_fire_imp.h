#pragma once
#include "gw_elemental_damage.h"
#include "gw_npc_action_control.h"
#include "gw_projectile_flight.h"
#include <set>
#include <string>
#include <utility>

namespace fw::gw::ai::fire_imp {
using Scope=skills::effects::Scope;
inline bool valid_scope(Scope scope){return scope.instance&&scope.actor&&scope.actor<=65535&&scope.generation;}
// Reviewed host binding, not reconstructed from a display name or damage owner.
// Low-level Fire Magic/maximum Energy are deliberately not guessed from level.
struct Binding {
    Scope caster,owner;
    unsigned level=0,fire_magic=0;
    std::string ownership_evidence,rank_evidence;
    bool operator==(const Binding&)const=default;
    bool valid()const{return valid_scope(caster)&&valid_scope(owner)&&caster!=owner&&caster.instance==owner.instance&&
        level>=1&&level<=20&&fire_magic<=20&&!ownership_evidence.empty()&&!rank_evidence.empty();}
};
// Host resource authority issues this only after actual NPC Energy payment.
// This module verifies the payment matches the admitted action; it owns no
// second Energy pool and never borrows the summoner's Energy.
struct Payment {
    Scope caster;skills::Slot slot;
    std::uint64_t action_sequence=0,at=0;
    unsigned before=0,after=0,maximum=0;
    std::string evidence;
};
struct Pose {Scope scope;float x=0,y=0;unsigned plane=0;bool alive=false;};
inline bool valid_pose(const Pose& pose){return valid_scope(pose.scope)&&std::isfinite(pose.x)&&std::isfinite(pose.y);}
struct Recipient {
    Pose pose;combat::Actor actor;combat::conditions::State conditions;
    bool hostile_to_summon=false;
};
struct Hit {
    Scope recipient,caster,credit_owner;
    skills::effects::ElementalResult result;
};
enum class Outcome {pending,hit,miss,stale,unsupported,consumed};
struct Impact {Outcome outcome=Outcome::pending;std::vector<Hit> hits;};
class Projectile {
    Binding binding_;Scope target_;skills::effects::ElementalPlan plan_;
    projectiles::Policy flight_;float aim_x_=0,aim_y_=0;unsigned plane_=0;
    std::uint64_t arrives_=0;bool consumed_=false;
    Projectile(Binding binding,Scope target,skills::effects::ElementalPlan plan,projectiles::Policy flight,
        projectiles::Aim aim,unsigned plane,std::uint64_t release):binding_(std::move(binding)),target_(target),plan_(plan),
        flight_(flight),aim_x_(aim.x),aim_y_(aim.y),plane_(plane),arrives_(release+aim.flight_ms){}
public:
    Projectile(const Projectile&)=delete;
    Projectile& operator=(const Projectile&)=delete;
    Projectile(Projectile&& other)noexcept:binding_(std::move(other.binding_)),target_(other.target_),plan_(other.plan_),
        flight_(other.flight_),aim_x_(other.aim_x_),aim_y_(other.aim_y_),plane_(other.plane_),arrives_(other.arrives_),
        consumed_(std::exchange(other.consumed_,true)){}
    Projectile& operator=(Projectile&& other)noexcept{
        if(this==&other)return *this;
        binding_=std::move(other.binding_);target_=other.target_;plan_=other.plan_;flight_=other.flight_;
        aim_x_=other.aim_x_;aim_y_=other.aim_y_;plane_=other.plane_;arrives_=other.arrives_;
        consumed_=std::exchange(other.consumed_,true);return *this;
    }
    static std::optional<Projectile> launch(const Binding& binding,const combat::npcs::control::Token& token,
        const Payment& paid,const Pose& source,const Pose& target,float target_vx,float target_vy,
        bool line_of_sight,bool hostile,projectiles::Policy policy,std::uint64_t release){
        if(!binding.valid()||token.actor!=binding.caster||!token.sequence||
           token.kind!=combat::npcs::control::Action::spell||!token.skill||
           (token.skill->skill!=186&&token.skill->skill!=194)||token.skill->copy>65535||release<token.completes||token.completes<=token.begins||
           paid.caster!=binding.caster||paid.slot!=*token.skill||paid.action_sequence!=token.sequence||paid.at!=token.begins||
           paid.evidence.empty()||!paid.maximum||paid.before>paid.maximum||paid.after>paid.before||
           !valid_pose(source)||!valid_pose(target)||!source.alive||!target.alive||source.scope!=binding.caster||
           target.scope==binding.owner||target.scope==binding.caster||target.scope.instance!=binding.caster.instance||
           source.plane!=target.plane||!line_of_sight||!hostile||!policy.valid())return {};
        const auto* row=skills::find_skill(token.skill->skill);
        if(paid.before<row->energy_cost()||paid.before-paid.after!=row->energy_cost())return {};
        const auto plan=skills::effects::elemental(row->id,binding.fire_magic);
        if(!plan||std::hypot(source.x-target.x,source.y-target.y)>plan->range)return {};
        const auto aim=projectiles::intercept(policy,source.x,source.y,target.x,target.y,target_vx,target_vy);
        if(!aim||release>UINT64_MAX-aim->flight_ms)return {};
        return Projectile(binding,target.scope,*plan,policy,*aim,target.plane,release);
    }
    std::uint64_t arrives()const{return arrives_;}
    const Binding& binding()const{return binding_;}
    // World supplies live generations/health/ownership under its instance lock.
    // Replacing the owner, caster or selected target consumes the old release.
    Impact impact(const Binding& current,bool owner_present,bool caster_alive,
        const std::vector<Recipient>& contacts,std::uint64_t now){
        if(consumed_)return {Outcome::consumed,{}};
        if(now<arrives_)return {};
        consumed_=true;
        if(current!=binding_||!owner_present||!caster_alive)return {Outcome::stale,{}};
        const auto selected=std::find_if(contacts.begin(),contacts.end(),[&](const auto& c){return c.pose.scope==target_;});
        if(selected==contacts.end()||!valid_pose(selected->pose)||!selected->pose.alive||
           selected->pose.plane!=plane_||!selected->hostile_to_summon||selected->actor.id!=target_.actor||
           selected->actor.generation!=target_.generation||!selected->actor.health||
           selected->actor.health->current<=0)return {Outcome::stale,{}};
        if(std::hypot(selected->pose.x-aim_x_,selected->pose.y-aim_y_)>flight_.hit_radius)return {Outcome::miss,{}};
        Impact impact{Outcome::hit,{}};std::set<Scope> seen;
        for(const auto& contact:contacts){
            if(!valid_pose(contact.pose)||!contact.pose.alive||contact.pose.plane!=plane_||!contact.hostile_to_summon||
               contact.pose.scope.instance!=binding_.caster.instance||contact.pose.scope==binding_.caster||
               contact.pose.scope==binding_.owner||!seen.insert(contact.pose.scope).second||
               contact.actor.id!=contact.pose.scope.actor||contact.actor.generation!=contact.pose.scope.generation||
               (plan_.skill==194?contact.pose.scope!=target_:
                   std::hypot(contact.pose.x-aim_x_,contact.pose.y-aim_y_)>plan_.radius))continue;
            auto result=skills::effects::elemental_target(plan_,contact.actor,contact.conditions,binding_.level,binding_.caster.actor,now);
            if(!result.accepted)return {Outcome::unsupported,{}}; // No partial caller commit.
            impact.hits.push_back({contact.pose.scope,binding_.caster,binding_.owner,std::move(result)});
        }
        return impact;
    }
};
}
