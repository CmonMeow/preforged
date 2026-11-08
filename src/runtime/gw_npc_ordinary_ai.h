#pragma once
#include "gw_npc_action_control.h"
#include "gw_projectile_flight.h"
#include <map>
#include <string>
#include <vector>
#include <stdexcept>

namespace fw::gw::combat::ai::ordinary {
using Scope=skills::effects::Scope;
using Token=npcs::control::Token;
using Time=std::uint64_t;
inline bool valid_scope(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
enum class Delivery {melee,ranged};
// Captured mixed spell/ordinary actions establish capability, not these
// numbers. Every enabled profile must explicitly document authored timing,
// damage, range and projectile policy. No inventory item is manufactured.
// Evidence: build/reports/ai/four-action-sequences.json, Firestarter lifetime
// 53:161:1: Flare141363, ordinary141435, stop141447, Flare141448 in the pinned
// four-professions packets. This does not establish an Energy-driven retail
// priority rule; choosing ready spells first is explicit private policy.
struct Reviewed {
    unsigned minimum=0,maximum=0,mastery=0,damage_type=0;
    bool caster_weapon=false;
    Delivery delivery=Delivery::melee;
    Time windup_ms=0,period_ms=0;
    double range=0;
    projectiles::Policy flight;
    std::string evidence;
    bool valid()const{
        return minimum&&maximum>=minimum&&maximum<=1000000&&mastery<=20&&
            (damage_type==0||damage_type==1||damage_type==2||damage_type==3||damage_type==4||damage_type==5||damage_type==11)&&
            (delivery==Delivery::melee||delivery==Delivery::ranged)&&windup_ms&&windup_ms<=60000&&
            period_ms>=windup_ms&&period_ms<=60000&&std::isfinite(range)&&range>0&&range<=5000&&
            flight.valid()&&!evidence.empty()&&evidence.size()<=1024;
    }
};
struct Pose {
    Scope scope;float x=0,y=0;unsigned plane=0;bool alive=false;
    Time clock=0;float vx=0,vy=0;bool observed=true;
};
inline bool valid_pose(const Pose& p){return valid_scope(p.scope)&&std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.vx)&&std::isfinite(p.vy);}
struct Windup {std::uint64_t id=0;Scope target;Token token;bool operator==(const Windup&)const=default;};
struct Release {
    Windup attack;float aim_x=0,aim_y=0;unsigned plane=0;Time at=0,arrives=0;float origin_x=0,origin_y=0;
    bool operator==(const Release&)const=default;
};
enum class ReleaseStatus {invalid,deferred,stopped,ready};
struct ReleaseProposal {
    ReleaseStatus status=ReleaseStatus::invalid;Windup attack;Time at=0;
    std::optional<Release> release;
    bool operator==(const ReleaseProposal&)const=default;
};
enum class Contact {invalid,deferred,retired,obstruction,miss,contact};
struct Impact {Contact status=Contact::invalid;Release release;Time at=0;bool operator==(const Impact&)const=default;};

// Action control remains LiveActor's authority. This ledger only tracks the
// accepted attack, ordinary cadence and immutable released deliveries. World
// performs spell-aftercast readiness, hostile eligibility, accuracy, block,
// armor, damage and attack-outcome reactions under its lock.
class Ledger {
    Scope source_;Reviewed profile_;std::size_t capacity_;
    Time clock_=0,ready_=0;std::uint64_t nonce_=0,last_sequence_=0;
    std::optional<Windup> pending_;
    std::optional<ReleaseProposal> prepared_;
    std::map<std::uint64_t,Release> flights_;
public:
    Ledger(Scope source,Reviewed profile,std::size_t capacity=8):source_(source),profile_(std::move(profile)),capacity_(capacity){
        if(!valid_scope(source)||!profile_.valid()||!capacity||capacity>64)throw std::invalid_argument("invalid NPC ordinary attack policy");
    }
    const Reviewed& profile()const{return profile_;}
    std::size_t in_flight()const{return flights_.size();}
    Time clock()const{return clock_;}
    std::optional<Time> next_arrival()const{
        std::optional<Time> next;for(const auto&[id,shot]:flights_)if(!next||shot.arrives<*next)next=shot.arrives;return next;
    }
    std::optional<Release> released(std::uint64_t id)const{
        const auto shot=flights_.find(id);return shot==flights_.end()?std::nullopt:std::optional(shot->second);
    }
    const std::optional<Windup>& pending()const{return pending_;}
    bool ready(Time now)const{return !pending_&&now>=clock_&&now>=ready_&&flights_.size()<capacity_&&nonce_!=UINT64_MAX;}
    bool accept(Scope target,const Token& token,Time period){
        if(!ready(token.begins)||!valid_scope(target)||target.instance!=source_.instance||target==source_||
           token.actor!=source_||token.kind!=npcs::control::Action::attack||token.skill||!token.sequence||token.sequence<=last_sequence_||
           token.completes<=token.begins||token.completes-token.begins>60000||period<token.completes-token.begins||period>60000||
           token.begins>UINT64_MAX-period)return false;
        clock_=token.begins;ready_=token.begins+period;last_sequence_=token.sequence;
        pending_=Windup{++nonce_,target,token};prepared_.reset();return true;
    }
    bool cancel(Time now){
        if(!pending_||now<clock_)return false;clock_=now;pending_.reset();prepared_.reset();return true;
    }
    ReleaseProposal prepare_release(const Token& token,const Pose& source,const Pose& target,std::uint64_t epoch,bool clear,Time now){
        ReleaseProposal p;
        prepared_.reset();
        if(!pending_||pending_->token!=token||now<clock_)return p;
        p.attack=*pending_;p.at=now;
        if(now<token.completes||source.clock>now||target.clock>now||!source.observed||!target.observed){p.status=ReleaseStatus::deferred;return p;}
        p.status=ReleaseStatus::stopped;
        if(valid_pose(source)&&valid_pose(target)&&source.scope==source_&&target.scope==pending_->target&&
           source.alive&&target.alive&&source.plane==target.plane&&token.epoch==epoch&&clear&&
           std::hypot(double(target.x)-source.x,double(target.y)-source.y)<=profile_.range){
            Release r{*pending_,target.x,target.y,target.plane,now,now,source.x,source.y};
            if(profile_.delivery==Delivery::ranged){
                const auto aim=projectiles::intercept(profile_.flight,source.x,source.y,target.x,target.y,target.vx,target.vy);
                if(!aim||now>UINT64_MAX-aim->flight_ms){prepared_=p;return p;}
                r.aim_x=aim->x;r.aim_y=aim->y;r.arrives+=aim->flight_ms;
            }
            p.status=ReleaseStatus::ready;p.release=r;
        }
        prepared_=p;return p;
    }
    // Call only after real control completion/cancellation. Any callbacks that
    // kill the source before release pass survives=false. Released flights are
    // thereafter independent of source death, selection or control epoch.
    bool commit_release(const ReleaseProposal& p,bool survives=true){
        if(!prepared_||*prepared_!=p||!pending_||p.attack!=*pending_||p.at<clock_)return false;
        clock_=p.at;pending_.reset();prepared_.reset();
        if(p.status==ReleaseStatus::ready&&survives)flights_.emplace(p.attack.id,*p.release);
        return true;
    }
    std::vector<std::uint64_t> released_ids()const{
        std::vector<std::uint64_t> result;for(const auto& [id,shot]:flights_)result.push_back(id);return result;
    }
    Impact prepare_impact(std::uint64_t id,const Pose& target,bool clear,Time now)const{
        const auto it=flights_.find(id);if(it==flights_.end()||now<clock_)return {};
        Impact p{Contact::deferred,it->second,now};
        if(now<p.release.arrives||target.clock>now||!target.observed)return p;
        if(!valid_pose(target)||target.scope!=p.release.attack.target||!target.alive||target.plane!=p.release.plane)p.status=Contact::retired;
        else if(!clear)p.status=Contact::obstruction;
        else if(profile_.delivery==Delivery::ranged&&std::hypot(double(target.x)-p.release.aim_x,double(target.y)-p.release.aim_y)>profile_.flight.hit_radius)p.status=Contact::miss;
        else p.status=Contact::contact;
        return p;
    }
    // Consume before World callbacks. A geometric contact is not a hit: World
    // still resolves blind/miss, block and all shared ordinary-hit observers.
    bool commit_impact(const Impact& p){
        const auto it=flights_.find(p.release.attack.id);
        if(it==flights_.end()||it->second!=p.release||p.at<clock_||p.at<p.release.arrives||
           (p.status!=Contact::retired&&p.status!=Contact::obstruction&&p.status!=Contact::miss&&p.status!=Contact::contact))return false;
        clock_=p.at;flights_.erase(it);return true;
    }
};
}
