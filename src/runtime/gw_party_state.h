#pragma once
#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>
namespace fw::gw::party {
// Instance-owned; caller holds the world lock. Parties never cross instances.
class State {
public:
    using Id=std::uint16_t;
    struct Invitation {Id from,to;std::uint64_t expires;};
private:
    unsigned capacity_;
    std::map<Id,std::vector<Id>> groups_;
    std::map<Id,Id> membership_;
    std::map<std::pair<Id,Id>,std::uint64_t> invitations_;
    Id free_id()const{
        for(unsigned id=1;id<=65535;++id)if(!groups_.contains(static_cast<Id>(id)))return static_cast<Id>(id);
        throw std::runtime_error("party ID capacity exhausted");
    }
    void invalidate(Id group){
        std::erase_if(invitations_,[group](const auto& entry){return entry.first.first==group||entry.first.second==group;});
    }
public:
    explicit State(unsigned capacity):capacity_(capacity){
        if(!capacity||capacity>12)throw std::invalid_argument("invalid party capacity");
    }
    Id group(Id player)const {auto it=membership_.find(player);return it==membership_.end()?0:it->second;}
    const std::vector<Id>& members(Id group)const{return groups_.at(group);}
    bool contains(Id group)const{return groups_.contains(group);}
    bool leader(Id player)const {const auto id=group(player);return id&&groups_.at(id).front()==player;}
    bool add(Id player){
        if(!player||membership_.contains(player))return false;
        const auto id=groups_.contains(player)?free_id():player;
        groups_.emplace(id,std::vector<Id>{player});membership_.emplace(player,id);return true;
    }
    void expire(std::uint64_t now){std::erase_if(invitations_,[now](const auto& entry){return entry.second<=now;});}
    std::vector<Invitation> invitations()const{
        std::vector<Invitation> result;for(auto [pair,deadline]:invitations_)result.push_back({pair.first,pair.second,deadline});return result;
    }
    bool invite(Id sender,Id target,std::uint64_t now){
        expire(now);
        const auto from=group(sender),to=group(target);
        if(!leader(sender)||!leader(target)||from==to||groups_.at(from).size()+groups_.at(to).size()>capacity_)return false;
        const auto key=std::pair{from,to};
        if(invitations_.contains(key))return true; // repeats must not prolong consent
        // The 2022 client stores each remote party in one intrusive invitation
        // list. A reciprocal request would link the same object twice and assert.
        // Keep the existing invitation available for explicit accept/reject.
        if(invitations_.contains({to,from}))return false;
        if(invitations_.size()>=1024)return false;
        const auto remaining=std::numeric_limits<std::uint64_t>::max()-now;
        if(!remaining)return false;
        invitations_[key]=now+std::min<std::uint64_t>(60000,remaining);return true;
    }
    bool cancel(Id sender,Id target_party){return leader(sender)&&invitations_.erase({group(sender),target_party})!=0;}
    bool reject(Id recipient,Id source_party){return leader(recipient)&&invitations_.erase({source_party,group(recipient)})!=0;}
    bool accept(Id recipient,Id source_party,std::uint64_t now){
        expire(now);const auto destination=group(recipient);
        if(!leader(recipient)||!invitations_.contains({source_party,destination})||!groups_.contains(source_party))return false;
        auto& source=groups_.at(source_party);const auto& incoming=groups_.at(destination);
        if(source.size()+incoming.size()>capacity_)return false;
        source.insert(source.end(),incoming.begin(),incoming.end());
        for(auto player:incoming)membership_.at(player)=source_party;
        groups_.erase(destination);invalidate(source_party);invalidate(destination);return true;
    }
    bool remove(Id player){
        const auto id=group(player);if(!id)return false;
        auto& members=groups_.at(id);std::erase(members,player);membership_.erase(player);invalidate(id);
        if(members.empty())groups_.erase(id);return true;
    }
    bool leave(Id player){
        const auto id=group(player);if(!id)return false;
        if(groups_.at(id).size()==1)return true;
        const auto solo=free_id(); // validate allocation before changing membership
        auto& members=groups_.at(id);std::erase(members,player);invalidate(id);
        groups_.emplace(solo,std::vector<Id>{player});membership_.at(player)=solo;return true;
    }
    bool kick(Id sender,Id target){
        if(sender==target||!leader(sender)||group(sender)!=group(target))return false;
        return leave(target);
    }
};
}
