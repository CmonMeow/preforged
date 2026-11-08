#pragma once
#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <limits>
#include <array>
#include <memory>

namespace fw::gw {
// Synchronization belongs to the transfer registry. An active connection owns
// its ticket exclusively; only disconnect starts the short reconnect window.
template<class Grant> class ReconnectTickets {
public:
    using Key=std::pair<std::uint32_t,std::uint32_t>;
    static constexpr std::uint64_t reconnect_ms=30000;
private:
    struct Entry {Grant grant;bool active=false;std::uint64_t expires=0;std::shared_ptr<void> lifetime;};
    std::map<Key,Entry> entries_;
public:
    void prune(std::uint64_t now){std::erase_if(entries_,[&](const auto& e){return !e.second.active&&e.second.expires<=now;});}
    bool issue(Key key,Grant grant,std::uint64_t now){
        prune(now);if(grant.expires<=now||entries_.size()>=4096)return false;
        const auto expires=grant.expires;
        return entries_.emplace(key,Entry{std::move(grant),false,expires}).second;
    }
    // Caller holds the registry mutex. Both allocations are prepared before
    // publication, so rejection or allocation failure publishes neither ticket.
    bool issue_pair(std::array<Key,2> keys,std::array<Grant,2> grants,std::uint64_t now){
        prune(now);
        if(keys[0]==keys[1]||entries_.size()>4094||entries_.contains(keys[0])||entries_.contains(keys[1]))return false;
        std::map<Key,Entry> staged;
        for(unsigned i=0;i<2;++i){const auto expires=grants[i].expires;
            if(expires<=now)return false;
            staged.emplace(keys[i],Entry{std::move(grants[i]),false,expires});
        }
        // Node transfer allocates nothing and std::less<Key> cannot throw.
        entries_.merge(staged);return true;
    }
    template<class Authorized> std::optional<Grant> consume(Key key,std::uint64_t now,Authorized authorized){
        auto it=entries_.find(key);if(it==entries_.end())return {};
        if(!it->second.active&&it->second.expires<=now){entries_.erase(it);return {};}
        if(it->second.active||!authorized(it->second.grant))return {};
        auto result=it->second.grant;it->second.active=true;return result;
    }
    // Keep the connected world's lease alive through the reconnect window.
    // The owner must outlive this registry. Expiry/revocation drops the lease.
    bool retain(Key key,std::shared_ptr<void> lifetime){
        auto it=entries_.find(key);
        if(it==entries_.end()||!it->second.active||!lifetime)return false;
        it->second.lifetime=std::move(lifetime);return true;
    }
    void disconnect(Key key,std::uint64_t now,bool resume){
        auto it=entries_.find(key);if(it==entries_.end()||!it->second.active)return;
        if(!resume){entries_.erase(it);return;}
        it->second.active=false;
        it->second.expires=now>std::numeric_limits<std::uint64_t>::max()-reconnect_ms?
            std::numeric_limits<std::uint64_t>::max():now+reconnect_ms;
    }
    void erase(Key key){entries_.erase(key);}
};
}
