#pragma once
#include "gw_guild_wire.h"
#include <mutex>
#include <string>
namespace fw::gw::guild {
// Native302 appends nonempty announcements to chat. This cache belongs to an
// authenticated login, shared by its map grants; never a global account cache.
class AnnouncementDelivery {
    std::mutex mutex_;std::string guild_;Packet published_;bool delivered_=false;
public:
    void clear(){std::lock_guard lock(mutex_);guild_.clear();published_.clear();delivered_=false;}
    template<class Send> bool publish(const std::string& guild,const Packet& packet,Send&& send){
        std::lock_guard lock(mutex_);
        if(delivered_&&guild_==guild&&published_==packet)return false;
        send(packet); // Commit only after successful transport publication.
        guild_=guild;published_=packet;delivered_=true;return true;
    }
};
}
