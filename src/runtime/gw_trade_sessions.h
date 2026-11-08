#pragma once
#include "gw_trade_wire.h"
#include "gw_trade_exchange.h"
#include <array>
#include <cmath>
#include <deque>
#include <map>
#include <mutex>
#include <string>

namespace fw::gw::trade {
// Handles identify a connection lease, never an account or reused agent slot.
// Inventory operations are deliberately separate from this UI lifecycle.
class Sessions {
public:
    using Handle=std::uint64_t;
    enum class Result { Requested,Initiated,Acknowledged,Rejected };
    struct Presence {
        std::string owner;std::uint64_t world;std::uint32_t player;bool pvp;float x,y;
        Presence(std::string o,std::uint64_t w,std::uint32_t n,bool p,float px,float py):owner(std::move(o)),world(w),player(n),pvp(p),x(px),y(py){}
        Presence(std::uint64_t o,std::uint64_t w,std::uint32_t n,bool p,float px,float py):Presence(o?std::to_string(o):"",w,n,p,px,py){}
    };
    struct InventoryOffer {std::string character;std::uint64_t revision=0;exchange::Offer offered;};
    struct Commit {Presence left,right;InventoryOffer left_offer,right_offer;};
    explicit Sessions(std::size_t capacity=4096):capacity_(capacity){if(!capacity)throw std::invalid_argument("trade capacity");}
    Handle attach(Presence p){
        if(!valid(p))return 0;
        std::lock_guard lock(mutex_);
        if(players_.size()>=capacity_||next_==UINT64_MAX)return 0;
        for(const auto&[id,live]:players_)if(live.p.world==p.world&&live.p.player==p.player)return 0;
        const auto id=next_+1;players_.emplace(id,Live{p});next_=id;return id;
    }
    bool update(Handle id,Presence p){
        if(!valid(p))return false;
        std::lock_guard lock(mutex_);auto it=players_.find(id);if(it==players_.end())return false;
        if(p.owner!=it->second.p.owner||p.world!=it->second.p.world||p.player!=it->second.p.player)return false;
        if(p.pvp)cancel_locked(id);it->second.p=p;return true;
    }
    Result open(Handle id,std::uint32_t target,double max_distance){
        if(!target||!std::isfinite(max_distance)||max_distance<=0)return Result::Rejected;
        std::lock_guard lock(mutex_);auto a=players_.find(id);if(a==players_.end()||a->second.p.pvp)return Result::Rejected;
        auto b=players_.end();for(auto it=players_.begin();it!=players_.end();++it)
            if(it->second.p.world==a->second.p.world&&it->second.p.player==target){b=it;break;}
        if(b==players_.end()||a==b||b->second.p.pvp||a->second.p.owner==b->second.p.owner)return Result::Rejected;
        if(std::hypot(double(a->second.p.x)-b->second.p.x,double(a->second.p.y)-b->second.p.y)>max_distance)return Result::Rejected;
        for(const auto&[other,live]:players_)
            if(other!=id&&other!=b->first&&live.peer&&
               (live.p.owner==a->second.p.owner||live.p.owner==b->second.p.owner))return Result::Rejected;
        auto& left=a->second;auto& right=b->second;
        if(left.peer){
            if(left.peer!=b->first||right.peer!=id)return Result::Rejected;
            if(left.active)return Result::Acknowledged;
            if(left.initiator)return Result::Requested; // repeated request is idempotent
            if(left.pending.size()>=maximum_pending||right.pending.size()>=maximum_pending)return Result::Rejected;
            left.active=right.active=true;return Result::Initiated;
        }
        if(right.peer||right.pending.size()>=maximum_pending)return Result::Rejected;
        left.pending.push_back(requested_2022(right.p.player));
        try{right.pending.push_back(initiated_2022(left.p.player));}catch(...){left.pending.pop_back();throw;}
        left.peer=b->first;right.peer=id;left.initiator=true;right.initiator=false;
        return Result::Requested;
    }
    bool cancel(Handle id){std::lock_guard lock(mutex_);return cancel_locked(id);}
    void detach(Handle id){std::lock_guard lock(mutex_);cancel_locked(id);players_.erase(id);}
    std::vector<std::vector<std::uint8_t>> drain(Handle id){
        std::lock_guard lock(mutex_);auto it=players_.find(id);if(it==players_.end())return {};
        std::vector<std::vector<std::uint8_t>> out;out.reserve(it->second.pending.size());
        while(!it->second.pending.empty()){out.push_back(std::move(it->second.pending.front()));it->second.pending.pop_front();}return out;
    }
    bool active(Handle id){std::lock_guard lock(mutex_);auto it=players_.find(id);return it!=players_.end()&&it->second.active;}
    // Inventory ownership/quantity validity is checked by the caller and again
    // by the atomic account operation. Offer edit revokes both acceptances.
    bool offer(Handle id,InventoryOffer offer){
        if(offer.character.empty()||offer.offered.items.size()>7||offer.offered.gold>100000)return false;
        std::set<std::uint32_t> items;for(auto item:offer.offered.items)
            if(!item.item||!item.quantity||item.quantity>250||!items.insert(item.item).second)return false;
        std::lock_guard lock(mutex_);auto it=players_.find(id);
        if(it==players_.end()||!it->second.active)return false;
        auto peer=players_.find(it->second.peer);if(peer==players_.end()||!peer->second.active)return false;
        it->second.accepted=peer->second.accepted=false;it->second.offer=std::move(offer);return true;
    }
    // Hold lifecycle lock through the supplied atomic commit so cancel/travel
    // cannot race between the second acceptance and committed exchange.
    template<class AtomicCommit> bool accept(Handle id,AtomicCommit commit){
        std::lock_guard lock(mutex_);auto it=players_.find(id);
        if(it==players_.end()||!it->second.active||!it->second.offer)return false;
        auto peer=players_.find(it->second.peer);if(peer==players_.end()||!peer->second.active||!peer->second.offer)return false;
        it->second.accepted=true;
        if(!peer->second.accepted)return true;
        // Consume both approvals before copying the snapshot or committing:
        // either operation may throw, and a retry needs fresh mutual consent.
        it->second.accepted=peer->second.accepted=false;
        const Commit request{it->second.p,peer->second.p,*it->second.offer,*peer->second.offer};
        if(!commit(request))return false;
        it->second.refresh=peer->second.refresh=true;close(it->second);close(peer->second);return true;
    }
    bool refresh_inventory(Handle id){std::lock_guard lock(mutex_);auto it=players_.find(id);if(it==players_.end())return false;return std::exchange(it->second.refresh,false);}
    bool relay(Handle id,std::vector<std::vector<std::uint8_t>> packets){
        std::lock_guard lock(mutex_);auto it=players_.find(id);if(it==players_.end()||!it->second.active)return false;
        auto peer=players_.find(it->second.peer);if(peer==players_.end()||peer->second.pending.size()+packets.size()>maximum_pending)return false;
        for(auto& packet:packets)peer->second.pending.push_back(std::move(packet));return true;
    }
    std::optional<Handle> peer(Handle id){std::lock_guard lock(mutex_);auto it=players_.find(id);if(it==players_.end()||!it->second.peer)return {};return it->second.peer;}
private:
    static constexpr std::size_t maximum_pending=64;
    struct Live {Presence p;Handle peer=0;bool initiator=false,active=false;std::deque<std::vector<std::uint8_t>> pending;bool accepted=false;std::optional<InventoryOffer> offer;bool refresh=false;};
    static bool valid(const Presence&p){return !p.owner.empty()&&p.owner.size()<=128&&p.world&&p.player&&std::isfinite(p.x)&&std::isfinite(p.y);}
    static void close(Live&live){
        // Drop obsolete request/open events before close so a slow recipient
        // cannot reopen a cancelled UI from an old queued event.
        live.pending.clear();live.pending.push_back(cancelled_2022());
        live.peer=0;live.initiator=false;live.active=false;
        live.accepted=false;live.offer.reset();
    }
    bool cancel_locked(Handle id){
        auto it=players_.find(id);if(it==players_.end()||!it->second.peer)return false;
        const auto peer=it->second.peer;close(it->second);
        auto other=players_.find(peer);if(other!=players_.end()&&other->second.peer==id)close(other->second);return true;
    }
    std::size_t capacity_;Handle next_=0;std::mutex mutex_;std::map<Handle,Live> players_;
};
}
