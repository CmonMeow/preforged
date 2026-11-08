#pragma once
#include "gw_social_wire.h"
#include "gw_world_simulation.h"
#include <map>
#include <mutex>
namespace fw::gw::social {
struct Presence {std::uint64_t lease=0;unsigned status=1;std::u16string character;std::uint16_t map=0;std::shared_ptr<world::Outbox> world_outbox,auth_outbox;};
// Each authenticated connection owns a lease. A stale disconnect cannot mark
// a replacement session offline. World sockets never own auth presence.
class Directory {
    std::mutex mutex_;std::uint64_t next_=1;std::map<std::string,Presence> people_;
public:
    std::uint64_t login(const std::string& account,std::u16string name,std::shared_ptr<world::Outbox> auth){std::lock_guard lock(mutex_);const auto lease=next_++;people_[account]={lease,1,std::move(name),0,{},std::move(auth)};return lease;}
    void logout(const std::string& account,std::uint64_t lease){std::lock_guard lock(mutex_);const auto it=people_.find(account);if(it!=people_.end()&&it->second.lease==lease)people_.erase(it);}
    void status(const std::string& account,std::uint64_t lease,unsigned status){if(status>3)throw std::invalid_argument("presence outside0..3");std::lock_guard lock(mutex_);auto it=people_.find(account);if(it!=people_.end()&&it->second.lease==lease)it->second.status=status;}
    bool character(const std::string& account,std::uint64_t lease,std::u16string name,std::uint16_t map,std::shared_ptr<world::Outbox> outbox={}){
        std::lock_guard lock(mutex_);auto it=people_.find(account);
        if(!lease||it==people_.end()||it->second.lease!=lease)return false;
        it->second.character=std::move(name);it->second.map=map;if(outbox)it->second.world_outbox=std::move(outbox);return true;
    }
    void leave_world(const std::string& account,const std::shared_ptr<world::Outbox>& outbox){std::lock_guard lock(mutex_);auto it=people_.find(account);if(it!=people_.end()&&it->second.world_outbox==outbox)it->second.world_outbox.reset();}
    std::optional<Presence> get(const std::string& account){std::lock_guard lock(mutex_);auto it=people_.find(account);return it==people_.end()?std::nullopt:std::optional(it->second);}
    std::optional<std::pair<std::string,Presence>> find(std::u16string_view character){std::lock_guard lock(mutex_);std::optional<std::pair<std::string,Presence>> result;for(const auto& [account,value]:people_)if(value.status&&chat::commands::same_name(value.character,character)){if(result)return {};result=std::pair(account,value);}return result;}
};
}
