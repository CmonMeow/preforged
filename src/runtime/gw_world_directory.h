#pragma once
#include <cstdint>
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <tuple>

namespace fw::gw::world {
// Reservations survive transfer consumption and are released only after the
// world connection has left its instance. Directory entries are weak owners.
class WorldDirectory {
    struct World {
        std::uint32_t id;
        std::mutex mutex;
        std::set<std::uint32_t> players, reserved;
        World(std::uint32_t value,std::span<const std::uint32_t> blocked):id(value),reserved(blocked.begin(),blocked.end()){}
    };
    struct Reservation {
        std::shared_ptr<World> world;
        std::uint32_t player=0;
        explicit Reservation(std::shared_ptr<World> value):world(std::move(value)){}
        ~Reservation(){if(player){std::lock_guard lock(world->mutex);world->players.erase(player);}}
    };
public:
    using Key=std::tuple<std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t>;
    class Lease {
        std::shared_ptr<Reservation> reservation_;
        friend class WorldDirectory;
        explicit Lease(std::shared_ptr<Reservation> value):reservation_(std::move(value)){}
    public:
        Lease()=default;
        std::uint32_t world()const{return reservation_?reservation_->world->id:0;}
        std::uint32_t player()const{return reservation_?reservation_->player:0;}
    };
private:
    std::mutex mutex_;
    std::map<Key,std::weak_ptr<World>> districts_;
    std::map<std::uint32_t,std::weak_ptr<World>> worlds_;
    std::function<std::uint32_t()> random_;
public:
    explicit WorldDirectory(std::function<std::uint32_t()> random):random_(std::move(random)){}
    // Allocate both tickets' ownership before either is published. A match
    // shares one private world, never a public district or another match.
    std::array<Lease,2> reserve_private_pair(std::span<const std::uint32_t> npc_ids={}){
        auto first=reserve(std::nullopt,npc_ids);
        auto world=first.reservation_->world;
        auto second=std::make_shared<Reservation>(world);
        {
            std::lock_guard lock(world->mutex);
            std::uint32_t id=1;
            while(id<=32767&&(world->players.contains(id)||world->reserved.contains(id)))++id;
            if(id>32767)throw std::runtime_error("private match player capacity");
            world->players.insert(id);second->player=id;
        }
        return {std::move(first),Lease(std::move(second))};
    }
    Lease reserve(std::optional<Key> district,std::span<const std::uint32_t> npc_ids={}){
        std::lock_guard directory_lock(mutex_);
        std::erase_if(districts_,[](const auto& entry){return entry.second.expired();});
        std::erase_if(worlds_,[](const auto& entry){return entry.second.expired();});
        std::shared_ptr<World> world;
        if(district){const auto it=districts_.find(*district);if(it!=districts_.end())world=it->second.lock();}
        if(!world){
            if(worlds_.size()>=4096)throw std::runtime_error("world directory capacity");
            std::uint32_t id=0;
            for(unsigned attempt=0;attempt<16;++attempt){const auto candidate=random_();if(candidate&&!worlds_.contains(candidate)){id=candidate;break;}}
            if(!id)throw std::runtime_error("world id generation failed");
            world=std::make_shared<World>(id,npc_ids);
            worlds_[id]=world;
            if(district)districts_[*district]=world;
        }
        auto reservation=std::make_shared<Reservation>(world);
        {
            std::lock_guard world_lock(world->mutex);
            std::uint32_t id=1;
            // Private allocation policy: floor actors use32768..65535.
            // Reserving players only below that pool prevents a pending
            // transfer from racing a drop before the player joins the world.
            while(id<=32767&&(world->players.contains(id)||world->reserved.contains(id)))++id;
            if(id>32767)throw std::runtime_error("instance player ID capacity");
            world->players.insert(id);
            reservation->player=id;
        }
        return Lease(std::move(reservation));
    }
};
}
