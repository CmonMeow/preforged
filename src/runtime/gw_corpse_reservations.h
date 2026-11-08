#pragma once
#include "gw_effect_registry.h"
#include <map>
#include <set>
namespace fw::gw::quests {
// Instance mutex covers validation, reservation, publication and consumption.
// Lifetime keys prevent a recycled actor number from inheriting a reservation.
class CorpseReservations {
public:
    using Scope=skills::effects::Scope;
    struct Ticket {Scope owner;unsigned quest=0,cycle=0,skill=0;std::uint64_t attempt=0;bool operator==(const Ticket&)const=default;};
private:
    std::map<Scope,Ticket> reserved_;
    std::set<Scope> consumed_;
    static bool valid(Scope s){return s.instance&&s.actor&&s.actor<=65535&&s.generation;}
public:
    bool reserve(Scope corpse,Ticket ticket){
        const bool quest=ticket.quest&&ticket.cycle&&!ticket.skill&&!ticket.attempt;
        const bool exploitation=!ticket.quest&&!ticket.cycle&&
            ((ticket.skill>=91&&ticket.skill<=94)||ticket.skill==97)&&ticket.attempt;
        if(!valid(corpse)||!valid(ticket.owner)||corpse.instance!=ticket.owner.instance||corpse==ticket.owner||
           (!quest&&!exploitation)||consumed_.contains(corpse))return false;
        const auto found=reserved_.find(corpse);
        if(found!=reserved_.end())return found->second==ticket;
        if(reserved_.size()+consumed_.size()>=65535)return false;
        return reserved_.emplace(corpse,ticket).second;
    }
    bool owns(Scope corpse,Ticket ticket)const{
        const auto found=reserved_.find(corpse);return found!=reserved_.end()&&found->second==ticket;
    }
    bool available(Scope corpse)const{return valid(corpse)&&!reserved_.contains(corpse)&&!consumed_.contains(corpse);}
    // A known pet lifetime always leaves a pre-exploited corpse, including
    // after its owner departs. This is independent of the ownership map.
    bool can_pre_exploit(Scope corpse)const{
        return valid(corpse)&&!reserved_.contains(corpse)&&
            (consumed_.contains(corpse)||reserved_.size()+consumed_.size()<65535);
    }
    bool pre_exploit(Scope corpse){
        if(!can_pre_exploit(corpse))return false;consumed_.insert(corpse);return true;
    }
    bool reserve_exploitation(Scope corpse,Scope owner,unsigned skill,std::uint64_t attempt){return reserve(corpse,{owner,0,0,skill,attempt});}
    bool owns_exploitation(Scope corpse,Scope owner,unsigned skill,std::uint64_t attempt)const{return owns(corpse,{owner,0,0,skill,attempt});}
    bool consume_exploitation(Scope corpse,Scope owner,unsigned skill,std::uint64_t attempt){return consume(corpse,{owner,0,0,skill,attempt});}
    bool reserve_well(Scope corpse,Scope owner,unsigned skill,std::uint64_t attempt){return skill>=91&&skill<=94&&reserve_exploitation(corpse,owner,skill,attempt);}
    bool owns_well(Scope corpse,Scope owner,unsigned skill,std::uint64_t attempt)const{return skill>=91&&skill<=94&&owns(corpse,{owner,0,0,skill,attempt});}
    bool consume_well(Scope corpse,Scope owner,unsigned skill,std::uint64_t attempt){return skill>=91&&skill<=94&&consume(corpse,{owner,0,0,skill,attempt});}
    bool consume(Scope corpse,Ticket ticket){
        if(!owns(corpse,ticket))return false;
        consumed_.insert(corpse);reserved_.erase(corpse);return true;
    }
    bool release(Scope corpse,Ticket ticket){
        if(!owns(corpse,ticket))return false;reserved_.erase(corpse);return true;
    }
    void retire(Scope corpse){reserved_.erase(corpse);consumed_.erase(corpse);}
};
}
