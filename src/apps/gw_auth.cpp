#include "gw_stalker_population.h"
#include "pet_actions.h"
#include "gw_maintained_request.h"
#include "gw_bundle_equipment.h"
#include "gw_player_holy_equipment.h"
#include "gw_quest_wayward_dialogue.h"
#include "gw_quest_further_adventures.h"
#include "gw_quest_new_elementalist_trainer.h"
#include "gw_inspired_hex_commit.h"
#include "gw_echo_commit.h"
#include "gw_dagger_session_gateway.h"
#include "gw_equipment_visibility.h"
#include "gw_creation_response.h"
#include "quest_actions.h"
#include "quest_second_profession.h"
#include "quest_message_from_friend.h"
#include "quest_path_to_glory.h"
#include "quest_ranger_companion.h"
#include "gw_ranger_companion_population.h"
#include "gw_quest_event_time.h"
#include <chrono>
#include "gw_quest_text_binding.h"
#include "gw_quest_menu.h"
#include "gw_presearing_items.h"
#include "gw_experience_level.h"
#include "gw_collector_dialogue.h"
#include "gw_collector_panel.h"
#include "gw_merchant_panel.h"
#include "gw_crafter_panel.h"
#include "gw_merchant_exchange.h"
#include "gw_creation_spawn.h"
#include "gw_instance_admission.h"
#include "gw_manifest.h"
#include "gw_basic_attack_request.h"
#include "gw_ping.h"
#include "gw_observer_replay.h"
#include "gw_guild_cape.h"
#include "gw_guild_trim.h"
#include "gw_equipment_assignments.h"
#include "gw_arena_match.h"
#include "gw_arena_death_time.h"
#include "gw_development_mode.h"
#include "gw_development_commands.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include "tcp.h"
#include "common.h"
#include "transport.h"
#include "network.h"
#include "gw_listeners.h"
#include "gw_auth_session.h"
#include "gw_session_lifetime.h"
#include "gw_agent_clock.h"
#include "gw_world_map_dimensions_generated.h"
#include "gw_exploration.h"
#include "gw_map_unlocks.h"
#include "gw_auth_responses.h"
#include "gw_map_catalog_generated.h"
#include "gw_party_limits_generated.h"
#include "gw_map_assets_generated.h"
#include "gw_world_simulation.h"
#include "gw_quickening_zephyr_spawn.h"
#include "gw_trade_sessions.h"
#include "gw_basic_armor_energy.h"
#include "gw_social_presence.h"
#include "gw_npc_catalog.h"
#include "gw_quest_encounters.h"
#include "gw_quest_encounter_admission.h"
#include "gw_presearing_collection.h"
#include "gw_world_directory.h"
#include "gw_callout.h"
#include "gw_party_wire.h"
#include "gw_world_wire.h"
#include "gw_transfer_wire.h"
#include "gw_vitals.h"
#include "gw_creation_request.h"
#include "gw_creation_armor.h"
#include "gw_summary_2022.h"
#include "gw_outgoing_frame.h"
#include "gw_character_skills.h"
#include "gw_skill_use_request.h"
#include "gw_skill_casting.h"
#include "gw_life_steal_completion.h"
#include "gw_elemental_completion.h"
#include "gw_wound_spell_completion.h"
#include "gw_glimmering_mark_completion.h"
#include "gw_snow_storm_completion.h"
#include "gw_defensive_ward_completion.h"
#include "gw_reconnect_tickets.h"
#include "gw_arena_queue.h"
#include "gw_arena_wire.h"
#include "gw_attributes.h"
#include "gw_profession.h"
#include "gw_navigation.h"
#include "gw_spawn.h"
#include "gw_map_portals.h"
#include "gw_equipment_wire.h"
#include "gw_inventory.h"
#include "gw_inventory_identity.h"
#include "gw_inventory_ground.h"
#include "gw_inventory_gold.h"
#include "gw_item_transactions.h"
#include "gw_armor_upgrades.h"
#include "gw_dye_transactions.h"
#include "gw_salvage_catalog.h"
#include "gw_identification_catalog.h"
#include "gw_armor_attribute_runes.h"
#include "gw_attribute_condition_bonuses.h"
#include "gw_equipment_metadata_provider.h"
#include "gw_storage.h"
#include "gw_guild_wire.h"
#include "gw_guild_announcement_delivery.h"
#include "gw_guild_request.h"
#include "gw_progression.h"
#include <sodium.h>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <charconv>
#include <iterator>
#include <list>
#include <limits>
#include <memory>
#include <mutex>
#include <map>
#include <set>
#include <syncstream>
#include <string_view>
#include <tuple>
#include <utility>

namespace {
constexpr std::size_t max_connection_workers = 128;
fw::gw::development::Modes development_modes;
constexpr std::uint64_t world_idle_timeout_ms = 5 * 60 * 1000;
using fw::gw::auth::Session;
using fw::gw::auth::Channel;
using fw::gw::GameToken;
using fw::gw::world::Instance;
using fw::gw::world::Instances;
using fw::gw::world::Outbox;
using fw::gw::world::Packet;
using fw::gw::world::f32;
fw::gw::social::Directory social_directory;
using fw::gw::world::player_identity_packets;
using fw::gw::world::u16;
using fw::gw::world::u32;
using fw::gw::navigation::Mesh;
std::atomic<bool> stopping{false};
std::atomic<bool> ready{false};
std::atomic<uint32_t> active_connections{0};
struct ConnectionLease {
    ConnectionLease() { ++active_connections; }
    ~ConnectionLease() { --active_connections; }
};
std::mutex key_mutex;
fw::gw::arena::Queue arena_queue;
fw::gw::trade::Sessions trade_sessions;
class TransferRegistry {
public:
    struct ArenaMatchState;
    struct Grant {
        GameToken account{}, character{};
        std::uint32_t world=0, player=0, map_type=0, map=0, region=0, district=0, language=0;
        float x=0,y=0;
        std::uint64_t expires=0;
        std::string name;
        std::uint32_t appearance=0;
        std::uint32_t level=0;
        std::uint8_t secondary=0;
        std::uint8_t creation_campaigns=0;
        std::uint8_t origin_campaign=0;
        std::uint16_t plane=0;
        float facing=0;
        std::uint32_t team_token=0;
        bool auto_arena_queue=false;
        std::shared_ptr<ArenaMatchState> arena_match;
        std::shared_ptr<const fw::gw::observer::Match> observer_match;
        std::uint32_t observer_return_map=0;
        std::shared_ptr<fw::gw::guild::AnnouncementDelivery> guild_announcements;
        std::uint64_t social_lease=0;
        std::shared_ptr<fw::gw::consumables::continuity::Morale> morale;
        std::vector<fw::gw::CharacterProfile::EquipmentAppearance> equipment;
        fw::gw::world::WorldDirectory::Lease reservation;
    };
    struct ArenaMatchState {
        std::mutex mutex;
        std::array<Grant,2> waiting;
        std::array<std::uint32_t,2> players;
        fw::gw::arena::MatchLifecycle lifecycle;
        std::optional<fw::gw::arena::MatchLifecycle::Result> result;
        std::array<std::optional<std::pair<std::uint32_t,std::uint32_t>>,2> return_tickets;
        ArenaMatchState(std::array<Grant,2> source,std::array<std::uint32_t,2> ids)
            :waiting(std::move(source)),players(ids),lifecycle(ids){}
    };
private:
    using InstanceKey=fw::gw::world::WorldDirectory::Key;
    using CharacterKey=std::pair<GameToken,GameToken>;
    struct TravelIntent {
        std::uint32_t map_type=0,map=0,region=0,district=0,language=0;
        std::uint64_t expires=0;
        std::optional<fw::gw::map_portals::Arrival> arrival;
        bool map_committed=false;
        std::optional<std::pair<std::uint32_t,std::uint32_t>> paired_ticket;
    };
    std::mutex mutex_;
    fw::gw::ReconnectTickets<Grant> grants_;
    fw::gw::world::WorldDirectory directory_{[]{std::uint32_t id=0;randombytes_buf(&id,sizeof id);return id;}};
    std::map<CharacterKey,TravelIntent> travel_intents_;
    std::map<GameToken,std::weak_ptr<fw::gw::guild::AnnouncementDelivery>> guild_sessions_;
    std::map<CharacterKey,std::weak_ptr<fw::gw::consumables::continuity::Morale>> morale_sessions_;
    const fw::gw::world::NpcCatalog& npcs_;
public:
    explicit TransferRegistry(const fw::gw::world::NpcCatalog& npcs):npcs_(npcs){}
    std::pair<std::uint32_t,std::uint32_t> issue_observer(const Grant& current,std::shared_ptr<const fw::gw::observer::Match> match){
        std::lock_guard lock(mutex_);const auto now=fw::now_ms();
        std::vector<std::uint32_t> blocked;for(const auto& [id,a]:match->actors)blocked.push_back(id);
        auto lease=directory_.reserve(std::nullopt,blocked);const auto ids=std::pair{lease.world(),lease.player()};
        auto next=current;next.reservation=lease;next.world=ids.first;next.player=ids.second;next.map=match->map;
        const auto* map=fw::gw::find_map(match->map);if(!map)throw std::runtime_error("replay map absent from 2022 catalog");next.map_type=map->type;
        next.observer_return_map=current.map;next.observer_match=std::move(match);next.arena_match.reset();next.auto_arena_queue=false;next.expires=now+30000;
        const auto& frame=next.observer_match->actors.begin()->second.frames.front();next.x=frame.x;next.y=frame.y;next.plane=frame.plane;next.facing=frame.angle;
        fw::gw::map_portals::Arrival arrival;arrival.map=next.map;arrival.point={next.x,next.y};arrival.plane=0;arrival.facing=next.facing;
        const CharacterKey owner{next.account,next.character};
        if(travel_intents_.contains(owner)&&travel_intents_.at(owner).expires>now)throw std::runtime_error("observer owner already transferring");
        if(!grants_.issue(ids,next,now))throw std::runtime_error("observer transfer capacity");
        travel_intents_[owner]=TravelIntent{next.map_type,next.map,next.region,next.district,next.language,now+30000,arrival,true,ids};return ids;
    }
    std::shared_ptr<fw::gw::guild::AnnouncementDelivery> begin_guild_session(const GameToken& account){
        std::lock_guard lock(mutex_);
        for(auto it=guild_sessions_.begin();it!=guild_sessions_.end();)
            if(it->second.expired())it=guild_sessions_.erase(it);else ++it;
        for(auto it=morale_sessions_.begin();it!=morale_sessions_.end();)if(it->first.first==account||it->second.expired())it=morale_sessions_.erase(it);else ++it;
        auto state=std::make_shared<fw::gw::guild::AnnouncementDelivery>();guild_sessions_[account]=state;return state;
    }
    bool authorize_travel(const Grant& current,std::uint32_t map_type,std::uint32_t map,
            std::uint32_t region,std::uint32_t district,std::uint32_t language,
            std::optional<fw::gw::map_portals::Arrival> arrival={}) {
        std::lock_guard lock(mutex_);const auto now=fw::now_ms();
        for(auto it=travel_intents_.begin();it!=travel_intents_.end();)
            if(it->second.expires<=now)it=travel_intents_.erase(it);else ++it;
        const CharacterKey key{current.account,current.character};
        if(travel_intents_.size()>=4096&&!travel_intents_.contains(key))return false;
        travel_intents_[key]=TravelIntent{map_type,map,region,district,language,now+30000,std::move(arrival)};
        return true;
    }
    bool consume_travel(const GameToken& account,const GameToken& character,std::uint32_t map_type,
            std::uint32_t map,std::uint32_t region,std::uint32_t district,std::uint32_t language,
            std::optional<fw::gw::map_portals::Arrival>* arrival=nullptr,bool* map_committed=nullptr,
            std::optional<std::pair<std::uint32_t,std::uint32_t>>* paired_ticket=nullptr) {
        std::lock_guard lock(mutex_);const CharacterKey key{account,character};
        auto it=travel_intents_.find(key);if(it==travel_intents_.end())return false;
        if(it->second.expires<=fw::now_ms()){travel_intents_.erase(it);return false;}
        const auto& intent=it->second;
        if(intent.map_type!=map_type||intent.map!=map||intent.region!=region||
           intent.district!=district||intent.language!=language)return false;
        if(arrival)*arrival=intent.arrival;
        if(map_committed)*map_committed=intent.map_committed;
        if(paired_ticket)*paired_ticket=intent.paired_ticket;
        travel_intents_.erase(it);return true;
    }
    void mark_travel_committed(const Grant& current,std::uint32_t map){
        std::lock_guard lock(mutex_);const auto it=travel_intents_.find(CharacterKey{current.account,current.character});
        if(it!=travel_intents_.end()&&it->second.map==map)it->second.map_committed=true;
    }
    void revoke(std::pair<std::uint32_t,std::uint32_t> ids,const Grant& current){
        std::lock_guard lock(mutex_);grants_.erase(ids);
        travel_intents_.erase(CharacterKey{current.account,current.character});
    }
    std::pair<std::uint32_t,std::uint32_t> issue(const GameToken& account,const GameToken& character,std::uint64_t social_lease,
            std::uint32_t map_type,std::uint32_t map,std::uint32_t region,std::uint32_t district,
            std::uint32_t language,const std::string& name,float x,float y,std::uint32_t appearance=0,std::uint32_t level=0,
            const std::vector<fw::gw::CharacterProfile::EquipmentAppearance>& equipment={},std::uint8_t creation_campaigns=0,
            std::uint8_t secondary=0,std::uint16_t plane=0,float facing=0,bool auto_arena_queue=false,std::uint8_t origin_campaign=0) {
        if(!social_lease)throw std::invalid_argument("world transfer requires auth presence lease");
        std::lock_guard lock(mutex_); const auto now=fw::now_ms();
        grants_.prune(now);
        // Auth map types 0, 3 and 15 map to cities, outposts and marketplaces
        // in the copied client reference. Their district tuple is public; other
        // map types receive a private world until party-instance policy exists.
        const bool public_district=map_type==0 || map_type==3 || map_type==15;
        std::vector<std::uint32_t> npc_ids;
        for(const auto& npc:npcs_.for_map(map))npc_ids.push_back(npc.id);
        if(map==146||map==147){npc_ids.push_back(62000);npc_ids.push_back(62001);}
        auto reservation=directory_.reserve(public_district
            ?std::optional<InstanceKey>{{map_type,map,region,district,language}}:std::nullopt,npc_ids);
        const auto id=std::pair{reservation.world(),reservation.player()};
        Grant grant{account,character,id.first,id.second,map_type,map,region,district,language,x,y,now+30000,name};
        grant.social_lease=social_lease;
        if(const auto it=guild_sessions_.find(account);it!=guild_sessions_.end())grant.guild_announcements=it->second.lock();
        if(map_type!=11){
            const CharacterKey key{account,character};
            for(auto it=morale_sessions_.begin();it!=morale_sessions_.end();)if(it->second.expired())it=morale_sessions_.erase(it);else ++it;
            if(auto it=morale_sessions_.find(key);it!=morale_sessions_.end())grant.morale=it->second.lock();
            if(!grant.morale){if(morale_sessions_.size()>=4096)throw std::runtime_error("morale session capacity");grant.morale=std::make_shared<fw::gw::consumables::continuity::Morale>();morale_sessions_[key]=grant.morale;}
        }
        grant.reservation=std::move(reservation);
        grant.appearance=appearance;
        grant.level=level;
        grant.secondary=secondary;
        grant.creation_campaigns=creation_campaigns;
        grant.origin_campaign=origin_campaign;
        grant.plane=plane;grant.facing=facing;
        grant.auto_arena_queue=auto_arena_queue;
        grant.equipment=equipment;
        if(!grants_.issue(id,std::move(grant),now)) throw std::runtime_error("world transfer collision or capacity");
        return id;
    }
    std::array<std::pair<std::uint32_t,std::uint32_t>,2> issue_arena_pair(
            const std::array<Grant,2>& waiting,
            const std::array<fw::gw::map_portals::Arrival,2>& arrivals){
        std::lock_guard lock(mutex_);const auto now=fw::now_ms();
        if(waiting[0].map!=2||waiting[1].map!=2||waiting[0].map_type!=5||waiting[1].map_type!=5||
           waiting[0].account==waiting[1].account||waiting[0].character==waiting[1].character)
            throw std::runtime_error("invalid arena pair ownership or staging mode");
        for(const auto& arrival:arrivals)
            if(arrival.map!=2||arrival.plane>65535||!fw::gw::map_portals::finite(arrival.point)||!std::isfinite(arrival.facing))
                throw std::runtime_error("invalid arena pair arrival");
        std::vector<std::uint32_t> blocked;
        for(const auto& npc:npcs_.for_map(2))blocked.push_back(npc.id);
        auto leases=directory_.reserve_private_pair(blocked);
        auto match_state=std::make_shared<ArenaMatchState>(waiting,std::array{leases[0].player(),leases[1].player()});
        auto next=waiting;
        std::map<CharacterKey,TravelIntent> intents;
        std::array<std::pair<std::uint32_t,std::uint32_t>,2> keys;
        for(unsigned i=0;i<2;++i){
            keys[i]={leases[i].world(),leases[i].player()};
            next[i].world=keys[i].first;next[i].player=keys[i].second;
            next[i].map_type=6;next[i].expires=now+30000;
            next[i].team_token=i+1;
            next[i].arena_match=match_state;next[i].auto_arena_queue=false;
            next[i].reservation=std::move(leases[i]);
            next[i].x=arrivals[i].point.x;next[i].y=arrivals[i].point.z;
            next[i].plane=static_cast<std::uint16_t>(arrivals[i].plane);next[i].facing=arrivals[i].facing;
            const CharacterKey owner{next[i].account,next[i].character};
            if(auto it=travel_intents_.find(owner);it!=travel_intents_.end()){
                if(it->second.expires>now)throw std::runtime_error("arena owner already transferring");
                travel_intents_.erase(it);
            }
            intents.emplace(owner,TravelIntent{6,2,next[i].region,next[i].district,next[i].language,now+30000,arrivals[i],true,keys[i]});
        }
        if(!grants_.issue_pair(keys,std::move(next),now))
            throw std::runtime_error("arena pair ticket collision or capacity");
        travel_intents_.merge(intents);
        return keys;
    }
    std::optional<Grant> consume(std::uint32_t world,std::uint32_t player,std::uint32_t map,
                 std::span<const std::uint8_t,16> account,std::span<const std::uint8_t,16> character) {
        std::lock_guard lock(mutex_);
        return grants_.consume({world,player},fw::now_ms(),[&](const Grant& grant){return grant.map==map&&
            sodium_memcmp(grant.account.data(),account.data(),account.size())==0&&
            sodium_memcmp(grant.character.data(),character.data(),character.size())==0;});
    }
    void disconnected(std::pair<std::uint32_t,std::uint32_t> key,bool resume){
        std::lock_guard lock(mutex_);grants_.disconnect(key,fw::now_ms(),resume);
    }
    bool retain_world(std::pair<std::uint32_t,std::uint32_t> key,std::shared_ptr<Instances::Lease> lease){
        std::lock_guard lock(mutex_);return grants_.retain(key,std::move(lease));
    }
    void prune(){std::lock_guard lock(mutex_);grants_.prune(fw::now_ms());}
};
struct ArenaParticipant {
    TransferRegistry::Grant grant;
    std::optional<std::pair<std::uint32_t,std::uint32_t>> ticket;
};
std::mutex arena_mutex;
std::map<std::uint64_t,ArenaParticipant> arena_participants;
const char* phase_name(fw::gw::auth::Session::State state) {
    using State = fw::gw::auth::Session::State;
    switch (state) {
    case State::preface: return "preface";
    case State::seed: return "seed";
    case State::client_info: return "client-info";
    case State::client_hash: return "client-hash";
    case State::hello: return "hello";
    case State::login: return "login";
    case State::ready: return "ticket";
    default: return "rejected";
    }
}

void wait_socket(SOCKET socket, bool writing, uint64_t deadline, unsigned wait_ms=50) {
    if (stopping || fw::now_ms() >= deadline) throw std::runtime_error("auth deadline");
    fd_set set; FD_ZERO(&set); FD_SET(socket, &set); timeval time{0, static_cast<long>(wait_ms*1000)};
    if (select(0, writing ? nullptr : &set, writing ? &set : nullptr, nullptr, &time) < 0)
        throw std::runtime_error("auth socket wait");
}
void send_pending(SOCKET socket, Session& session, uint64_t deadline) {
    auto response = session.take_reply();
    std::size_t sent = 0;
    while (sent < response.size()) {
        wait_socket(socket, true, deadline);
        auto size = send(socket, reinterpret_cast<const char*>(response.data() + sent), int(response.size() - sent), 0);
        if (size < 0 && WSAGetLastError() == WSAEWOULDBLOCK) continue;
        if (size <= 0) throw std::runtime_error("auth response send failed after " + std::to_string(sent) + " bytes, WSA=" + std::to_string(WSAGetLastError()));
        sent += static_cast<std::size_t>(size);
    }
}
std::vector<std::uint8_t> raw_token(const std::string& hex) {
    if (hex.size() != 32 || hex.find_first_not_of("0123456789abcdef") != hex.npos)
        throw std::runtime_error("invalid account identity");
    std::vector<std::uint8_t> bytes(16);
    if (sodium_hex2bin(bytes.data(), bytes.size(), hex.data(), hex.size(), nullptr, nullptr, nullptr))
        throw std::runtime_error("invalid account identity");
    return bytes;
}
std::string hex_bytes(std::span<const std::uint8_t> bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string text; text.reserve(bytes.size() * 2);
    for (auto byte : bytes) { text.push_back(digits[byte >> 4]); text.push_back(digits[byte & 15]); }
    return text;
}
std::string character_key(const GameToken& account,const GameToken& character) {
    return hex_bytes(account)+"\t"+hex_bytes(character);
}
std::string text_hex(std::u16string_view text){
    const auto utf8=fw::gw::character_name_to_utf8(std::u16string(text),255);
    return hex_bytes(std::span(reinterpret_cast<const std::uint8_t*>(utf8.data()),utf8.size()));
}
std::u16string hex_text(const std::string& text,unsigned limit=255){
    if(text.size()%2||text.size()>limit*8)throw std::runtime_error("social response text bounds");
    std::string decoded(text.size()/2,'\0');
    if(!text.empty()&&sodium_hex2bin(reinterpret_cast<unsigned char*>(decoded.data()),decoded.size(),text.data(),text.size(),nullptr,nullptr,nullptr))throw std::runtime_error("social response text encoding");
    return decoded.empty()?u"":fw::gw::character_name_to_utf16(decoded,limit);
}
std::map<std::string,fw::gw::social::Friend> friend_list(const std::string& owner){
    const auto response=gwserver::NetworkManager::current().server().friends(gwserver::Identity::from_hex(owner));if(response.status!=FW_OK)throw std::runtime_error("friend list storage failed");
    std::map<std::string,fw::gw::social::Friend> result;
    for(const auto& entry:response.entries){fw::gw::social::Friend value;
        const auto id=gwserver::storage_hex(entry.account.bytes);value.account=entry.account.bytes;
        value.type=entry.type;value.alias=fw::gw::character_name_to_utf16(entry.alias,19);
        if(value.type==1)if(const auto presence=social_directory.get(id);presence&&presence->status){value.status=presence->status;value.character=presence->character;}
        result.emplace(id,std::move(value));
    }return result;
}
fw::gw::navigation::Point nearest_walkable_map_center(const Mesh& mesh) {
    const auto bounds=mesh.source_bounds();
    if(!bounds||mesh.plane_count()==0)throw std::runtime_error("map navigation has no ground bounds");
    const fw::gw::navigation::Point center{((*bounds)[0]+(*bounds)[2])*.5f,((*bounds)[1]+(*bounds)[3])*.5f};
    if(mesh.walkable(0,center))return center;
    float best=std::numeric_limits<float>::infinity();
    fw::gw::navigation::Point selected{};
    const auto consider=[&](fw::gw::navigation::Point p){
        if(!mesh.walkable(0,p))return;
        const auto dx=p.x-center.x,dz=p.z-center.z,d=dx*dx+dz*dz;
        if(d<best){best=d;selected=p;}
    };
    const auto project=[&](fw::gw::navigation::Point a,fw::gw::navigation::Point b){
        const auto dx=b.x-a.x,dz=b.z-a.z,length=dx*dx+dz*dz;
        const auto t=length>0?std::clamp(((center.x-a.x)*dx+(center.z-a.z)*dz)/length,0.f,1.f):0.f;
        consider({a.x+t*dx,a.z+t*dz});
    };
    for(std::size_t i=0;i<mesh.trapezoid_count(0);++i){
        const auto& cell=mesh.trapezoid(0,i);
        const fw::gw::navigation::Point tl{cell.xtl,cell.yt},tr{cell.xtr,cell.yt},br{cell.xbr,cell.yb},bl{cell.xbl,cell.yb};
        project(tl,tr);project(tr,br);project(br,bl);project(bl,tl);
    }
    if(!std::isfinite(best))throw std::runtime_error("map navigation has no walkable ground point");
    return selected;
}
std::optional<std::pair<float,float>> map_entry_spawn(const std::filesystem::path& state_directory,std::uint32_t map) {
    std::ifstream catalog(state_directory/"navigation"/"catalog.txt");
    std::string row;std::filesystem::path mesh_path;
    while(std::getline(catalog,row)){
        const auto split=row.find('=');if(split==row.npos)continue;
        if(fw::number(std::string_view(row).substr(0,split),65535)==map){mesh_path=row.substr(split+1);break;}
    }
    if(mesh_path.empty()){
        if(!fw::gw::find_map(map)){
            std::osyncstream(std::cout)<<"AUTH map_entry_rejected map="<<map<<" missing=map_catalog_entry"<<std::endl;
            return std::nullopt;
        }
        // A configured arrival remains useful when the map has client data
        // but no extracted collision mesh. Trust this server-owned point, but
        // report that its walkability could not be checked.
        std::ifstream configured("config/spawn-points.txt");
        while(std::getline(configured,row)){
            std::istringstream fields(row);std::uint32_t configured_map=0;float x=0,z=0;
            if(!(fields>>configured_map>>x>>z)||configured_map!=map||!std::isfinite(x)||!std::isfinite(z))continue;
            std::osyncstream(std::cout)<<"AUTH map_entry_spawn map="<<map
                <<" source=server_configuration_no_navigation x="<<x<<" y="<<z<<" plane=0 collision=unavailable"<<std::endl;
            return std::pair{x,z};
        }
        // Some known logical maps have no matching collision asset in this
        // client DAT. Use the map-space origin as the least-assumptive entry
        // when no bounds exist; Instance already degrades collision explicitly
        // for these maps. This is a fallback, not a claim that (0,0) is a
        // captured retail spawn or walkable center.
        std::osyncstream(std::cout)<<"AUTH map_entry_spawn map="<<map
            <<" source=map_origin_no_navigation_data x=0 y=0 plane=0 collision=unavailable"<<std::endl;
        return std::pair{0.0f,0.0f};
    }
    try {
    const auto mesh=Mesh::load(fw::gw::navigation::catalog_asset_path(mesh_path.string(),
        std::filesystem::absolute(state_directory/"navigation")));
    const auto point=fw::gw::world::map_spawn("config/spawn-points.txt",map,mesh);
    if(!point){
        const auto fallback=nearest_walkable_map_center(mesh);
        std::osyncstream(std::cout)<<"AUTH map_entry_spawn map="<<map
            <<" source=map_center_snapped_to_walkable_ground x="<<fallback.x<<" y="<<fallback.z<<" plane=0"<<std::endl;
        return std::pair{fallback.x,fallback.z};
    }
    std::osyncstream(std::cout)<<"AUTH map_entry_spawn map="<<map<<" source=server_configuration x="<<point->x<<" y="<<point->y<<" plane=0"<<std::endl;
    return std::pair{point->x,point->y};
    } catch(const std::runtime_error&){
        // Configuration faults reject this destination through the normal auth
        // response instead of tearing down the authenticated connection.
        std::osyncstream(std::cout)<<"AUTH map_entry_rejected map="<<map
            <<" reason=invalid_navigation_or_spawn_configuration"<<std::endl;
        return std::nullopt;
    }
}
bool transition_character_map(const GameToken& account,const GameToken& character,
                              std::uint32_t from,std::uint32_t to) {
    auto result=gwserver::NetworkManager::current().server().selectMap({{account},{character},from,to});
    return result.status==FW_OK;
}
std::optional<std::uint32_t> configured_recovery_map(const std::filesystem::path& state_directory) {
    std::ifstream input(state_directory/"recovery-map.txt");
    std::uint32_t map=0;
    if(!(input>>map)||map>65535)return {};
    const auto* record=fw::gw::find_map(map);
    if(!record||(record->type!=0&&record->type!=3&&record->type!=15)||
       !fw::gw::map_asset_file(static_cast<std::uint16_t>(map))||!map_entry_spawn(state_directory,map))return {};
    return map;
}
std::optional<fw::gw::map_portals::Arrival> map_entry_arrival(const std::filesystem::path& state_directory,
        std::uint32_t map,std::optional<fw::gw::map_portals::Arrival> supplied={}){
    if(!supplied){const auto spawn=map_entry_spawn(state_directory,map);if(!spawn)return {};
        return fw::gw::map_portals::Arrival{map,0,{spawn->first,spawn->second},0};}
    if(supplied->map!=map||supplied->plane>65535||!fw::gw::map_portals::finite(supplied->point)||!std::isfinite(supplied->facing))return {};
    try{std::ifstream catalog(state_directory/"navigation"/"catalog.txt");std::string row;
        while(std::getline(catalog,row)){const auto split=row.find('=');if(split==row.npos)continue;
            if(fw::number(std::string_view(row).substr(0,split),65535)!=map)continue;
            const auto mesh=Mesh::load(fw::gw::navigation::catalog_asset_path(row.substr(split+1),
                std::filesystem::absolute(state_directory/"navigation")));
            if(mesh.asset_hash()!=fw::gw::map_asset_file(static_cast<std::uint16_t>(map))||
               !mesh.walkable(supplied->plane,supplied->point))return {};
            return supplied;
        }
    }catch(const std::exception& error){std::osyncstream(std::cout)<<"AUTH portal_arrival_rejected map="<<map<<" reason="<<error.what()<<std::endl;}
    return {};
}
in_addr resolve_ipv4(const std::string& host) {
    addrinfo hints{}; hints.ai_family=AF_INET; hints.ai_socktype=SOCK_STREAM;
    addrinfo* raw=nullptr;
    const auto status=getaddrinfo(host.c_str(),nullptr,&hints,&raw);
    if(status || !raw) throw std::runtime_error("public world host DNS resolution failed");
    std::unique_ptr<addrinfo,decltype(&freeaddrinfo)> addresses(raw,freeaddrinfo);
    return reinterpret_cast<const sockaddr_in*>(raw->ai_addr)->sin_addr;
}
void handle(SOCKET socket, fw::gw::auth::DiffieHellman& key,
            const sockaddr_in& world_endpoint, TransferRegistry& transfers,
            const fw::gw::auth::AccountInfoProfile* account_profile, std::uint32_t build, const std::filesystem::path& state_directory) {
    using namespace fw::gw::auth;
    const auto accepted_at = fw::now_ms();
    u_long nonblocking = 1;
    if (ioctlsocket(socket, FIONBIO, &nonblocking)) return;
    std::uint32_t session_id = 0;
    randombytes_buf(&session_id, sizeof(session_id));
    Session session(key, session_id ? session_id : 1,build);
    // Phase-only diagnostics: never emit credentials, ticket bytes or account names.
    struct Trace {
        Session& session;
        ~Trace() {
            std::osyncstream(std::cout) << "AUTH connection closed phase=" << phase_name(session.state())
                << " keepalives=" << session.keepalive_count()
                << " connection_hint=" << session.has_connection_hint() << std::endl;
        }
    } trace{session};
    std::osyncstream(std::cout) << "AUTH connection accepted" << std::endl;
    // Portal-backed login is asynchronous and can take longer than the raw DH
    // handshake; keep a bounded window wide enough for that client callback path.
    PendingLifetime pending_lifetime(fw::now_ms());
    while (session.state() != Session::State::ready) {
        const auto deadline=pending_lifetime.deadline();
        wait_socket(socket, false, deadline);
        std::array<std::uint8_t, 512> bytes{};
        auto count = recv(socket, reinterpret_cast<char*>(bytes.data()), int(bytes.size()), 0);
        if (count < 0 && WSAGetLastError() == WSAEWOULDBLOCK) continue;
        if (count <= 0) return;
        auto previous = session.state();
        const auto previous_keepalives=session.keepalive_count();
        {
            // Serialize use of the shared imported CNG key; socket waits never hold this lock.
            std::lock_guard lock(key_mutex);
            session.feed(std::span(bytes).first(count));
        }
        sodium_memzero(bytes.data(), bytes.size());
        pending_lifetime.observed(fw::now_ms(),previous,session.state(),previous_keepalives,session.keepalive_count());
        if (previous != session.state())
            std::osyncstream(std::cout) << "AUTH phase " << phase_name(previous) << " -> " << phase_name(session.state()) << std::endl;
        if (session.state() == Session::State::failed) {
            std::osyncstream log(std::cout);
            log << "AUTH rejection=" << session.failure() << " observed_opcode=" << session.observed()
                << " decoded_bytes=" << session.observed_size_total();
            if (const auto& instance = session.early_instance_request(); instance)
                log << " request_transaction=" << instance->transaction
                    << " map_type=" << instance->map_type << " map=" << instance->map_id
                    << " region=" << instance->region << " district=" << instance->district
                    << " language=" << instance->language;
            if (session.preface_bytes().size()) log << " preface_hex=" << hex_bytes(session.preface_bytes());
            log << std::endl;
            return;
        }
        send_pending(socket, session, pending_lifetime.deadline());
    }
    const auto& login = session.login();
    std::osyncstream(std::cout) << "AUTH login transaction=" << login.transaction << std::endl;
    std::string alias;
    for (auto unit : login.login) {
        if (unit > 127 || unit < 32) return;
        alias += static_cast<char>(unit);
    }
    if (!alias.empty() && !fw::account_name_valid(alias)) return;
    auto character_name = login.character.empty() ? std::string{} : fw::gw::character_name_to_utf8(login.character,19);
    const auto ticket_lookup_started=fw::now_ms();
    auto reply = gwserver::NetworkManager::current().server().authenticateTicket({login.ticket,{login.user},alias,character_name});
    const auto status=reply.status;
    std::osyncstream(std::cout) << "AUTH ticket_lookup status=" << status
        << " elapsed_ms=" << (fw::now_ms() - ticket_lookup_started)
        << " connection_elapsed_ms=" << (fw::now_ms() - accepted_at) << std::endl;
    if(status!=FW_OK){std::osyncstream(std::cout)<<"AUTH ticket status="<<status<<" reason="<<reply.message<<std::endl;return;}
    if(!reply.identity)return;
    const auto& identity=*reply.identity;
    struct AccountSessionCleanup {
        std::optional<gwserver::GameSessionLease> lease;
        ~AccountSessionCleanup(){if(lease)try{
            const auto result=gwserver::NetworkManager::current().server().releaseGameSession(*lease);
            std::osyncstream(std::cout)<<"AUTH account_session_released status="<<result.status<<std::endl;
        }catch(...){}}
    } account_session_cleanup{identity.lease};
    const auto roster_size=identity.roster_size;
    std::vector<gwserver::RosterEntry> roster_entries;
    for(unsigned offset=0;offset<roster_size;offset+=3) {
        auto page=gwserver::NetworkManager::current().server().characterRoster(
            {identity.account,offset});
        if(page.status!=FW_OK||page.total!=roster_size) {
            std::osyncstream(std::cout)<<"AUTH roster_page_rejected offset="<<offset<<" status="<<page.status<<std::endl;
            return;
        }
        roster_entries.insert(roster_entries.end(),std::make_move_iterator(page.entries.begin()),std::make_move_iterator(page.entries.end()));
    }
    const auto account_id = gwserver::storage_hex(identity.account.bytes);
    const auto character_id = identity.character?gwserver::storage_hex(identity.character->bytes):std::string{};
    auto account_token = raw_token(account_id);
    GameToken account_guid{}, character_guid{};
    std::copy(account_token.begin(), account_token.end(), account_guid.begin());
    sodium_memzero(account_token.data(), account_token.size());
    if(!character_id.empty()) {
        auto character_token = raw_token(character_id);
        std::copy(character_token.begin(), character_token.end(), character_guid.begin());
        sodium_memzero(character_token.data(), character_token.size());
    }
    auto campaigns_result=gwserver::NetworkManager::current().server().accountCampaigns(gwserver::Identity::from_hex(account_id));
    if(campaigns_result.status!=FW_OK){std::osyncstream(std::cout)<<"AUTH identity_rejected reason=campaign_lookup status="<<campaigns_result.status<<std::endl;return;}
    const auto& account_campaigns=campaigns_result.campaigns;
    auto map_id = std::uint32_t(identity.map);
    if (!map_id) { std::osyncstream(std::cout)<<"AUTH identity_rejected reason=selected_map"<<std::endl;return; }
    std::vector<CharacterInfo> roster;
    struct SelectableCharacter { GameToken id{}; std::uint32_t map=1; std::string name; };
    std::map<std::u16string,SelectableCharacter> selectable_characters;
    std::map<GameToken,std::uint32_t> character_appearances;
    std::map<GameToken,std::uint32_t> character_levels;
    std::map<GameToken,std::uint8_t> character_secondaries;
    std::map<GameToken,std::uint8_t> character_origins;
    std::map<GameToken,std::vector<fw::gw::CharacterProfile::EquipmentAppearance>> character_equipment;
    roster.reserve(roster_size);
    bool selected_found=character_id.empty();
    for(std::size_t index=0;index<roster_size;++index) {
        const auto& entry=roster_entries.at(index);
        const auto member_hex=gwserver::storage_hex(entry.character.bytes);
        std::u16string name16;
        try{name16=fw::gw::character_name_to_utf16(entry.name,19);}catch(const std::invalid_argument&){std::osyncstream(std::cout)<<"AUTH identity_rejected reason=roster_name index="<<index<<std::endl;return;}
        const auto& name=entry.name;
        auto member_map=entry.map; if(!member_map) { std::osyncstream(std::cout)<<"AUTH identity_rejected reason=roster_map index="<<index<<std::endl;return; }
        const auto* member_area=fw::gw::find_map(static_cast<std::uint32_t>(member_map));
        const auto member_campaign=member_area?member_area->campaign:0;
        const bool member_campaign_entitled=member_area&&
            (member_campaign==0||account_campaigns.contains(member_campaign));
        std::osyncstream(std::cout) << "AUTH roster_entry index=" << index
            << " name=" << name << " map=" << member_map
            << " campaign=" << static_cast<unsigned>(member_campaign)
            << " entitled=" << member_campaign_entitled
            << " selected=" << (member_hex==character_id) << std::endl;
        const GameToken member_id=entry.character.bytes;
        if (member_hex == character_id) {
            if (name != identity.character_name || member_map != map_id) { std::osyncstream(std::cout)<<"AUTH identity_rejected reason=selected_record_mismatch index="<<index<<std::endl;return; }
        }
        CharacterInfo character_info;
        character_info.transaction=login.transaction; character_info.character=member_id;
        if(entry.profile) {
            const auto& profile=*entry.profile;
            character_info.apply(profile.for_client(build));
            if(build==37130 && (profile.unknown1==6||profile.unknown1==8) && profile.extension_count>=37)
                character_equipment[member_id]=profile.equipment_2022();
            // Pinned 2022 summary decoder copies summary+8 into its first
            // decoded DWORD: the packed Appearance used by PlayerInfo.
            if(profile.extension_count>=12)
                character_appearances[member_id]=std::uint32_t(profile.extended[2])|
                    (std::uint32_t(profile.extended[3])<<16);
            // Summary decoder RVA 47c3e0: DWORD at summary+28, bits 4..8.
            if(profile.extension_count>=32){
                character_levels[member_id]=(profile.extended[12]>>4)&31;
                character_origins[member_id]=profile.origin_campaign_2022();
                character_secondaries[member_id]=static_cast<std::uint8_t>((profile.extended[12]>>10)&15);
            }
        } else {
            // CharacterInfo's opaque extension contains client-rendering data
            // (profession, appearance and other summary fields). Never invent
            // a plausible-looking summary for characters created locally until
            // the retail create flow can produce one. Reject the roster rather
            // than displaying a placeholder or corrupt character summary.
            std::osyncstream(std::cout)<<"AUTH identity_rejected reason=missing_character_summary index="<<index
                <<" name="<<name<<std::endl;
            return;
        }
        character_info.name=name16;
        auto selection_key=character_info.name;
        for(auto& c:selection_key) if(c>=u'A' && c<=u'Z') c=static_cast<char16_t>(c+u'a'-u'A');
        selectable_characters.emplace(std::move(selection_key),SelectableCharacter{member_id,static_cast<std::uint32_t>(member_map),name});
        if(member_hex==character_id) selected_found=true;
        roster.push_back(std::move(character_info));
    }
    if(!selected_found) { std::osyncstream(std::cout)<<"AUTH identity_rejected reason=selected_not_in_roster"<<std::endl;return; }
    std::string current_character_name=identity.character_name;
    // A login without a character name still needs a real initial selection.
    // Advertise the same GUID in AccountInfo that subsequent Play uses.
    if(character_id.empty()&&!roster.empty()) {
        const auto& initial=roster.front();
        character_guid=initial.character;
        current_character_name=fw::gw::character_name_to_utf8(initial.name,19);
        map_id=static_cast<std::uint32_t>(roster_entries.front().map);
        std::osyncstream(std::cout)<<"AUTH initial_character name="<<current_character_name
            <<" map="<<map_id<<std::endl;
    }
    bool current_instance_known=false;
    std::uint32_t current_map_type=0,current_region=0,current_district=0,current_language=0;
    // Resolve the captured flags for this authenticated local account. A
    // stored profile must never be substituted with another account's capture.
    auto captured_profile=gwserver::NetworkManager::current().server().accountProfile({account_guid});
    if(captured_profile.status!=FW_OK) {
        std::osyncstream(std::cout)<<"AUTH identity_rejected reason=account_profile_lookup"<<std::endl;return;
    }
    auto own_profile=std::move(captured_profile.value);
    if(own_profile) {
        account_profile=&*own_profile;
    }
    AccountInfo account_info;
    account_info.transaction = login.transaction;
    account_info.account = account_guid;
    account_info.character = character_guid;
    if (account_profile) account_profile->apply(account_info);
    // Only an agreement accepted by this client session is persisted. A missing
    // account-service response must never silently turn acceptance on.
    auto eula_state=gwserver::NetworkManager::current().server().accountAgreement({account_guid});
    if(eula_state.status!=FW_OK||!eula_state.value){
        std::osyncstream(std::cout)<<"AUTH identity_rejected reason=agreement_lookup status="
            <<eula_state.status<<std::endl;
        return;
    }
    // Build 38888 compares this byte against required agreement version 27
    // before creating the retail terms dialog. Accounts policy is boolean;
    // translate accepted private-server accounts to the current client version.
    constexpr std::uint8_t fallback_agreement_version=0x1b;
    const auto accepted_agreement_version=account_profile ? account_profile->agreement_version : fallback_agreement_version;
    account_info.agreement_version=(eula_state.status==FW_OK&&eula_state.value.value_or(false))
        ? accepted_agreement_version : 0;
    std::osyncstream(std::cout) << "AUTH eula_lookup status=" << eula_state.status
        << " accepted=" << (eula_state.status==FW_OK&&eula_state.value.value_or(false)) << std::endl;
    ErrorMessage login_result;
    login_result.transaction = login.transaction;
    std::osyncstream(std::cout) << "AUTH response transaction=" << login_result.transaction << std::endl;
    for(auto& character_info:roster) session.queue_response(character_info.encode());
    std::osyncstream(std::cout) << "AUTH character_roster count=" << roster.size() << std::endl;
    session.queue_response(account_info.encode());
    AuthDataLookupResult auth_data_result;
    // Pinned 2022: constructor485360 stores an incrementing request ID at
    // object+20. Login sender489830/48b7b0 emits that ID as CMSG38.transaction;
    // settings receiver486150 matches SMSG22 against the SAME pending object.
    // The counter survives logout, so every reused-client login needs its own
    // decoded transaction; a constant1 leaves s_authDataError NET_PENDING.
    auth_data_result.transaction=build==37130?login.transaction:1;
    auth_data_result.status=0;
    if(build==37130) {
        // The older opcode-22 callback copies the entire ARRAY_8 as account
        // settings. It does not consume a status DWORD prefix.
        const auto settings_path=state_directory/"account-settings"/(hex_bytes(account_guid)+".bin");
        std::ifstream settings(settings_path,std::ios::binary);
        if(settings)auth_data_result.data.assign(std::istreambuf_iterator<char>(settings),{});
        else if(std::filesystem::exists(settings_path))throw std::runtime_error("2022 account settings unreadable");
        // A new local account has no preferences yet. Native486150/48a390
        // accepts an empty ARRAY8, and auth_data regression covers that exact
        // frame. Do not require another account's captured settings to log in.
        if(auth_data_result.data.size()>2048)throw std::runtime_error("account settings bound");
        session.queue_response(auth_data_result.encode(false));
    } else session.queue_response(auth_data_result.encode());
    const auto social_owner=hex_bytes(account_guid);
    auto auth_outbox=std::make_shared<Outbox>();
    const auto guild_delivery_session=transfers.begin_guild_session(account_guid);
    const auto social_lease=social_directory.login(social_owner,current_character_name.empty()?std::u16string{}:fw::gw::character_name_to_utf16(current_character_name),auth_outbox);
    struct SocialLease {std::string owner;std::uint64_t lease;~SocialLease(){social_directory.logout(owner,lease);}} social_cleanup{social_owner,social_lease};
    auto friends=friend_list(social_owner);
    if(build==37130)for(const auto& [id,value]:friends){
        session.queue_response(fw::gw::social::friend_added(login.transaction,value));
        session.queue_response(fw::gw::social::friend_record(value));
    }
    session.queue_response(login_result.encode());
    send_pending(socket, session, fw::now_ms() + 5000);
    std::osyncstream(std::cout) << "AUTHENTICATED character_account_login_result_sent=1" << std::endl;

    fw::gw::AuthenticatedLifetime lifetime(fw::now_ms());
    std::uint32_t settings_upload_id=0,settings_upload_size=0;
    std::vector<std::uint8_t> settings_upload;
    std::uint64_t social_poll=0;
    while (!stopping) {
        const auto deadline = lifetime.deadline();
        if (lifetime.expired(fw::now_ms())) {
            std::osyncstream(std::cout) << "AUTH post_login_timeout=idle" << std::endl;
            break;
        }
        if(build==37130&&fw::now_ms()>=social_poll){
            if(character_guid!=GameToken{}){
                const auto stored_name=gwserver::NetworkManager::current().server().characterName({{account_guid},{character_guid}});
                if(stored_name.status==FW_OK&&*stored_name.value!=current_character_name){
                    const auto wide=fw::gw::character_name_to_utf16(*stored_name.value);
                    for(auto it=selectable_characters.begin();it!=selectable_characters.end();){
                        if(it->second.id==character_guid){auto member=it->second;member.name=*stored_name.value;it=selectable_characters.erase(it);auto key=wide;for(auto& c:key)if(c>=u'A'&&c<=u'Z')c+=u'a'-u'A';selectable_characters.emplace(std::move(key),std::move(member));break;}else ++it;
                    }
                    current_character_name=*stored_name.value;
                    for(auto& member:roster)if(member.character==character_guid){member.name=wide;session.queue_response(member.encode());break;}
                }
            }
            auto updated=friend_list(social_owner);
            for(const auto& [id,value]:updated){const auto old=friends.find(id);if(old==friends.end()||fw::gw::social::friend_record(value)!=fw::gw::social::friend_record(old->second))session.queue_response(fw::gw::social::friend_record(value,true));}
            for(const auto& [id,value]:friends)if(!updated.contains(id)){auto removed=value;removed.status=0;session.queue_response(fw::gw::social::friend_record(removed,true));}
            friends=std::move(updated);social_poll=fw::now_ms()+1000;
        }
        for(auto& packet:auth_outbox->drain())session.queue_response(std::move(packet));
        send_pending(socket,session,fw::now_ms()+5000);
        wait_socket(socket, false, deadline);
        std::array<std::uint8_t, 512> incoming{};
        auto count = recv(socket, reinterpret_cast<char*>(incoming.data()), int(incoming.size()), 0);
        if (count < 0 && WSAGetLastError() == WSAEWOULDBLOCK) continue;
        if (count <= 0) break;
        session.observe_authenticated(std::span(incoming).first(static_cast<std::size_t>(count)));
        sodium_memzero(incoming.data(), incoming.size());
        if (session.state() == Session::State::failed) {
            std::osyncstream(std::cout) << "AUTH post_login_rejection=" << session.failure()
                << " opcode=" << session.observed() << " buffered_bytes=" << session.observed_size_total()
                << " packet_hex=" << hex_bytes(session.rejected_authenticated_input()) << std::endl;
            break;
        }
        for (auto command : session.take_post_login_commands()) {
            lifetime.activity(fw::now_ms());
            std::osyncstream(std::cout) << "AUTH post_login_opcode=" << command.opcode
                << " bytes=" << command.wire_size << std::endl;
            if (command.opcode == 0x0e) {
                if(command.value<=3)social_directory.status(social_owner,social_lease,command.value);
                std::osyncstream(std::cout) << "AUTH player_status value="
                    << command.value << std::endl;
            } else if(build==37130&&(command.opcode==26||command.opcode==27)){
                fw::gw::social::Request request;
                if(fw::gw::social::parse(command.payload,request)!=fw::gw::social::Request::Result::complete)throw std::runtime_error("social parsed request lost");
                std::uint32_t result_status=FW_INVALID;
                if(command.opcode==26){
                    const auto result=gwserver::NetworkManager::current().server().addFriend({gwserver::Identity::from_hex(social_owner),request.type,fw::gw::character_name_to_utf8(request.name,19),request.alias.empty()?std::string{}:fw::gw::character_name_to_utf8(request.alias,19)}); result_status=result.status;
                    if(result.status==FW_OK){if(!result.value)throw std::runtime_error("friend add response");
                        auto updated=friend_list(social_owner);const auto added=updated.find(gwserver::storage_hex(result.value->target.bytes));if(added==updated.end())throw std::runtime_error("saved friend absent");
                        session.queue_response(fw::gw::social::friend_added(request.transaction,added->second));session.queue_response(fw::gw::social::friend_record(added->second,true));friends=std::move(updated);
                    }
                }else{
                    auto target=hex_bytes(request.account);
                    if(std::all_of(request.account.begin(),request.account.end(),[](auto b){return b==0;})){
                        target.clear();for(const auto& [id,value]:friends)if(fw::gw::chat::commands::same_name(value.alias,request.name)){if(!target.empty()){target.clear();break;}target=id;}
                    }
                    const auto result=target.empty()?gwserver::FriendRemoveResult{FW_INVALID,"unknown friend"}:gwserver::NetworkManager::current().server().removeFriend({gwserver::Identity::from_hex(social_owner),gwserver::Identity::from_hex(target)}); result_status=result.status;
                    if(result.status==FW_OK){auto found=friends.find(target);if(found!=friends.end()){session.queue_response(fw::gw::social::friend_removed(request.transaction,found->second));friends.erase(found);}}
                }
                ErrorMessage reply;reply.transaction=request.transaction;reply.code=result_status==FW_OK?0:1;session.queue_response(reply.encode());
                send_pending(socket,session,fw::now_ms()+5000);std::osyncstream(std::cout)<<"AUTH friend_operation opcode="<<command.opcode<<" outcome="<<(result_status==FW_OK?"accepted":"rejected")<<std::endl;
            } else if (command.opcode == 0x21) {
                std::osyncstream(std::cout) << "AUTH post_login_opcode_33_fields="
                    << command.field0 << "," << command.field1 << std::endl;
                if(build==37130){
                    if(!command.field0||command.field1>2048)throw std::runtime_error("settings upload announcement bound");
                    settings_upload_id=command.field0;settings_upload_size=command.field1;
                    settings_upload.clear();settings_upload.reserve(settings_upload_size);
                }
            } else if (command.opcode == 0x0d) {
                std::osyncstream(std::cout) << "AUTH client_disconnect value="
                    << command.value << std::endl;
                return;
            } else if (command.opcode == 0x20) {
                // Pinned 2022 FUN_0048b900 sends a size announcement (33),
                // then settings chunks (32) carrying the same request id.
                std::osyncstream(std::cout) << "AUTH post_login_opcode_32 transaction="
                    << command.transaction << " payload_bytes=" << command.payload.size() << std::endl;
                if(build==37130){
                    if(!settings_upload_id||command.transaction!=settings_upload_id||
                       command.payload.size()>settings_upload_size-settings_upload.size())
                        throw std::runtime_error("settings upload sequence mismatch");
                    settings_upload.insert(settings_upload.end(),command.payload.begin(),command.payload.end());
                    if(settings_upload.size()==settings_upload_size){
                        const auto target=state_directory/"account-settings"/(hex_bytes(account_guid)+".bin");
                        auto temporary=target;temporary+=std::string(".")+std::to_string(session_id)+".tmp";
                        std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
                        output.write(reinterpret_cast<const char*>(settings_upload.data()),settings_upload.size());
                        output.close();
                        if(!output||!MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
                            std::filesystem::remove(temporary);
                            throw std::runtime_error("settings upload persistence failed");
                        }
                        std::osyncstream(std::cout)<<"AUTH settings_upload_saved bytes="<<settings_upload.size()<<std::endl;
                        ErrorMessage completed;completed.transaction=settings_upload_id;completed.code=0;
                        session.queue_response(completed.encode());
                        send_pending(socket,session,fw::now_ms()+5000);
                        settings_upload_id=0;settings_upload.clear();
                    }
                }
            } else if (command.opcode == 0x25) {
                // The current client uses REQUEST_GUILD_HALL as its Play
                // transfer request for characters whose saved location is a
                // guild hall. It carries map_type=2 and no map id; route to
                // the authenticated character's saved map, then use the
                // ordinary GameServerInfo/world-ticket handoff. The legacy
                // packet table's opcode-37 empty frame is not sufficient: a
                // live current-client probe ignored it and disconnected.
                std::osyncstream(std::cout) << "AUTH guild_hall_request transaction="
                    << command.transaction << " map_type=" << command.field0
                    << " field1=" << command.field1 << " field2=" << command.field2
                    << " field3=" << command.field3 << " opaque_bytes=" << command.payload.size() << std::endl;
                // Guild-window travel supplies the owned guild key. Character
                // selection can use this opcode too, so only the matching key
                // overrides the character's normal saved destination.
                const auto guild_state=gwserver::NetworkManager::current().server().guild({account_guid});
                if(guild_state.status==FW_OK&&guild_state.value&&hex_bytes(command.payload)==gwserver::storage_hex(guild_state.value->id.bytes)&&guild_state.value->hall)map_id=guild_state.value->hall;
                auto* destination=fw::gw::find_map(map_id);
                auto position=map_entry_spawn(state_directory,map_id);
                if(destination&&!fw::gw::map_asset_file(static_cast<std::uint16_t>(map_id))){
                    const auto recovery=configured_recovery_map(state_directory);
                    if(recovery&&*recovery!=map_id&&transition_character_map(account_guid,character_guid,map_id,*recovery)){
                        std::osyncstream(std::cout)<<"AUTH saved_map_recovered from="<<map_id<<" to="<<*recovery
                            <<" reason=missing_map_asset"<<std::endl;
                        map_id=*recovery;destination=fw::gw::find_map(map_id);
                        position=map_entry_spawn(state_directory,map_id);
                    }else position.reset();
                }
                if(!command.transaction||command.field0!=2||!destination||destination->type==255||!position) {
                    ErrorMessage rejected;rejected.transaction=command.transaction;rejected.code=1;
                    session.queue_response(rejected.encode());
                } else {
                    // A captured roster can retain its guild-hall Play opcode
                    // after private travel. The request kind is not the
                    // destination's instance type; use the owned map catalog.
                    const auto ids=transfers.issue(account_guid,character_guid,social_lease,destination->type,map_id,
                        0,0,0,current_character_name,position->first,position->second,character_appearances[character_guid],character_levels[character_guid],character_equipment[character_guid],0,character_secondaries[character_guid],0,0,false,character_origins[character_guid]);
                    GameServerInfo transfer;transfer.transaction=command.transaction;
                    transfer.world_id=ids.first;transfer.map_id=map_id;transfer.player_id=ids.second;
                    static_assert(sizeof(world_endpoint)<=24);
                    std::memcpy(transfer.host.data(),&world_endpoint,sizeof(world_endpoint));
                    session.queue_response(transfer.encode());
                    std::osyncstream(std::cout) << "AUTH guild_hall_instance transaction="
                        << command.transaction << " map=" << map_id << " world=" << transfer.world_id << std::endl;
                    current_instance_known=true;current_map_type=destination->type;
                    current_region=0;current_district=0;current_language=0;
                }
                send_pending(socket, session, fw::now_ms()+5000);
            } else if (command.opcode == 0x35) {
                ServerResponse server_response; server_response.transaction = command.transaction;
                ErrorMessage hardware_result; hardware_result.transaction = command.transaction;
                session.queue_response(server_response.encode());
                session.queue_response(hardware_result.encode());
                send_pending(socket, session, fw::now_ms() + 5000);
                std::osyncstream(std::cout) << "AUTH hardware_info_exchange_complete=1" << std::endl;
            } else if (command.opcode == 0) {
                std::vector<std::uint8_t> heartbeat{0,0};
                heartbeat.insert(heartbeat.end(), 4, 0);
                session.queue_response(std::move(heartbeat));
                send_pending(socket, session, fw::now_ms() + 5000);
            } else if(build==37130&&command.opcode==7){
                auto key=command.character_name;
                for(auto& c:key)if(c>=u'A'&&c<=u'Z')c+=u'a'-u'A';
                ErrorMessage result;result.transaction=command.transaction;result.code=1;
                const auto selected=selectable_characters.find(key);
                if(selected!=selectable_characters.end()){
                    const auto id=selected->second.id;
                    const auto deleted=gwserver::NetworkManager::current().server().deleteCharacter({{account_guid},{id}});
                    if(deleted.status==FW_OK){
                        result.code=0;selectable_characters.erase(selected);
                        std::erase_if(roster,[&](const auto& entry){return entry.character==id;});
                        character_appearances.erase(id);character_levels.erase(id);character_equipment.erase(id);
                        if(character_guid==id){character_guid={};current_character_name.clear();current_instance_known=false;}
                    }
                }
                session.queue_response(result.encode());send_pending(socket,session,fw::now_ms()+5000);
                std::osyncstream(std::cout)<<"AUTH character_delete_2022 result="<<(result.code?"rejected":"deleted")<<std::endl;
            } else if(command.opcode==0x0a || (build==37130&&command.opcode==9)) {
                std::u16string key_name=command.character_name;
                for(auto& c:key_name)if(c>=u'A'&&c<=u'Z')c=static_cast<char16_t>(c+u'a'-u'A');
                ErrorMessage changed; changed.transaction=command.transaction;
                auto selected=selectable_characters.find(key_name);
                if(selected==selectable_characters.end()){
                    // Creation commits on the world connection after this auth
                    // session's initial roster snapshot. Reload only from the
                    // authenticated account; the client name grants no ownership.
                    for(unsigned offset=0;offset<8;offset+=3){
                        const auto page=gwserver::NetworkManager::current().server().characterRoster(
                            {{account_guid},offset});
                        if(page.status!=FW_OK)break;
                        const auto count=page.entries.size();
                        for(const auto& entry:page.entries){
                            auto name=fw::gw::character_name_to_utf16(entry.name,19);
                            auto key=name;for(auto& c:key)if(c>=u'A'&&c<=u'Z')c+=u'a'-u'A';
                            if(key!=key_name)continue;
                            if(!entry.profile)throw std::runtime_error("missing refreshed profile");
                            const auto profile=entry.profile->for_client(build);
                            const GameToken id=entry.character.bytes;
                            const auto map=entry.map;
                            CharacterInfo info;info.character=id;info.name=name;info.apply(profile);roster.push_back(info);
                            character_equipment[id]=profile.equipment_2022();
                            character_appearances[id]=std::uint32_t(profile.extended[2])|(std::uint32_t(profile.extended[3])<<16);
                            character_levels[id]=(profile.extended[12]>>4)&31;
                            character_origins[id]=profile.origin_campaign_2022();
                            character_secondaries[id]=static_cast<std::uint8_t>((profile.extended[12]>>10)&15);
                            selectable_characters.emplace(key,SelectableCharacter{id,map,entry.name});
                        }
                        selected=selectable_characters.find(key_name);
                        if(selected!=selectable_characters.end()||offset+count>=page.total)break;
                    }
                }
                if(selected==selectable_characters.end()) changed.code=1;
                else {
                    character_guid=selected->second.id;
                    map_id=selected->second.map;
                    current_character_name=selected->second.name;
                    current_instance_known=false;
                    if(build==37130&&command.opcode==10){
                        // 486230 query0x11 copies AccountInfo.character into
                        // pending selection+50. Completion alone installs zero.
                        auto selection=account_info;
                        selection.transaction=command.transaction;selection.character=character_guid;
                        session.queue_response(selection.encode());
                    }
                    if(build==37130&&command.opcode==9){
                        // The client submits a newly generated summary on
                        // return from a world. Until world state is complete,
                        // retain the imported authoritative profile and send
                        // it back instead of leaving the empty summary in UI.
                        for(const auto& original:roster)if(original.character==character_guid){
                            auto restored=original;restored.transaction=command.transaction;
                            session.queue_response(restored.encode());
                            std::osyncstream(std::cout)<<"AUTH character_summary_restored summary_bytes="<<original.extended_count
                                <<" version="<<original.unknown1<<" equipment_count="
                                <<(original.extended_count>=37?(original.extended[14]&255):0)<<std::endl;
                            break;
                        }
                        current_instance_known=false;
                    }
                }
                session.queue_response(changed.encode());
                send_pending(socket,session,fw::now_ms()+5000);
            } else if (command.opcode == 0x26) {
                // The one-byte command is multiplexed. Value 1 is the observed
                // character-delete action; never interpret other values as
                // deletion requests.
                if (command.value != 1) {
                    std::osyncstream(std::cout) << "AUTH opcode_0x26_unsupported value="
                        << unsigned(command.value) << std::endl;
                    continue;
                }
                auto deleted=gwserver::NetworkManager::current().server().deleteCharacter({{account_guid},{character_guid}});
                ServerResponse result;
                result.code=(deleted.status==FW_OK)?0:1;
                if (!result.code) {
                    auto key=current_character_name;
                    std::u16string wide_key;for(unsigned char c:key)wide_key.push_back(static_cast<char16_t>(c));
                    for(auto& c:wide_key)if(c>=u'A'&&c<=u'Z')c=static_cast<char16_t>(c+u'a'-u'A');
                    selectable_characters.erase(wide_key);
                    character_guid={};
                }
                std::osyncstream(std::cout) << "AUTH character_delete result="
                    << (result.code?"rejected":"deleted") << std::endl;
                session.queue_response(result.encode());
                send_pending(socket,session,fw::now_ms()+5000);
            } else if (command.opcode == 0x29) {
                // Character creation asks for a private preview service (11/0),
                // not travel by the selected character. Preserve its saved map.
                if(build==37130&&command.transaction&&command.map_type==11&&command.map_id==0&&
                   command.region==0&&command.district==0&&command.language==0&&account_campaigns.contains(1)){
                    std::uint8_t creation_mask=0;
                    for(auto campaign:account_campaigns)if(campaign<=3)creation_mask|=static_cast<std::uint8_t>(1u<<campaign);
                    if(creation_mask)creation_mask|=1;
                    const auto ids=transfers.issue(account_guid,character_guid,social_lease,11,0,0,0,0,
                        current_character_name,0,0,character_appearances[character_guid],character_levels[character_guid],{},creation_mask);
                    GameServerInfo preview;preview.transaction=command.transaction;
                    preview.world_id=ids.first;preview.map_id=0;preview.player_id=ids.second;
                    std::memcpy(preview.host.data(),&world_endpoint,sizeof(world_endpoint));
                    session.queue_response(preview.encode());send_pending(socket,session,fw::now_ms()+5000);
                    std::osyncstream(std::cout)<<"AUTH creation_preview_granted transaction="<<command.transaction
                        <<" world="<<ids.first<<std::endl;
                    continue;
                }
                // World travel can change the saved map while this auth
                // connection retains its original character roster snapshot.
                if(!current_instance_known){
                    const auto saved=gwserver::NetworkManager::current().server().characterMap({{account_guid},{character_guid}});
                    if(saved.status!=FW_OK){
                        ErrorMessage rejected;rejected.transaction=command.transaction;rejected.code=1;
                        session.queue_response(rejected.encode());send_pending(socket,session,fw::now_ms()+5000);
                        continue;
                    }
                    const auto saved_map=*saved.value;
                    if(saved_map!=map_id){
                        std::osyncstream(std::cout)<<"AUTH selected_map_refreshed previous="<<map_id
                            <<" current="<<saved_map<<std::endl;
                        map_id=saved_map;
                    }
                }
                // The 2022 Play action requests its selected character's saved
                // destination with an all-zero tuple; fresh login may use a
                // final -1 sentinel, while character-select reentry uses zero.
                // Resolve only that initial request from authenticated state.
                if(build==37130&&!current_instance_known&&command.map_type==0&&command.map_id==0&&
                   command.region==0&&command.district==0&&(command.language==0||command.language==0xffffffffu)) {
                    const auto* saved=fw::gw::find_map(map_id);
                    if(saved){command.map_id=map_id;command.map_type=saved->type;command.language=0;}
                }
                // Native reentry can retain a fallback town in its displayed
                // summary even after multiple world transfers. Initial default
                // entry always resolves to the selected owned character's saved
                // map; the requested stale destination never grants travel.
                if(build==37130&&!current_instance_known&&command.map_id!=map_id&&
                   command.region==0&&command.district==0&&command.language==0){
                    const auto* requested=fw::gw::find_map(command.map_id);const auto* saved=fw::gw::find_map(map_id);
                    if(requested&&saved&&command.map_type==requested->type){command.map_id=map_id;command.map_type=saved->type;}
                }
                // Instance routing is scoped to the map of the character selected
                // on this authenticated connection. The client-provided tuple is
                // a request, not authority to transfer a character elsewhere.
                // Retail redirect callback7ccec0 requests the default region
                // and language (-3/-1). Resolve defaults only before entry.
                if(build==37130&&!current_instance_known){
                    if(command.region==0xfffffffdu)command.region=0;
                    if(command.language==0xffffffffu)command.language=0;
                }
                const auto* destination=fw::gw::find_map(command.map_id);
                const bool map_changed=command.map_id!=map_id;
                const bool instance_changed=current_instance_known&&
                    (command.map_type!=current_map_type||command.region!=current_region||
                     command.district!=current_district||command.language!=current_language);
                const auto tuple_failure=fw::gw::instance_tuple_failure(command.transaction,command.map_id,command.map_type,
                    command.region,command.district,command.language,destination&&account_campaigns.contains(destination->campaign));
                const bool tuple_valid=tuple_failure.empty();
                std::optional<fw::gw::map_portals::Arrival> directed_arrival;
                bool travel_map_committed=false;
                std::optional<std::pair<std::uint32_t,std::uint32_t>> arena_ticket;
                const bool arena_mode_request=command.map_id==2&&command.map_type==6;
                // A first-entry creation intent can target the already selected
                // map. Consume it before the ordinary same-map authorization.
                const bool directed=tuple_valid&&transfers.consume_travel(account_guid,character_guid,
                    command.map_type,command.map_id,command.region,command.district,command.language,
                    &directed_arrival,&travel_map_committed,&arena_ticket);
                const bool travel_authorized=tuple_valid&&(directed||(!arena_mode_request&&
                    (!current_instance_known?!map_changed:!(map_changed||instance_changed))));
                if (!tuple_valid || !travel_authorized) {
                    std::osyncstream(std::cout)<<"AUTH instance_rejected transaction="<<command.transaction
                        <<" map="<<command.map_id<<" type="<<command.map_type<<" selected_map="<<map_id
                        <<" region="<<command.region<<" district="<<command.district<<" language="<<command.language
                        <<" tuple_valid="<<tuple_valid<<" travel_authorized="<<travel_authorized
                        <<" reason="<<(tuple_valid?std::string_view("no_authorized_transfer"):tuple_failure)<<std::endl;
                    ErrorMessage rejected; rejected.transaction = command.transaction; rejected.code = 1;
                    session.queue_response(rejected.encode());
                } else {
                    GameServerInfo transfer;
                    transfer.transaction = command.transaction;
                    // Validate the destination before committing its map to the
                    // character. A rejected transfer must preserve the old map.
                    const auto position=arena_ticket&&directed_arrival?directed_arrival:map_entry_arrival(state_directory,command.map_id,directed_arrival);
                    if(!position){
                        ErrorMessage rejected;rejected.transaction=command.transaction;rejected.code=1;
                        session.queue_response(rejected.encode());
                        send_pending(socket,session,fw::now_ms()+5000);
                        continue;
                    }
                    if(map_changed&&!travel_map_committed&&!transition_character_map(account_guid,character_guid,map_id,command.map_id)){
                        ErrorMessage rejected;rejected.transaction=command.transaction;rejected.code=1;
                        session.queue_response(rejected.encode());
                        send_pending(socket,session,fw::now_ms()+5000);
                        continue;
                    }
                    if(map_changed)map_id=command.map_id;
                    {
                        const auto ids=arena_ticket?*arena_ticket:transfers.issue(account_guid,character_guid,social_lease,command.map_type,command.map_id,
                            command.region,command.district,command.language,current_character_name,position->point.x,position->point.z,character_appearances[character_guid],character_levels[character_guid],character_equipment[character_guid],0,character_secondaries[character_guid],static_cast<std::uint16_t>(position->plane),position->facing,false,character_origins[character_guid]);
                        transfer.world_id = ids.first;
                        transfer.map_id = command.map_id;
                        transfer.player_id = ids.second;
                        static_assert(sizeof(world_endpoint) <= 24);
                        std::memcpy(transfer.host.data(), &world_endpoint, sizeof(world_endpoint));
                        session.queue_response(transfer.encode());
                        std::osyncstream(std::cout) << "AUTH instance transaction=" << command.transaction
                            << " map=" << command.map_id << " world=" << transfer.world_id << std::endl;
                        current_instance_known=true;current_map_type=command.map_type;current_region=command.region;
                        current_district=command.district;current_language=command.language;
                    }
                }
                send_pending(socket, session, fw::now_ms() + 5000);
            }
        }
    }
}

void receive_exact(SOCKET socket, std::span<std::uint8_t> bytes, uint64_t deadline) {
    std::size_t received = 0;
    while (received < bytes.size()) {
        wait_socket(socket, false, deadline);
        auto count = recv(socket, reinterpret_cast<char*>(bytes.data() + received),
                          int(bytes.size() - received), 0);
        if (count < 0 && WSAGetLastError() == WSAEWOULDBLOCK) continue;
        if (count <= 0) throw std::runtime_error("game handshake disconnected after " +
            std::to_string(received) + " of " + std::to_string(bytes.size()) + " bytes");
        received += static_cast<std::size_t>(count);
    }
}

std::uint16_t read16(std::span<const std::uint8_t> data,std::size_t offset){return std::uint16_t(data[offset])|(std::uint16_t(data[offset+1])<<8);}
std::uint32_t read32(std::span<const std::uint8_t> data,std::size_t offset){return std::uint32_t(data[offset])|(std::uint32_t(data[offset+1])<<8)|(std::uint32_t(data[offset+2])<<16)|(std::uint32_t(data[offset+3])<<24);}

void send_world(SOCKET socket,Channel& channel,std::vector<std::uint8_t> packet,uint64_t deadline){
    // Opt-in, bounded movement-only plaintext trace for matching native pose
    // samples to the exact finite goals sent by the authority. No auth data.
    if(std::getenv("FW_GW_TRACE_MOVEMENT")&&packet.size()>=6&&packet.size()<=64){
        const auto opcode=read16(packet,0);
        if(opcode>=40&&opcode<=44){
            std::array<char,129> encoded{};
            sodium_bin2hex(encoded.data(),encoded.size(),packet.data(),packet.size());
            std::osyncstream(std::cout)<<"WORLD movement_wire server_ms="<<fw::now_ms()
                <<" socket="<<socket<<" player="<<read32(packet,2)
                <<" opcode="<<opcode<<" packet="<<encoded.data()<<std::endl;
        }
    }
    if(packet.size()==14&&(read16(packet,0)&0x7fff)==69){
        std::array<char,29> wire{};sodium_bin2hex(wire.data(),wire.size(),packet.data(),packet.size());
        std::osyncstream(std::cout)<<"WORLD aftershock_shake_wire socket="<<socket<<" server_ms="<<fw::now_ms()<<" packet="<<wire.data()<<std::endl;
    }
    channel.encrypt(packet);std::size_t sent=0;
    while(sent<packet.size()){
        wait_socket(socket,true,deadline);
        auto count=send(socket,reinterpret_cast<const char*>(packet.data()+sent),int(packet.size()-sent),0);
        if(count<0&&WSAGetLastError()==WSAEWOULDBLOCK)continue;
        if(count<=0)throw std::runtime_error("world packet send failed");sent+=std::size_t(count);
    }
}

void send_player_state(SOCKET socket,Channel& channel,const TransferRegistry::Grant& grant,
        const std::vector<Instance::Snapshot>& existing,std::uint64_t deadline,const fw::gw::ClientProfile& profile){
    if(profile.build==37130)send_world(socket,channel,fw::gw::agent::team_token_2022(grant.team_token),deadline);
    const auto send_identity=[&](std::uint32_t id,const std::string& name,float x,float y,std::uint32_t appearance,std::uint16_t plane=0,bool local=false,float facing=0,std::uint32_t team_token=0){
        if(local&&grant.map_type==11){
            // The client requires a player record before its preview agent;
            // this record has no chosen name and is never registered in world.
            Packet preview;u16(preview,88);u32(preview,id);u32(preview,id);u32(preview,appearance);
            preview.push_back(0);u32(preview,0);u32(preview,0);u16(preview,0);
            send_world(socket,channel,std::move(preview),deadline);
            // Manifest completion creates the avatar once the scene is ready.
            // Sending it here too triggers the client's duplicate-agent assert.
            return;
        }
        auto packets=player_identity_packets(id,name,x,y,appearance,plane);
        if(profile.build==37130){
            // Legacy full player record precedes agents referencing that player.
            packets=fw::gw::world::player_identity_packets_2022(id,name,x,y,appearance,plane,team_token);
            if(local)for(auto& packet:packets)if(packet.size()>=2&&packet[0]==32&&packet[1]==0)
                packet=fw::gw::agent::spawn_2022(id,0x30000000u|id,x,y,plane,std::cos(facing),std::sin(facing),team_token);
            if(local&&std::getenv("FW_GW_PROBE_EMPTY_MANIFEST"))packets.resize(1);
        }
        for(auto& packet:packets)send_world(socket,channel,std::move(packet),deadline);
    };
    for(const auto& player:existing){
        send_identity(player.id,player.name,player.x,player.y,player.appearance,player.plane,false,0,player.team_token);
        for(const auto& packet:player.presentation)send_world(socket,channel,packet,deadline);
    }
    send_identity(grant.player,grant.name,grant.x,grant.y,grant.appearance,grant.plane,true,grant.facing,grant.team_token);
    if(profile.build!=37130||!std::getenv("FW_GW_PROBE_EMPTY_MANIFEST")){
        // 2022 chclibase bit 1 assigns the local player; bit 2 refreshes its
        // presentation and emits 0x10000013, initializing UIGame's game view.
        std::vector<std::uint8_t> set_player;u16(set_player,34);u32(set_player,grant.player);u32(set_player,profile.build==37130?(std::getenv("FW_GW_PROBE_PLAYER_VIEW")?3:1):0);send_world(socket,channel,std::move(set_player),deadline);
    }
    // Legacy opcode 241 is not a current-client map completion message.
    // Its current descriptor requires two DWORDs and targets another subsystem.
    // Map completion below uses the verified current callback/descriptor.
    std::vector<std::uint8_t> finish;u16(finish,profile.map_finish);send_world(socket,channel,std::move(finish),deadline);
}

void send_map_load(SOCKET socket,Channel& channel,const TransferRegistry::Grant& grant,uint64_t deadline,const fw::gw::ClientProfile& profile){
    auto asset=grant.map<=65535 ? fw::gw::map_asset_file(static_cast<std::uint16_t>(grant.map)) : 0;
    if(grant.observer_match){
        // Existing Guild Wars GvG FFNA IDs identified in the observer reference.
        // Binding a recorded arena does not provision its live collision mesh.
        switch(grant.map){case 167:asset=0x1f24d;break;case 168:asset=0x1f27a;break;case 171:asset=0x1f1fc;break;case 356:asset=0x26625;break;case 358:asset=0x28784;break;case 541:asset=0x3314e;break;case 542:asset=0x334a2;break;default:break;}
    }
    // Build 37130 preview scene, verified through Create -> campaign and
    // appearance screens. Never substitute it for a world destination.
    if(grant.map_type==11&&grant.map==0){
        if(profile.build==37130)asset=165843;
        if(const auto* candidate=std::getenv("FW_GW_CREATION_SCENE_ASSET"))
            asset=static_cast<std::uint32_t>(fw::number(candidate,0x7fffffff));
        std::osyncstream(std::cout)<<"WORLD creation_scene_asset="<<asset<<std::endl;
    }
    if(!asset)throw std::runtime_error("map="+std::to_string(grant.map)+" missing=asset_binding");
    std::vector<std::uint8_t> head;u16(head,profile.map_head);head.insert(head.end(),4,0);send_world(socket,channel,std::move(head),deadline);
    send_world(socket,channel,fw::gw::character_name_packet(profile.map_name,grant.name,grant.map_type==11),deadline);
    std::vector<std::uint8_t> spawn;u16(spawn,profile.map_spawn);u32(spawn,asset);f32(spawn,grant.x);f32(spawn,grant.y);u16(spawn,grant.plane);spawn.push_back(0);spawn.push_back(0);spawn.insert(spawn.end(),8,0);send_world(socket,channel,std::move(spawn),deadline);
}

void handle_observer(SOCKET socket,Channel& channel,const TransferRegistry::Grant& grant,const fw::gw::ClientProfile& profile){
    if(profile.build!=37130||!grant.observer_match)throw std::runtime_error("unsupported observer client");
    const auto& match=*grant.observer_match;fw::gw::observer::Playback replay(match);
    unsigned focus=replay.focus();std::vector<std::uint8_t> pending;std::array<std::uint8_t,4096> incoming{};
    bool map_sent=false,players_sent=false,ready=false;std::uint64_t begun=0,last_report=0;
    auto send_batch=[&](const auto& packets){Packet bytes;for(const auto& p:packets)bytes.insert(bytes.end(),p.begin(),p.end());if(!bytes.empty())send_world(socket,channel,std::move(bytes),fw::now_ms()+5000);};
    auto feedback=[&](std::u16string text){send_batch(fw::gw::chat::global(0,u"Observer",u"",text));};
    auto select=[&](unsigned id,bool initialize=false){if(!match.actors.contains(id))return;focus=id;send_world(socket,channel,fw::gw::agent::team_token_2022(match.actors.at(id).frames.front().team),fw::now_ms()+5000);Packet p;u16(p,34);u32(p,id);u32(p,initialize?3:1);send_world(socket,channel,std::move(p),fw::now_ms()+5000);};
    send_world(socket,channel,fw::gw::world::instance_info(focus,static_cast<std::uint16_t>(match.map),1,grant.district,static_cast<std::uint8_t>(grant.language),1,profile),fw::now_ms()+5000);
    struct Summary {const fw::gw::observer::Playback& replay;const fw::gw::observer::Match& match;~Summary(){try{const auto& m=replay.metrics();std::ostringstream report;report<<"match="<<match.id<<" map="<<match.map<<" time="<<replay.time()<<" finished="<<replay.finished()<<" frames="<<m.frames<<" events="<<m.events<<" adapted="<<m.applied<<" unsupported="<<m.unsupported<<" invalid="<<m.invalid<<" missing_actor="<<m.missing<<" quarantined="<<match.quarantined_records<<'\n';for(const auto& [reason,count]:m.failures)report<<count<<'\t'<<reason<<'\n';std::filesystem::create_directories("data/logs");std::ofstream file("data/logs/observer-"+std::to_string(match.id)+"-"+std::to_string(fw::now_ms())+".txt");file<<report.str();std::osyncstream(std::cout)<<"OBSERVER closed "<<report.str()<<std::flush;}catch(...){}}} summary{replay,match};
    while(!stopping){
        const auto count=recv(socket,reinterpret_cast<char*>(incoming.data()),static_cast<int>(incoming.size()),0);
        if(count==0)return;if(count<0&&WSAGetLastError()!=WSAEWOULDBLOCK)throw std::runtime_error("observer receive failed");
        if(count>0){auto bytes=std::span(incoming).first(count);channel.decrypt(bytes);pending.insert(pending.end(),bytes.begin(),bytes.end());if(pending.size()>65536)throw std::runtime_error("observer input bound");}
        while(pending.size()>=2){const auto frame=fw::gw::world::outgoing_frame_2022(pending);if(frame.state==fw::gw::world::FrameState::incomplete)break;if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("observer malformed client frame");
            const auto op=read16(pending,0)&0x7fff;std::osyncstream(std::cout)<<"OBSERVER request opcode="<<op<<" bytes="<<frame.size<<std::endl;
            if((op==143||op==137||op==145)&&!map_sent){send_map_load(socket,channel,grant,fw::now_ms()+5000,profile);map_sent=true;}
            else if(op==142&&!players_sent&&map_sent){
                begun=fw::now_ms();send_world(socket,channel,fw::gw::agent::instance_timer_2022(match.start),fw::now_ms()+5000);
                send_batch(replay.initialize());send_batch(replay.teams());
                for(const auto& [id,a]:match.actors){send_world(socket,channel,fw::gw::attributes::initial_packet_2022(id,0,0),fw::now_ms()+5000);}
                select(focus,true);send_world(socket,channel,fw::gw::observer::party_ui::activate_hud(),fw::now_ms()+5000);send_world(socket,channel,{141,1},fw::now_ms()+5000); // 397 stream completion
                send_batch(fw::gw::manifest::packets({static_cast<std::uint16_t>(match.map),0,{}}));players_sent=true;
            }else if(op==134&&players_sent&&!ready){
                const auto& actor=match.actors.at(focus);fw::gw::skills::Profile skills;skills.primary=actor.primary;skills.secondary=actor.secondary;
                send_batch(fw::gw::skills::restore_packets(focus,skills));
                send_world(socket,channel,fw::gw::items::inventory_2022(1),fw::now_ms()+5000);
                send_batch(fw::gw::inventory::restore(fw::gw::inventory::baseline()));
                Packet stats;u16(stats,232);for(unsigned i=0;i<15;++i)u32(stats,i==9?20:i==10?100:0);send_world(socket,channel,std::move(stats),fw::now_ms()+5000);
                select(focus);send_world(socket,channel,{109,1,0},fw::now_ms()+5000); // 365 map loaded
                ready=true;send_batch(replay.advance(static_cast<unsigned>(std::min<std::uint64_t>(match.end,match.start+fw::now_ms()-begun))));feedback(u"Recorded match playback. Select a player to follow. Gameplay input is disabled.");
                std::osyncstream(std::cout)<<"OBSERVER ready match="<<match.id<<" map="<<match.map<<" actors="<<match.actors.size()<<" start="<<match.start<<" end="<<match.end<<" host_begin_ms="<<begun<<" unresolved_surfaces="<<match.unresolved_surfaces<<" quarantined="<<match.quarantined_records<<std::endl;
            }else if(op==191&&frame.size==10&&ready){select(read32(pending,2));}
            else if(op==38)return;
            else if(op==98&&ready){feedback(u"This is recorded playback; gameplay and account changes are disabled. Log out to leave observer mode.");}
            pending.erase(pending.begin(),pending.begin()+frame.size);
        }
        if(ready&&!replay.finished()){
            const auto now=fw::now_ms();const auto target=static_cast<unsigned>(std::min<std::uint64_t>(match.end,match.start+now-begun));if(target>replay.time())send_batch(replay.advance(target));
            if(now-last_report>=5000){last_report=now;std::osyncstream(std::cout)<<"OBSERVER progress match="<<match.id<<" time="<<replay.time()<<" events="<<replay.metrics().events<<" packets="<<replay.metrics().packets<<std::endl;}
            if(replay.finished())feedback(u"Recorded match finished. Log out to return to character selection.");
        }
        wait_socket(socket,false,fw::now_ms()+5000,20);
    }
}

void handle_game(SOCKET socket, fw::gw::auth::DiffieHellman& key, const sockaddr_in& world_endpoint, TransferRegistry& transfers,Instances& instances,
                 const fw::gw::world::NpcCatalog& npcs,
                 const std::map<std::uint32_t,std::shared_ptr<const Mesh>>& navmeshes,
                 const std::optional<fw::gw::map_portals::Catalog>& portals,std::uint32_t build,const std::filesystem::path& state_directory) {
    const auto profile=*fw::gw::client_profile(build);
    const BOOL no_delay=TRUE;
    if(setsockopt(socket,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<const char*>(&no_delay),sizeof(no_delay)))
        throw std::runtime_error("world TCP_NODELAY failed");
    using namespace fw::gw::auth;
    u_long nonblocking = 1;
    if (ioctlsocket(socket, FIONBIO, &nonblocking)) return;
    const auto deadline = fw::now_ms() + 10000;
    // The current client sends a 64-byte preface followed by a length-prefixed
    // DH seed (up to 66 bytes), read separately below.
    std::array<std::uint8_t,64> version{};
    receive_exact(socket, version, deadline);
    fw::gw::world::Preface preface;
    if (fw::gw::world::parse_preface(version,preface)!=Parse::complete)
        throw std::runtime_error("game version preface malformed");
    std::osyncstream(std::cout) << "WORLD version_received version=" << preface.build
        << " world=" << preface.world << " map=" << preface.map
        << " player=" << preface.player << std::endl;
    if (preface.build != build ||
        !preface.world || preface.map > 65535 || !preface.player)
        throw std::runtime_error("game version preface rejected");
    auto grant=transfers.consume(preface.world,preface.player,preface.map,
        preface.account,preface.character);
    if(!grant)throw std::runtime_error("world transfer grant rejected");
    bool reconnect_allowed=true;
    struct ReleaseWorldTicket {
        TransferRegistry& registry;std::pair<std::uint32_t,std::uint32_t> key;bool& resume;
        ~ReleaseWorldTicket(){registry.disconnected(key,resume);}
    } release_ticket{transfers,{grant->world,grant->player},reconnect_allowed};
    if(grant->map_type!=11){
        const auto current_character_name=gwserver::NetworkManager::current().server().characterName({{grant->account},{grant->character}});
        if(current_character_name.status!=FW_OK)throw std::runtime_error("world character name unavailable");
        grant->name=*current_character_name.value;
    }
    std::optional<std::uint32_t> experience;
    fw::gw::skills::Profile skill_profile;
    skill_profile.primary=static_cast<std::uint8_t>((grant->appearance>>20)&15);
    skill_profile.secondary=grant->secondary;
    std::uint64_t skill_revision=0;
    std::uint32_t skill_campaigns=0;
    const bool staging=fw::gw::attributes::is_outpost_type(grant->map_type);
    fw::gw::skills::SkillbarState skillbar(grant->player);
    const bool trial_area=fw::gw::quests::presearing::trial_area(grant->map);
    std::optional<fw::gw::skills::effects::HexCopySession> hex_copy_session;
    std::optional<fw::gw::skills::effects::HexCopyIds> hex_copy_ids;
    const auto base_session_skills=[&](){return fw::gw::skills::match_profile(fw::gw::skills::trial_view(skill_profile,trial_area),grant->map_type==6);};
    const auto session_skills=[&](){auto profile=base_session_skills();
        if(hex_copy_session&&hex_copy_session->base()==profile.bar)profile.bar=hex_copy_session->bar();
        return profile;};
    fw::gw::attributes::Profile attribute_profile;
    auto inventory_profile=fw::gw::inventory::baseline();
    auto storage_profile=fw::gw::storage::baseline();
    fw::gw::inventory::identity::Session inventory_ids;
    fw::gw::inventory::gold::Balances client_gold;
    std::uint64_t inventory_revision=0,storage_revision=0;
    bool storage_open=false;
    bool storage_bags_present=false;
    std::uint32_t storage_actor=0;
    fw::gw::progression::Factions faction_profile;
    fw::gw::progression::Character progression_profile;
    std::uint64_t progression_revision=0;
    const auto reload_progression=[&](){
        const auto factions=gwserver::NetworkManager::current().server().accountFactions({grant->account});
        const auto character=gwserver::NetworkManager::current().server().characterProgression({{grant->account},{grant->character}});
        if(factions.status!=FW_OK||character.status!=FW_OK)throw std::runtime_error("progression lookup failed");
        faction_profile=factions.record?factions.record->profile:fw::gw::progression::Factions{};
        progression_revision=character.record?character.record->revision:0;
        progression_profile=character.record?character.record->profile:fw::gw::progression::Character{};
    };
    const auto reload_inventory=[&](){
        const auto restore=[&](bool bank,auto& value,std::uint64_t& revision){
            const auto result=bank?gwserver::NetworkManager::current().server().accountStorage({grant->account}):
                gwserver::NetworkManager::current().server().characterInventory({{grant->account},{grant->character}});
            if(result.status!=FW_OK)throw std::runtime_error("inventory lookup failed: "+result.message);
            if(result.record){revision=result.record->revision;value=result.record->profile;}
        };
        inventory_profile=fw::gw::inventory::baseline();storage_profile=fw::gw::storage::baseline();
        inventory_revision=storage_revision=0;
        restore(false,inventory_profile,inventory_revision);
        if(!inventory_revision&&grant->map_type!=11&&skill_profile.primary>=1&&skill_profile.primary<=10){
            auto starter=fw::gw::energy::private_starter_armor(skill_profile.primary,grant->equipment,fw::gw::character_name_to_utf16(grant->name,31));
            if(!starter.items.empty()){
                const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},0,starter});
                if(saved.status==FW_OK){inventory_revision=saved.revision;inventory_profile=std::move(starter);
                    std::osyncstream(std::cout)<<"WORLD private_starter_armor_created count="<<inventory_profile.items.size()<<std::endl;
                }else restore(false,inventory_profile,inventory_revision);
            }
        }
        restore(true,storage_profile,storage_revision);
        fw::gw::storage::validate(storage_profile);
        // One base pane plus one per owned campaign. Expansion4 does not
        // grant a campaign pane; already-owned additional panes are retained.
        unsigned entitled_panes=1;
        for(unsigned campaign=1;campaign<=3;++campaign)if(skill_campaigns&(1u<<campaign))++entitled_panes;
        auto provisioned=fw::gw::storage::provision_panes(storage_profile,entitled_panes);
        if(provisioned.bags.size()!=storage_profile.bags.size()){
            const auto saved=gwserver::NetworkManager::current().server().transferStorage({{{grant->account},{grant->character}},inventory_revision,storage_revision,inventory_profile,provisioned});
            if(saved.status!=FW_OK)throw std::runtime_error("storage pane provisioning failed: "+saved.message);
            inventory_revision=saved.inventory_revision;storage_revision=saved.storage_revision;
            storage_profile=std::move(provisioned);
            std::osyncstream(std::cout)<<"WORLD storage_panes_provisioned count="<<entitled_panes<<std::endl;
        }
        auto wire=inventory_ids.hydrate(inventory_profile,storage_profile);
        inventory_profile=std::move(wire.character);storage_profile=std::move(wire.bank);
    };
    fw::gw::attributes::AttributeState attribute_state(grant->player);
    std::uint64_t attribute_revision=0;
    const auto attribute_policy=[&](){
        return fw::gw::attributes::Policy{skill_profile.primary,skill_profile.secondary,
            attribute_profile.total_points(std::clamp(grant->level,1u,20u)),
            fw::gw::attributes::can_increase_type(grant->map_type),staging};
    };
    const auto skill_owner=[&](){return gwserver::OwnedCharacter{{grant->account},{grant->character}};};
    const auto reload_attributes=[&](){
        const auto result=gwserver::NetworkManager::current().server().characterAttributes(skill_owner());
        if(result.status!=FW_OK)throw std::runtime_error("character attribute lookup failed");
        if(result.record){attribute_profile=result.record->profile;attribute_revision=result.record->revision;}
        if(attribute_state.restore(attribute_profile.ranks,attribute_policy())!=fw::gw::attributes::Reason::accepted)
            throw std::runtime_error("stored attributes violate profession or point policy");
    };
    const auto reload_skills=[&](){
        const auto result=gwserver::NetworkManager::current().server().characterSkills(skill_owner());
        if(result.status!=FW_OK)throw std::runtime_error("character skill lookup failed");
        if(!result.record)return;
        const auto& restored=result.record->profile;
        const auto effective=fw::gw::skills::trial_view(restored,trial_area);
        if(skillbar.restore(effective.bar,fw::gw::skills::policy(effective,skill_campaigns,staging,false,trial_area))!=fw::gw::skills::Reason::accepted)
            throw std::runtime_error("stored skillbar violates account policy");
        skill_profile=restored;skill_revision=result.record->revision;
    };
    if(grant->map_type!=11){
        const auto campaigns=gwserver::NetworkManager::current().server().accountCampaigns({grant->account});
        if(campaigns.status!=FW_OK)throw std::runtime_error("skill campaign lookup failed");
        for(auto id:campaigns.campaigns)skill_campaigns|=1u<<id;
        reload_skills();
        reload_attributes();
        reload_inventory();
        reload_progression();
        const auto result=gwserver::NetworkManager::current().server().characterExperience({{grant->account},{grant->character}});
        if(result.status!=FW_OK)throw std::runtime_error("character experience lookup failed");
        if(result.value)experience=*result.value;
    }
    // Established clients may stay quiet while reading creation screens or
    // standing in town. Let the transport detect a vanished peer instead of
    // treating absence of gameplay input as a disconnect.
    tcp_keepalive keepalive{1,60000,10000};DWORD keepalive_bytes=0;
    if(WSAIoctl(socket,SIO_KEEPALIVE_VALS,&keepalive,sizeof keepalive,nullptr,0,&keepalive_bytes,nullptr,nullptr)==SOCKET_ERROR)
        throw std::runtime_error("world TCP keepalive configuration failed");
    std::osyncstream(std::cout) << "WORLD transfer_grant_validated map=" << grant->map << std::endl;
    auto world_lease=std::make_shared<Instances::Lease>(instances.acquire(grant->world,[&](auto& instance){
        instance.configure_movement_clock([]{return fw::now_ms();});
        // Wiki Range: creatures/objects fade and cannot be selected beyond5020.
        instance.configure_npc_visibility(5020.f);
        // Pre-Reforged pre-Searing supports two players (four is a 2025 change).
        instance.configure_party_capacity(fw::gw::map_party_limit_2022(grant->map));
        instance.configure_recovery(fw::gw::recovery::development_schedule);
        if(const auto navmesh=navmeshes.find(grant->map);navmesh!=navmeshes.end())
            instance.configure_navigation(navmesh->second);
        if(grant->map_type!=11&&!grant->observer_match)instance.configure_npcs(npcs.for_primary(grant->map,skill_profile.primary));
        if(grant->map_type!=11&&!grant->observer_match&&(grant->map==146||grant->map==147))
            instance.configure_northlands(grant->map==146?fw::gw::northlands::Scene::south146:fw::gw::northlands::Scene::north147);
        if(grant->map==162&&grant->map_type!=11&&!grant->observer_match){
            for(const auto& npc:npcs.for_primary(162,skill_profile.primary))if(npc.id==23000){
                if(!fw::gw::world::enroll_presearing_stalker(instance,npc,fw::now_ms()))
                    throw std::runtime_error("reviewed Stalker enrollment rejected");
            }
        }
        // Exact reviewed development bindings, not model-wide arming.
        if(grant->map==146&&grant->map_type!=11&&!grant->observer_match){
            for(const auto& npc:npcs.for_map(146))if(npc.combat_definition&&
                    ((npc.id==59000&&npc.combat_definition->binding.model_file==116226)||
                     (npc.id>=59001&&npc.id<=59005&&npc.combat_definition->binding.model_file==141267)||
                      (npc.id>=10002&&npc.id<=10003&&npc.combat_definition->binding.model_file==82023))){
                const bool tad=npc.id>=10002&&npc.id<=10003;
                const bool melee=tad||(npc.id>=59001&&npc.id<=59005);
                const std::string name=tad?"tad-development":melee?"raider-development":"firestarter-development";
                const auto mesh=navmeshes.find(146);const auto source=instance.effect_scope(npc.id);
                if(mesh==navmeshes.end()||!mesh->second->locate_cell(npc.plane,{npc.x,npc.y})||!source){
                    std::osyncstream(std::cout)<<"WORLD population_ai_refused map=146 actor="<<npc.id<<" reason=navigation_or_scope"<<std::endl;
                    continue;
                }
                fw::gw::combat::ai::FlarePolicy policy;policy.source=*source;policy.level=tad?0:melee?2:1;
                policy.proximity_acquisition=!tad;
                policy.speed=220;policy.acquire=1000;policy.follow=1500;policy.leash=2000;
                policy.flight={1000,32};policy.max_flights=4;
                policy.evidence="data/gw/ai/"+name+"-policy.txt";
                const auto result=melee?instance.configure_npc_population_ordinary(*source,npc.combat_definition->binding,policy,
                    {5,5,0,0,false,fw::gw::combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},policy.evidence},
                    {{{80,0},{0,80},{-80,0},{0,-80}},1200,6000,policy.evidence},fw::now_ms()):
                    instance.configure_npc_population_elemental(*source,npc.combat_definition->binding,policy,
                    {{{194,0},*fw::gw::skills::effects::elemental(194,0),policy.evidence}},
                    {{{80,0},{0,80},{-80,0},{0,-80}},1200,6000,policy.evidence},fw::now_ms());
                using Enrollment=fw::gw::world::Instance::NpcPopulationEnrollment;
                const char* status="invalid_policy";
                switch(result){
                    case Enrollment::armed:status="armed";break;
                    case Enrollment::already_armed:status="already_armed";break;
                    case Enrollment::identity_mismatch:status="identity_mismatch";break;
                    case Enrollment::unknown_combat:status="unknown_combat";break;
                    case Enrollment::conflicting_controller:status="conflicting_controller";break;
                    case Enrollment::deferred:status="deferred";break;
                    case Enrollment::invalid_policy:break;
                }
                std::osyncstream(std::cout)<<"WORLD population_ai map=146 actor="<<npc.id<<" policy="<<name<<" status="<<status<<std::endl;
            }
        }
    }));
    if(!transfers.retain_world({grant->world,grant->player},world_lease))throw std::runtime_error("world transfer ownership lost");
    auto& world=world_lease->instance();
    const auto ai_coverage=world.npc_ai_coverage();
    std::osyncstream(std::cout)<<"WORLD npc_ai_coverage map="<<grant->map<<" placed="<<ai_coverage.placed
        <<" reviewed_hostiles="<<ai_coverage.reviewed_hostiles<<" active="<<ai_coverage.active_hostiles
        <<" unknown_health="<<ai_coverage.unknown_health<<" wandering="<<ai_coverage.wandering<<std::endl;
    std::osyncstream(std::cout)<<"WORLD map_support map="<<grant->map
        <<" asset="<<fw::gw::map_asset_file(static_cast<std::uint16_t>(grant->map))
        <<" collision="<<(navmeshes.contains(grant->map)?"loaded":"missing")
        <<" zone_entry=unverified"<<std::endl;
    auto outbox=std::make_shared<Outbox>();
    bool joined=false;unsigned entry_morale=100;
    struct LeaveOnExit { Instance& world;std::uint32_t player;bool& joined;
        ~LeaveOnExit(){if(joined)world.leave(player);}
    } leave{world,grant->player,joined};    std::array<std::uint8_t,2> seed_header{};
    receive_exact(socket, seed_header, deadline);
    if (seed_header[0]!=0 || seed_header[1]<3 || seed_header[1]>66)
        throw std::runtime_error("game seed framing rejected");
    std::vector<std::uint8_t> seed(seed_header[1]);
    std::copy(seed_header.begin(),seed_header.end(),seed.begin());
    receive_exact(socket,std::span(seed).subspan(2),deadline);
    std::osyncstream(std::cout) << "WORLD client_seed_received=" << seed.size() << std::endl;
    std::optional<Channel> channel;
    {
        std::lock_guard lock(key_mutex);
        channel.emplace(key, seed);
    }
    std::size_t sent = 0;
    const auto reply = channel->reply();
    while (sent < reply.size()) {
        wait_socket(socket, true, deadline);
        auto count = send(socket, reinterpret_cast<const char*>(reply.data()+sent), int(reply.size()-sent), 0);
        if (count < 0 && WSAGetLastError() == WSAEWOULDBLOCK) continue;
        if (count <= 0) throw std::runtime_error("game seed response failed");
        sent += static_cast<std::size_t>(count);
    }
    std::osyncstream(std::cout) << "WORLD encrypted_channel=1 map=" << preface.map << std::endl;
    if(grant->observer_match){reconnect_allowed=false;handle_observer(socket,*channel,*grant,profile);return;}
    // The current request gate requires InstanceInfo plus connection completion.
    // Sending this only after the map request creates a client/server deadlock.
    // Opcode 426 is a separate UI notification, not connection readiness.
    send_world(socket,*channel,fw::gw::world::instance_info(grant->player,
        static_cast<std::uint16_t>(grant->map),
        static_cast<std::uint8_t>(fw::gw::skills::casting::map_allows_casts(grant->map_type)),grant->district,
        static_cast<std::uint8_t>(grant->language),0,profile),fw::now_ms()+5000);
    std::vector<std::uint8_t> pending;pending.reserve(1024);std::array<std::uint8_t,512> incoming{};
    unsigned equipment_visibility=0;
    if(!grant->name.empty()){const auto state=gwserver::NetworkManager::current().server().equipmentVisibility({{grant->account},{grant->character}});if(state.status!=FW_OK||!state.value)throw std::runtime_error("equipment visibility lookup failed");equipment_visibility=*state.value;}
    bool map_sent=false,player_sent=false,loaded=false,gameplay_ready=false,manifest_spawned=false,manifest_sent=false;
    const auto send_manifest=[&]{
        if(manifest_sent||build!=37130)return;
        const auto override_path=std::getenv("FW_GW_MANIFEST_CATALOG");
        const std::filesystem::path path=override_path?override_path:"data/gw/map-manifests.txt";
        std::optional<fw::gw::manifest::Entry> entry;
        if(std::filesystem::exists(path)){
            std::ifstream file(path);if(!file)throw std::runtime_error("manifest catalog cannot be opened");
            const auto catalog=fw::gw::manifest::read(file);
            if(const auto found=catalog.find(grant->map);found!=catalog.end())entry=found->second;
        }else if(override_path)throw std::runtime_error("configured manifest catalog missing");
        const bool diagnostic=!entry;
        if(diagnostic){
            if(!std::getenv("FW_GW_PROBE_EMPTY_MANIFEST"))return;
            entry=fw::gw::manifest::Entry{static_cast<std::uint16_t>(grant->map),0,{}};
        }
        const auto manifest_context=grant->map_type==11?fw::gw::manifest::Context::creation_preview:fw::gw::manifest::Context::world;
        for(auto packet:fw::gw::manifest::packets(*entry,manifest_context))send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
        manifest_sent=true;
        std::osyncstream(std::cout)<<"WORLD "<<(diagnostic?"diagnostic_empty_manifest_sent=1":"catalog_manifest_sent=1")
            <<" map="<<grant->map<<" version="<<entry->version<<" files="<<entry->phases[0].size()
            <<" composites="<<entry->phases[1].size()<<std::endl;
    };
    std::uint64_t world_entered_at=0,next_timed_quest_check=0;
    std::optional<fw::gw::exploration::Progress> exploration;
    std::optional<std::array<float,4>> exploration_bounds;
    std::filesystem::path exploration_path;
    bool exploration_storage_enabled=true;
    std::uint64_t last_exploration_sample=0;
    // The initial player stream owns SMSG323/local inventory initialization.
    fw::gw::creation::Equipment preview_equipment(true);
    std::array<std::uint8_t,9> preview_colors{};
    unsigned preview_profession=0,preview_campaign=0;
    std::optional<fw::gw::creation::Created> created_character;
    std::array<std::uint8_t,8> created_appearance{};
    auto idle_deadline = fw::now_ms() + world_idle_timeout_ms;
    fw::gw::agent::Clock agent_clock;
    fw::gw::map_portals::Detector portal_detector;
    const auto flush_outbox=[&](){for(auto& packet:outbox->drain())send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);};
    fw::gw::skills::casting::Runtime skill_runtime(grant->player);
    const auto social_owner=hex_bytes(grant->account);
    fw::gw::trade::Sessions::Handle trade_handle=0;
    struct TradeLease {
        fw::gw::trade::Sessions::Handle& handle;
        ~TradeLease(){try{trade_sessions.detach(handle);}catch(...){}}
    } trade_cleanup{trade_handle};
    const auto leave_trade=[&](){trade_sessions.detach(trade_handle);trade_handle=0;};
    // Creation preview has no character identity until the naming step.
    auto social_character=(grant->map_type==11&&grant->name.empty())
        ?std::u16string{}:fw::gw::character_name_to_utf16(grant->name,19);
    bool gm_authorized=false,development_disconnect=false,development_die=false;
    {std::ifstream admins("config/private-admins.txt");std::string line;while(std::getline(admins,line)){
        if(const auto comment=line.find('#');comment!=line.npos)line.resize(comment);
        std::istringstream record(line);std::string account,extra;if(record>>account&&!(record>>extra)&&account==social_owner)gm_authorized=true;
    }}
    std::optional<std::uint16_t> gm_travel;
    struct LeaveWorldPresence {std::string owner;std::shared_ptr<fw::gw::world::Outbox> outbox;~LeaveWorldPresence(){social_directory.leave_world(owner,outbox);}} leave_presence{social_owner,outbox};
    std::string guild_snapshot;
    Packet guild_general_snapshot,guild_role_snapshot,guild_announcement_snapshot;
    const auto guild_delivery=grant->guild_announcements?grant->guild_announcements:std::make_shared<fw::gw::guild::AnnouncementDelivery>();
    std::map<std::string,fw::gw::guild::Member> guild_members;
    std::uint64_t last_guild_poll=0;
    const auto guild_state=[&](){const auto result=gwserver::NetworkManager::current().server().guild(gwserver::Identity::from_hex(social_owner));if(result.status!=FW_OK)throw std::runtime_error("guild storage unavailable");return result.value;};
    const auto restore_guild=[&](){
        const auto state=guild_state();
        if(!state){
            guild_delivery->clear();
            if(!guild_snapshot.empty())send_world(socket,*channel,fw::gw::guild::affiliation(grant->player,0),fw::now_ms()+5000);
            if(!guild_snapshot.empty()){send_world(socket,*channel,fw::gw::guild::clear_membership(),fw::now_ms()+5000);guild_snapshot.clear();guild_members.clear();guild_role_snapshot.clear();guild_announcement_snapshot.clear();}
            return;
        }
        const auto guild_id=gwserver::storage_hex(state->id.bytes);
        fw::gw::guild::General general;general.local_id=1;const auto bytes=state->id.bytes;
        for(unsigned i=0;i<4;++i)general.key[i]=read32(bytes,i*4);
        general.name=fw::gw::character_name_to_utf16(state->name,31);general.tag=fw::gw::character_name_to_utf16(state->tag,4);
        const auto owned_hall=state->hall;
        if(owned_hall){
            // Native GuildUI848A10 enables travel at feature level40.
            // Native7C0F70 maps final flag1 to record8C bit4 (claimed hall);
            // flag2 additionally selects this record as the currently visited hall.
            general.features=40;general.unknown=1|(grant->map==owned_hall?2:0);
        }
        const auto faction_state=gwserver::NetworkManager::current().server().guildFaction(gwserver::Identity::from_hex(social_owner));
        if(faction_state.status!=FW_OK)throw std::runtime_error("guild faction storage unavailable");
        if(!faction_state.value)throw std::runtime_error("guild faction state missing");
        general.faction=static_cast<std::uint8_t>(faction_state.value->allegiance);
        general.faction_points=faction_state.value->points;
        const auto cape_state=gwserver::NetworkManager::current().server().guildCape(gwserver::Identity::from_hex(social_owner));
        if(cape_state.status!=FW_OK)throw std::runtime_error("guild cape storage unavailable");
        if(cape_state.value)fw::gw::guild::cape::apply(general,*cape_state.value);
        unsigned own_rank=3;std::map<std::string,fw::gw::guild::Member> members;
        for(const auto& entry:state->members){fw::gw::guild::Member value;
            const auto id=gwserver::storage_hex(entry.account.bytes);
            value.invited_name=fw::gw::character_name_to_utf16(entry.alias,19);
            value.rank=fw::gw::guild::native_rank(entry.rank);
            if(id==social_owner)own_rank=entry.rank;
            if(const auto presence=social_directory.get(id);presence&&presence->status){value.current_name=presence->character;value.status=presence->status;}
            members.emplace(id,std::move(value));
        }
        if(own_rank>2)throw std::runtime_error("guild own membership missing");
        const bool initial_guild=guild_snapshot!=guild_id;
        if(initial_guild){
            if(!guild_snapshot.empty())send_world(socket,*channel,fw::gw::guild::clear_membership(),fw::now_ms()+5000);
            guild_members.clear();guild_snapshot=guild_id;guild_general_snapshot.clear();guild_role_snapshot.clear();guild_announcement_snapshot.clear();
        }
        const auto general_packet=fw::gw::guild::general(general);
        if(general_packet!=guild_general_snapshot){
            for(auto& packet:fw::gw::guild::appearance_update(general,grant->player,!initial_guild))
                send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
            guild_general_snapshot=general_packet;
        }
        auto role=fw::gw::guild::role(1,general.key,fw::gw::guild::native_rank(own_rank));
        if(role!=guild_role_snapshot){send_world(socket,*channel,role,fw::now_ms()+5000);guild_role_snapshot=std::move(role);}
        for(const auto& [id,value]:guild_members)if(!members.contains(id))send_world(socket,*channel,fw::gw::guild::remove(value.invited_name),fw::now_ms()+5000);
        for(const auto& [id,value]:members){const auto old=guild_members.find(id);if(old==guild_members.end()||fw::gw::guild::member(old->second)!=fw::gw::guild::member(value))
            send_world(socket,*channel,fw::gw::guild::member(value),fw::now_ms()+5000);}
        const auto announcement_body=(state->announcement.empty()?std::u16string{}:fw::gw::character_name_to_utf16(state->announcement,255));const auto announcement_author=(state->author.empty()?std::u16string{}:fw::gw::character_name_to_utf16(state->author,19));
        auto announcement=fw::gw::guild::announcement(announcement_body,announcement_author);
        if(announcement!=guild_announcement_snapshot){
            const bool published=guild_delivery->publish(guild_id,announcement,[&](const Packet& packet){send_world(socket,*channel,packet,fw::now_ms()+5000);});
            guild_announcement_snapshot=std::move(announcement);
            std::osyncstream(std::cout)<<"WORLD guild_announcement_published body_units="<<announcement_body.size()
                <<" author_units="<<announcement_author.size()<<" native_chat="<<(published&&!announcement_body.empty())<<" session_duplicate="<<!published<<std::endl;
        }
        // Native283 marks GuildContext ready and creates the Guild window.
        if(initial_guild)send_world(socket,*channel,fw::gw::guild::complete(),fw::now_ms()+5000);
        guild_members=std::move(members);
    };
    std::optional<fw::gw::skills::effects::Scope> skill_actor_scope;
    std::optional<bool> pet_presence_ready;
    const auto equipment_rune_bonuses=[&]{
        return fw::gw::equipment::attribute_runes::worn(inventory_profile,skill_profile.primary).value_or(fw::gw::equipment::rules::Bonuses{});
    };
    const auto equipment_health_bonus=[&]{
        return equipment_rune_bonuses().health+static_cast<int>(fw::gw::energy::owned_basic_health(inventory_profile,skill_profile.primary));
    };
    const auto attribute_bonuses=[&]{
        fw::gw::attributes::Bonuses result{};const auto equipment=equipment_rune_bonuses();
        for(unsigned id=0;id<result.size();++id)result[id]=static_cast<std::int16_t>(equipment.attributes[id]);
        if(skill_actor_scope)if(const auto snapshot=world.status_snapshot(*skill_actor_scope,fw::now_ms())){
            auto bonus=fw::gw::skills::effects::elemental_attribute_bonus(*snapshot,fw::now_ms());
            if(const auto glyph=world.elemental_glyph_snapshot(*skill_actor_scope,fw::now_ms()))bonus+=glyph->plan.elemental_bonus;
            for(unsigned id=8;id<=11;++id)result[id]+=static_cast<std::int16_t>(bonus);
        }
        const bool weakness=skill_actor_scope&&world.has_condition(*skill_actor_scope,fw::gw::combat::conditions::Kind::weakness,fw::now_ms());
        return fw::gw::attributes::condition_bonuses(attribute_state.ranks(),result,weakness);
    };
    const auto effective_attributes=[&]{
        auto result=attribute_state.ranks();const auto bonus=attribute_bonuses();
        for(unsigned id=0;id<result.size();++id)result[id]=static_cast<std::uint8_t>(std::clamp(int(result[id])+int(bonus[id]),0,20));return result;
    };
    std::optional<fw::gw::attributes::Bonuses> published_equipment_bonuses;
    bool attribute_wire_ready=false;
    fw::gw::inventory::upgrades::Pending pending_upgrade;
    fw::gw::salvage::Pending pending_salvage;
    fw::gw::inventory::upgrades::PreviewReservation salvage_preview;
    bool salvage_committed=false;
    const auto& identification_catalog=gwserver::NetworkManager::current().item_catalogs().identification();
    const auto& salvage_catalog=gwserver::NetworkManager::current().item_catalogs().salvage();
    const auto& consumable_catalog=gwserver::NetworkManager::current().item_catalogs().consumables();
    const auto refresh_energy_capacity=[&]{
        if(joined&&grant->map_type!=11&&skill_profile.primary>=1&&skill_profile.primary<=10){
            const auto suppressed=skill_actor_scope&&world.held_bundle(*skill_actor_scope)
                ?std::optional{fw::gw::bundles::without_weapon_benefits(inventory_profile)}:std::nullopt;
            const auto& equipment=suppressed?*suppressed:inventory_profile;
            if(skill_actor_scope){fw::gw::combat::weapon::Policy policy;policy.primary=skill_profile.primary;policy.character=grant->name;policy.effective=effective_attributes();policy.caster_weapon=true;
                const fw::gw::combat::weapon::CriticalProfile critical{policy.primary,policy.effective[35],grant->level};
                // Share current equipment/condition-adjusted Beast Mastery with owned pets.
                world.pet_owner_combat(*skill_actor_scope,grant->level,policy.effective[22],fw::now_ms());
                world.ordinary_weapon(*skill_actor_scope,fw::gw::combat::weapon::owned_weapon(equipment,inventory_revision,policy),grant->level,fw::now_ms(),
                    critical.valid()?std::optional{critical}:std::nullopt);
                world.observe_condition_equipment(*skill_actor_scope,fw::gw::equipment::condition_equipment(equipment,inventory_revision),fw::now_ms());
                world.shield_equipment(*skill_actor_scope,fw::gw::skills::effects::shield_equipment(inventory_profile,inventory_revision),policy.effective[17],fw::now_ms());}
            world.energy_capacity(grant->player,skill_profile.primary,effective_attributes()[12],fw::now_ms(),
                [&]{auto armor=fw::gw::energy::owned_basic_armor(inventory_profile,skill_profile.primary);armor.capacity+=static_cast<unsigned>(equipment_rune_bonuses().energy)+fw::gw::items::weapon_passives::worn_energy(inventory_profile);return armor;}(),
                fw::gw::items::weapon_passives::worn_energy(inventory_profile));
            if(const auto scope=world.effect_scope(grant->player))world.combat_armor(*scope,
                fw::gw::energy::fully_modeled_earth_armor(inventory_profile,skill_profile.primary));
            if(const auto scope=world.effect_scope(grant->player))world.holy_armor(*scope,
                fw::gw::energy::fully_modeled_neutral_armor(inventory_profile,skill_profile.primary));
            if(const auto scope=world.effect_scope(grant->player))world.holy_armor_locations(*scope,
                fw::gw::energy::neutral_armor_locations(inventory_profile,skill_profile.primary));
            if(const auto scope=world.effect_scope(grant->player))world.physical_absorption(*scope,equipment_rune_bonuses().physical_reduction);
            if(const auto scope=world.effect_scope(grant->player))world.player_holy_equipment(*scope,fw::gw::equipment::reviewed_player_holy_bonus(inventory_profile,skill_profile.primary),fw::now_ms());
                if(const auto scope=world.effect_scope(grant->player))world.physical_armor(*scope,fw::gw::energy::fully_modeled_physical_armor(inventory_profile,skill_profile.primary));
            if(const auto scope=world.effect_scope(grant->player))world.physical_armor_locations(*scope,
                fw::gw::energy::physical_armor_locations(inventory_profile,skill_profile.primary));
            if(const auto scope=world.effect_scope(grant->player))world.lightning_armor(*scope,
                fw::gw::energy::fully_modeled_elemental_armor(inventory_profile,skill_profile.primary));
            if(const auto scope=world.effect_scope(grant->player)){const auto armor=fw::gw::energy::fully_modeled_elemental_armor(inventory_profile,skill_profile.primary);world.elemental_armor(*scope,armor,armor);}
            if(const auto scope=world.effect_scope(grant->player))world.elemental_armor_locations(*scope,
                fw::gw::energy::elemental_armor_locations(inventory_profile,skill_profile.primary));
            if(const auto scope=world.effect_scope(grant->player);scope&&grant->level>=1&&grant->level<=20)
                world.reviewed_health_capacity(*scope,fw::gw::vitals::base(grant->level).max_health,equipment_health_bonus(),fw::now_ms());
            const auto bonuses=attribute_bonuses();
            if(loaded&&attribute_wire_ready&&(!published_equipment_bonuses||*published_equipment_bonuses!=bonuses)){
                for(auto& packet:fw::gw::attributes::packets_2022(grant->player,attribute_state.ranks(),attribute_policy(),bonuses))
                    send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
                published_equipment_bonuses=bonuses;
            }
        }
    };
    std::map<std::uint32_t,unsigned> trade_items;
    unsigned trade_gold=0;
    std::set<std::uint32_t> trade_display_items;
    const auto flush_trade=[&]{
        for(auto& packet:trade_sessions.drain(trade_handle)){
            const auto opcode=read16(packet,0)&0x7fff;
            if(opcode==352&&packet.size()>=6)trade_display_items.insert(read32(packet,2));
            send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
            if(opcode==9||opcode==8||opcode==7){
                for(auto id:trade_display_items)send_world(socket,*channel,fw::gw::items::delete_2022(id),fw::now_ms()+5000);
                trade_display_items.clear();if(opcode==9){trade_items.clear();trade_gold=0;}
            }
        }
        if(trade_sessions.refresh_inventory(trade_handle)){
            const auto previous=inventory_profile;
            reload_inventory();
            for(const auto&[id,item]:previous.items)if(!inventory_profile.items.contains(id)){
                if(!fw::gw::inventory::installed(item))send_world(socket,*channel,fw::gw::items::detach_2022(previous.inventory,id),fw::now_ms()+5000);
                send_world(socket,*channel,fw::gw::items::delete_2022(id),fw::now_ms()+5000);
            }
            for(const auto&[id,item]:inventory_profile.items){
                const auto before=previous.items.find(id);
                if(before==previous.items.end()){
                    for(auto& packet:fw::gw::inventory::item_packets(item))send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
                    send_world(socket,*channel,fw::gw::items::location_2022(inventory_profile.inventory,id,item.bag,item.slot),fw::now_ms()+5000);
                }else if(before->second.definition.quantity!=item.definition.quantity)
                    send_world(socket,*channel,fw::gw::items::quantity_2022(id,item.definition.quantity),fw::now_ms()+5000);
            }
            for(auto& packet:fw::gw::inventory::weapon_packets(inventory_profile))send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
            const fw::gw::inventory::gold::Balances next{inventory_profile.gold,storage_profile.gold};
            for(auto& packet:fw::gw::inventory::gold::updates(client_gold,next,inventory_profile.inventory))send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
            client_gold=next;refresh_energy_capacity();
        }
    };
    const auto publish_trade_offer=[&](bool publish){
        fw::gw::trade::exchange::Offer offered;offered.gold=trade_gold;
        std::vector<std::vector<std::uint8_t>> relay;
        if(publish)relay.push_back(fw::gw::trade::revoke_2022());
        unsigned index=0;
        for(auto[wire,quantity]:trade_items){
            const auto origin=inventory_ids.origin(wire);const auto found=inventory_profile.items.find(wire);
            if(!origin||origin->domain!=fw::gw::inventory::identity::Domain::character||found==inventory_profile.items.end())return false;
            if(!fw::gw::trade::exchange::offerable(inventory_profile,wire)||!quantity||quantity>found->second.definition.quantity)return false;
            offered.items.push_back({origin->logical,quantity});
            if(publish){auto definition=found->second.definition;definition.id=61000+index++;definition.quantity=quantity;
                relay.push_back(fw::gw::items::definition_2022(definition));
                relay.push_back(fw::gw::trade::partner_offer_item_2022(definition.id,quantity));}
        }
        if(trade_gold>inventory_profile.gold)return false;
        if(!trade_sessions.offer(trade_handle,{hex_bytes(grant->character),inventory_revision,std::move(offered)}))return false;
        if(publish){relay.push_back(fw::gw::trade::partner_offer_gold_2022(trade_gold));
            relay.push_back(fw::gw::trade::offered_count_2022(trade_items.size()));return trade_sessions.relay(trade_handle,std::move(relay));}
        return true;
    };
    const auto external_chat=[&](const fw::gw::chat::commands::External& request){
        const auto feedback=[&](std::u16string text){outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,text));};
        if(fw::gw::development::command(request.kind)){
            if(!gm_authorized){feedback(u"Unknown command. Use /commands for supported commands.");return;}
            if(request.kind==u".gm"){
                const bool enabled=development_modes.toggle(social_owner,gm_authorized);
                feedback(enabled?u"Private development commands enabled. /allskills, /s <map>, /who, /die, /kill <name or ID>, /resurrect [target], /setname <target> = <new name>.":u"Private development commands disabled.");return;
            }
            if(!development_modes.enabled(social_owner,gm_authorized)){feedback(u"Enable private development commands with /.gm first.");return;}
            if(request.kind==u"allskills"){
                const auto result=gwserver::NetworkManager::current().server().unlockDevelopmentCharacter({{grant->account},{grant->character}});
                if(result.status!=FW_OK){feedback(fw::gw::character_name_to_utf16(result.message,255));return;}
                reload_skills();reload_attributes();
                refresh_energy_capacity();
                for(auto& response:fw::gw::skills::restore_packets(grant->player,session_skills()))send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                if(skill_actor_scope)world.configure_adrenaline(*skill_actor_scope,session_skills().bar,fw::now_ms());
                for(auto& response:fw::gw::attributes::packets_2022(grant->player,attribute_state.ranks(),attribute_policy(),attribute_bonuses()))send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                feedback(u"Player skills, secondary professions and maximum level-20 attribute points unlocked.");
            }else if(request.kind==u"s"){
                const auto id=fw::gw::chat::commands::number(fw::gw::chat::commands::trim(request.message),65535);
                if(id&&*id)gm_travel=static_cast<std::uint16_t>(*id);else feedback(u"Usage: /s <map ID>.");
            }else if(request.kind==u"setname"){
                const auto rename=fw::gw::development::parse(u"/setname "+request.message);
                if(rename.kind!=fw::gw::development::Kind::setname){feedback(u"Usage: /setname <player name or ID> = <new character name>.");return;}
                const auto id=fw::gw::chat::commands::number(rename.target,65535);
                const auto scope=id?world.effect_scope(*id):world.named_effect_scope(rename.target);
                if(!joined||!loaded||!scope){feedback(u"No player matches that name or ID in this map.");return;}
                const auto identity=world.character_identity(*scope);
                if(!identity){feedback(u"Character identity unavailable.");return;}
                const auto replacement=fw::gw::character_name_to_utf8(rename.text);
                gwserver::CharacterNameResult result{FW_NOT_FOUND,"Character left this map.",{}};
                const bool applied=world.rename_character(*scope,identity->name,replacement,[&](const auto& character,const auto& next){
                    result=gwserver::NetworkManager::current().server().renameCharacter({{gwserver::Identity::from_hex(character.account),gwserver::Identity::from_hex(character.character)},character.name,next});
                    return result.status==FW_OK;
                });
                if(!applied){feedback(fw::gw::character_name_to_utf16(result.message,255));return;}
                feedback(u"Character renamed. Identity, inventory and learned skills preserved.");
            }else if(request.kind==u"resurrect"){
                const auto target=fw::gw::chat::commands::trim(request.message);
                const auto id=fw::gw::chat::commands::number(target,65535);
                const auto scope=target.empty()?world.effect_scope(grant->player):id?world.effect_scope(*id):world.named_effect_scope(target);
                if(!joined||!loaded||!scope||!world.resurrect(*scope,fw::now_ms())){
                    feedback(u"No dead player matches that name or ID in this map. /resurrect defaults to yourself.");return;
                }
                if(scope->actor==grant->player)development_die=true;
                feedback(u"Private development resurrection applied.");
            }else if(request.kind==u"kill"){
                const auto target=fw::gw::chat::commands::trim(request.message);
                const auto id=fw::gw::chat::commands::number(target,65535);
                const auto scope=target.empty()?world.effect_scope(grant->player):id?world.effect_scope(*id):world.named_effect_scope(target);
                if(!joined||!loaded||!scope||!world.kill(*scope,fw::now_ms())){
                    feedback(u"No live player matches that name or ID in this map. /kill without a target kills yourself.");return;
                }
                if(scope->actor==grant->player)development_die=true;
                feedback(u"Private development kill applied.");
            }else if(request.kind==u"die"){
                const auto scope=world.effect_scope(grant->player);
                if(!joined||!loaded||!scope||!world.kill(*scope,fw::now_ms())){
                    feedback(u"Self-death requires a live character in a loaded map.");return;
                }
                development_die=true;
                feedback(u"Private development self-death applied.");
            }else if(request.kind==u"disconnect"){
                development_disconnect=true;feedback(u"Testing the private world reconnect path.");
            }else if(request.kind==u"who"){
                feedback(social_character+u": private map "+fw::gw::chat::commands::decimal(grant->map)+u", learned skills "+
                    fw::gw::chat::commands::decimal(static_cast<unsigned>(skill_profile.learned.size()))+u", attribute points "+
                    fw::gw::chat::commands::decimal(attribute_policy().total_points)+u".");
            }else feedback(u"This development command is not implemented. Available: /allskills, /s <map>, /who, /die, /kill <name or ID>, /resurrect [target], /setname <target> = <new name>.");
            std::osyncstream(std::cout)<<"WORLD gm_command authorized=1 command="<<fw::gw::character_name_to_utf8(request.kind,32)<<std::endl;
        }else if(request.kind==u"whisper"){
            const auto target=social_directory.find(request.target);
            if(!target||!target->second.auth_outbox){feedback(u"That character is not online.");return;}
            const auto ignored=gwserver::NetworkManager::current().server().friendBlocked({gwserver::Identity::from_hex(target->first),gwserver::Identity::from_hex(social_owner)});
            if(ignored.status!=FW_OK||ignored.value!=false){feedback(u"The whisper could not be delivered.");return;}
            target->second.auth_outbox->enqueue({fw::gw::social::whisper(social_character,request.message)});
            feedback(u"Whisper sent to "+request.target+u".");
        }else if(request.kind==u"guildchat"){
            const auto state=guild_state();if(!state){feedback(u"You are not in a private-server guild.");return;}
            const auto packets=fw::gw::chat::global(9,social_character,fw::gw::character_name_to_utf16(state->tag,4),request.message);
            for(const auto& member:state->members)if(const auto recipient=social_directory.get(gwserver::storage_hex(member.account.bytes));recipient&&recipient->world_outbox)recipient->world_outbox->enqueue(packets);
        }else if(request.kind==u"guild"){
            const auto argument=fw::gw::chat::commands::trim(request.message);const auto end=argument.find(u' ');
            const auto action=fw::gw::chat::commands::folded(argument.substr(0,end));
            if(action.empty()){feedback(u"/guild create Name | TAG; invite, join, leave, motd, kick, promote, demote, leader.");return;}
            const auto rest=end==argument.npos?std::u16string{}:std::u16string(fw::gw::chat::commands::trim(argument.substr(end+1)));
            const auto parsed_action=gwserver::guild_action(fw::gw::character_name_to_utf8(action,20));
            if(!parsed_action){feedback(u"Unknown guild command.");return;}
            const auto result=gwserver::NetworkManager::current().server().guildCommand({{{grant->account},{grant->character}},*parsed_action,rest.empty()?std::string{}:fw::gw::character_name_to_utf8(rest,255)});
            feedback(fw::gw::character_name_to_utf16(result.message,255));if(result.status==FW_OK)restore_guild();
        }else if(request.kind==u"friends"){
            const auto list=friend_list(social_owner);feedback(u"Private-server friend and ignore list:");
            for(const auto& [id,value]:list)feedback((value.type==2?u"Ignored: ":u"Friend: ")+value.alias);
        }else if(request.kind==u"friend"||request.kind==u"ignore"){
            const auto name=std::u16string(fw::gw::chat::commands::trim(request.message));if(name.empty()){feedback(u"Provide an exact private-server character name.");return;}
            const auto result=gwserver::NetworkManager::current().server().addFriend({gwserver::Identity::from_hex(social_owner),request.kind==u"ignore"?2u:1u,fw::gw::character_name_to_utf8(name,19),fw::gw::character_name_to_utf8(name,19)});
            feedback(result.status==FW_OK?u"List updated.":fw::gw::character_name_to_utf16(result.message,255));
        }else if(request.kind==u"unfriend"){
            const auto list=friend_list(social_owner);std::string target;
            for(const auto& [id,value]:list)if(fw::gw::chat::commands::same_name(value.alias,fw::gw::chat::commands::trim(request.message))){if(!target.empty()){feedback(u"That alias is ambiguous.");return;}target=id;}
            if(target.empty()){feedback(u"That alias is not in your list.");return;}
            const auto result=gwserver::NetworkManager::current().server().removeFriend({gwserver::Identity::from_hex(social_owner),gwserver::Identity::from_hex(target)});feedback(result.status==FW_OK?u"List entry removed.":u"List update failed.");
        }
    };

    const auto enchanting_quote=[&](unsigned skill,std::uint64_t base,std::uint64_t now)->std::optional<fw::gw::skills::effects::EnchantmentDuration>{
        if(!skill_actor_scope)return {};
        const auto suppressed=world.held_bundle(*skill_actor_scope)
            ?std::optional{fw::gw::bundles::without_weapon_benefits(inventory_profile)}:std::nullopt;
        const auto equipment=fw::gw::items::weapon_passives::equipped_enchanting(suppressed?*suppressed:inventory_profile,inventory_revision);
        const auto effects=world.status_snapshot(*skill_actor_scope,now);
        if(!equipment||!effects)return {};
        return fw::gw::skills::effects::enchanting_duration(skill,base,*skill_actor_scope,*equipment,*effects,now);
    };
    // Filled after quest packet/name binding is available; publication runs
    // after the World adoption lock has been released and storage committed.
    std::function<void()> pet_quest_reconcile;
    fw::gw::skills::casting::Hooks skill_hooks{
        .pet_charm_reserve = [&](auto owner,auto target,auto now){
            return world.reserve_pet_charm(owner,target,now);
        },
        .pet_charm_accept = [&](const auto& reservation,auto sequence,auto starts,auto duration){
            return world.accept_pet_charm(reservation,sequence,starts,duration);
        },
        .pet_charm_cancel_reservation = [&](const auto& reservation){world.cancel_pet_charm(reservation);},
        .pet_charm_cancel = [&](const auto& attempt){world.cancel_pet_charm(attempt);},
        .pet_charm_completed = [&](const auto& attempt,auto now){
            fw::gw::skills::casting::EffectCompletion result;
            if(!skill_actor_scope||attempt.owner!=*skill_actor_scope)return result;
            const auto commit=gwserver::pet_adoption_commit(gwserver::NetworkManager::current().server(),skill_owner());
            auto adopted=world.complete_pet_charm(attempt,true,commit,now);
            result.accepted=adopted.status==fw::gw::pets::CharmCompletion::adopted;
            if(result.accepted){
                result.visuals=std::move(adopted.packets);
                result.publication=[&]{if(pet_quest_reconcile)pet_quest_reconcile();};
                result.visuals_before_publication=true;
            }
            return result;
        },
        .pet_offensive_completed = [&](auto owner,auto target,unsigned skill,auto sequence,auto now){
            if(skill_actor_scope&&owner==*skill_actor_scope)world.pet_offensive_completed(owner,target,skill,sequence,now);
        },
        .pet_owned_scope = [&](auto owner){return world.owned_pet_scope(owner);},
        .pet_comfort_ready = [&](auto owner,auto pet,auto now){return world.pet_comfort_ready(owner,pet,now);},
        .pet_comfort_completed = [&](auto owner,auto pet,unsigned rank,auto sequence,auto now){
            fw::gw::skills::casting::EffectCompletion result;
            if(!skill_actor_scope||owner!=*skill_actor_scope)return result;
            auto healed=world.complete_pet_comfort(owner,pet,rank,sequence,true,now,true);
            result.accepted=healed.accepted;
            if(result.accepted){result.visuals=std::move(healed.packets);result.publication=std::move(healed.publication);}
            return result;
        },
        .target_valid = [&](std::uint32_t target,float range,bool self){return world.valid_heal_target(grant->player,target,range,self);},
        .consume_energy = [&](unsigned cost){return skill_actor_scope&&world.consume_energy(grant->player,cost,*skill_actor_scope);},
        .consume_energy_at = [&](unsigned cost,std::uint64_t now){return skill_actor_scope&&world.consume_energy(grant->player,cost,*skill_actor_scope,now);},
        .consume_selfless_energy = [&](fw::gw::skills::Slot slot,std::optional<fw::gw::skills::effects::Scope> target,unsigned cost,std::uint64_t now){return skill_actor_scope&&world.consume_selfless_energy(*skill_actor_scope,slot,target,cost,now);},
        .resources = [&](std::uint32_t target){return world.resources(target);},
        .heal = [&](std::uint32_t target,unsigned amount,float range,bool self){
            const auto recipient=world.effect_scope(target);
            return skill_actor_scope&&recipient&&world.heal_target_scoped(*skill_actor_scope,*recipient,amount,range,self,fw::now_ms());
        },
        .events = [&](const std::vector<Packet>& packets){world.skill_event_live(grant->player,packets,[]{return fw::now_ms();});},
        .restore_energy = [&](unsigned amount){return skill_actor_scope&&world.restore_energy(grant->player,amount,*skill_actor_scope);},
        .speed_multiplier = [&](double multiplier){return world.movement_multiplier(grant->player,static_cast<float>(multiplier),fw::now_ms());},
        .hostile_aoe = [&](const fw::gw::skills::effects::AoEPlan& plan){return skill_actor_scope&&world.shockwave(*skill_actor_scope,plan,grant->level,fw::now_ms());},
        .slippery_ground = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::SlipperyGroundPlan& plan,std::uint64_t now){
            fw::gw::skills::casting::EffectCompletion result;
            result.accepted=skill_actor_scope&&world.slippery_ground(*skill_actor_scope,target,plan,now,&result.visuals);
            if(!result.accepted)result.visuals.clear();
            return result;
        },
        .diagnostic = [&](unsigned skill,const char* phase){std::osyncstream(std::cout)<<"WORLD skill_effect id="<<skill<<" phase="<<phase<<std::endl;},
        .completed_action = [&](unsigned skill,std::uint64_t now)->fw::gw::skills::casting::ActionCompletion{
            fw::gw::skills::casting::ActionCompletion result;
            if(skill_actor_scope)result.accepted=world.completed_skill_action(*skill_actor_scope,skill,now,nullptr,&result.publication);
            return result;
        },
        .prepare_completion_publication = [&](auto owner,auto now){return world.prepare_completion_publication(owner,now);},
        .succor_spell_completed = [&](fw::gw::skills::effects::Scope actor,unsigned skill,std::uint64_t sequence,std::uint64_t now){
            return skill_actor_scope&&actor==*skill_actor_scope&&world.succor_spell_completed(actor,skill,sequence,now);
        },
        .actor_generation = {},
        .status_mutation = [&](unsigned skill,std::uint32_t effect,unsigned rank,std::uint64_t now,std::optional<std::uint64_t> expires,bool active){
            if(!skill_actor_scope)return false;
            if(active&&skill==1375){
                const auto plan=fw::gw::skills::effects::stoneflesh_aura(rank);
                const auto quote=plan?enchanting_quote(skill,plan->duration,now):std::nullopt;
                return quote&&world.status_mutation(*skill_actor_scope,skill,effect,rank,now,expires,true,{},&*quote);
            }
            if(active&&skill==1261){
                const auto plan=fw::gw::skills::effects::frigid_armor(rank);
                const auto quote=plan?enchanting_quote(skill,plan->duration,now):std::nullopt;
                return quote&&world.status_mutation(*skill_actor_scope,skill,effect,rank,now,expires,true,{},&*quote);
            }
            if(active&&skill==823){
                const auto plan=fw::gw::skills::effects::burning_speed(rank);
                const auto quote=plan?enchanting_quote(skill,plan->duration_ms,now):std::nullopt;
                return quote&&world.status_mutation(*skill_actor_scope,skill,effect,rank,now,expires,true,{},&*quote);
            }
            return world.status_mutation(*skill_actor_scope,skill,effect,rank,now,expires,active);
        },
        .burning_speed = [&](unsigned rank,std::uint32_t effect,std::uint64_t now,std::uint64_t expires){
            if(!skill_actor_scope)return fw::gw::skills::casting::EffectCompletion{};
            const auto plan=fw::gw::skills::effects::burning_speed(rank);
            const auto quote=plan?enchanting_quote(823,plan->duration_ms,now):std::nullopt;
            if(!quote)return fw::gw::skills::casting::EffectCompletion{};
            std::function<void()> publication;
            const bool accepted=world.status_mutation(*skill_actor_scope,823,effect,rank,now,expires,
                true,{},&*quote,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        },
        .burning_speed_completed = [&](unsigned rank,std::uint32_t effect,std::uint64_t event,std::uint64_t now,std::uint64_t expires){
            fw::gw::skills::casting::EffectCompletion result;if(!skill_actor_scope||!event)return result;
            const auto plan=fw::gw::skills::effects::burning_speed(rank);const auto quote=plan?enchanting_quote(823,plan->duration_ms,now):std::nullopt;
            std::uint64_t completed_at=0;
            result.accepted=quote&&world.status_mutation(*skill_actor_scope,823,effect,rank,now,expires,true,{},&*quote,&result.publication,event,&completed_at);
            if(result.accepted)result.effect_snapshot_at=completed_at;return result;
        },
        .has_enchantment = [&](std::uint32_t target,std::uint64_t now){return skill_actor_scope&&world.has_enchantment(*skill_actor_scope,target,now);},
        .enchantment_duration_ready = [&](unsigned skill,std::uint64_t base,std::uint64_t now){return bool(enchanting_quote(skill,base,now));},
        .local_enchantment_duration = [&](unsigned skill,std::uint64_t base,std::uint64_t now)->std::optional<std::uint64_t>{
            const auto duration=enchanting_quote(skill,base,now);
            return duration&&skill_actor_scope?duration->for_enchantment(skill,base,*skill_actor_scope,now):std::nullopt;
        },
        .apply_health_regeneration = [&](std::uint32_t target,const fw::gw::skills::regeneration::Plan& plan,std::uint64_t now,std::uint64_t expires,float range,bool self){
            if(!skill_actor_scope)return false;
            const auto* row=fw::gw::skills::find_skill(plan.skill);
            if(!row||row->type!=6)return world.apply_health_regeneration(*skill_actor_scope,target,plan,now,expires,range,self);
            const auto duration=enchanting_quote(plan.skill,plan.duration,now);
            const auto adjusted=duration?duration->for_enchantment(plan.skill,plan.duration,*skill_actor_scope,now):std::nullopt;
            return adjusted&&world.apply_health_regeneration(*skill_actor_scope,target,plan,now,now+*adjusted,range,self,{},0,nullptr,nullptr,&*duration);
        },
        .begin_action = [&](fw::gw::skills::Slot used,std::uint64_t now){
            if(skill_actor_scope)world.accepted_skill_start(*skill_actor_scope,used,now);
            const auto skill=used.skill;
            if(const auto* row=fw::gw::skills::find_skill(skill);row&&fw::gw::skills::timing::instant_action(*row))return;
            if(skill_actor_scope)world.cancel_ordinary_attack(*skill_actor_scope,now);
            world.cancel_emote(grant->player);
            if(const auto* row=fw::gw::skills::find_skill(skill);row&&(row->activation>0||fw::gw::combat::weapon::attack_skill(skill,0)))
                world.steer(grant->player,0,0,now);
        },
        .party_heal = [&](const fw::gw::skills::effects::PartyHealPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.party_heal(*skill_actor_scope,plan,now);
        },
        .party_heal_completed = [&](const fw::gw::skills::effects::PartyHealPlan& plan,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.party_heal(*skill_actor_scope,plan,now,event);
        },
        .party_heal_published = [&](const fw::gw::skills::effects::PartyHealPlan& plan,std::uint64_t event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            std::function<void()> publication;
            const bool accepted=world.party_heal(*skill_actor_scope,plan,now,event,&publication);
            return {accepted,{},std::nullopt,std::move(publication)};
        },
        .remove_effects = [&](std::uint32_t target,const fw::gw::skills::effects::RemovalPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.remove_effects(*skill_actor_scope,target,plan,now);
        },
        .remove_effects_completed = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::RemovalPlan& plan,std::uint64_t event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            const auto body=fw::gw::skills::particles::target_body_2022(target.actor,plan.skill);
            if(!body)return {};
            std::function<void()> publication;
            const bool accepted=world.remove_effects(*skill_actor_scope,target.actor,plan,now,event,target,&publication);
            return {accepted,{*body},{},std::move(publication)};
        },
        .remove_conditions = [&](std::uint32_t target,const fw::gw::skills::effects::ConditionRemovalPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.remove_conditions(*skill_actor_scope,target,plan,now);
        },
        .remove_conditions_completed = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::ConditionRemovalPlan& plan,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.remove_conditions(*skill_actor_scope,target.actor,plan,now,event,target);
        },
        .status_heal = [&](std::uint32_t target,const fw::gw::skills::effects::StatusHealPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.status_heal(*skill_actor_scope,target,plan,now);
        },
        .status_heal_completed = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::StatusHealPlan& plan,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.status_heal(*skill_actor_scope,target.actor,plan,now,event,target);
        },
        .status_heal_target_valid = [&](fw::gw::skills::effects::Scope target,unsigned skill,std::uint64_t now){
            return skill_actor_scope&&world.valid_status_heal_target(*skill_actor_scope,target,skill,now);
        },
        .crystal_wave = [&](const fw::gw::skills::effects::CrystalWavePlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.crystal_wave(*skill_actor_scope,plan,now);
        },
        .projectile_target_valid = [&](std::uint32_t target,float range){
            return skill_actor_scope&&world.projectile_target_valid(*skill_actor_scope,target,range,fw::now_ms());
        },
        .rodgort_target_valid = [&](fw::gw::skills::effects::Scope target,std::uint64_t now){
            return skill_actor_scope&&world.effect_scope(target.actor)==target&&
                world.projectile_target_valid(*skill_actor_scope,target.actor,1248,now,false,true);
        },
        .launch_projectile = [&](std::uint32_t target,const fw::gw::skills::effects::EarthProjectilePlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.launch_projectile(*skill_actor_scope,target,plan,grant->level,now);
        },
        .shout_target_valid = [&](std::uint32_t target,float range){
            return skill_actor_scope&&world.shout_target_valid(*skill_actor_scope,target,range,fw::now_ms());
        },
        .resurrection_target_valid = [&](std::uint32_t target,float range){
            return skill_actor_scope&&world.resurrection_target_valid(*skill_actor_scope,target,range);
        },
        .target_scope = [&](std::uint32_t target){return world.effect_scope(target);},
        .resurrect = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ResurrectionPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.resurrect_target(*skill_actor_scope,target,plan,now);
        },
        .flesh_my_flesh = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ResurrectionPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.flesh_my_flesh(*skill_actor_scope,target,plan,now,&publication);
            std::vector<Packet> visuals;
            if(accepted)if(const auto body=fw::gw::skills::particles::targeted_body_2022(target.actor,skill_actor_scope->actor,791))visuals.push_back(*body);
            return {accepted,std::move(visuals),std::nullopt,std::move(publication),true};
        },
        .resurrect_party = [&](const fw::gw::skills::effects::PartyResurrectionPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.resurrect_party(*skill_actor_scope,plan,now);
        },

        .resurrection_completed = [&](fw::gw::skills::effects::Scope owner,fw::gw::skills::effects::Scope target,unsigned skill,unsigned rank,std::uint64_t event,std::uint64_t now){
            fw::gw::skills::casting::EffectCompletion result;
            if(!skill_actor_scope||owner!=*skill_actor_scope||!event)return result;
            if(const auto party=fw::gw::skills::effects::party_resurrection(skill))
                result.accepted=world.resurrect_party(owner,*party,now,event,&result.publication);
            else if(const auto plan=fw::gw::skills::effects::resurrection(skill,rank))
                result.accepted=skill==791?world.flesh_my_flesh(owner,target,*plan,now,&result.publication,event):
                    world.resurrect_target(owner,target,*plan,now,event,&result.publication);
            if(result.accepted&&skill==791){
                if(const auto body=fw::gw::skills::particles::targeted_body_2022(target.actor,owner.actor,791))result.visuals.push_back(*body);
                result.visuals_before_publication=true;
            }
            return result;
        },
        .lightning = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::LightningPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool reviewed=plan.skill==226||plan.skill==2804||plan.skill==222||plan.skill==224||plan.skill==865||plan.skill==3396||plan.skill==1369||plan.skill==229||plan.skill==232||plan.skill==2059||plan.skill==228||plan.skill==205;
            const bool accepted=skill_actor_scope&&world.lightning(*skill_actor_scope,target,plan,grant->level,now,reviewed?&visuals:nullptr,(plan.skill==226||plan.skill==2804)?&publication:nullptr);
            return {accepted,std::move(visuals),{},std::move(publication),true};
        },
        .blindness = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::BlindnessPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.blindness(*skill_actor_scope,target,plan,now);
        },
        .condition_transfer = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ConditionTransferPlan& plan,std::uint64_t now,std::optional<double> roll){
            return skill_actor_scope&&world.condition_transfer(*skill_actor_scope,target,plan,now,roll);
        },
        .condition_spell = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ConditionSpellPlan& plan,std::uint64_t now){
            if(!skill_actor_scope)return false;
            if(!plan.spread&&(plan.kind==fw::gw::combat::conditions::Kind::weakness||plan.kind==fw::gw::combat::conditions::Kind::disease))
                return world.condition_spell_equipped(*skill_actor_scope,target,plan,now);
            return world.condition_spell(*skill_actor_scope,target,plan,now);
        },
        .condition_spell_completed = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ConditionSpellPlan& plan,std::uint64_t event,std::uint64_t now){
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.complete_condition_spell(*skill_actor_scope,target,plan,now,visuals,publication,
                event?std::optional<std::uint64_t>{event}:std::nullopt);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .virulence = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::VirulencePlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;const bool accepted=skill_actor_scope&&world.virulence(*skill_actor_scope,target,plan,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals)};
        },
        .rotting_flesh = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::RottingFleshPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;const bool accepted=skill_actor_scope&&world.rotting_flesh(*skill_actor_scope,target,plan,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals)};
        },
        .tainted_flesh_target_valid = [&](std::uint32_t target,const fw::gw::skills::effects::TaintedFleshPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.valid_tainted_flesh_target(*skill_actor_scope,target,plan,now);
        },
        .tainted_flesh = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::TaintedFleshPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;const auto duration=enchanting_quote(113,plan.duration,now);
            const bool accepted=skill_actor_scope&&duration&&world.tainted_flesh(*skill_actor_scope,target,plan,now,&visuals,&*duration);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals)};
        },
        .spiteful_spirit = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::SpitefulSpiritPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;const bool accepted=skill_actor_scope&&world.spiteful_spirit(*skill_actor_scope,target,plan,grant->level,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals)};
        },
        .energy_enchantment_target_valid = [&](std::uint32_t target,const fw::gw::skills::effects::EnergyEnchantmentPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.valid_energy_enchantment_target(*skill_actor_scope,target,plan,now);
        },
        .energy_enchantment = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::EnergyEnchantmentPlan& plan,std::uint64_t now){
            std::optional<fw::gw::skills::sacrifice::Loss> paid;std::vector<Packet> visuals;
            const auto duration=enchanting_quote(plan.skill,plan.duration,now);
            const bool accepted=skill_actor_scope&&duration&&world.energy_enchantment(*skill_actor_scope,target,plan,now,&paid,&visuals,&*duration);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),paid};
        },
        .energy_enchantment_completed = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::EnergyEnchantmentPlan& plan,std::uint64_t event,std::uint64_t now){
            std::optional<fw::gw::skills::sacrifice::Loss> paid;std::vector<Packet> visuals;std::function<void()> publication;
            const auto duration=enchanting_quote(plan.skill,plan.duration,now);
            const bool accepted=skill_actor_scope&&event&&duration&&world.energy_enchantment(*skill_actor_scope,target,plan,now,&paid,&visuals,&*duration,event,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),paid,std::move(publication),true};
        },
        .life_steal = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::LifeStealPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.life_steal(*skill_actor_scope,target,plan,now);
        },
        .complete_life_steal = fw::gw::skills::casting::make_life_steal_completion(world,[&]{return skill_actor_scope;}),
        .life_steal_spell_completed = fw::gw::skills::casting::make_admitted_life_steal_completion(world,[&]{return skill_actor_scope;}),
        .area_life_steal = [&](const fw::gw::skills::effects::AreaLifeStealPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.area_life_steal(*skill_actor_scope,plan,now);
        },
        .elemental = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ElementalPlan& plan,std::uint64_t now){
            std::vector<fw::gw::world::Packet> visuals;std::function<void()> publication;
            std::optional<fw::gw::skills::sacrifice::Loss> paid;
            const bool accepted=skill_actor_scope&&world.elemental(*skill_actor_scope,target,plan,grant->level,now,&visuals,&paid,nullptr,{},&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),paid,std::move(publication),true};
        },
        .conditional_elemental = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ElementalPlan& plan,std::uint64_t now)->std::optional<fw::gw::skills::casting::ConditionalElementalCompletion>{
            if(!skill_actor_scope)return {};
            fw::gw::skills::effects::ElementalOutcome outcome;std::vector<Packet> visuals;std::function<void()> publication;
            if(!world.elemental(*skill_actor_scope,target,plan,grant->level,now,&visuals,nullptr,&outcome,{},&publication))return {};
            return fw::gw::skills::casting::ConditionalElementalCompletion{outcome,std::move(visuals),std::move(publication),true};
        },
        .elemental_completed = fw::gw::skills::casting::make_admitted_elemental_completion(world,[&]{return skill_actor_scope;},[&]{return grant->level;}),
        .mind_burn = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::MindBurnPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;std::function<void()> publication;const bool accepted=skill_actor_scope&&world.mind_burn(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication)};
        },
        .energy_drain = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::EnergyDrainPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;
            const bool accepted=skill_actor_scope&&world.energy_drain(*skill_actor_scope,target,plan,grant->level,now,&visuals);
            return {accepted,std::move(visuals)};
        },
        .energy_drain_completed = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::EnergyDrainPlan& plan,std::uint64_t event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.energy_drain(*skill_actor_scope,target,plan,grant->level,now,&visuals,event,&publication);
            return {accepted,std::move(visuals),{},std::move(publication),true};
        },
        .resource_gain = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ResourceGainPlan& plan,std::uint64_t now)->std::optional<fw::gw::skills::effects::ResourceGainOutcome>{
            if(!skill_actor_scope)return {};
            return world.resource_gain(*skill_actor_scope,target,plan,now);
        },
        .effect_snapshot = [&](std::uint64_t now)->std::optional<std::vector<fw::gw::skills::effects::Entry>>{
            return skill_actor_scope?world.status_snapshot(*skill_actor_scope,now):std::nullopt;
        },
        .quickening_zephyr_aura = [&](std::uint64_t now)->std::optional<bool>{
            return skill_actor_scope?world.quickening_zephyr_affects_player(*skill_actor_scope,now):std::nullopt;
        },
        .health_sacrifice = [&](unsigned skill,std::uint64_t now)->std::optional<fw::gw::skills::sacrifice::Loss>{
            return skill_actor_scope?world.health_sacrifice(*skill_actor_scope,skill,now):std::nullopt;
        },
        .shadow_damage = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ShadowPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.shadow_damage(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .necrosis = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::NecrosisPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.necrosis(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return {accepted,std::move(visuals),{},std::move(publication),true};
        },
        .energy_blast = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::EnergyBlastPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.energy_blast(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return {accepted,std::move(visuals),{},std::move(publication),true};
        },
        // Aneurysm2055 has an opt-in paid cast and trusted World effect. Keep
        // production disabled until its radius/reaction and rendered packet
        // consumers are verified against the fixed capture checkpoint.
        .aneurysm = {},
        .ear_bite = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::EarBitePlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.ear_bite(*skill_actor_scope,target,plan,grant->level,now,&publication);
            return {accepted,{},std::nullopt,std::move(publication)};
        },
        .death_charge_target_valid = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::DeathCharge& plan,std::uint64_t now){
            return skill_actor_scope&&world.death_charge_target_valid(*skill_actor_scope,target,plan,now);
        },
        .death_charge = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::DeathCharge& plan,std::uint64_t now){
            std::vector<fw::gw::world::Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.death_charge(*skill_actor_scope,target,plan,now,&visuals,&publication);
            if(accepted&&!visuals.empty())visuals.erase(visuals.begin());
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .ash_blast = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::AshBlastPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.stage_ash_blast(*skill_actor_scope,target,plan,grant->level,now,&publication);
            std::vector<fw::gw::world::Packet> visuals;
            if(accepted){
                const auto body=fw::gw::skills::particles::targeted_body_2022(target.actor,skill_actor_scope->actor,1085);
                if(body)visuals.push_back(*body);
            }
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        // Well casting remains opt-in until the full client field visuals and
        // remaining movement/Profane consumers are reviewed.
        .well_corpse = {},
        //97 paid adapter is native-tested; clipped/reaction/visual consumers remain gated.
        .necrotic_available = {},
        .necrotic_traversal = {},
        .feast_of_corruption = [&](const fw::gw::skills::effects::FeastCastAnchor& anchor,const fw::gw::skills::effects::FeastOfCorruptionPlan& plan,std::uint64_t now){
            return skill_actor_scope&&*skill_actor_scope==anchor.caster&&world.feast_of_corruption(*skill_actor_scope,anchor,plan,now,grant->level);
        },
        .ally_cleansing = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::AllyCleansingPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.ally_cleansing(*skill_actor_scope,target,plan,now);
        },
        .ally_cleansing_outcome = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::AllyCleansingPlan& plan,std::uint64_t now)->std::optional<fw::gw::skills::effects::CleansingOutcome>{
            if(!skill_actor_scope)return {};
            return world.ally_cleansing_outcome(*skill_actor_scope,target,plan,now);
        },
        .ally_cleansing_completed = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::AllyCleansingPlan& plan,std::uint64_t event,std::uint64_t now){
            fw::gw::skills::casting::CleansingCompletion result;
            if(!skill_actor_scope)return result;
            std::function<void()> publication;
            const auto outcome=world.ally_cleansing_outcome(*skill_actor_scope,target,plan,now,event,&publication);
            if(!outcome)return result;
            std::vector<fw::gw::world::Packet> visuals;
            const auto body=plan.skill==311?fw::gw::skills::particles::targeted_body_2022(target.actor,skill_actor_scope->actor,plan.skill):fw::gw::skills::particles::target_body_2022(target.actor,plan.skill);
            if(body)visuals.push_back(*body);
            result.effect={true,std::move(visuals),{},std::move(publication),true};result.outcome=*outcome;return result;
        },
        .weapon_spell_target_valid = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::WeaponSpellPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.valid_weapon_spell_target(*skill_actor_scope,target,plan,now);
        },
        .weapon_spell = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::WeaponSpellPlan& plan,std::uint64_t now){
            std::vector<fw::gw::world::Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.apply_weapon_spell(*skill_actor_scope,target,plan,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .wound_spell = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::WoundSpellPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.wound_spell(*skill_actor_scope,target,plan,grant->level,now);
        },
        .wound_spell_completed = fw::gw::skills::casting::make_wound_spell_completion(world,[&]{return skill_actor_scope;},grant->level),
        .glimmering_mark_completed = fw::gw::skills::casting::make_glimmering_mark_completion(world,[&]{return skill_actor_scope;},grant->level),
        .snow_storm_completed = fw::gw::skills::casting::make_snow_storm_completion(world,[&]{return skill_actor_scope;},grant->level),
        .degeneration_hex = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::DegenerationHexPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.degeneration_hex(*skill_actor_scope,target,plan,now,grant->level,&visuals,plan.skill==883?&publication:nullptr);
            return {accepted,std::move(visuals),std::nullopt,std::move(publication)};
        },
        .degeneration_hex_completed = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::DegenerationHexPlan& plan,std::uint64_t event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope||plan.skill!=883)return {};
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=world.degeneration_hex(*skill_actor_scope,target,plan,now,grant->level,&visuals,&publication,event);
            return {accepted,std::move(visuals),std::nullopt,std::move(publication)};
        },
        // Trusted Runtime tests exercise125/140 with World completion;
        // production admission awaits the explicit lifecycle/retail gates.
        .energy_degeneration_hex = {},
        .action_damage_hex = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ActionDamageHexPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;
            const bool accepted=skill_actor_scope&&world.stage_action_damage_hex(*skill_actor_scope,target,plan,grant->level,now,&visuals);
            return {accepted,std::move(visuals)};
        },
        .icy_prism = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::IcyPrismPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;
            const bool accepted=skill_actor_scope&&world.stage_icy_prism(*skill_actor_scope,target,plan,grant->level,now,&visuals);
            return {accepted,std::move(visuals)};
        },
        .soul_leech = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::SoulLeechPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;
            const bool accepted=skill_actor_scope&&world.stage_soul_leech(*skill_actor_scope,target,plan,now,&visuals);
            return {accepted,std::move(visuals)};
        },
        .mark_subversion = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::MarkSubversionPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;
            const bool accepted=skill_actor_scope&&world.stage_mark_subversion(*skill_actor_scope,target,plan,now,&visuals);
            return {accepted,std::move(visuals)};
        },
        .chilblains = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ChilblainsPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const bool accepted=skill_actor_scope&&world.complete_chilblains(*skill_actor_scope,target,plan,grant->level,now);
            return {accepted,{},{},{},false,true};
        },
        .ally_enchantment_target_valid = [&](fw::gw::skills::effects::Scope target,unsigned divine_rank,std::uint64_t now){return skill_actor_scope&&world.valid_ally_enchantment_target(*skill_actor_scope,target,divine_rank,now);},
        .watchful_healing = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::WatchfulHealingPlan& plan,unsigned divine_rank,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const auto duration=enchanting_quote(1392,plan.duration,now);
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&duration&&world.stage_watchful_healing(*skill_actor_scope,target,plan,divine_rank,now,&visuals,&publication,&*duration);
            return {accepted,std::move(visuals),{},std::move(publication),true};
        },
        .restful_breeze = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::regeneration::Plan& plan,unsigned divine_rank,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const auto duration=enchanting_quote(886,plan.duration,now);
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&duration&&world.stage_restful_breeze(*skill_actor_scope,target,plan,divine_rank,now,&visuals,&publication,&*duration);
            return {accepted,std::move(visuals),{},std::move(publication),true};
        },
        .protective_spirit = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ProtectiveSpiritPlan& plan,unsigned divine_rank,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const auto duration=enchanting_quote(245,plan.duration,now);
            const bool accepted=skill_actor_scope&&duration&&world.complete_protective_spirit(*skill_actor_scope,target,plan,divine_rank,now,&*duration);
            return {accepted,{},{},{},false,true};
        },
        .reversal_target_valid = {},
        .reversal_fortune_completed = {},
        .simple_enchantment_completed = [&](fw::gw::skills::effects::Scope target,unsigned skill,unsigned rank,unsigned divine,std::uint64_t event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope||!event)return {};
            const auto source=*skill_actor_scope;
            const auto apply=[&](const auto& plan,auto commit){
                if(!plan)return false;
                const auto duration=enchanting_quote(skill,plan->duration,now);
                return duration&&commit(*plan,&*duration);
            };
            bool accepted=false;std::uint64_t completed_at=0;std::function<void()> publication;
            using namespace fw::gw::skills::effects;
            switch(skill){
                case 113:accepted=apply(tainted_flesh(rank),[&](const auto& p,const auto* d){return world.complete_tainted_flesh(source,target,p,now,d,event);});break;
                case 2138:{const auto plan=fw::gw::skills::regeneration::reviewed(2138,rank);const auto duration=plan?enchanting_quote(skill,plan->duration,now):std::nullopt;
                    accepted=target==source&&duration&&world.hexers_vigor_enchantment(source,rank,now,event,&*duration,&completed_at,&publication);break;}
                case 1261:{const auto plan=frigid_armor(rank);const auto duration=plan?enchanting_quote(skill,plan->duration,now):std::nullopt;
                    accepted=target==source&&duration&&world.frigid_armor_enchantment(source,rank,now,event,&*duration,&completed_at);break;}
                case 1375:{const auto plan=stoneflesh_aura(rank);const auto duration=plan?enchanting_quote(skill,plan->duration,now):std::nullopt;
                    accepted=target==source&&duration&&world.stoneflesh_aura_enchantment(source,rank,now,event,&*duration,&completed_at,&publication);break;}
                case 38:{const auto plan=channeling(rank);const auto duration=plan?enchanting_quote(skill,plan->duration,now):std::nullopt;
                    accepted=target==source&&duration&&world.channeling_enchantment(source,rank,now,event,&*duration,&completed_at);break;}
                case 180:case 3375:{const auto plan=aura_restoration_plan(rank);const auto duration=plan?enchanting_quote(skill,plan->duration,now):std::nullopt;
                    accepted=target==source&&duration&&world.aura_restoration_enchantment(source,skill,rank,now,event,&*duration,&completed_at);break;}
                case 164:case 169:case 184:case 208:case 225:
                    accepted=target==source&&apply(attunement(skill,rank),[&](const auto& p,const auto* d){return world.attunement_enchantment(source,p,now,event,d,&completed_at);});break;
                case 82:accepted=target==source&&apply(mantra_recall(rank),[&](const auto&,const auto* d){return world.mantra_recall(source,rank,now,event,d);});break;
                case 182:case 207:case 221:{
                    accepted=target==source&&apply(conjure(skill,rank),[&](const auto& p,const auto* d){return world.conjure_enchantment(source,p,now,event,d);});break;
                }
                case 245:accepted=apply(protective_spirit(rank),[&](const auto& p,const auto* d){return world.complete_protective_spirit(source,target,p,divine,now,d,event);});break;
                case 254:accepted=apply(vigorous_spirit(rank),[&](const auto& p,const auto* d){return world.complete_vigorous_spirit(source,target,p,divine,now,d,event);});break;
                case 257:accepted=target==source&&apply(aegis(rank),[&](const auto& p,const auto* d){return world.complete_aegis(source,p,divine,now,d,event);});break;
                case 259:accepted=apply(shield_deflection(rank),[&](const auto& p,const auto* d){return world.complete_shield_deflection(source,target,p,divine,now,d,event);});break;
                case 261:accepted=apply(shield_regeneration(rank),[&](const auto& p,const auto* d){return world.complete_shield_regeneration(source,target,p,divine,now,d,event);});break;
                case 262:accepted=apply(shield_judgment(rank),[&](const auto& p,const auto* d){return world.complete_shield_judgment(source,target,p,divine,now,d,event);});break;
                case 274:accepted=apply(healing_seed(rank),[&](const auto& p,const auto* d){return world.complete_healing_seed(source,target,p,divine,now,d,event);});break;
                case 310:accepted=target==source&&apply(divine_spirit(rank),[&](const auto& p,const auto* d){return world.complete_divine_spirit(source,p,divine,now,d,event);});break;
            }
            const bool prefix_published=fw::gw::skills::casting::owned_self_enchantment_base(skill,rank)==0;
            fw::gw::skills::casting::EffectCompletion result{accepted,{},{},std::move(publication),false,prefix_published};
            if(accepted&&completed_at)result.effect_snapshot_at=completed_at;
            return result;
        },
        .aegis = [&](const fw::gw::skills::effects::AegisPlan& plan,unsigned divine_rank,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const auto duration=enchanting_quote(257,plan.duration,now);
            const bool accepted=skill_actor_scope&&duration&&world.complete_aegis(*skill_actor_scope,plan,divine_rank,now,&*duration);
            return {accepted,{},{},{},false,true};
        },
        .vigorous_spirit = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::VigorousSpiritPlan& plan,unsigned divine_rank,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const auto duration=enchanting_quote(254,plan.duration,now);
            const bool accepted=skill_actor_scope&&duration&&world.complete_vigorous_spirit(*skill_actor_scope,target,plan,divine_rank,now,&*duration);
            return {accepted,{},{},{},false,true};
        },
        .healing_seed_target_valid = [&](fw::gw::skills::effects::Scope target,unsigned divine_rank,std::uint64_t now){return skill_actor_scope&&world.valid_healing_seed_target(*skill_actor_scope,target,divine_rank,now);},
        .healing_seed = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::HealingSeedPlan& plan,unsigned divine_rank,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const auto duration=enchanting_quote(274,plan.duration,now);
            const bool accepted=skill_actor_scope&&duration&&world.complete_healing_seed(*skill_actor_scope,target,plan,divine_rank,now,&*duration);
            return {accepted,{},{},{},false,true};
        },
        .shield_deflection = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ShieldDeflectionPlan& plan,unsigned divine_rank,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const auto duration=enchanting_quote(259,plan.duration,now);
            const bool accepted=skill_actor_scope&&duration&&world.complete_shield_deflection(*skill_actor_scope,target,plan,divine_rank,now,&*duration);
            return {accepted,{},{},{},false,true};
        },
        .shield_regeneration = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ShieldRegenerationPlan& plan,unsigned divine_rank,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const auto duration=enchanting_quote(261,plan.duration,now);
            const bool accepted=skill_actor_scope&&duration&&world.complete_shield_regeneration(*skill_actor_scope,target,plan,divine_rank,now,&*duration);
            return {accepted,{},{},{},false,true};
        },
        .shield_judgment = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ShieldJudgmentPlan& plan,unsigned divine_rank,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const auto duration=enchanting_quote(262,plan.duration,now);
            const bool accepted=skill_actor_scope&&duration&&world.complete_shield_judgment(*skill_actor_scope,target,plan,divine_rank,now,&*duration);
            return {accepted,{},{},{},false,true};
        },
        .shield_judgment_ready = [&](std::uint64_t now){return skill_actor_scope&&world.shield_judgment_caster_ready(*skill_actor_scope,now);},
        .weapon_snapshot = [&](){return skill_actor_scope?world.attack_weapon(*skill_actor_scope):std::nullopt;},
        .critical_attack_ready = [&](std::uint32_t target,const fw::gw::combat::weapon::Owned& weapon,std::uint64_t now){return skill_actor_scope&&world.critical_attack_ready(*skill_actor_scope,target,weapon,now);},
        .melee_target = [&](std::uint32_t target,std::uint64_t now){return skill_actor_scope&&world.melee_target(*skill_actor_scope,target,now);},
        .melee_attack = [&](fw::gw::skills::effects::Scope target,const fw::gw::combat::weapon::Owned& weapon,const fw::gw::combat::weapon::AttackPlan& plan,std::uint64_t now){return skill_actor_scope&&world.melee_attack(*skill_actor_scope,target,weapon,plan,now);},
        .completed_bow_attack = [&](auto target,const auto& weapon,const auto& plan,auto event,auto now,auto base,auto existing,auto& visuals)->std::optional<std::uint64_t>{if(!skill_actor_scope)return {};return world.completed_bow_attack(*skill_actor_scope,target,weapon,plan,event,now,base,existing,&visuals);},
        .attack_activity = [&](std::optional<std::uint64_t> until){if(skill_actor_scope)world.attack_activity(*skill_actor_scope,until,fw::now_ms());},
        .condition_damage = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ConditionDamagePlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.condition_damage(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .consume_adrenaline = [&](fw::gw::skills::Slot slot,std::uint64_t now){return skill_actor_scope&&world.consume_adrenaline(*skill_actor_scope,slot,now);},
        .reconcile_adrenaline = [&](std::uint64_t now){if(skill_actor_scope)world.reconcile_adrenaline(*skill_actor_scope,now);},
        .healing_signet_activity = [&](std::uint64_t now,std::optional<std::uint64_t> until){return skill_actor_scope&&world.healing_signet_activity(*skill_actor_scope,now,until);},
        .attack_interrupt_epoch = [&]()->std::optional<std::uint64_t>{return skill_actor_scope?world.attack_interrupt_epoch(*skill_actor_scope):std::nullopt;},
        .lightning_javelin = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::LightningJavelinPlan& plan,std::uint64_t now){
            // Explicit development adjacent-radius policy shared with existing
            // AoE handlers; flight speed/arc remain uncalibrated against retail.
            return skill_actor_scope&&world.lightning_javelin(*skill_actor_scope,target,plan,grant->level,166,now);
        },
        .empathy = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::EmpathyPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.empathy_npc(*skill_actor_scope,target,plan.skill,plan.rank,now);
        },
        .deathly_swarm = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::DeathlySwarmPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.deathly_swarm(*skill_actor_scope,target,plan,grant->level,now);
        },
        .faintheartedness = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::FaintheartednessPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.faintheartedness(*skill_actor_scope,target,plan,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        },
        .defensive_ward_completed = fw::gw::skills::casting::make_defensive_ward_completion(world,[&]{return skill_actor_scope;}),
        .ward_elements = [&](const fw::gw::skills::effects::WardElementsPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;
            const bool accepted=skill_actor_scope&&world.apply_ward_elements(*skill_actor_scope,plan,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals)};
        },
        .ward_melee = [&](const fw::gw::skills::effects::WardMeleePlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;
            const bool accepted=skill_actor_scope&&world.apply_ward_melee(*skill_actor_scope,plan,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals)};
        },
        .ward_foes = [&](const fw::gw::skills::effects::WardFoesPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;
            const bool accepted=skill_actor_scope&&world.apply_ward_foes(*skill_actor_scope,plan,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals)};
        },
        .attack_duration = [&](std::uint64_t base,std::uint64_t now)->std::optional<std::uint64_t>{
            return skill_actor_scope?world.attack_duration(*skill_actor_scope,base,now,true):std::nullopt;
        },
        .attack_skill_target = [&](std::uint32_t target,unsigned skill,std::uint64_t now){
            return skill_actor_scope&&world.melee_target(*skill_actor_scope,target,now,skill);
        },
        .shatter_delusions = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ShatterDelusionsPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.shatter_delusions(*skill_actor_scope,target,plan,grant->level,now);
        },
        .movement_hex = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::MovementHexPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(plan.skill==209||plan.skill==2803){
                const auto freeze=fw::gw::skills::effects::mind_freeze(plan.skill,plan.rank);std::vector<Packet> visuals;std::function<void()> publication;
                const bool accepted=skill_actor_scope&&freeze&&world.mind_freeze(*skill_actor_scope,target,*freeze,grant->level,now,&visuals,&publication);
                return {accepted,std::move(visuals),{},std::move(publication),true};
            }
            if(plan.skill==213){
                const auto projectile=fw::gw::skills::effects::water_projectile(plan.skill,plan.rank);std::vector<Packet> visuals;
                const bool accepted=skill_actor_scope&&projectile&&world.water_projectile(*skill_actor_scope,target,*projectile,grant->level,now,&visuals);
                return {accepted,std::move(visuals)};
            }
            if(plan.skill==1382){
                std::vector<Packet> visuals;std::function<void()> publication;
                const bool accepted=skill_actor_scope&&world.freezing_gust(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
                return {accepted,std::move(visuals),{},std::move(publication),true};
            }
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&(plan.damage?world.water_damage_hex(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication):world.movement_hex(*skill_actor_scope,target,plan,now,plan.skill==210?&visuals:nullptr));
            return {accepted,std::move(visuals),{},std::move(publication),plan.damage!=0};
        },
        .water_projectile = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::WaterProjectilePlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            std::vector<Packet> visuals;const bool accepted=skill_actor_scope&&world.water_projectile(*skill_actor_scope,target,plan,grant->level,now,&visuals);
            return {accepted,std::move(visuals)};
        },
        .protective_enchantment = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::regeneration::Plan& plan,unsigned divine_rank,std::uint64_t now){
            const auto duration=enchanting_quote(plan.skill,plan.duration,now);
            return skill_actor_scope&&duration&&world.protective_enchantment(*skill_actor_scope,target,plan,divine_rank,now,&*duration);
        },
        .signet_judgment = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::SignetJudgmentPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const bool accepted=skill_actor_scope&&world.complete_signet_judgment(*skill_actor_scope,target,plan,grant->level,now);
            return {accepted,{},{},{},false,true};
        },
        .signet_sorrow = [&](fw::gw::skills::effects::Scope target,
                             const fw::gw::skills::effects::SignetSorrowPlan& plan,
                             std::uint64_t now)->fw::gw::skills::casting::SignetSorrowCompletion{
            if(!skill_actor_scope)return {};
            auto result=world.complete_signet_sorrow(*skill_actor_scope,target,plan,grant->level,now);
            return {{result.accepted,std::move(result.visuals),{},
                std::move(result.publication),true},result.recharge_ms};
        },
        .signet_judgment_ready = [&](std::uint64_t now){return skill_actor_scope&&world.shield_judgment_caster_ready(*skill_actor_scope,now);},
        .blessed_signet = [&](fw::gw::skills::effects::Scope source,const fw::gw::skills::effects::BlessedSignetPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const bool accepted=skill_actor_scope&&source==*skill_actor_scope&&world.complete_blessed_signet(source,plan,now);
            return {accepted,{},{},{},false,true};
        },
        .blessed_signet_ready = [&](fw::gw::skills::effects::Scope source,std::uint64_t now){return skill_actor_scope&&source==*skill_actor_scope&&world.blessed_signet_caster_ready(source,now);},
        .mending = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::MendingPlan& plan,unsigned divine,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const bool accepted=skill_actor_scope&&world.complete_mending(*skill_actor_scope,target,plan,divine,now);
            return {accepted,{},{},{},false,true};
        },
        .mending_target_valid = [&](fw::gw::skills::effects::Scope target,unsigned divine,std::uint64_t now){return skill_actor_scope&&world.mending_target_valid(*skill_actor_scope,target,divine,now);},
        .succor = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::SuccorPlan& plan,unsigned divine,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            const auto canonical=fw::gw::skills::effects::succor();
            const bool accepted=skill_actor_scope&&canonical&&plan==*canonical&&
                world.complete_succor(*skill_actor_scope,target,divine,now,nullptr,true);
            return {accepted,{},{},{},false,true};
        },
        .succor_target_valid = [&](fw::gw::skills::effects::Scope target,unsigned divine,std::uint64_t now){return skill_actor_scope&&world.succor_target_valid(*skill_actor_scope,target,divine,now);},
        // Recall's trusted cast tests are opt-in;bundle/recast
        // fidelity remains unresolved,so normal clients must not enter it yet.
        .recall = {},
        .recall_target_valid = {},
        .mimicry = {},
        .mimicry_target_valid = {},
        .recall_feedback_pending = [&](fw::gw::skills::Slot slot,std::uint64_t now)->std::optional<bool>{
            return skill_actor_scope?world.recall_feedback_pending(*skill_actor_scope,slot,now):std::nullopt;
        },
        .holy_damage = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::HolyPlan& plan,std::uint64_t now){
            std::vector<fw::gw::world::Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.holy_damage(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .spear_light = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::SpearLight& plan,std::uint64_t now){
            std::vector<fw::gw::world::Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.spear_light(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .spirit_rift = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::SpiritRift& plan,std::uint64_t now){
            std::vector<fw::gw::world::Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.spirit_rift(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .discord = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::Discord& plan,std::uint64_t now){
            std::vector<fw::gw::world::Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.discord(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .signet_shadows = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::SignetShadowsPlan& plan,std::uint64_t now){
            std::vector<fw::gw::world::Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.signet_shadows(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .backfire = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::BackfirePlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.backfire(*skill_actor_scope,target,plan,grant->level,now);
        },
        .before_spell_effect = [&](unsigned skill,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.before_spell_effect(*skill_actor_scope,skill,event,now);
        },
        .targeted_spell_failure = [&](unsigned skill,std::optional<fw::gw::skills::effects::Scope> target,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope?world.mark_subversion_spell_failure(*skill_actor_scope,skill,target,event,now):fw::gw::skills::casting::SpellFailure::rejected;
        },
        .heal_scoped = [&](fw::gw::skills::effects::Scope target,unsigned amount,float range,bool self,std::uint64_t now){
            return skill_actor_scope&&world.heal_target_scoped(*skill_actor_scope,target,amount,range,self,now);
        },
        .direct_heal_target_valid = [&](fw::gw::skills::effects::Scope target,unsigned skill,std::uint64_t now){
            return skill_actor_scope&&world.valid_direct_heal_target(*skill_actor_scope,target,skill,now);
        },
        .direct_heal_completed = [&](fw::gw::skills::effects::Scope target,unsigned skill,unsigned rank,unsigned divine,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.complete_direct_heal(*skill_actor_scope,target,skill,rank,divine,event,now);
        },
        .zealous_benediction = [&](fw::gw::skills::effects::Scope target,unsigned rank,unsigned divine,std::uint64_t now){
            return skill_actor_scope&&world.zealous_benediction(*skill_actor_scope,target,rank,divine,now);
        },
        .zealous_target_valid = [&](std::uint32_t target,float range,std::uint64_t now){
            return skill_actor_scope&&world.valid_shatter_ally(*skill_actor_scope,target,range,now);
        },
        .gift_health_target_valid = [&](fw::gw::skills::effects::Scope target,std::uint64_t now){
            return skill_actor_scope&&world.valid_gift_health_target(*skill_actor_scope,target,now);
        },
        .gift_health = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::GiftHealthPlan& plan,unsigned divine,
                std::uint64_t event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            std::function<void()> publication;
            const bool accepted=world.resources(target.actor)?
                world.stage_gift_health_player(*skill_actor_scope,target,plan,divine,event,now,&publication):
                world.stage_gift_health_npc(*skill_actor_scope,target,plan,divine,event,now,&publication);
            return {accepted,{}, {},std::move(publication)};
        },
        .gift_health_completed = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::GiftHealthPlan& plan,unsigned divine,
                std::uint64_t use_event,std::uint64_t reaction_event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            std::function<void()> publication;
            const bool accepted=world.resources(target.actor)?
                world.stage_gift_health_player(*skill_actor_scope,target,plan,divine,use_event,now,&publication,reaction_event):
                world.stage_gift_health_npc(*skill_actor_scope,target,plan,divine,use_event,now,&publication,reaction_event);
            return {accepted,{}, {},std::move(publication)};
        },
        .spirit_light_target_valid = [&](fw::gw::skills::effects::Scope target,std::uint64_t now){
            return skill_actor_scope&&world.valid_gift_health_target(*skill_actor_scope,target,now);
        },
        .spirit_light = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::SpiritLightPlan& plan,
                std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            std::function<void()> publication;
            const auto loss=world.resources(target.actor)?
                world.stage_spirit_light_player(*skill_actor_scope,target,plan,now,&publication):
                world.stage_spirit_light_npc(*skill_actor_scope,target,plan,now,&publication);
            if(!loss)return {};
            const auto visual=fw::gw::skills::particles::targeted_body_2022(
                target.actor,skill_actor_scope->actor,915);
            if(!visual)return {};
            return {true,{*visual},*loss,std::move(publication)};
        },
        .spirit_light_completed = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::SpiritLightPlan& plan,
                std::uint64_t event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            std::function<void()> publication;
            const auto loss=world.resources(target.actor)?
                world.stage_spirit_light_player(*skill_actor_scope,target,plan,now,&publication,event):
                world.stage_spirit_light_npc(*skill_actor_scope,target,plan,now,&publication,event);
            if(!loss)return {};
            const auto body=fw::gw::skills::particles::targeted_body_2022(target.actor,skill_actor_scope->actor,915);
            return {true,body?std::vector<fw::gw::world::Packet>{*body}:std::vector<fw::gw::world::Packet>{},*loss,std::move(publication)};
        },
        .mend_body_soul_target_valid = [&](fw::gw::skills::effects::Scope target,std::uint64_t now){
            return skill_actor_scope&&world.valid_mend_body_soul_target(*skill_actor_scope,target,now);
        },
        .mend_body_soul = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::MendBodySoulPlan& plan,
                std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            std::function<void()> publication;
            const bool accepted=world.resources(target.actor)?
                world.stage_mend_body_soul_player(*skill_actor_scope,target,plan,now,&publication):
                world.stage_mend_body_soul_npc(*skill_actor_scope,target,plan,now,&publication);
            return {accepted,{}, {},std::move(publication)};
        },
        .mend_body_soul_completed = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::MendBodySoulPlan& plan,
                std::uint64_t event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            std::function<void()> publication;
            const bool accepted=world.resources(target.actor)?
                world.stage_mend_body_soul_player(*skill_actor_scope,target,plan,now,&publication,event):
                world.stage_mend_body_soul_npc(*skill_actor_scope,target,plan,now,&publication,event);
            return {accepted,{}, {},std::move(publication)};
        },
        .ghostmirror_light_target_valid = [&](fw::gw::skills::effects::Scope target,std::uint64_t now){
            return skill_actor_scope&&world.valid_gift_health_target(*skill_actor_scope,target,now);
        },
        .ghostmirror_light = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::GhostmirrorLightPlan& plan,
                std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            const auto visual=fw::gw::skills::particles::targeted_body_2022(
                target.actor,skill_actor_scope->actor,1741);
            if(!visual)return {};
            std::function<void()> publication;
            const bool accepted=world.resources(target.actor)?
                world.stage_ghostmirror_light_player(*skill_actor_scope,target,plan,now,&publication):
                world.stage_ghostmirror_light_npc(*skill_actor_scope,target,plan,now,&publication);
            if(!accepted)return {};
            return {true,{*visual},{},std::move(publication)};
        },
        .ghostmirror_light_completed = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::GhostmirrorLightPlan& plan,
                std::uint64_t event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            const auto visual=fw::gw::skills::particles::targeted_body_2022(target.actor,skill_actor_scope->actor,1741);
            if(!visual)return {};
            std::function<void()> publication;
            const bool accepted=world.resources(target.actor)?
                world.stage_ghostmirror_light_player(*skill_actor_scope,target,plan,now,&publication,event):
                world.stage_ghostmirror_light_npc(*skill_actor_scope,target,plan,now,&publication,event);
            return {accepted,{*visual},{},std::move(publication)};
        },
        .healing_ribbon_target_valid = [&](fw::gw::skills::effects::Scope target,std::uint64_t now){
            return skill_actor_scope&&world.valid_gift_health_target(*skill_actor_scope,target,now);
        },
        .healing_ribbon = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::HealingRibbonPlan& plan,
                unsigned divine,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            // The fixed DoA receipts show the native target body once on the
            // selected ally, while several health recipients may follow.
            const auto visual=fw::gw::skills::particles::targeted_body_2022(
                target.actor,skill_actor_scope->actor,2062);
            if(!visual)return {};
            std::function<void()> publication;
            const auto recipients=world.stage_healing_ribbon(
                *skill_actor_scope,target,plan,divine,now,&publication);
            if(!recipients||recipients->empty()||recipients->front()!=target)return {};
            return {true,{*visual},{},std::move(publication)};
        },
        .healing_ribbon_completed = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::HealingRibbonPlan& plan,unsigned divine,
                std::uint64_t event,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            const auto visual=fw::gw::skills::particles::targeted_body_2022(target.actor,skill_actor_scope->actor,2062);
            if(!visual)return {};
            std::function<void()> publication;
            const auto recipients=world.stage_healing_ribbon(*skill_actor_scope,target,plan,divine,now,&publication,event);
            return {recipients.has_value(),{*visual},{},std::move(publication)};
        },
        .rejuvenation_target_valid = [&](fw::gw::skills::effects::Scope target,std::uint64_t now){
            return skill_actor_scope&&world.valid_signet_rejuvenation_target(
                *skill_actor_scope,target,now);
        },
        .rejuvenation = [&](fw::gw::skills::effects::Scope target,
                const fw::gw::skills::effects::RejuvenationPlan& plan,
                std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope)return {};
            const auto visual=fw::gw::skills::particles::targeted_body_2022(
                target.actor,skill_actor_scope->actor,887);
            if(!visual)return {};
            std::function<void()> publication;
            const bool accepted=world.resources(target.actor)?
                world.stage_signet_rejuvenation_player(*skill_actor_scope,target,plan,now,&publication):
                world.stage_signet_rejuvenation_npc(*skill_actor_scope,target,plan,now,&publication);
            if(!accepted)return {};
            return {true,{*visual},{},std::move(publication)};
        },
        .soul_barbs = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::SoulBarbsPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.soul_barbs(*skill_actor_scope,target,plan,grant->level,now);
        },
        .damage_field = [&](const fw::gw::skills::effects::DamageFieldPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.damage_field(*skill_actor_scope,plan,grant->level,now);
        },
        .control_epoch = [&]()->std::optional<std::uint64_t>{
            return skill_actor_scope?world.control_epoch(*skill_actor_scope):std::nullopt;
        },
        .control_event_time = [&](std::uint64_t epoch)->std::optional<std::uint64_t>{
            return skill_actor_scope?world.control_event_time(*skill_actor_scope,epoch):std::nullopt;
        },
        .cast_slot_activity = [&](fw::gw::skills::Slot slot,std::uint64_t begins,std::uint64_t ends){
            if(skill_actor_scope)world.cast_activity(*skill_actor_scope,slot,begins,ends);
        },
        .interrupt_skill = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::InterruptPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.interrupt_skill(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .signet_humility = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::HumilityPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.signet_humility(*skill_actor_scope,target,plan,now);
        },
        .hex_breaker = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::HexBreakerPlan& plan,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.hex_breaker(owner,plan,grant->level,now);
        },
        .take_grenths_reaction_death = [&](std::uint64_t event,std::uint64_t now)->std::optional<std::uint64_t>{
            return skill_actor_scope?world.take_grenths_reaction_death(*skill_actor_scope,event,now):std::nullopt;
        },
        .grenths_balance = [&](fw::gw::skills::effects::Scope target,std::optional<std::uint64_t> epoch,std::uint64_t now){
            return skill_actor_scope&&world.grenths_balance(*skill_actor_scope,target,now,bool(epoch),epoch);
        },
        .diversion = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::DiversionPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.diversion(*skill_actor_scope,target,plan,now);
        },
        .inspired_hex = [&](fw::gw::skills::effects::Scope ally,const fw::gw::skills::effects::InspiredHexPlan& plan,fw::gw::skills::Slot source,std::uint64_t now){
            if(!skill_actor_scope||!hex_copy_session||!hex_copy_ids)return false;
            if(hex_copy_session->base()!=base_session_skills().bar)return false;
            const auto result=fw::gw::skills::effects::commit_inspired_hex_copy(world,*hex_copy_session,*hex_copy_ids,*skill_actor_scope,ally,plan,source,now);
            if(!result)return false;
            if(result->changed)send_world(socket,*channel,fw::gw::skills::slot_packet_2022(grant->player,result->changed->index,result->changed->after),now+5000);
            return true;
        },
        .cry_frustration = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::CryFrustrationPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.cry_frustration(*skill_actor_scope,target,plan,grant->level,now,&visuals,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};
        },
        .mantra_recall = [&](fw::gw::skills::effects::Scope owner,unsigned rank,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.mantra_recall(owner,rank,now);
        },
        .unnatural_signet = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::UnnaturalPlan& plan,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.unnatural_signet(*skill_actor_scope,target,plan,grant->level,now,event);
        },
        .i_will_survive = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::SurvivalPlan& plan,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.i_will_survive(owner,plan,now);
        },
        .i_am_unstoppable = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::UnstoppablePlan& plan,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.i_am_unstoppable(owner,plan,now);
        },
        .i_am_the_strongest = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::IAmTheStrongestPlan& plan,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.i_am_the_strongest(owner,plan,now);
        },
        .dodge_this = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::DodgeThisPlan& plan,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.dodge_this(owner,plan,now);
        },
        .for_great_justice = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::JusticePlan& plan,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.for_great_justice(owner,plan,now);
        },
        .to_limit = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::ToLimitPlan& plan,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.complete_to_limit(owner,plan,now);
        },
        .watch_yourself = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::WatchYourselfPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&owner==*skill_actor_scope&&world.stage_watch_yourself_party(owner,plan,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        },
        .shields_up = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::ShieldsUpPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&owner==*skill_actor_scope&&world.stage_shields_up_party(owner,plan,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        },
        .dont_trip = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::DontTripPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&owner==*skill_actor_scope&&world.stage_dont_trip_party(owner,plan,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        },
        .save_yourselves = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::SaveYourselvesPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&owner==*skill_actor_scope&&world.stage_save_yourselves_party(owner,plan,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        },
        .signet_of_corruption = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::SignetOfCorruptionPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.signet_of_corruption(*skill_actor_scope,target,plan,grant->level,now);
        },
        // Provisional capture-candidate appearance; native mechanics enabled,
        // with rendered2022 identity/ownership acceptance still outstanding.
        .quickening_zephyr_spawn = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::QuickeningZephyrPlan& plan,std::uint64_t now)->fw::gw::skills::casting::EffectCompletion{
            if(!skill_actor_scope||owner!=*skill_actor_scope)return {false};
            return fw::gw::world::stage_quickening_zephyr_candidate(world,owner,plan,grant->map,now);
        },
        .charge = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::ChargePlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&owner==*skill_actor_scope&&world.stage_charge_allies(owner,plan,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        },
        .endure_pain = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::EndurePainPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&owner==*skill_actor_scope&&world.stage_endure_pain_health(owner,plan,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        },
        .stamina_signet = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::StaminaSignetPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&owner==*skill_actor_scope&&world.stage_stamina_signet_health(owner,plan,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        },
        .defy_pain = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::DefyPainPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&owner==*skill_actor_scope&&world.stage_defy_pain_health(owner,plan,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{}, {},std::move(publication)};
        },
        .convert_hexes = [&](fw::gw::skills::effects::Scope owner,fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ConvertHexesPlan& plan,unsigned divine,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.complete_convert_hexes(owner,target,plan,divine,now);
        },
        .armor_earth = [&](fw::gw::skills::effects::Scope owner,unsigned rank,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.armor_earth(owner,rank,now);
        },
        .armor_earth_completed = [&](fw::gw::skills::effects::Scope owner,unsigned rank,std::uint64_t event,std::uint64_t now){
            const auto plan=fw::gw::skills::effects::armor_earth(rank);
            const auto duration=plan?enchanting_quote(165,plan->duration,now):std::nullopt;
            return skill_actor_scope&&owner==*skill_actor_scope&&duration&&world.armor_earth(owner,rank,now,event,&*duration);
        },
        .flame_djinn_haste = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::FlameDjinnHastePlan& plan,std::uint64_t event,std::uint64_t now)->std::optional<bool>{
            if(!skill_actor_scope||owner!=*skill_actor_scope)return {};
            const auto duration=enchanting_quote(1381,plan.duration,now);
            return duration?world.flame_djinn_haste(owner,plan,grant->level,now,&*duration,event):std::nullopt;
        },
        .healing_ring = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::HealingRingPlan& plan,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.healing_ring(owner,plan,now);
        },
        .healing_ring_completed = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::HealingRingPlan& plan,std::uint64_t event,std::uint64_t now){
            fw::gw::skills::casting::EffectCompletion result;
            result.accepted=skill_actor_scope&&owner==*skill_actor_scope&&world.healing_ring(owner,plan,now,event,&result.publication);
            return result;
        },
        .antidote_signet = [&](fw::gw::skills::effects::Scope owner,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.antidote_signet(owner,now);
        },
        .extinguish = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::ExtinguishPlan& plan,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.extinguish(owner,plan,now);
        },
        .martyr = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::MartyrPlan& plan,unsigned divine,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.complete_martyr(owner,plan,divine,now);
        },
        .conjure_enchantment = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::ConjurePlan& plan,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.conjure_enchantment(owner,plan,now);
        },
        .panic = [&](fw::gw::skills::effects::Scope target,unsigned rank,std::uint64_t now){
            return skill_actor_scope&&world.apply_panic(*skill_actor_scope,target,rank,now,grant->level);
        },
        .panic_skill_used = [&](const fw::gw::skills::effects::PanicUse& use){
            return skill_actor_scope&&use.owner==*skill_actor_scope&&world.panic_skill_used(use);
        },
        .mind_wrack = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::MindWrackPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.apply_mind_wrack(*skill_actor_scope,target,plan.rank,plan.skill==2734,now,grant->level);
        },
        .wandering_eye = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::WanderingEyePlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.apply_wandering_eye(*skill_actor_scope,target,plan.rank,now,grant->level);
        },
        .mind_wrack_targeted = [&](const fw::gw::skills::effects::MindWrackUse& use){
            return skill_actor_scope&&use.caster==*skill_actor_scope&&world.mind_wrack_targeted(use);
        },
        .glimmering_targeted_use = [&](const fw::gw::skills::effects::GlimmeringTargetedUse& use){
            return skill_actor_scope&&use.caster==*skill_actor_scope&&world.complete_glimmering_targeted_use(use);
        },
        .fevered_dreams = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::FeveredDreamsPlan& plan,std::uint64_t now){
            // Explicit provisional renewal policy: retain longer target Dazed,
            // but copy the generated attempt duration. Retail comparison is
            // still tracked in the immutable-checkpoint verification review.
            return skill_actor_scope&&world.apply_fevered_dreams(*skill_actor_scope,target,plan.rank,plan.skill==3289,now,
                fw::gw::skills::effects::FeveredDazedSpan::attempted_duration,grant->level);
        },
        .prepare_keystone_reset = [&](fw::gw::skills::effects::Scope owner){
            return skill_actor_scope&&owner==*skill_actor_scope?world.disable_reset_revision(owner):std::nullopt;
        },
        .commit_keystone_reset = [&](fw::gw::skills::effects::Scope owner,const fw::gw::skills::effects::KeystonePlan& plan,const std::vector<fw::gw::skills::Slot>& cleared,const fw::gw::skills::effects::DisableRevisions& permit,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope&&world.keystone_signet(owner,plan,cleared,permit,event,now);
        },
        .keystone_skill_used = [&](const fw::gw::skills::effects::KeystoneUse& use){
            return skill_actor_scope&&use.caster==*skill_actor_scope&&world.keystone_skill_used(use,grant->level);
        },
        .prepare_flourish_reset = [&](fw::gw::skills::effects::Scope owner,std::uint64_t now){
            return skill_actor_scope&&owner==*skill_actor_scope?world.prepare_flourish_reset(owner,now):std::nullopt;
        },
        .commit_flourish_reset = [&](const fw::gw::skills::effects::FlourishRecharge& proposal,const fw::gw::skills::effects::FlourishPermit& permit,std::uint64_t now){
            return skill_actor_scope&&proposal.before.owner==*skill_actor_scope?world.commit_flourish_reset(proposal,permit,now):std::nullopt;
        },
        .prepare_skill_use = [&](fw::gw::skills::Slot slot,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.prepare_diversion_skill_use(*skill_actor_scope,slot,event,now);
        },
        .finalize_skill_use = [&](std::uint64_t event,std::uint64_t now,std::uint64_t base,std::uint64_t existing,bool instant)->std::optional<std::uint64_t>{
            return skill_actor_scope?world.finalize_diversion_skill_use(*skill_actor_scope,event,now,base,existing,instant):std::nullopt;
        },
        .abort_skill_use = [&](std::uint64_t event){return skill_actor_scope&&world.abort_diversion_skill_use(*skill_actor_scope,event);},
        .cooldown_completion_source = [&](std::uint64_t event){
            return skill_actor_scope?world.cooldown_completion_stamp(*skill_actor_scope,event):std::nullopt;
        },
        .cooldown_control_source = [&](std::uint64_t after){
            return skill_actor_scope?world.cooldown_control_source(*skill_actor_scope,after):std::nullopt;
        },
        .settle_attack_interrupt = [&](const fw::gw::skills::effects::CooldownOrigin& origin,fw::gw::skills::Slot slot,std::uint64_t base,std::uint64_t now){
            return skill_actor_scope&&origin.stamp.owner==*skill_actor_scope?world.settle_pending_attack_interrupt(origin,slot,base,now):std::nullopt;
        },
        .skill_disabled_slot = [&](fw::gw::skills::Slot slot,std::uint64_t now)->std::optional<std::uint64_t>{
            return skill_actor_scope?world.skill_disabled_slot(*skill_actor_scope,slot,now):std::nullopt;
        },
        .current_skillbar = [&]()->std::optional<fw::gw::skills::Slots>{return session_skills().bar;},
        .admit_session_cast = [&](const fw::gw::skills::Slots& bar,const fw::gw::skills::Policy& policy,fw::gw::skills::casting::Request request,std::uint64_t now)->std::optional<fw::gw::skills::effects::HexCopySession::CastAdmission>{
            if(!skill_actor_scope||!hex_copy_session)return {};
            const auto base=base_session_skills();
            if(hex_copy_session->base()!=base.bar&&!fw::gw::skills::effects::replace_hex_copy_base(world,*hex_copy_session,*skill_actor_scope,base.bar,policy,now))return {};
            if(bar!=hex_copy_session->bar())return {};
            const auto found=std::find(bar.begin(),bar.end(),fw::gw::skills::Slot{request.skill,request.copy});
            if(found==bar.end())return {};
            return hex_copy_session->admit(*skill_actor_scope,policy,unsigned(found-bar.begin()),*found,now);
        },
        .valid_session_cast = [&](const fw::gw::skills::effects::HexCopySession::CastAdmission& admission,std::uint64_t now){
            if(!skill_actor_scope||!hex_copy_session||admission.cast.owner!=*skill_actor_scope)return false;
            const auto base=base_session_skills();
            const auto policy=fw::gw::skills::policy(base,skill_campaigns,staging,grant->map_type==6,trial_area);
            return hex_copy_session->base()==base.bar&&hex_copy_session->valid(admission,policy,now);
        },
        .infuse_health = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::InfuseHealthPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.infuse_health(*skill_actor_scope,target,plan,now);
        },
        .infuse_health_completed = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::InfuseHealthPlan& plan,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.infuse_health(*skill_actor_scope,target,plan,now,event);
        },
        .imbue_health = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ImbueHealthPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.imbue_health(*skill_actor_scope,target,plan,now);
        },
        .imbue_health_completed = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::ImbueHealthPlan& plan,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.imbue_health(*skill_actor_scope,target,plan,now,event);
        },
        .infuse_target_valid = [&](unsigned target,std::uint64_t now){
            return skill_actor_scope&&world.valid_infuse_target(*skill_actor_scope,target,now);
        },
        .weapon_attack_timing = [&](std::uint64_t now)->std::optional<fw::gw::combat::weapon::Timing>{
            return skill_actor_scope?world.weapon_attack_timing(*skill_actor_scope,now,true):std::nullopt;
        },
        .skill_roll = []()->std::optional<double>{
            return double(randombytes_random())/4294967296.0;
        },
        .activation_multiplier = [&](unsigned skill,std::uint64_t now)->std::optional<double>{
            return skill_actor_scope?world.activation_multiplier(*skill_actor_scope,skill,now):std::nullopt;
        },
        .prepare_channeling = [&](unsigned skill,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.prepare_channeling(*skill_actor_scope,skill,event,now);
        },
        .complete_channeling = [&](std::uint64_t event,bool success,std::uint64_t now){
            return skill_actor_scope&&world.complete_channeling(*skill_actor_scope,event,success,now);
        },
        .shatter_ally_valid = [&](std::uint32_t target,float range,std::uint64_t now){
            return skill_actor_scope&&world.valid_shatter_ally(*skill_actor_scope,target,range,now);
        },
        .enchantment_removal = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::EnchantmentRemovalPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.enchantment_removal(*skill_actor_scope,target,plan,grant->level,now);
        },
        .enchantment_removal_result = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::EnchantmentRemovalPlan& plan,std::uint64_t now)->std::optional<fw::gw::skills::effects::EnchantmentRemovalOutcome>{
            fw::gw::skills::effects::EnchantmentRemovalOutcome result;
            if(!skill_actor_scope||!world.enchantment_removal(*skill_actor_scope,target,plan,grant->level,now,&result))return {};
            return result;
        },
        .drain_delusions = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::DrainDelusionsPlan& plan,std::uint64_t now){
            return skill_actor_scope&&world.drain_delusions(*skill_actor_scope,target,plan,now);
        },
        .prepare_melee_attack = [&](fw::gw::skills::effects::Scope target,const fw::gw::combat::weapon::Owned& weapon,const fw::gw::combat::weapon::AttackPlan& plan,std::uint64_t now){
            return skill_actor_scope?world.prepare_melee_attack(*skill_actor_scope,target,weapon,plan,now):fw::gw::combat::weapon::AttackPermit{};
        },
        .prepared_melee_attack = [&](fw::gw::skills::effects::Scope target,const fw::gw::combat::weapon::Owned& weapon,const fw::gw::combat::weapon::AttackPlan& plan,const fw::gw::combat::weapon::AttackPermit& permit,std::uint64_t now){
            return skill_actor_scope&&world.melee_attack(*skill_actor_scope,target,weapon,plan,now,permit);
        },
        .consume_overcast_energy = [&](unsigned skill,unsigned cost,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.activate_overcast_energy(*skill_actor_scope,skill,cost,event,now);
        },
        .consume_selfless_overcast_energy = [&](unsigned skill,std::optional<fw::gw::skills::effects::Scope> target,unsigned cost,std::uint64_t event,std::uint64_t now){
            return skill_actor_scope&&world.activate_selfless_overcast_energy(*skill_actor_scope,skill,target,cost,event,now);
        },
        .gale = [&](fw::gw::skills::effects::Scope target,unsigned rank,std::optional<double> roll,std::uint64_t now){
            return skill_actor_scope&&world.gale(*skill_actor_scope,target,rank,roll,now);
        },
        .complete_damage_field = [&](const fw::gw::skills::effects::DamageFieldPlan& plan,std::uint64_t now)->std::optional<std::vector<Packet>>{
            std::vector<Packet> visuals;
            if(!skill_actor_scope||!world.damage_field(*skill_actor_scope,plan,grant->level,now,&visuals))return {};
            return visuals;
        },
        .complete_fire_storm = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::FireStormPlan& plan,std::uint64_t now)->std::optional<std::vector<Packet>>{
            std::vector<Packet> visuals;
            if(!skill_actor_scope||!world.fire_storm(*skill_actor_scope,target,plan,grant->level,now,&visuals))return {};
            return visuals;
        },
        .complete_ray_of_judgment = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::RayOfJudgmentPlan& plan,std::uint64_t now)->std::optional<std::vector<Packet>>{
            std::vector<Packet> visuals;
            if(!skill_actor_scope||!world.ray_of_judgment(*skill_actor_scope,target,plan,grant->level,now,&visuals))return {};
            return visuals;
        },
        .apply_elemental_glyph = [&](unsigned effect,const fw::gw::skills::effects::ElementalGlyphPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;const bool ok=skill_actor_scope&&world.apply_elemental_glyph(*skill_actor_scope,effect,plan,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{ok,std::move(visuals)};
        },
        .elemental_glyph_snapshot = [&](std::uint64_t now){return skill_actor_scope?world.elemental_glyph_snapshot(*skill_actor_scope,now):std::nullopt;},
        .pay_elemental_glyph = [&](fw::gw::skills::Slot slot,unsigned cost,std::uint64_t event,std::uint64_t now,const fw::gw::skills::effects::ElementalGlyphState& expected){return skill_actor_scope?world.pay_elemental_glyph_use(*skill_actor_scope,slot,cost,event,now,expected):std::nullopt;},
        .pay_selfless_elemental_glyph = [&](fw::gw::skills::Slot slot,std::optional<fw::gw::skills::effects::Scope> target,unsigned cost,std::uint64_t event,std::uint64_t now,const fw::gw::skills::effects::ElementalGlyphState& expected){return skill_actor_scope?world.pay_selfless_elemental_glyph_use(*skill_actor_scope,slot,target,cost,event,now,expected):std::nullopt;},
        .settle_elemental_glyph = [&](std::uint64_t event,fw::gw::skills::effects::GlyphOutcome outcome,std::uint64_t now){return skill_actor_scope?world.settle_elemental_glyph_use(*skill_actor_scope,event,outcome,now):fw::gw::skills::effects::ElementalGlyphSettlement{};},
        .glyph_clock = []{return fw::now_ms();},
        .finish_renewal_spell = [&](std::uint64_t glyph,std::uint64_t use,fw::gw::skills::Slot slot,fw::gw::skills::effects::GlyphOutcome outcome,std::uint64_t now,std::uint64_t base,std::uint64_t existing,std::optional<std::uint64_t> control){
            return skill_actor_scope?world.finish_renewal_spell(*skill_actor_scope,glyph,use,slot,outcome,now,base,existing,control):std::nullopt;
        },
        .stone_daggers = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::StoneDaggersPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;const bool ok=skill_actor_scope&&world.stone_daggers(*skill_actor_scope,target,plan,grant->level,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{ok,std::move(visuals)};
        },
        .dancing_daggers = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::DancingDaggersPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;const bool ok=skill_actor_scope&&world.dancing_daggers(*skill_actor_scope,target,plan,grant->level,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{ok,std::move(visuals)};
        },
        .iron_palm = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::IronPalmPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.iron_palm(*skill_actor_scope,target,plan,grant->level,now,nullptr,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{},std::nullopt,std::move(publication)};
        },
        .finish_him = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::FinishHimPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.stage_finish_him(*skill_actor_scope,target,plan,grant->level,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{},std::nullopt,std::move(publication)};
        },
        .you_move_like_a_dwarf = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::YouMoveLikeADwarfPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.stage_you_move_like_a_dwarf(*skill_actor_scope,target,plan,grant->level,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{},std::nullopt,std::move(publication)};
        },
        .you_are_all_weaklings = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::YouAreAllWeaklingsPlan& plan,std::uint64_t now){
            std::function<void()> publication;
            const bool accepted=skill_actor_scope&&world.stage_you_are_all_weaklings(*skill_actor_scope,target,plan,now,&publication);
            return fw::gw::skills::casting::EffectCompletion{accepted,{},std::nullopt,std::move(publication)};
        },
        .disrupting_dagger = [&](fw::gw::skills::effects::Scope target,const fw::gw::skills::effects::DisruptingDaggerPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;const bool ok=skill_actor_scope&&world.launch_disrupting_dagger(*skill_actor_scope,target,plan,grant->level,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{ok,std::move(visuals)};
        },
        .demonic_flesh = [&](unsigned effect,const fw::gw::skills::effects::DemonicFleshPlan& plan,std::uint64_t now){
            std::vector<Packet> visuals;const bool ok=skill_actor_scope&&world.demonic_flesh(*skill_actor_scope,effect,plan,now,&visuals);
            return fw::gw::skills::casting::EffectCompletion{ok,std::move(visuals)};
        },
        // World application exists, but cost stacking with glyph discounts
        // is not yet established; normal paid casting remains gated.
        .divine_spirit = {},
        .selfless_spirit = [&](const fw::gw::skills::effects::SelflessSpiritPlan& plan,unsigned divine,std::uint64_t now){
            const auto duration=enchanting_quote(plan.skill,plan.duration_ms,now);
            const bool ok=skill_actor_scope&&duration&&world.complete_selfless_spirit(*skill_actor_scope,plan,divine,now,&*duration);
            return fw::gw::skills::casting::EffectCompletion{ok,{},{},{},false,true};
        },
        .prepare_demonic_flesh_use = [&](const fw::gw::skills::effects::DemonicFleshUse& use){
            return skill_actor_scope&&use.caster==*skill_actor_scope?world.prepare_demonic_flesh_use(use,grant->level):std::nullopt;
        },
        .commit_demonic_flesh_use = [&](const fw::gw::skills::effects::DemonicFleshReservation& receipt){
            return skill_actor_scope&&receipt.caster==*skill_actor_scope&&world.commit_demonic_flesh_use(receipt);
        },
        .cancel_demonic_flesh_use = [&](const fw::gw::skills::effects::DemonicFleshReservation& receipt){
            return skill_actor_scope&&receipt.caster==*skill_actor_scope&&world.cancel_demonic_flesh_use(receipt);
        },
        .arm_echo = [&](fw::gw::skills::Slot slot,unsigned effect,std::uint64_t now,std::uint64_t until){
            if(!skill_actor_scope||!hex_copy_session||hex_copy_session->owner()!=*skill_actor_scope||
               hex_copy_session->base()!=base_session_skills().bar||
               std::find(hex_copy_session->bar().begin(),hex_copy_session->bar().end(),slot)==hex_copy_session->bar().end())return false;
            return world.status_mutation(*skill_actor_scope,slot.skill,effect,0,now,until,true,slot);
        },
        .completed_skill_use = [&](const fw::gw::skills::effects::CooldownCompletion& completion){
            if(!skill_actor_scope||!hex_copy_session||!hex_copy_ids||hex_copy_session->base()!=base_session_skills().bar)return false;
            const auto result=fw::gw::skills::effects::observe_echo_completion(world,*hex_copy_session,*hex_copy_ids,*skill_actor_scope,completion);
            if(!result)return false;
            outbox->enqueue(result->packets); // Private owned copy feedback stays in causal Outbox order.
            return true;
        },
        .ebon_vanguard_rank = [&](){return fw::gw::progression::ebon_vanguard_rank(progression_profile);},
        .norn_rank = [&](){return fw::gw::progression::norn_rank(progression_profile);},
        .deldrimor_rank = [&](){return fw::gw::progression::deldrimor_rank(progression_profile);},
        .sunspear_rank = [&](){return fw::gw::progression::sunspear_rank(progression_profile);},
        .allegiance_rank = [&](unsigned skill){return fw::gw::progression::allegiance_rank(progression_profile,skill);},
        .wary_stance_reward_ready = [](){return true;}

    };
    // Closed by default: merely compiling these callbacks must not enroll daggers.
    fw::gw::skills::casting::bind_dagger_session_gateway(skill_hooks,world,skill_actor_scope,
        fw::gw::skills::casting::dagger_session_enrollment);
    const auto publish_cooldown_sources=[&](std::uint64_t now){
        if(!skill_actor_scope)return;
        const auto pending=world.pending_attack_resets(*skill_actor_scope);
        if(!pending){world.invalidate_local_cooldowns(*skill_actor_scope);return;}
        for(const auto& notice:*pending){
            const auto result=skill_runtime.consume_attack_reset(notice,now,skill_hooks);
            using Result=fw::gw::skills::effects::OrderedAttackCooldowns::Result;
            if((result!=Result::accepted&&result!=Result::duplicate)||!world.acknowledge_attack_reset(notice.stamp)){
                world.invalidate_local_cooldowns(*skill_actor_scope);return;
            }
        }
        const auto recall=world.pending_recall_feedback(*skill_actor_scope);
        if(!recall){world.invalidate_local_cooldowns(*skill_actor_scope);return;}
        for(const auto& notice:*recall){
            if(!skill_runtime.consume_recall_feedback(notice,now,skill_hooks,
                [&](const auto& reply){return world.publish_recall_feedback(reply);} )){
                world.invalidate_local_cooldowns(*skill_actor_scope);return;
            }
        }
        const auto snapshot=skill_runtime.cooldown_source_snapshot(*skill_actor_scope,now,skill_hooks);
        if(!snapshot||!world.publish_local_cooldowns(*skill_actor_scope,*snapshot)){world.invalidate_local_cooldowns(*skill_actor_scope);return;}
        if(skill_runtime.needs_cooldown_checkpoint())
            if(const auto permit=world.prepare_cooldown_checkpoint(*skill_actor_scope,*snapshot,now))
                skill_runtime.apply_cooldown_checkpoint(*permit,now,skill_hooks);
    };

    struct CancelCastOnExit {
        fw::gw::skills::casting::Runtime& runtime;const fw::gw::skills::casting::Hooks& hooks;
        ~CancelCastOnExit(){try{runtime.cancel(hooks);}catch(...){/* Disconnect cleanup cannot terminate the host. */}}
    } cancel_cast_on_exit{skill_runtime,skill_hooks};
    // Map UI, /tp and authoritative door crossings all use the same account,
    // map, navigation and transaction checks. Only a server-owned gate may
    // supply a direction-specific arrival; client map requests use map_spawn.
    const auto travel=[&](std::uint32_t destination_id,std::uint32_t region,std::uint32_t district,
            std::uint32_t language,std::optional<fw::gw::map_portals::Arrival> arrival={},std::string_view gate={}){
        const auto* destination=fw::gw::find_map(destination_id);
        const auto reject=[&](const char* reason){std::osyncstream(std::cout)<<"WORLD travel_rejected source_map="<<grant->map
            <<" map="<<destination_id<<" gate="<<(gate.empty()?"none":gate)<<" reason="<<reason<<std::endl;return false;};
        if(!loaded||!joined||grant->map_type==11||!destination||destination->type==255||
           !(region<=4||region==0xfffffffeu)||district>65535||
           !(language==0||language==2||language==3||language==4||language==5||language==9||language==10))
            return reject("unsupported_destination_tuple");
        if(!gate.empty()&&((grant->map==146&&destination_id==147)||(grant->map==147&&destination_id==146))&&
           !world.northlands_installed())return reject("northlands_controller_unavailable");
        if(!fw::gw::map_asset_file(static_cast<std::uint16_t>(destination_id))){
            for(auto& response:fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,
                u"The server has no client data for that map yet. Your current location was kept."))
                send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
            return reject("missing_map_asset");
        }
        // The private development backend grants every campaign. Official
        // account entitlements are deliberately not a gate on private /tp or
        // portal travel; nonexistent logical maps still fail catalog lookup.
        constexpr bool entitled=true;
        const auto spawn=entitled?map_entry_arrival(state_directory,destination_id,arrival):std::nullopt;
        if(!spawn){
            for(auto& response:fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,
                entitled?u"This destination has no configured navigation and arrival point yet.":
                         u"This account does not have access to that destination's campaign."))
                send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
            return reject(entitled?"destination_unavailable":"campaign_unavailable");
        }
        std::pair<std::uint32_t,std::uint32_t> ids;
        try{ids=transfers.issue(grant->account,grant->character,grant->social_lease,destination->type,destination_id,
            region,district,language,grant->name,spawn->point.x,spawn->point.z,grant->appearance,
            grant->level,grant->equipment,grant->creation_campaigns,grant->secondary,
            static_cast<std::uint16_t>(spawn->plane),spawn->facing,false,grant->origin_campaign);
        }catch(const std::exception& error){std::osyncstream(std::cout)<<"WORLD travel_issue_failed reason="<<error.what()<<std::endl;return reject("transfer_capacity");}
        if(!transfers.authorize_travel(*grant,destination->type,destination_id,region,district,language,arrival)){
            transfers.revoke(ids,*grant);return reject("authorization_capacity");}
        if(destination_id!=grant->map&&!transition_character_map(grant->account,grant->character,grant->map,destination_id)){
            transfers.revoke(ids,*grant);return reject("character_map_changed");}
        transfers.mark_travel_committed(*grant,destination_id);
        leave_trade();
        skill_runtime.cancel(skill_hooks);world.steer(grant->player,0,0,fw::now_ms());flush_outbox();
        std::array<std::uint8_t,24> host{};static_assert(sizeof(world_endpoint)<=24);
        std::memcpy(host.data(),&world_endpoint,sizeof(world_endpoint));
        const auto signed_region=region==0xfffffffeu?-2:static_cast<std::int32_t>(region);
        send_world(socket,*channel,fw::gw::world::transfer_2022(host,ids.first,ids.second,destination_id,
            signed_region,!fw::gw::attributes::is_outpost_type(destination->type)),fw::now_ms()+5000);
        std::osyncstream(std::cout)<<"WORLD direct_transfer_sent source_map="<<grant->map<<" map="<<destination_id
            <<" gate="<<(gate.empty()?"none":gate)<<" world="<<ids.first<<" x="<<spawn->point.x<<" y="<<spawn->point.z
            <<" plane="<<spawn->plane<<" facing="<<spawn->facing<<std::endl;
        reconnect_allowed=false;
        return true;
    };
    fw::gw::collectors::Menu collector_menu;
    fw::gw::collectors::native::Binding native_collector_menu;
    fw::gw::merchant::native::Binding native_merchant_menu;
    fw::gw::crafter::Binding native_crafter_menu;
    fw::gw::collectors::native::MenuIds native_collector_ids;
    const auto collector_offers=fw::gw::collectors::presearing_offers();
    const auto living_npc_interaction=[&](fw::gw::skills::effects::Scope npc,std::uint64_t now){
        return skill_actor_scope&&world.can_interact_npc(*skill_actor_scope,npc,now);
    };
    std::uint32_t cape_actor=0;
    std::uint32_t trim_actor=0,trim_choice=0;
    std::uint64_t trim_deadline=0;
    std::string trim_design;
    std::uint32_t quest_actor=0;std::uint64_t quest_deadline=0;
    std::optional<fw::gw::skills::effects::Scope> quest_npc_scope;
    std::uint64_t quest_objective_poll=0;
    std::optional<std::tuple<unsigned,std::size_t,std::size_t,std::size_t,bool>> reported_blood_ai;
    std::set<std::pair<std::uint32_t,std::uint32_t>> reported_wave_failures;
    std::vector<fw::gw::quests::presearing::Choice> quest_choices;
    const auto session_quest_catalog=[&]{
        auto& manager=gwserver::NetworkManager::current();
        const auto name=manager.server().characterName({{grant->account},{grant->character}});
        if(name.status!=FW_OK||!name.value)throw std::runtime_error("quest character name unavailable");
        auto bound=fw::gw::quests::bind_catalog_character(manager.quest_catalog(),fw::gw::character_name_to_utf16(*name.value,19));
        bound=fw::gw::quests::second_profession::bind_markers(bound,grant->map);
        bound=fw::gw::quests::message_from_friend::bind_markers(bound,grant->map);
        bound=fw::gw::quests::path_to_glory::bind_markers(bound,grant->map);
        bound=fw::gw::quests::unsettling_rumors::bind_markers(bound,grant->map);
        bound=fw::gw::quests::opposition::bind_markers(bound,grant->map);
        return fw::gw::quests::ranger_companion::bind_markers(bound,grant->map);
    };
    pet_quest_reconcile=[&]{
        if(!joined||!loaded||grant->map_type==11||grant->origin_campaign!=1||
           !fw::gw::quests::presearing::trial_area(grant->map))return;
        auto& endpoint=gwserver::NetworkManager::current().server();
        const auto quests=endpoint.characterQuests(skill_owner());
        if(quests.status!=FW_OK||!quests.record)return;
        const auto found=quests.record->profile.records.find(76);
        if(found==quests.record->profile.records.end()||found->second.stage!=fw::gw::quests::Stage::active)return;
        const auto result=gwserver::completeRangerCompanion(endpoint,skill_owner(),session_quest_catalog(),found->second.cycle,true,true);
        for(const auto& packet:result.packets)send_world(socket,*channel,packet,fw::now_ms()+5000);
        if(result.status!=FW_OK)std::osyncstream(std::cout)<<"WORLD quest76_reconcile status="<<result.status<<" reason="<<result.message<<std::endl;
    };
    const auto reconcile_second_profession=[&](std::optional<unsigned> expected_cycle=std::nullopt){
        if(!joined||!loaded||grant->map_type==11||grant->origin_campaign!=1||
           !fw::gw::quests::presearing::trial_area(grant->map))return;
        auto& endpoint=gwserver::NetworkManager::current().server();
        const auto quests=endpoint.characterQuests(skill_owner());
        if(quests.status!=FW_OK||!quests.record)return;
        const auto found=quests.record->profile.records.find(79);
        if(found==quests.record->profile.records.end()||found->second.stage!=fw::gw::quests::Stage::active)return;
        const auto result=gwserver::completeSecondProfession(endpoint,skill_owner(),session_quest_catalog(),
            expected_cycle.value_or(found->second.cycle),true,true);
        for(const auto& packet:result.packets)send_world(socket,*channel,packet,fw::now_ms()+5000);
        if(result.status!=FW_OK)std::osyncstream(std::cout)<<"WORLD quest79_reconcile status="<<result.status<<" reason="<<result.message<<std::endl;
    };
    const auto open_npc_dialogue=[&](std::uint32_t actor){
        collector_menu.clear();
        native_collector_menu={};
        native_merchant_menu={};
        native_crafter_menu={};
        quest_actor=0;quest_choices.clear();quest_deadline=0;
        cape_actor=0;
        trim_actor=trim_choice=0;trim_design.clear();trim_deadline=0;
        const bool authorized=joined&&loaded&&grant->map_type!=11&&world.can_access_storage(grant->player,actor,fw::now_ms());
        if(authorized){
            fw::gw::storage::validate_presentation_ids(inventory_profile);
            for(auto& response:fw::gw::storage::open_packets(storage_profile,actor,storage_bags_present))
                send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
            storage_open=true;storage_actor=actor;storage_bags_present=true;
        }
        if(joined&&loaded&&grant->map_type!=11&&world.can_access_emblemer(grant->player,actor,fw::now_ms())){
            for(auto& response:fw::gw::guild::cape::open(actor))send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
            cape_actor=actor;std::osyncstream(std::cout)<<"WORLD cape_editor_open actor="<<actor<<" price="<<fw::gw::guild::cape::price<<std::endl;
        }
        if(joined&&loaded&&grant->map_type!=11&&world.can_access_trim_vendor(grant->player,actor,fw::now_ms())){
            const auto balance=gwserver::NetworkManager::current().server().tournamentPoints({grant->account});
            const auto design=gwserver::NetworkManager::current().server().guildCape({grant->account});
            if(balance.status==FW_OK&&balance.value&&design.status==FW_OK&&design.value){
                trim_actor=actor;trim_design=gwserver::storage_hex(fw::gw::guild::cape::encode(*design.value));trim_deadline=fw::now_ms()+120000;
                for(auto& response:fw::gw::guild::trim::menu(actor,*balance.value))send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                std::osyncstream(std::cout)<<"WORLD trim_menu_open actor="<<actor<<std::endl;
            }
        }
        std::osyncstream(std::cout)<<"WORLD npc_interaction actor="<<actor<<" storage="<<authorized<<std::endl;
        const auto npc_name=world.nearby_npc_name(grant->player,actor,fw::now_ms());
        const auto service_scope=world.interaction_scope(actor);
        const bool service_allowed=joined&&loaded&&grant->map_type!=11&&service_scope&&
            living_npc_interaction(*service_scope,fw::now_ms());
        if(service_allowed&&npc_name&&!trade_sessions.active(trade_handle)){
            if(const auto scope=service_scope){
                const auto now=fw::now_ms();
                const fw::gw::collectors::native::Observation observed{actor,grant->map,skill_profile.primary,
                    scope->instance,scope->generation,inventory_revision,now,true};
                if(auto panel=fw::gw::crafter::open_panel(native_crafter_menu,native_collector_ids,
                        observed,*npc_name,inventory_profile)){
                    for(auto& response:*panel)send_world(socket,*channel,std::move(response),now+5000);
                    std::osyncstream(std::cout)<<"WORLD crafter_native_panel_open actor="<<actor
                        <<" recipes="<<native_crafter_menu.recipes.size()<<" catalog=PRIVATE_ARTHUR_WIKI_CATALOG_V1"<<std::endl;
                    return;
                }
                if(auto panel=fw::gw::merchant::native::open_panel(native_merchant_menu,native_collector_ids,
                        observed,*npc_name,inventory_profile)){
                    for(auto& response:*panel)send_world(socket,*channel,std::move(response),now+5000);
                    std::osyncstream(std::cout)<<"WORLD merchant_native_panel_open actor="<<actor
                        <<" stock="<<native_merchant_menu.stock.size()<<std::endl;
                    return;
                }
                if(auto panel=fw::gw::collectors::native::open_panel(native_collector_menu,native_collector_ids,
                        collector_offers,observed,*npc_name)){
                    for(auto& response:panel->packets)send_world(socket,*channel,std::move(response),now+5000);
                    std::osyncstream(std::cout)<<"WORLD collector_native_panel_open actor="<<actor
                        <<" offers="<<native_collector_menu.offers.size()<<std::endl;
                    return;
                }
            }
            auto menu=fw::gw::collectors::open_menu(collector_menu,collector_offers,inventory_profile,
                actor,grant->map,skill_profile.primary,*npc_name,inventory_revision,fw::now_ms());
            if(!menu.empty()){
                for(auto& response:menu)send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                std::osyncstream(std::cout)<<"WORLD collector_menu_open actor="<<actor<<" collector="<<collector_menu.collector
                    <<" offers="<<collector_menu.offers.size()<<std::endl;
                return;
            }
        }
        if(joined&&loaded&&grant->map_type!=11&&npc_name&&!gwserver::NetworkManager::current().quest_catalog().empty()){
            if(*npc_name=="Armin Saberlin"&&grant->map==148)reconcile_second_profession();
            if(*npc_name=="Master Ranger Nente"&&grant->map==162)pet_quest_reconcile();
            const auto quests=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
            if(quests.status==FW_OK){
                quest_choices=fw::gw::quests::presearing::choices(quests.record?quests.record->profile:fw::gw::quests::Profile{},
                    session_quest_catalog(),skill_profile.primary,skill_profile.secondary,grant->map,*npc_name,grant->origin_campaign==1?grant->level:0);
                std::erase_if(quest_choices,[&](const auto& choice){
                    if(choice.quest==76&&!choice.reward){
                        const auto pets=gwserver::NetworkManager::current().server().characterPets(skill_owner());
                        if(pets.status!=FW_OK||!pets.record||pets.record->profile.pet)return true;
                    }
                    if(choice.quest==73){
                        const auto wave=npcs.wave_encounters.find(choice.quest);const auto guardian=world.effect_scope(actor);
                        if(!skill_actor_scope||wave==npcs.wave_encounters.end()||wave->second.map!=grant->map||!guardian||
                           !world.quest_guardian_available(*guardian,grant->map))return true;
                        if(choice.reward){const auto completed=world.pending_quest_completions(*skill_actor_scope);
                            return std::find(completed.begin(),completed.end(),Instance::QuestCompletion{choice.quest,choice.cycle})==completed.end();}
                    }
                    if(choice.quest==84){
                        if(!choice.reward)return !npcs.escort_encounters.contains(84)||npcs.escort_encounters.at(84).map!=grant->map;
                        return !skill_actor_scope||!world.escort_reward_ready(*skill_actor_scope,choice.cycle);
                    }
                    if(choice.quest==82&&!choice.reward)return !npcs.corpse_encounters.contains(82)||npcs.corpse_encounters.at(82).map!=grant->map;
                    if(choice.quest==83&&!choice.reward)return !npcs.loot_encounters.contains(83)||npcs.loot_encounters.at(83).map!=grant->map;
                    const auto encounter=npcs.kill_encounters.find(choice.quest);
                    return !choice.reward&&!fw::gw::quests::presearing::kill_encounter_offer_available(
                        choice.quest,grant->map,encounter==npcs.kill_encounters.end()?std::nullopt:std::optional<unsigned>{encounter->second.map});
                });
                std::vector<fw::gw::dialogue::Button> buttons;
                if(*npc_name=="Verata the Necromancer"&&quests.record&&skill_actor_scope){
                    const auto progress=quests.record->profile.records.find(82);
                    if(progress!=quests.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active&&
                       world.corpse_objective_state(*skill_actor_scope,82,progress->second.cycle)==fw::gw::quests::CorpseObjective::State::waiting_death)
                        buttons.push_back({u"Come with me.",0x7e030052}); // private dialogue token
                }
                for(const auto& choice:quest_choices){const auto* rule=fw::gw::quests::presearing::find(choice.quest);
                    buttons.push_back({fw::gw::character_name_to_utf16(std::string(choice.reward?"Reward: ":"Quest: ")+std::string(rule->name),124),choice.token});}
                auto reminder=fw::gw::quests::wayward_wizard::reminder(
                    quests.record?quests.record->profile:fw::gw::quests::Profile{},session_quest_catalog(),
                    {grant->origin_campaign==1,fw::gw::quests::presearing::trial_area(grant->map),grant->level},grant->map,*npc_name);
                if(!reminder)reminder=fw::gw::quests::opposition::reply(
                    quests.record?quests.record->profile:fw::gw::quests::Profile{},session_quest_catalog(),
                    {grant->origin_campaign==1,fw::gw::quests::presearing::trial_area(grant->map)},grant->map,*npc_name);
                if(!reminder&&*npc_name=="Armin Saberlin"&&grant->map==148&&quests.record){
                    const auto progress=quests.record->profile.records.find(79);
                    if(progress!=quests.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active)
                        reminder=u"Warrior: Grast in Green Hills. Ranger: Nente in Regent Valley. Monk: Mhenlo in Ashford Abbey. Necromancer: Munne in the Catacombs. Mesmer: Althea near the theatre. Elementalist: Aziure in Wizard's Folly."; // Authored wiki directions.
                }
                if(!buttons.empty()||reminder){
                    quest_actor=actor;quest_npc_scope=world.interaction_scope(actor);quest_deadline=fw::now_ms()+120000;
                    for(auto& response:fw::gw::dialogue::open(actor,reminder.value_or(u"Available quests"),buttons))send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                }
            }
        }
    };
    const auto pickup_ground=[&](fw::gw::skills::effects::Scope target){
        const auto floor=world.ground_item(target.actor);const auto picker=world.effect_scope(grant->player);
        if(!joined||!loaded||grant->map_type==11||!floor||floor->scope!=target||!picker)return false;
        auto proposed=fw::gw::inventory::ground::credit(inventory_profile,*floor,floor->definition.quantity,true,fw::gw::character_name_to_utf16(grant->name,19));
        if(!proposed.accepted())return false;
        auto prepared=inventory_ids.prepare(proposed.profile,storage_profile);
        const bool committed=world.pickup_ground_item(*picker,target,proposed.quantity,160.f,fw::now_ms(),[&](const auto&,unsigned){
            const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},inventory_revision,prepared.durable.character});
            if(saved.status!=FW_OK)return false;
            if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed pickup projection");
            inventory_revision=saved.revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);return true;
        });
        if(committed)for(auto& response:proposed.packets)send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
        std::osyncstream(std::cout)<<"WORLD ground_pickup actor="<<target.actor<<" outcome="<<(committed?"committed":"rejected")<<std::endl;
        return committed;
    };
    const auto arena_player=(std::uint64_t(grant->world)<<32)|grant->player;
    const auto arena_lobby=1+((std::uint64_t(grant->region==0xfffffffeu?5:grant->region)<<24)|
        (std::uint64_t(grant->district)<<8)|grant->language);
    std::uint64_t arena_clock_deadline=0;
    std::uint64_t arena_result_deadline=0;
    fw::gw::PingChallenge ping_challenge;
    struct ArenaCleanup {
        std::uint64_t lobby,player;TransferRegistry& transfers;
        ~ArenaCleanup(){try{std::lock_guard lock(arena_mutex);arena_queue.leave(lobby,player);
            auto it=arena_participants.find(player);if(it!=arena_participants.end()){
                if(it->second.ticket)transfers.revoke(*it->second.ticket,it->second.grant);
                arena_participants.erase(it);
            }}catch(...){}}
    } arena_cleanup{arena_lobby,arena_player,transfers};
    while (!stopping) {
        if(player_sent&&pending.empty())idle_deadline=fw::now_ms()+world_idle_timeout_ms;
        wait_socket(socket, false, idle_deadline, fw::gw::agent::Clock::interval_ms);
        auto count = recv(socket, reinterpret_cast<char*>(incoming.data()), int(incoming.size()), 0);
        if (count < 0 && WSAGetLastError() == WSAEWOULDBLOCK) count=0;
        else if (count <= 0) return;
        if(count>0){
        idle_deadline = fw::now_ms() + world_idle_timeout_ms;
        auto length=static_cast<std::size_t>(count);channel->decrypt(std::span(incoming).first(length));
        if(length>4096-pending.size())throw std::runtime_error("world input buffer limit");
        pending.insert(pending.end(),incoming.begin(),incoming.begin()+count);sodium_memzero(incoming.data(),length);
        for(;;){
            if(pending.size()<2)break;auto opcode=static_cast<std::uint16_t>(read16(pending,0)&0x7fff);
            // Settle a due physical gate boundary before any input handler
            // can commit skill/movement observations beyond that boundary.
            if(joined&&loaded)world.synchronize_northlands(fw::now_ms());
            if(build==37130&&opcode==0x90)opcode=0x92; // observed 2022 counted items array
            if(build==37130&&opcode==0x8f)opcode=0x91; // observed 2022 map request: 8f800000
            std::size_t packet_size=0;
            if(build==37130&&(opcode==16||opcode==18)){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid quest query frame");
                const auto quest=read32(pending,2);
                if(opcode==18){fw::gw::quests::MarkerRequest request;if(fw::gw::quests::parse_marker_request_2022(pending,request)!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid quest marker query");}
                if(joined&&loaded&&grant->map_type!=11){
                    const auto result=gwserver::readCharacterQuest(gwserver::NetworkManager::current().server(),{{grant->account},{grant->character}},session_quest_catalog(),quest,opcode==18);
                    if(result.packet)send_world(socket,*channel,*result.packet,fw::now_ms()+5000);
                    std::osyncstream(std::cout)<<"WORLD quest_query opcode="<<opcode<<" quest="<<quest<<" status="<<result.status<<" reason="<<result.message<<std::endl;
                }
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&opcode==15){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete||frame.size!=6)throw std::runtime_error("invalid quest abandon frame");
                const auto quest=read32(pending,2);
                if(joined&&loaded&&grant->map_type!=11){
                    const auto result=gwserver::abandonCharacterQuest(gwserver::NetworkManager::current().server(),{{grant->account},{grant->character}},quest,grant->player);
                    if(result.status==FW_OK&&skill_actor_scope){
                        world.clear_kill_objective(*skill_actor_scope,quest,result.committed_cycle,true);
                        world.clear_quest_loot(*skill_actor_scope,quest,result.committed_cycle);
                    }
                    if(result.packet)send_world(socket,*channel,*result.packet,fw::now_ms()+5000);
                    if(result.status==FW_OK){const auto previous_bar=session_skills().bar;
                        skill_runtime.cancel(skill_hooks);reload_skills();const auto restored_bar=session_skills().bar;
                        for(unsigned slot=0;slot<restored_bar.size();++slot)if(previous_bar[slot]!=restored_bar[slot])
                            send_world(socket,*channel,fw::gw::skills::slot_packet_2022(grant->player,slot,restored_bar[slot]),fw::now_ms()+5000);}
                    std::osyncstream(std::cout)<<"WORLD quest_abandon quest="<<quest<<" status="<<result.status<<" reason="<<result.message<<std::endl;
                }
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&(opcode<=7||opcode==71)){
                const auto size=(opcode==0||opcode==3||opcode==71)?6u:(opcode==2||opcode==5)?10u:2u;
                if(pending.size()<size)break;
                const auto request=fw::gw::trade::parse_2022(std::span(pending).first(size));
                if(!request)throw std::runtime_error("invalid trade request");
                const auto now=fw::now_ms();

                if(trade_handle&&joined&&loaded){
                    const auto position=world.position(grant->player,now);
                    trade_sessions.update(trade_handle,{social_owner,grant->world,grant->player,grant->map_type==6,position.first,position.second});
                }
                if(request->action==fw::gw::trade::Action::Open){
                    // No numeric trade radius is present in the pinned sender
                    // or local wiki. Require same live world without inventing one.
                    const auto result=trade_sessions.open(trade_handle,request->value,std::numeric_limits<double>::max());
                    if(result==fw::gw::trade::Sessions::Result::Rejected&&!trade_sessions.active(trade_handle))
                        send_world(socket,*channel,fw::gw::trade::cancelled_2022(),now+5000);
                    std::osyncstream(std::cout)<<"WORLD trade_open partner="<<request->value<<" result="<<static_cast<unsigned>(result)<<std::endl;
                }else if(request->action==fw::gw::trade::Action::Cancel){
                    if(!trade_sessions.cancel(trade_handle))send_world(socket,*channel,fw::gw::trade::cancelled_2022(),now+5000);
                }else{
                    bool accepted=false;
                    if(trade_sessions.active(trade_handle)){
                        const auto old_items=trade_items;const auto old_gold=trade_gold;
                        if(request->action==fw::gw::trade::Action::AddItem){
                            const auto found=inventory_profile.items.find(request->value);
                            if(found!=inventory_profile.items.end()&&request->quantity&&request->quantity<=250&&
                               request->quantity<=found->second.definition.quantity&&(trade_items.contains(request->value)||trade_items.size()<7)){
                                trade_items[request->value]=request->quantity;accepted=publish_trade_offer(false);}
                        }else if(request->action==fw::gw::trade::Action::RemoveItem){
                            const auto it=trade_items.find(request->value);
                            if(it!=trade_items.end()&&it->second==request->quantity){trade_items.erase(it);accepted=publish_trade_offer(false);}
                        }else if(request->action==fw::gw::trade::Action::Offer){
                            trade_gold=request->value;accepted=publish_trade_offer(true);
                        }else if(request->action==fw::gw::trade::Action::ChangeOffer||request->action==fw::gw::trade::Action::RevokeOffer){
                            accepted=publish_trade_offer(false);
                            if(accepted)accepted=trade_sessions.relay(trade_handle,{fw::gw::trade::revoke_2022()});
                        }else if(request->action==fw::gw::trade::Action::Accept){
                            accepted=trade_sessions.accept(trade_handle,[&](const fw::gw::trade::Sessions::Commit& commit){
                                const auto& a=commit.left_offer;const auto& b=commit.right_offer;
                                const auto result=gwserver::NetworkManager::current().server().exchangeTrade({
                                    {{gwserver::Identity::from_hex(commit.left.owner),gwserver::Identity::from_hex(a.character)},a.revision,a.offered},
                                    {{gwserver::Identity::from_hex(commit.right.owner),gwserver::Identity::from_hex(b.character)},b.revision,b.offered}});
                                std::osyncstream(std::cout)<<"WORLD trade_atomic_exchange status="<<result.status<<std::endl;return result.status==FW_OK;
                            });
                            if(accepted&&trade_sessions.active(trade_handle))trade_sessions.relay(trade_handle,{fw::gw::trade::partner_accepted_2022()});
                        }
                        if(!accepted){trade_items=old_items;trade_gold=old_gold;}
                    }
                    send_world(socket,*channel,fw::gw::trade::operation_result_2022(accepted?0:1),now+5000);
                    std::osyncstream(std::cout)<<"WORLD trade_operation opcode="<<opcode<<" accepted="<<accepted<<std::endl;
                }
                flush_trade();
                pending.erase(pending.begin(),pending.begin()+size);continue;
            }
            if(build==37130&&(opcode==163||opcode==161)){
                const auto size=opcode==163?3u:2u;if(pending.size()<size)break;
                const auto request=fw::gw::arena::parse_request(std::span(pending).first(size));
                if(!request)throw std::runtime_error("invalid arena request");
                if(!request->enter())grant->auto_arena_queue=false;
                const auto now=fw::now_ms();
                std::optional<fw::gw::arena::Queue::State> state;
                {
                    std::lock_guard lock(arena_mutex);
                    if(request->enter()&&joined&&loaded&&grant->map==2&&grant->map_type==5){
                        state=arena_queue.join(arena_lobby,arena_player,now);
                        if(state)arena_participants.try_emplace(arena_player,ArenaParticipant{*grant,{}});
                    }else if(!request->enter()){
                        arena_queue.leave(arena_lobby,arena_player);
                        auto it=arena_participants.find(arena_player);
                        if(it!=arena_participants.end()){
                            if(it->second.ticket)transfers.revoke(*it->second.ticket,it->second.grant);
                            arena_participants.erase(it);
                        }
                    }
                }
                if(state){
                    send_world(socket,*channel,fw::gw::arena::pending_2022(true),now+5000);
                    send_world(socket,*channel,fw::gw::arena::countdown_plain_2022(u"Entering mission",30000-static_cast<std::uint32_t>(state->deadline-now),30000),now+5000);
                    arena_clock_deadline=state->deadline;
                }else{
                    for(auto& packet:fw::gw::arena::cancel_2022())send_world(socket,*channel,std::move(packet),now+5000);
                    arena_clock_deadline=0;
                }
                std::osyncstream(std::cout)<<"WORLD arena_request enter="<<request->enter()<<" selector="<<unsigned(request->selector)
                    <<" accepted="<<bool(state)<<" player="<<arena_player<<std::endl;
                pending.erase(pending.begin(),pending.begin()+size);continue;
            }
            if(build==37130&&opcode==137){
                fw::gw::creation::Request request;
                const auto status=fw::gw::creation::parse_request(pending,request);
                if(status==fw::gw::world::FrameState::incomplete)break;
                if(status!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid character creation request");
                if(grant->map_type!=11)throw std::runtime_error("character creation outside preview session");
                // Never reinterpret this as the newer client's map request.
                if(created_character){
                    if(request.name!=created_character->name||request.appearance!=created_appearance)
                        throw std::runtime_error("creation session already committed");
                    send_world(socket,*channel,created_character->encode_2022(),fw::now_ms()+5000);
                }else{
                    const auto appearance=read32(request.appearance,0);
                    // Catalog queries enumerate classes; their last profession is
                    // not the final selection. The committed appearance carries it.
                    const auto requested_profession=(appearance>>20)&15;
                    if(!fw::gw::creation::authorized_request(request,preview_campaign,requested_profession,grant->creation_campaigns)){
                        send_world(socket,*channel,fw::gw::creation::failure_2022(19),fw::now_ms()+5000);
                        std::osyncstream(std::cout)<<"WORLD character_creation_rejected reason=preview_mismatch campaign="<<preview_campaign
                            <<" profession="<<preview_profession<<" requested_profession="<<((appearance>>20)&15)<<" persisted=0"<<std::endl;
                        pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
                    }
                    preview_profession=requested_profession;
                    const auto starting_map=fw::gw::creation::starting_map(preview_campaign);
                    if(!fw::gw::map_asset_file(static_cast<std::uint16_t>(starting_map))||
                       !map_entry_arrival(state_directory,starting_map)){
                        send_world(socket,*channel,fw::gw::creation::failure_2022(19),fw::now_ms()+5000);
                        std::osyncstream(std::cout)<<"WORLD character_creation_rejected map="<<starting_map
                            <<" reason=starting_world_unavailable persisted=0"<<std::endl;
                        pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
                    }
                    fw::gw::Summary2022 summary;
                    summary.appearance=appearance;summary.level=1;summary.map=static_cast<std::uint16_t>(fw::gw::creation::starting_map(preview_campaign));
                    // Preserve the creation campaign in the native summary origin field.
                    // Opaque account/guild metadata stays empty for new local characters.
                    summary.low_nibble=static_cast<std::uint8_t>(preview_campaign);
                    summary.equipment=fw::gw::creation::summary_equipment_2022(
                        fw::gw::creation::campaign_armor_preview(preview_campaign,preview_profession,preview_colors));
                    fw::gw::creation::Created response;response.name=request.name;
                    // The first world supplies the native runtime summary.
                    // Keep the persisted profile complete, but do not prime
                    // the client's change detector before its roster insertion.
                    response.summary.clear();
                    std::string name;
                    try{name=fw::gw::character_name_to_utf8(request.name,19);}
                    catch(const std::invalid_argument&){
                        send_world(socket,*channel,fw::gw::creation::failure_2022(19),fw::now_ms()+5000);
                        std::osyncstream(std::cout)<<"WORLD character_creation_rejected reason=name_encoding persisted=0"<<std::endl;
                        pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
                    }
                    auto result=gwserver::NetworkManager::current().server().createCharacter({{grant->account},name,summary.profile()});
                    if(result.status!=FW_OK){
                        if(const auto rejection=fw::gw::creation::persistence_rejection(result.status)){
                            auto error=fw::gw::creation::failure_2022(*rejection);
                            send_world(socket,*channel,std::move(error),fw::now_ms()+5000);
                            std::osyncstream(std::cout)<<"WORLD character_creation_rejected reason=persistence status="
                                <<result.status<<" client_error="<<*rejection<<" persisted=0"<<std::endl;
                            pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
                        }
                        throw std::runtime_error("character creation persistence status="+std::to_string(result.status));
                    }
                    if(!result.created)throw std::runtime_error("missing character creation result");
                    response.character=result.created->character.bytes;
                    created_appearance=request.appearance;created_character=std::move(response);
                    send_world(socket,*channel,created_character->encode_2022(),fw::now_ms()+5000);
                    std::osyncstream(std::cout)<<"WORLD character_created summary_bytes="
                        <<created_character->summary.size()<<" map="<<fw::gw::creation::starting_map(preview_campaign)<<" profession="<<preview_profession
                        <<" gameplay_inventory_complete=0"<<std::endl;
                }
                pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
            }
            if(build==37130&&opcode==36){
                if(pending.size()<7)break;
                const auto request=fw::gw::combat::parse_attack(std::span<const std::uint8_t>(pending).first(7));
                if(!request)throw std::runtime_error("invalid ordinary attack frame");
                refresh_energy_capacity();
                const bool accepted=joined&&loaded&&grant->map_type!=11&&skill_actor_scope&&world.ordinary_attack(*skill_actor_scope,request->target,fw::now_ms());
                std::osyncstream(std::cout)<<"WORLD ordinary_attack target="<<request->target<<" call="<<unsigned(request->call)
                    <<" outcome="<<(accepted?"scheduled":"rejected")<<" reason="<<(accepted?"reviewed_weapon_target_policy":!joined?"not_joined":!loaded?"not_loaded":grant->map_type==11?"creation_instance":"weapon_or_target_unavailable")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+7);continue;
            }
            if(build==37130&&opcode==173){
                // ptplayer -> 7d5810 -> 7d8ba0 -> 7da130, BYTE readiness.
                if(pending.size()<3)break;
                const bool accepted=pending[2]<=1&&joined&&grant->map_type!=11&&
                    world.set_ready(grant->player,pending[2]!=0);
                std::osyncstream(std::cout)<<"WORLD party_ready value="<<unsigned(pending[2])
                    <<" outcome="<<(accepted?"sent":"invalid_value_or_state")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+3);continue;
            }
            if(build==37130&&opcode==191){
                // 37130 avselect -> 8758a0: two selected-agent DWORDs.
                // Selection is local UI state; no acknowledgement is requested.
                if(pending.size()<10)break;
                const auto selected=read32(pending,2),secondary=read32(pending,6);
                const bool accepted=joined&&grant->map_type!=11&&
                    world.select_target(grant->player,selected,secondary);
                std::osyncstream(std::cout)<<"WORLD selection selected="<<selected
                    <<" secondary="<<secondary<<" outcome="<<(accepted?"accepted":"invalid_agent_or_state")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+10);continue;
            }
            if(build==37130&&opcode==49){
                // 37130 FUN_0050ad10 player action -> 798100 -> 876280:
                // opcode 0x31 followed by the target agent DWORD.
                if(pending.size()<6)break;
                const auto target=read32(pending,2);
                const bool accepted=joined&&grant->map_type!=11&&world.follow(grant->player,target,fw::now_ms());
                std::osyncstream(std::cout)<<"WORLD player_follow target="<<target
                    <<" outcome="<<(accepted?"accepted":"invalid_or_unreachable")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+6);continue;
            }
            if(build==37130&&opcode==33){
                if(pending.size()<7)break;
                const auto request=fw::gw::callout::parse(std::span(pending).first(7));
                if(!request)throw std::runtime_error("invalid callout frame");
                if(joined&&grant->map_type!=11&&request->kind==fw::gw::callout::Kind::following){
                    const bool sent=world.call_following(grant->player,request->argument);
                    std::osyncstream(std::cout)<<"WORLD following_call target="<<request->argument
                        <<" outcome="<<(sent?"sent":"not_following_target")<<std::endl;
                    pending.erase(pending.begin(),pending.begin()+7);continue;
                }
                if(joined&&grant->map_type!=11&&request->kind==fw::gw::callout::Kind::target){
                    const bool sent=world.call_target(grant->player,request->argument);
                    std::osyncstream(std::cout)<<"WORLD target_call target="<<request->argument
                        <<" outcome="<<(sent?"sent":"invalid_target")<<std::endl;
                    pending.erase(pending.begin(),pending.begin()+7);continue;
                }
                if(joined&&grant->map_type!=11&&(request->kind==fw::gw::callout::Kind::health||
                    request->kind==fw::gw::callout::Kind::energy||request->kind==fw::gw::callout::Kind::morale||request->kind==fw::gw::callout::Kind::experience)){
                    const bool sent=world.call_status(grant->player,*request);
                    std::osyncstream(std::cout)<<"WORLD status_call kind="<<unsigned(request->kind)
                        <<" outcome="<<(sent?"sent":"invalid_agent_or_state")<<std::endl;
                    pending.erase(pending.begin(),pending.begin()+7);continue;
                }
                std::osyncstream(std::cout)<<"WORLD callout kind="<<unsigned(request->kind)
                    <<" argument_kind="<<fw::gw::callout::argument_kind(request->kind)
                    <<" argument="<<request->argument<<" outcome=unsupported reason=callout_delivery_not_implemented"<<std::endl;
                pending.erase(pending.begin(),pending.begin()+7);continue;
            }
            if(build==37130&&opcode==63){
                fw::gw::professions::Request request;
                const auto state=fw::gw::professions::parse_request(pending,request);
                if(state==fw::gw::world::FrameState::incomplete)break;
                if(state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid secondary profession frame");
                if(joined&&loaded&&grant->map_type!=11){
                    const fw::gw::professions::Policy policy{grant->player,grant->map_type,skill_campaigns,std::clamp(grant->level,1u,20u),false,trial_area};
                    const auto update=fw::gw::professions::propose(request,skill_profile,attribute_profile,policy);
                    bool accepted=update.accepted(),secondary_committed=false;std::string reason=fw::gw::professions::reason_text(update.status);
                    if(accepted&&update.changed){
                        const auto saved=gwserver::NetworkManager::current().server().saveSecondaryProfession({skill_owner(),skill_revision,attribute_revision,update.skill_profile,update.attribute_profile});
                        if(saved.status==FW_OK){
                            secondary_committed=true;
                            skill_revision=saved.skill_revision;attribute_revision=saved.attribute_revision;
                            skill_profile=update.skill_profile;attribute_profile=update.attribute_profile;grant->secondary=skill_profile.secondary;
                            if(skillbar.restore(session_skills().bar,fw::gw::skills::policy(session_skills(),skill_campaigns,staging,false,trial_area))!=fw::gw::skills::Reason::accepted||
                                attribute_state.restore(attribute_profile.ranks,attribute_policy())!=fw::gw::attributes::Reason::accepted)
                                throw std::runtime_error("invalid committed profession state");
                            skill_runtime.cancel(skill_hooks);
                            world.public_professions(grant->player,skill_profile.primary,skill_profile.secondary);
                            refresh_energy_capacity();
                        }else{
                            accepted=false;reason=saved.status==FW_CONFLICT?"storage_revision_conflict":"storage_unavailable";
                            if(saved.status==FW_CONFLICT){reload_skills();reload_attributes();}
                        }
                    }
                    for(auto& response:fw::gw::professions::packets_2022(session_skills(),attribute_profile,policy))
                        send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                    if(secondary_committed)reconcile_second_profession();
                    if(skill_actor_scope)world.configure_adrenaline(*skill_actor_scope,session_skills().bar,fw::now_ms());
                    std::osyncstream(std::cout)<<"WORLD secondary_edit profession="<<request.secondary
                        <<" outcome="<<(accepted?"accepted":"rejected")<<" reason="<<reason<<std::endl;
                }
                pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
            }
            if(build==37130&&(opcode==12||opcode==13||opcode==14)){
                fw::gw::attributes::Request request;
                const auto state=fw::gw::attributes::parse_request(pending,request);
                if(state==fw::gw::world::FrameState::incomplete)break;
                if(state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid attribute frame");
                if(joined&&loaded&&grant->map_type!=11){
                    const auto policy=attribute_policy();const auto update=attribute_state.propose(request,policy,attribute_bonuses());
                    bool accepted=false;std::string reason=fw::gw::attributes::reason_text(update.status);
                    if(update.accepted()){
                        auto proposed=attribute_profile;proposed.ranks=update.proposed;
                        const auto saved=gwserver::NetworkManager::current().server().saveAttributes({skill_owner(),attribute_revision,proposed});
                        if(saved.status==FW_OK){
                            attribute_revision=saved.revision;attribute_state.commit(update,policy);
                            attribute_profile=proposed;accepted=true;
                        }else{
                            reason=saved.status==FW_CONFLICT?"storage_revision_conflict":"storage_unavailable";
                            if(saved.status==FW_CONFLICT)reload_attributes();
                        }
                    }
                    for(auto& response:fw::gw::attributes::packets_2022(grant->player,attribute_state.ranks(),attribute_policy(),attribute_bonuses()))
                        send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                    refresh_energy_capacity();
                    std::osyncstream(std::cout)<<"WORLD attribute_edit opcode="<<opcode<<" outcome="<<(accepted?"accepted":"rejected")
                        <<" reason="<<reason<<" revision="<<attribute_revision<<std::endl;
                }
                pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
            }
            if(build==37130&&opcode==39){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete||frame.size!=6)throw std::runtime_error("invalid upkeep cancellation frame");
                const auto result=fw::gw::skills::effects::dispatch_drop_maintained(world,skill_actor_scope,
                    joined&&loaded&&grant->map_type!=11,std::span(pending).first(frame.size),fw::now_ms());
                std::osyncstream(std::cout)<<"WORLD upkeep_cancel effect="<<read32(pending,2)
                    <<" outcome="<<fw::gw::skills::effects::drop_maintained_reason(result)<<std::endl;
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&(opcode==19||opcode==20)){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid companion command frame");
                const auto command=fw::gw::pets::wire::command(std::span(pending).first(frame.size));
                if(command&&joined&&loaded&&skill_actor_scope){
                    if(auto replies=world.command_pet(*skill_actor_scope,*command,fw::now_ms()))
                        world.skill_event_live(grant->player,*replies,[]{return fw::now_ms();});
                }
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&(opcode==68||opcode==37)){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid skill activation frame");
                // Both native serializers carry skill/copy before target;
                // their descriptor tags differ, not the field order. Cancel
                // optimistic pending state rather than hanging or faking hits.
                const auto request=fw::gw::skills::parse_use_request(std::span(pending).first(frame.size));
                if(!request)throw std::runtime_error("invalid skill activation fields");
                const fw::gw::skills::Slot slot{request->skill,request->copy};
                const auto target=request->target;
                if(joined&&loaded&&grant->map_type!=11&&slot.skill&&slot.skill<=3432){
                    const auto now=fw::now_ms();
                    const bool resources_settled=skill_actor_scope&&world.settle_resources(*skill_actor_scope,now);
                    const bool combat_mode=resources_settled&&fw::gw::skills::casting::map_allows_casts(grant->map_type)&&skill_actor_scope&&
                        (!fw::gw::skills::casting::Runtime::knockdown_restricted(slot.skill)||!world.knocked_down(*skill_actor_scope,now))&&
                        (slot.skill!=361||world.dolyak_available(*skill_actor_scope,now));
                    if(skill_actor_scope&&world.take_morale_recharge(*skill_actor_scope))skill_runtime.morale_boost(skill_hooks);
                    const auto effective_skills=session_skills();
                    const auto reason=skill_runtime.start(now,{slot.skill,slot.copy,target,request->call!=0},effective_skills.bar,
                        fw::gw::skills::policy(effective_skills,skill_campaigns,staging,grant->map_type==6,trial_area),effective_attributes(),combat_mode,skill_hooks);
                    // Instant skills commit before processing another frame in
                    // this receive batch. A following movement frame must not
                    // cancel a zero-time stance that has already activated.
                    if(reason==fw::gw::skills::casting::Reason::accepted)
                        if(const auto* skill=fw::gw::skills::find_skill(slot.skill);skill&&skill->activation==0)
                            skill_runtime.update(now,skill_hooks);
                    publish_cooldown_sources(now);
                    if(reason==fw::gw::skills::casting::Reason::unsupported)
                        outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,u"This skill is not available on this server yet."));
                    if(reason==fw::gw::skills::casting::Reason::weapon)
                        outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,u"Equip the required weapon to use this skill."));
                    std::osyncstream(std::cout)<<"WORLD skill_use opcode="<<request->opcode
                        <<" id="<<slot.skill<<" copy="<<slot.copy<<" target="<<target<<" call="<<unsigned(request->call)
                        <<" outcome="<<(reason==fw::gw::skills::casting::Reason::accepted?"accepted":"rejected")
                        <<" reason="<<fw::gw::skills::casting::reason_text(reason)<<std::endl;
                }else{
                    std::osyncstream(std::cout)<<"WORLD skill_use opcode="<<request->opcode
                        <<" id="<<slot.skill<<" copy="<<slot.copy<<" target="<<target<<" call="<<unsigned(request->call)
                        <<" outcome=ignored reason="<<(!joined?"not_joined":!loaded?"not_loaded":grant->map_type==11?"creation_instance":"invalid_skill_id")<<std::endl;
                }
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&opcode>=90&&opcode<=93){
                fw::gw::skills::Request request;
                const auto state=fw::gw::skills::parse_request(pending,request);
                if(state==fw::gw::world::FrameState::incomplete)break;
                if(state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid skillbar frame");
                auto proposed=skillbar;
                const auto outcome=proposed.apply(request,fw::gw::skills::policy(session_skills(),skill_campaigns,staging&&joined&&loaded,grant->map_type==6,trial_area));
                std::string reason=fw::gw::skills::reason_text(outcome.status);
                bool accepted=false;
                if(outcome.accepted()){
                    const auto saved=gwserver::NetworkManager::current().server().saveSkillbar({skill_owner(),skill_revision,proposed.slots()});
                    if(saved.status==FW_OK){
                        skill_revision=saved.revision;skillbar=std::move(proposed);
                        skill_profile.bar=skillbar.slots();accepted=true;
                    }else{
                        reason=saved.status==FW_CONFLICT?"storage_revision_conflict":"storage_unavailable";
                        if(saved.status==FW_CONFLICT)reload_skills();
                    }
                }
                send_world(socket,*channel,fw::gw::skills::bar_packet_2022(grant->player,session_skills().bar),fw::now_ms()+5000);
                if(skill_actor_scope)world.configure_adrenaline(*skill_actor_scope,session_skills().bar,fw::now_ms());
                std::osyncstream(std::cout)<<"WORLD skillbar_edit opcode="<<opcode<<" outcome="<<(accepted?"accepted":"rejected")
                    <<" reason="<<reason<<" revision="<<skill_revision
                    <<" actor="<<request.actor<<" first_skill="<<request.first.skill<<" first_copy="<<request.first.copy
                    <<" second_skill="<<request.second.skill<<" second_copy="<<request.second.copy
                    <<" slot="<<request.slot<<std::endl;
                pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
            }
            if(build==37130&&(opcode==86||opcode==87)){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid title request frame");
                const auto proposed=fw::gw::progression::select_title(progression_profile,std::span(pending).first(frame.size));
                bool accepted=false;
                if(joined&&loaded&&grant->map_type!=11&&proposed){
                    if(proposed->active==progression_profile.active)accepted=true;
                    else{
                        const auto saved=gwserver::NetworkManager::current().server().saveProgression({{{grant->account},{grant->character}},progression_revision,*proposed});
                        if(saved.status==FW_OK){progression_revision=saved.revision;progression_profile=*proposed;accepted=true;}
                        else if(saved.status==FW_CONFLICT)reload_progression();
                    }
                }
                const auto rank=progression_profile.active==255?0:progression_profile.tracks.at(progression_profile.active).fields[2];
                if(joined&&grant->player<=65535)world.skill_event(grant->player,{fw::gw::progression::active_title(static_cast<std::uint16_t>(grant->player),rank)});
                std::osyncstream(std::cout)<<"WORLD title_selection opcode="<<opcode<<" outcome="<<(accepted?"accepted":"unearned_or_unavailable")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&opcode==61){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid pickup frame");
                const auto request=fw::gw::world::ground::pickup_request(std::span(pending).first(frame.size));
                if(request&&joined&&loaded&&grant->map_type!=11){
                    const auto floor=world.ground_item(request->actor);
                    if(floor){
                        const auto approach=world.approach_ground(grant->player,floor->scope,160.f,fw::now_ms());
                        if(approach==Instance::NpcApproach::ready)pickup_ground(floor->scope);
                        else std::osyncstream(std::cout)<<"WORLD ground_approach actor="<<request->actor<<" outcome="
                            <<(approach==Instance::NpcApproach::moving?"moving":"unavailable")<<std::endl;
                    }
                }
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&opcode==48){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid weapon-set frame");
                const auto selected=fw::gw::weapon_sets::switch_request_2022(Packet(pending.begin(),pending.begin()+frame.size));
                const auto previous_active=inventory_profile.weapons.active;
                bool committed=false;const char* reason="invalid_or_unavailable";
                if(selected&&joined&&loaded&&grant->map_type!=11){
                    if(*selected==inventory_profile.weapons.active){committed=true;reason="unchanged";}
                    else{
                        auto proposed=inventory_profile;proposed.weapons=fw::gw::weapon_sets::switched(proposed.weapons,*selected);
                        auto prepared=inventory_ids.prepare(proposed,storage_profile);
                        if(skill_actor_scope){
                            const auto held=world.held_bundle(*skill_actor_scope);
                            reason="bundle_or_world_preflight";
                            committed=world.switch_weapon_set(*skill_actor_scope,held?std::optional{held->generation}:std::nullopt,fw::now_ms(),[&]{
                                const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},inventory_revision,prepared.durable.character});
                                if(saved.status!=FW_OK){reason=saved.status==FW_CONFLICT?"inventory_revision_conflict":"storage_unavailable";return false;}
                                if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed inventory projection");
                                inventory_revision=saved.revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);reason="saved";return true;
                            });
                        }
                    }
                }
                if(joined&&loaded){
                    const auto response=committed&&inventory_profile.weapons.active!=previous_active
                        ?fw::gw::weapon_sets::active_2022(inventory_profile.inventory,inventory_profile.weapons.active)
                        :fw::gw::weapon_sets::cancel_2022();
                    send_world(socket,*channel,response,fw::now_ms()+5000);
                }
                std::osyncstream(std::cout)<<"WORLD weapon_set_selection requested="<<(selected?std::to_string(*selected):"invalid")
                    <<" active="<<unsigned(inventory_profile.weapons.active)<<" outcome="<<(committed?"committed":"rejected")<<" reason="<<reason<<std::endl;
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&(opcode==72||opcode==75)){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete){native_collector_menu={};native_merchant_menu={};native_crafter_menu={};throw std::runtime_error("invalid native transaction frame");}
                if(native_crafter_menu.observed.actor){
                    const auto now=fw::now_ms();const auto actor=native_crafter_menu.observed.actor;
                    const auto scope=world.interaction_scope(actor);
                    const auto name=world.nearby_npc_name(grant->player,actor,now);
                    const bool allowed=joined&&loaded&&grant->map_type!=11&&!trade_sessions.active(trade_handle)&&
                        scope&&name&&living_npc_interaction(*scope,now)&&fw::gw::crafter::supported(grant->map,*name);
                    const fw::gw::crafter::Observation observed{actor,grant->map,skill_profile.primary,
                        scope?scope->instance:0,scope?scope->generation:0,inventory_revision,now,allowed};
                    const auto plan=fw::gw::crafter::authorize_wire(native_crafter_menu,
                        std::span(pending).first(frame.size),inventory_profile,observed);
                    pending.erase(pending.begin(),pending.begin()+frame.size);
                    std::optional<fw::gw::inventory::Batch> result;
                    if(plan)result=fw::gw::crafter::commit(*plan,inventory_ids,inventory_profile,
                        storage_profile,inventory_revision,
                        [&](const fw::gw::inventory::Profile& durable,std::uint64_t expected)->std::optional<std::uint64_t>{
                            if(!scope||world.interaction_scope(actor)!=scope||!living_npc_interaction(*scope,fw::now_ms())||!world.nearby_npc_name(grant->player,actor,fw::now_ms())||
                               trade_sessions.active(trade_handle))return {};
                            const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},expected,durable});
                            if(saved.status!=FW_OK)return {};return saved.revision;
                        });
                    if(result){
                        for(auto& response:*result)send_world(socket,*channel,std::move(response),now+5000);
                        const fw::gw::inventory::gold::Balances desired{inventory_profile.gold,storage_profile.gold};
                        for(auto& response:fw::gw::inventory::gold::updates(client_gold,desired,inventory_profile.inventory))
                            send_world(socket,*channel,std::move(response),now+5000);
                        client_gold=desired;refresh_energy_capacity();
                        // Original203 BYTE3 clears only the matching pending
                        // bit after durable item/gold publication. Reopen gets
                        // fresh IDs and snapshot for repeated crafting.
                        send_world(socket,*channel,Packet{203,0,3},now+5000);
                        open_npc_dialogue(actor);
                    }
                    std::osyncstream(std::cout)<<"WORLD crafter_native_exchange actor="<<actor<<" opcode="<<opcode
                        <<" outcome="<<(result?"committed":"rejected")<<std::endl;
                    continue;
                }
                if(native_merchant_menu.actor){
                    const auto now=fw::now_ms();const auto actor=native_merchant_menu.actor;
                    const auto scope=world.interaction_scope(actor);
                    const auto name=world.nearby_npc_name(grant->player,actor,now);
                    const bool allowed=joined&&loaded&&grant->map_type!=11&&!trade_sessions.active(trade_handle)&&
                        scope&&name&&living_npc_interaction(*scope,now)&&fw::gw::merchant::native::supported(grant->map,*name);
                    const fw::gw::merchant::native::Observation observed{actor,grant->map,skill_profile.primary,
                        scope?scope->instance:0,scope?scope->generation:0,inventory_revision,now,allowed};
                    const auto stock=native_merchant_menu.stock;
                    const auto plan=fw::gw::merchant::native::authorize_wire(native_merchant_menu,
                        std::span(pending).first(frame.size),inventory_profile,observed);
                    pending.erase(pending.begin(),pending.begin()+frame.size);
                    std::optional<fw::gw::inventory::Batch> result;
                    if(plan)result=fw::gw::merchant::native::commit(*plan,inventory_ids,inventory_profile,
                        storage_profile,inventory_revision,stock,fw::gw::character_name_to_utf16(grant->name,19),
                        [&](const fw::gw::inventory::Profile& durable,std::uint64_t expected)->std::optional<std::uint64_t>{
                            // Each World query releases its lock before durable CAS.
                            if(!scope||world.interaction_scope(actor)!=scope||!living_npc_interaction(*scope,fw::now_ms())||!world.nearby_npc_name(grant->player,actor,fw::now_ms())||
                               trade_sessions.active(trade_handle))return {};
                            const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},expected,durable});
                            if(saved.status!=FW_OK)return {};return saved.revision;
                        });
                    if(result){
                        for(auto& response:*result)send_world(socket,*channel,std::move(response),now+5000);
                        const fw::gw::inventory::gold::Balances desired{inventory_profile.gold,storage_profile.gold};
                        for(auto& response:fw::gw::inventory::gold::updates(client_gold,desired,inventory_profile.inventory))
                            send_world(socket,*channel,std::move(response),now+5000);
                        client_gold=desired;refresh_energy_capacity();
                        // Original203 type1/11 clears the matching pending bit;
                        // commit and item/gold publication must precede this ack.
                        send_world(socket,*channel,Packet{203,0,static_cast<std::uint8_t>(plan->quote.type)},now+5000);
                        open_npc_dialogue(actor);
                    }
                    std::osyncstream(std::cout)<<"WORLD merchant_native_exchange actor="<<actor<<" opcode="<<opcode
                        <<" type="<<(plan?plan->quote.type:0)<<" outcome="<<(result?"committed":"rejected")<<std::endl;
                    continue;
                }
                const auto now=fw::now_ms();const auto actor=native_collector_menu.actor;
                const auto scope=world.interaction_scope(actor);
                const auto name=world.nearby_npc_name(grant->player,actor,now);
                const bool allowed=joined&&loaded&&grant->map_type!=11&&!trade_sessions.active(trade_handle)&&scope&&name&&living_npc_interaction(*scope,now);
                const fw::gw::collectors::native::Observation observed{actor,grant->map,skill_profile.primary,
                    scope?scope->instance:0,scope?scope->generation:0,inventory_revision,now,allowed};
                const auto plan=fw::gw::collectors::native::authorize_wire(native_collector_menu,
                    std::span(pending).first(frame.size),collector_offers,inventory_profile,observed);
                pending.erase(pending.begin(),pending.begin()+frame.size);
                std::optional<fw::gw::collectors::native::CommittedExchange> result;
                if(plan)result=fw::gw::collectors::native::commit_selected_exchange(*plan,inventory_ids,
                    inventory_profile,storage_profile,inventory_revision,grant->map,skill_profile.primary,
                    fw::gw::character_name_to_utf16(grant->name,19),
                    [&](const fw::gw::inventory::Profile& durable,std::uint64_t expected)->std::optional<std::uint64_t>{
                        // These getters finish their own World locks before the
                        // durable CAS; no World-held callback recurses into World.
                        if(!scope||world.interaction_scope(actor)!=scope||!living_npc_interaction(*scope,fw::now_ms())||!world.nearby_npc_name(grant->player,actor,fw::now_ms())||
                           trade_sessions.active(trade_handle))return {};
                        const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},expected,durable});
                        if(saved.status!=FW_OK)return {};return saved.revision;
                    });
                if(result){
                    for(auto& response:result->packets)send_world(socket,*channel,std::move(response),now+5000);
                    // Original203 BYTE(type2) clears only this collector's
                    // pending bit. Publish it only after committed inventory.
                    send_world(socket,*channel,Packet{203,0,2},now+5000);
                    // Fresh IDs/revision permit a subsequent purchase while
                    // rejecting any replay against the consumed old menu.
                    open_npc_dialogue(actor);
                }
                std::osyncstream(std::cout)<<"WORLD collector_native_exchange actor="<<actor<<" opcode="<<opcode
                    <<" outcome="<<(result?"committed":"rejected")<<std::endl;
                continue;
            }
            if(build==37130&&opcode==57){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid dialogue choice frame");
                const auto id=fw::gw::dialogue::selection(std::span(pending).first(frame.size));pending.erase(pending.begin(),pending.begin()+frame.size);
                const auto now=fw::now_ms();
                if(collector_menu.actor){
                    const auto actor=collector_menu.actor;
                    const auto name=world.nearby_npc_name(grant->player,actor,now);
                    const auto scope=world.interaction_scope(actor);
                    const bool allowed=joined&&loaded&&grant->map_type!=11&&!trade_sessions.active(trade_handle)&&name&&scope&&living_npc_interaction(*scope,now);
                    const auto* offer=fw::gw::collectors::select_menu(collector_menu,collector_offers,id,actor,grant->map,
                        skill_profile.primary,allowed?std::string_view(*name):std::string_view{},inventory_revision,now);
                    if(id==fw::gw::collectors::cancel_token)continue;
                    std::optional<fw::gw::collectors::CommittedExchange> result;
                    if(allowed&&offer)result=fw::gw::collectors::commit_exchange(inventory_ids,inventory_profile,storage_profile,
                        inventory_revision,*offer,grant->map,skill_profile.primary,fw::gw::character_name_to_utf16(grant->name,19),
                        [&](const fw::gw::inventory::Profile& durable,std::uint64_t expected)->std::optional<std::uint64_t>{
                            if(!scope||world.interaction_scope(actor)!=scope||!living_npc_interaction(*scope,fw::now_ms())||
                               !world.nearby_npc_name(grant->player,actor,fw::now_ms())||trade_sessions.active(trade_handle))return {};
                            const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},expected,durable});
                            if(saved.status!=FW_OK)return {};return saved.revision;
                        });
                    if(result)for(auto& response:result->packets)send_world(socket,*channel,std::move(response),now+5000);
                    const auto feedback=result?u"The exchange is complete.":
                        u"The exchange could not be completed. Check your trophies and inventory space, then speak to the collector again.";
                    for(auto& response:fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,feedback))
                        send_world(socket,*channel,std::move(response),now+5000);
                    std::osyncstream(std::cout)<<"WORLD collector_exchange actor="<<actor<<" token="<<id
                        <<" outcome="<<(result?"committed":"rejected")<<std::endl;
                    continue;
                }
                if(quest_actor){
                    const auto actor=quest_actor;const auto offered=quest_choices;
                    quest_actor=0;quest_choices.clear();
                    const auto name=world.nearby_npc_name(grant->player,actor,now);
                    const auto current_quest_scope=world.interaction_scope(actor);
                    if(!joined||!loaded||!name||!quest_npc_scope||current_quest_scope!=quest_npc_scope||now>=quest_deadline){
                        std::osyncstream(std::cout)<<"WORLD quest_dialogue_rejected actor="<<actor<<" token="<<id<<" reason="
                            <<(!joined?"not_joined":!loaded?"not_loaded":!name?"npc_not_nearby":!quest_npc_scope?"missing_npc_lifetime":current_quest_scope!=quest_npc_scope?"npc_lifetime_changed":"expired_menu")<<std::endl;continue;
                    }
                    auto& endpoint=gwserver::NetworkManager::current().server();
                    const auto quests=endpoint.characterQuests(skill_owner());
                    if(quests.status!=FW_OK){std::osyncstream(std::cout)<<"WORLD quest_dialogue_rejected actor="<<actor<<" token="<<id<<" reason=quest_lookup status="<<quests.status<<std::endl;continue;}
                    if(id==0x7e030052&&*name=="Verata the Necromancer"&&quests.record&&skill_actor_scope){
                        const auto progress=quests.record->profile.records.find(82);const auto trainer=world.effect_scope(actor);
                        if(progress!=quests.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active&&trainer)
                            world.arm_corpse_follow(*skill_actor_scope,progress->second.cycle,*trainer,now);
                        continue;
                    }
                    const auto fresh=fw::gw::quests::presearing::choices(quests.record?quests.record->profile:fw::gw::quests::Profile{},
                        session_quest_catalog(),skill_profile.primary,skill_profile.secondary,grant->map,*name,grant->origin_campaign==1?grant->level:0);
                    const auto selected=std::find_if(offered.begin(),offered.end(),[&](const auto& c){return c.token==id&&std::find(fresh.begin(),fresh.end(),c)!=fresh.end();});
                    if(selected==offered.end()){std::osyncstream(std::cout)<<"WORLD quest_dialogue_rejected actor="<<actor<<" token="<<id<<" reason=offer_changed_or_token_not_offered"<<std::endl;continue;}
                    const auto* rule=fw::gw::quests::presearing::find(selected->quest);
                    const bool trial=rule->profession&&!selected->reward&&trial_area&&!rule->primary_only&&
                        !skill_profile.secondary&&skill_profile.primary!=rule->profession;
                    std::vector<std::uint32_t> skills;for(auto skill:selected->reward?rule->reward_skills:rule->acceptance_skills)if(skill)skills.push_back(skill);
                    gwserver::QuestAcceptanceResult result;
                    if(rule->id==76){
                        namespace ranger=fw::gw::quests::ranger_companion;
                        const ranger::Eligibility eligible{grant->origin_campaign==1,trial_area,skill_profile.primary,skill_profile.secondary};
                        const ranger::Interaction interaction{*quest_npc_scope,*current_quest_scope,*name,grant->map,true};
                        if(selected->reward)result=gwserver::rewardRangerCompanion(endpoint,skill_owner(),selected->cycle,eligible,interaction,grant->player);
                        else result=gwserver::acceptRangerCompanion(endpoint,skill_owner(),session_quest_catalog(),selected->cycle,eligible,interaction,
                            grant->player,inventory_ids,inventory_profile,storage_profile,inventory_revision);
                    }else if(rule->id==89){
                        namespace glory=fw::gw::quests::path_to_glory;
                        const glory::Eligibility eligible{grant->origin_campaign==1,trial_area,skill_profile.primary};
                        const glory::Interaction interaction{*quest_npc_scope,*current_quest_scope,*name,grant->map,true};
                        if(selected->reward)result=gwserver::rewardPathToGlory(endpoint,skill_owner(),session_quest_catalog(),selected->cycle,eligible,interaction);
                        else result=gwserver::acceptPathToGlory(endpoint,skill_owner(),session_quest_catalog(),selected->cycle,eligible,interaction);
                    }else if(rule->id==80){
                        namespace message=fw::gw::quests::message_from_friend;
                        const message::Eligibility eligible{grant->origin_campaign==1,trial_area};
                        const message::Interaction interaction{*quest_npc_scope,*current_quest_scope,*name,grant->map,true};
                        if(selected->reward)result=gwserver::rewardMessageFromFriend(endpoint,skill_owner(),session_quest_catalog(),selected->cycle,
                            eligible,interaction,inventory_ids,inventory_profile,storage_profile,inventory_revision);
                        else result=gwserver::acceptMessageFromFriend(endpoint,skill_owner(),session_quest_catalog(),selected->cycle,eligible,interaction);
                    }else if(rule->id==79){
                        namespace second=fw::gw::quests::second_profession;
                        const second::Eligibility eligible{grant->origin_campaign==1,trial_area,skill_profile.primary,skill_profile.secondary};
                        const second::Interaction interaction{*quest_npc_scope,*current_quest_scope,*name,grant->map,true};
                        if(selected->reward){
                            result={FW_CONFLICT,"Second Profession reward prerequisites changed",{}};
                            if(quests.record&&second::plan(quests.record->profile,second::Action::reward,selected->cycle,eligible,interaction))
                                result=gwserver::rewardCharacterQuest(endpoint,{skill_owner(),quests.record->revision,79,selected->cycle,second::experience});
                        }else result=gwserver::acceptSecondProfession(endpoint,skill_owner(),session_quest_catalog(),selected->cycle,eligible,interaction);
                    }else if(rule->id==54){
                        namespace further=fw::gw::quests::further_adventures;
                        const further::Eligibility eligible{grant->origin_campaign==1,trial_area,skill_profile.primary};
                        const further::Interaction interaction{*quest_npc_scope,*current_quest_scope,*name,grant->map,true};
                        const auto current=quests.record?quests.record->profile:fw::gw::quests::Profile{};
                        const auto action=selected->reward?further::Action::reward:
                            selected->token==fw::gw::quests::presearing::further_devona_token?further::Action::talk_devona:further::Action::accept;
                        result={FW_CONFLICT,"Further Adventures interaction changed",{}};
                        if(further::plan(current,action,selected->cycle,eligible,interaction)){
                            if(action==further::Action::reward)
                                result=gwserver::rewardCharacterQuest(endpoint,{skill_owner(),quests.record->revision,54,selected->cycle,further::experience});
                            else if(action==further::Action::talk_devona)
                                result=gwserver::completeCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),54,selected->cycle);
                            else result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),54,false);
                        }
                    }else if(rule->id==65){
                        namespace trainer=fw::gw::quests::new_elementalist_trainer;
                        const trainer::Eligibility eligible{grant->origin_campaign==1,trial_area,skill_profile.primary};
                        const trainer::Interaction interaction{*quest_npc_scope,*current_quest_scope,*name,grant->map,true};
                        const auto current=quests.record?quests.record->profile:fw::gw::quests::Profile{};
                        const auto action=selected->reward?trainer::Action::reward:
                            selected->token==fw::gw::quests::presearing::new_elementalist_aziure_token?trainer::Action::talk_aziure:trainer::Action::accept;
                        result={FW_CONFLICT,"Elementalist trainer interaction changed",{}};
                        if(trainer::plan(current,action,selected->cycle,eligible,interaction)){
                            if(action==trainer::Action::reward)
                                result=gwserver::rewardCharacterQuest(endpoint,{skill_owner(),quests.record->revision,65,selected->cycle,trainer::experience});
                            else if(action==trainer::Action::talk_aziure)
                                result=gwserver::completeCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),65,selected->cycle);
                            else result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),65,false);
                        }
                    }else if(rule->id==78){
                        namespace opposition=fw::gw::quests::opposition;
                        const opposition::Eligibility eligible{grant->origin_campaign==1,trial_area};
                        const opposition::Interaction interaction{*quest_npc_scope,*current_quest_scope,*name,grant->map,true};
                        const auto current=quests.record?quests.record->profile:fw::gw::quests::Profile{};
                        result={FW_CONFLICT,"Opposition interaction changed",{}};
                        if(selected->reward){
                            if(opposition::plan(current,opposition::Action::reward,selected->cycle,eligible,interaction))
                                result=gwserver::rewardCharacterQuest(endpoint,{skill_owner(),quests.record->revision,78,selected->cycle,opposition::experience});
                        }else if(selected->token==fw::gw::quests::presearing::opposition_question_token)
                            result=gwserver::questionOppositionWitness(endpoint,skill_owner(),session_quest_catalog(),selected->cycle,eligible,interaction);
                        else if(opposition::plan(current,opposition::Action::accept,selected->cycle,eligible,interaction))
                            result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),78,false);
                    }else if(rule->id==62){
                        namespace wizard=fw::gw::quests::unsettling_rumors;
                        const wizard::Eligibility eligible{grant->origin_campaign==1,trial_area,fw::gw::quests::presearing::primary_test_rewarded(quests.record?quests.record->profile:fw::gw::quests::Profile{},skill_profile.primary)};
                        const wizard::Interaction interaction{*quest_npc_scope,*current_quest_scope,*name,grant->map,true};
                        if(selected->reward){
                            const auto proposed=quests.record?wizard::reward(quests.record->profile,inventory_profile,selected->cycle,eligible,interaction):std::nullopt;
                            result={FW_CONFLICT,"Unsettling Rumors reward unavailable or inventory full",{}};
                            if(proposed){
                                gwserver::RewardQuestExperience request{skill_owner(),quests.record->revision,62,selected->cycle,250};
                                request.expected_inventory_revision=inventory_revision;
                                auto saved=gwserver::rewardCharacterQuestItem(endpoint,request,inventory_ids,proposed->inventory,storage_profile);
                                result={saved.status,std::move(saved.message),std::move(saved.packets)};
                                if(saved.status==FW_OK){++inventory_revision;inventory_profile=std::move(saved.profiles->character);storage_profile=std::move(saved.profiles->bank);}
                            }
                        }else if(selected->token==fw::gw::quests::presearing::unsettling_meerak_token||selected->token==fw::gw::quests::presearing::unsettling_armin_token){
                            result=gwserver::advanceUnsettlingRumors(endpoint,skill_owner(),session_quest_catalog(),selected->cycle,eligible,interaction,
                                selected->token==fw::gw::quests::presearing::unsettling_meerak_token?wizard::Action::talk_meerak:wizard::Action::talk_armin);
                        }else if(wizard::plan(quests.record?quests.record->profile:fw::gw::quests::Profile{},wizard::Action::accept,selected->cycle,eligible,interaction))
                            result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),62,false);
                        else result={FW_CONFLICT,"Unsettling Rumors acceptance changed",{}};
                    }else if(rule->id==55){
                        namespace wizard=fw::gw::quests::wayward_wizard;
                        const wizard::Eligibility eligible{grant->origin_campaign==1,true,grant->level};
                        const wizard::Interaction interaction{*quest_npc_scope,*current_quest_scope,*name,grant->map,true};
                        if(selected->reward){
                            const auto proposed=quests.record?wizard::reward(quests.record->profile,inventory_profile,selected->cycle,eligible,interaction):std::nullopt;
                            result={FW_CONFLICT,"Wayward Wizard reward unavailable or inventory full",{}};
                            if(proposed){
                                gwserver::RewardQuestExperience request{skill_owner(),quests.record->revision,55,selected->cycle,250};
                                request.expected_inventory_revision=inventory_revision;
                                auto saved=gwserver::rewardCharacterQuestItem(endpoint,request,inventory_ids,proposed->inventory,storage_profile);
                                result={saved.status,std::move(saved.message),std::move(saved.packets)};
                                if(saved.status==FW_OK){++inventory_revision;inventory_profile=std::move(saved.profiles->character);storage_profile=std::move(saved.profiles->bank);}
                            }
                        }else if(selected->token==fw::gw::quests::presearing::wayward_ralena_token||selected->token==fw::gw::quests::presearing::wayward_orion_token){
                            result=gwserver::advanceWaywardWizard(endpoint,skill_owner(),session_quest_catalog(),selected->cycle,eligible,interaction,
                                selected->token==fw::gw::quests::presearing::wayward_ralena_token?wizard::Action::talk_ralena:wizard::Action::talk_orion);
                        }else if(wizard::plan(quests.record?quests.record->profile:fw::gw::quests::Profile{},wizard::Action::accept,selected->cycle,eligible,interaction))
                            result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),55,false);
                        else result={FW_CONFLICT,"Wayward Wizard acceptance changed",{}};
                    }else if(selected->reward){
                        if(rule->id==84&&(!skill_actor_scope||!world.escort_reward_ready(*skill_actor_scope,selected->cycle)))continue;
                        if(rule->id==73||rule->id==82){
                            if(!skill_actor_scope)continue;
                            const auto completed=world.pending_quest_completions(*skill_actor_scope);
                            if(std::find(completed.begin(),completed.end(),Instance::QuestCompletion{rule->id,selected->cycle})==completed.end())continue;
                        }
                        if(rule->id==83){
                            const auto proposed=fw::gw::inventory::presearing::consume_shimmering_scale(inventory_profile);
                            if(!proposed)continue;
                            auto prepared=inventory_ids.prepare(*proposed,storage_profile);
                            result=gwserver::rewardCharacterQuest(endpoint,{skill_owner(),quests.record->revision,rule->id,selected->cycle,
                                rule->experience,skill_revision,skills,inventory_revision,0,true,true},&inventory_ids,grant->player);
                            if(result.status==FW_OK){
                                if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed quest reward projection");
                                ++inventory_revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);
                            }
                        }else result=gwserver::rewardCharacterQuest(endpoint,{skill_owner(),quests.record->revision,rule->id,selected->cycle,rule->experience,skill_revision,skills,0,0,false,!skills.empty(),
                            !skill_profile.secondary&&skill_profile.primary!=rule->profession&&fw::gw::quests::reviewed_trial_unlock_quest(rule->id)},nullptr,grant->player);
                        if(result.status==FW_OK&&(rule->id==73||rule->id==82||rule->id==84)&&skill_actor_scope)
                            world.clear_kill_objective(*skill_actor_scope,rule->id,selected->cycle,true);
                        if(result.status==FW_OK&&skill_actor_scope)
                            world.clear_quest_loot(*skill_actor_scope,rule->id,selected->cycle);
                    }
                    else if(rule->id==83){
                        result={FW_UNAVAILABLE,"quest loot encounter unavailable",{}};
                        const auto encounter=npcs.loot_encounters.find(83);
                        if(skill_actor_scope&&encounter!=npcs.loot_encounters.end()&&encounter->second.map==grant->map)
                            world.spawn_loot_encounter(*skill_actor_scope,83,selected->cycle,encounter->second.actors,now,[&]{
                                result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),83,false,skills,{},grant->player);
                                return result.status==FW_OK;
                            });
                    }
                    else if(rule->id==73){
                        result={FW_UNAVAILABLE,"quest wave guardian unavailable",{}};
                        const auto wave=npcs.wave_encounters.find(rule->id);const auto guardian=world.effect_scope(actor);
                        if(skill_actor_scope&&guardian&&wave!=npcs.wave_encounters.end()&&wave->second.map==grant->map&&
                           world.quest_guardian_available(*guardian,grant->map))
                            world.accept_quest_waves(*skill_actor_scope,*guardian,rule->id,selected->cycle,wave->second.waves,[&]{
                                result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),rule->id,false,skills,{},grant->player,trial);
                                return result.status==FW_OK;
                            });
                    }
                    else if(rule->id==75){
                        result={FW_CONFLICT,"no free carried slot for Starter Sword",{}};
                        const auto proposed=fw::gw::inventory::presearing::grant_starter_sword(inventory_profile,
                            fw::gw::character_name_to_utf16(grant->name,19));
                        if(proposed){
                            auto prepared=inventory_ids.prepare(*proposed,storage_profile);
                            const auto& item=prepared.wire.character.items.at(proposed->next_item-1);
                            auto item_packets=fw::gw::inventory::item_packets(item);
                            item_packets.push_back(fw::gw::items::location_2022(prepared.wire.character.inventory,item.definition.id,item.bag,item.slot));
                            const gwserver::QuestInventoryGrant sword{{skill_owner(),inventory_revision,prepared.durable.character},std::move(item_packets)};
                            result={FW_UNAVAILABLE,"quest encounter unavailable",{}};
                            const auto encounter=npcs.kill_encounters.find(rule->id);
                            if(skill_actor_scope&&encounter!=npcs.kill_encounters.end()&&encounter->second.map==grant->map)
                                world.spawn_kill_encounter(*skill_actor_scope,rule->id,selected->cycle,encounter->second.actors,[&]{
                                    result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),rule->id,false,skills,sword,grant->player,trial);
                                    return result.status==FW_OK;
                                });
                            if(result.status==FW_OK){
                                if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed quest item projection");
                                ++inventory_revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);
                            }
                        }
                    }
                    else if(rule->id==84){
                        result={FW_UNAVAILABLE,"escort quest encounter unavailable",{}};
                        const auto encounter=npcs.escort_encounters.find(84);
                        if(skill_actor_scope&&encounter!=npcs.escort_encounters.end()){
                            const auto actors=fw::gw::world::escort_participants(world,npcs,encounter->second);
                            if(actors)fw::gw::world::accept_escort_encounter(world,*skill_actor_scope,actors->first,actors->second,grant->map,selected->cycle,encounter->second,[&]{
                                result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),rule->id,false,skills,{},grant->player,trial);return result.status==FW_OK;
                            });
                        }
                    }
                    else if(rule->id==82){
                        result={FW_UNAVAILABLE,"corpse quest encounter unavailable",{}};
                        const auto encounter=npcs.corpse_encounters.find(82);const auto trainer=world.effect_scope(actor);
                        if(skill_actor_scope&&trainer&&encounter!=npcs.corpse_encounters.end())
                            fw::gw::world::accept_corpse_encounter(world,*skill_actor_scope,*trainer,grant->map,selected->cycle,encounter->second,[&]{
                                result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),rule->id,false,skills,{},grant->player,trial);
                                return result.status==FW_OK;
                            });
                    }
                    else if(rule->id==42){
                        result={FW_CONFLICT,"no free carried slot for Starter Bow",{}};
                        const auto proposed=fw::gw::inventory::presearing::grant_starter_bow(inventory_profile,
                            fw::gw::character_name_to_utf16(grant->name,19));
                        if(proposed){
                            auto prepared=inventory_ids.prepare(*proposed,storage_profile);
                            const auto& item=prepared.wire.character.items.at(proposed->next_item-1);
                            auto item_packets=fw::gw::inventory::item_packets(item);
                            item_packets.push_back(fw::gw::items::location_2022(prepared.wire.character.inventory,item.definition.id,item.bag,item.slot));
                            const gwserver::QuestInventoryGrant bow{{skill_owner(),inventory_revision,prepared.durable.character},std::move(item_packets)};
                            result={FW_UNAVAILABLE,"timed quest encounter unavailable",{}};
                            const auto encounter=npcs.kill_encounters.find(42);
                            const auto utc=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                            if(skill_actor_scope&&utc>0&&encounter!=npcs.kill_encounters.end()&&encounter->second.map==grant->map)
                                world.spawn_marksmanship_encounter(*skill_actor_scope,selected->cycle,encounter->second.actors,[&]{
                                    result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),42,false,skills,bow,grant->player,trial,static_cast<std::uint64_t>(utc));
                                    return result.status==FW_OK;
                                },fw::now_ms());
                            if(result.status==FW_OK){
                                if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed quest bow projection");
                                ++inventory_revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);
                            }
                        }
                    }
                    else if(rule->id==85||rule->id==37||rule->id==81||rule->id==86||rule->id==71||rule->id==49||rule->id==43){
                        result={FW_UNAVAILABLE,"quest encounter unavailable",{}};
                        const auto encounter=npcs.kill_encounters.find(rule->id);
                        if(skill_actor_scope&&encounter!=npcs.kill_encounters.end())
                            fw::gw::world::accept_kill_encounter(world,*skill_actor_scope,grant->map,rule->id,selected->cycle,encounter->second,[&]{
                                result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),rule->id,false,skills,{},grant->player,trial);
                                return result.status==FW_OK;
                            });
                    }else result=gwserver::acceptCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),rule->id,false,skills,{},grant->player,trial);
                    if(result.status==FW_OK){
                        reload_skills();
                        const auto xp=endpoint.characterExperience(skill_owner());if(xp.status==FW_OK&&xp.value){
                            experience=*xp.value;world.set_experience(grant->player,experience);
                            const auto earned_level=fw::gw::experience::level(*experience);
                            if(earned_level>grant->level){
                                grant->level=earned_level;
                                send_world(socket,*channel,fw::gw::vitals::level_2022(grant->player,grant->level),now+5000);
                                refresh_energy_capacity();
                                for(auto& packet:fw::gw::attributes::packets_2022(grant->player,attribute_state.ranks(),attribute_policy(),attribute_bonuses()))
                                    send_world(socket,*channel,std::move(packet),now+5000);
                            }
                            if(selected->reward){
                                const auto statistics=fw::gw::progression::restore(faction_profile,progression_profile,
                                    *experience,grant->level,world.morale(*skill_actor_scope).value_or(entry_morale),static_cast<std::uint16_t>(grant->player));
                                send_world(socket,*channel,statistics.front(),now+5000);
                            }
                        }
                        for(auto& response:result.packets)send_world(socket,*channel,std::move(response),now+5000);
                        if(rule->id==79&&!selected->reward)reconcile_second_profession(selected->cycle);
                    }
                    if(result.status==FW_OK&&selected->reward&&rule->primary_only&&grant->map==146)
                        world.set_starter_trainer_stage(grant->player,{skill_profile.primary,true});
                    std::osyncstream(std::cout)<<"WORLD quest_dialogue quest="<<rule->id<<" reward="<<selected->reward<<" status="<<result.status<<" reason="<<result.message<<std::endl;
                    if(result.status==FW_OK&&(selected->token==fw::gw::quests::presearing::further_devona_token||
                        selected->token==fw::gw::quests::presearing::new_elementalist_aziure_token||
                        selected->token==fw::gw::quests::presearing::unsettling_armin_token||
                        selected->token==fw::gw::quests::presearing::opposition_question_token))open_npc_dialogue(actor);
                    continue;
                }

                if(!trim_actor||!joined||!loaded||grant->map_type==11||now>=trim_deadline||!world.can_access_trim_vendor(grant->player,trim_actor,now)){
                    trim_actor=trim_choice=0;trim_design.clear();trim_deadline=0;
                    std::osyncstream(std::cout)<<"WORLD dialogue_choice outcome=rejected reason=no_active_nearby_vendor id="<<id<<std::endl;continue;
                }
                if(id==fw::gw::guild::trim::cancel){trim_actor=trim_choice=0;trim_design.clear();trim_deadline=0;continue;}
                if(id>=fw::gw::guild::trim::choose&&id<fw::gw::guild::trim::choose+256){
                    const auto* offer=fw::gw::guild::trim::find(id-fw::gw::guild::trim::choose);
                    if(offer){trim_choice=offer->id;for(auto& response:fw::gw::guild::trim::confirmation(trim_actor,*offer))send_world(socket,*channel,std::move(response),now+5000);}
                    continue;
                }
                if(trim_choice&&id==fw::gw::guild::trim::confirm+trim_choice){
                    const auto actor=trim_actor,choice=trim_choice;const auto design=trim_design;
                    trim_actor=trim_choice=0;trim_design.clear();trim_deadline=0;
                    const auto saved=gwserver::NetworkManager::current().server().purchaseGuildTrim({{{grant->account},{grant->character}},choice,fw::gw::guild::cape::decode(gwserver::profile_bytes(design,8))});
                    if(saved.status==FW_OK){
                        restore_guild();
                        send_world(socket,*channel,fw::gw::progression::tournament_account(*saved.value,false),fw::now_ms()+5000);
                    }
                    const auto message=saved.status==FW_OK?std::u16string(u"Your guild's cape trim has been changed."):fw::gw::character_name_to_utf16(saved.message,255);
                    for(auto& response:fw::gw::dialogue::open(actor,message,{{u"Goodbye",fw::gw::guild::trim::cancel}}))send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                    std::osyncstream(std::cout)<<"WORLD trim_purchase actor="<<actor<<" trim="<<choice<<" outcome="<<(saved.status==FW_OK?"committed":"rejected")<<std::endl;
                }else std::osyncstream(std::cout)<<"WORLD dialogue_choice outcome=rejected reason=unoffered_confirmation id="<<id<<std::endl;
                continue;
            }
            if(build==37130&&opcode==54){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid cape purchase framing");
                const auto request=fw::gw::guild::cape::parse(std::span(pending).first(frame.size));
                gwserver::CapePurchaseResult saved{FW_CONFLICT,"Speak to a nearby Guild Emblemer first."};
                if(joined&&loaded&&grant->map_type!=11&&cape_actor&&world.can_access_emblemer(grant->player,cape_actor,fw::now_ms()))
                    saved=gwserver::NetworkManager::current().server().purchaseGuildCape({{{grant->account},{grant->character}},inventory_revision,request.quoted_price,request.design});
                if(saved.status==FW_OK){
                    inventory_revision=saved.inventory_revision;inventory_profile.gold=saved.gold;
                    const fw::gw::inventory::gold::Balances desired{inventory_profile.gold,storage_profile.gold};
                    for(auto& packet:fw::gw::inventory::gold::updates(client_gold,desired,inventory_profile.inventory))send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
                    client_gold=desired;restore_guild();
                }else outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,fw::gw::character_name_to_utf16(saved.message,255)));
                std::osyncstream(std::cout)<<"WORLD cape_purchase actor="<<cape_actor<<" outcome="<<(saved.status==FW_OK?"committed":"rejected")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&opcode==188){
                fw::gw::guild::AnnouncementRequest request;
                const auto state=fw::gw::guild::parse_announcement(pending,request);
                if(state==fw::gw::world::FrameState::incomplete)break;
                if(state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid guild announcement frame");
                const bool scene=joined&&loaded&&grant->map_type!=11;
                gwserver::GuildCommandResult result{FW_CONFLICT,"Guild announcement unavailable before world entry."};
                if(scene)result=gwserver::NetworkManager::current().server().guildCommand({{{grant->account},{grant->character}},gwserver::GuildAction::motd,request.body.empty()?std::string{}:fw::gw::character_name_to_utf8(request.body,255)});
                if(result.status==FW_OK)restore_guild();
                else if(scene)outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,fw::gw::character_name_to_utf16(result.message,255)));
                // Native302 publishes the announcement to chat and the Guild
                // window. No guessed transaction ACK or duplicate chat echo.
                std::osyncstream(std::cout)<<"WORLD guild_announcement_save transaction="<<request.transaction
                    <<" body_units="<<request.body.size()<<" outcome="<<(result.status==FW_OK?"saved":"rejected")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
            }
            if(build==37130&&opcode==122){
                fw::gw::inventory::gold::Request request;
                const auto frame=fw::gw::inventory::gold::parse(pending,request);
                if(frame==fw::gw::world::FrameState::incomplete)break;
                if(frame!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid gold transfer frame");
                const fw::gw::inventory::gold::Balances current{inventory_profile.gold,storage_profile.gold};
                const bool authorized=storage_open&&joined&&loaded&&world.can_access_storage(grant->player,storage_actor,fw::now_ms());
                const auto proposed=fw::gw::inventory::gold::propose(current,request.balances,{authorized,true,true},inventory_profile.inventory);
                bool committed=false;std::string reason=fw::gw::inventory::gold::reason_text(proposed.status);
                if(proposed.accepted()){
                    if(!proposed.amount)committed=true;
                    else{
                        auto character=inventory_profile,bank=storage_profile;character.gold=proposed.balances.carried;bank.gold=proposed.balances.stored;
                        auto prepared=inventory_ids.prepare(character,bank);
                        const auto saved=gwserver::NetworkManager::current().server().transferStorage({{{grant->account},{grant->character}},inventory_revision,storage_revision,prepared.durable.character,prepared.durable.bank});
                        if(saved.status==FW_OK){
                            if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed gold projection");
                            inventory_revision=saved.inventory_revision;storage_revision=saved.storage_revision;
                            inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);committed=true;
                        }else reason=saved.status==FW_CONFLICT?"storage_revision_conflict":"storage_unavailable";
                    }
                }
                const fw::gw::inventory::gold::Balances desired{inventory_profile.gold,storage_profile.gold};
                if(joined&&loaded&&grant->map_type!=11){
                    for(auto& response:fw::gw::inventory::gold::updates(client_gold,desired,inventory_profile.inventory))send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                    client_gold=desired;
                }
                std::osyncstream(std::cout)<<"WORLD gold_transfer outcome="<<(committed?"committed":"rejected")<<" reason="<<reason<<std::endl;
                pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
            }
            if(build==37130&&(opcode==104||opcode==106||(opcode>=117&&opcode<=121)||(opcode>=125&&opcode<=128))){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid item transaction frame");
                const auto request=fw::gw::item_transactions::request_2022(Packet(pending.begin(),pending.begin()+frame.size));
                if(!request)throw std::runtime_error("invalid item transaction fields");
                if(opcode==104){
                    const auto now=fw::now_ms();bool committed=false;std::string reason="invalid_owned_dye_mix";
                    const bool scene=joined&&loaded&&grant->map_type!=11&&!trade_sessions.active(trade_handle);
                    auto proposed=scene?fw::gw::inventory::dye::apply(inventory_profile,request->target,std::span(request->dyes).first(request->dye_count)):std::nullopt;
                    if(proposed){
                        auto prepared=inventory_ids.prepare(proposed->profile,storage_profile);
                        const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},inventory_revision,prepared.durable.character});
                        if(saved.status==FW_OK){
                            if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed dye projection");
                            inventory_revision=saved.revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);
                            for(const auto& [id,amount]:proposed->consumed){
                                if(inventory_profile.items.contains(id))send_world(socket,*channel,fw::gw::items::quantity_2022(id,inventory_profile.items.at(id).definition.quantity),now+5000);
                                else{send_world(socket,*channel,fw::gw::items::detach_2022(inventory_profile.inventory,id),now+5000);send_world(socket,*channel,fw::gw::items::delete_2022(id),now+5000);}
                            }
                            const auto& target=inventory_profile.items.at(request->target);
                            for(auto& packet:fw::gw::inventory::item_packets(target))send_world(socket,*channel,std::move(packet),now+5000);
                            const auto bag=inventory_profile.bags.find(target.bag);
                            if(bag!=inventory_profile.bags.end()&&bag->second.type==2&&bag->second.index==21){
                                constexpr std::array<unsigned,5> slots{4,2,6,3,5};
                                const auto slot=std::find(slots.begin(),slots.end(),target.slot);
                                if(slot!=slots.end()){const auto index=static_cast<unsigned>(slot-slots.begin());
                                    if(index<grant->equipment.size()&&grant->equipment[index].composite==target.definition.file){
                                        grant->equipment[index].color=proposed->color;
                                        if(const auto scope=world.effect_scope(grant->player))
                                            if(world.equipment_color(*scope,index,proposed->color))
                                                outbox->enqueue(fw::gw::equipment::restore_assignments(
                                                    grant->player,fw::gw::equipment::assignments(inventory_profile)));
                                    }
                                }
                            }
                            committed=true;reason="committed";
                        }else reason=saved.status==FW_CONFLICT?"inventory_revision_conflict":"storage_unavailable";
                    }else if(!scene)reason="scene_or_trade_unavailable";
                    if(!committed&&scene)outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,u"That dye mixture cannot be applied. Your items were not changed."));
                    std::osyncstream(std::cout)<<"WORLD dye_apply target="<<request->target<<" outcome="<<(committed?"committed":"rejected")<<" reason="<<reason<<std::endl;
                    pending.erase(pending.begin(),pending.begin()+frame.size);continue;
                }
                if(opcode==106){
                    const auto now=fw::now_ms();
                    const bool scene=joined&&loaded&&grant->map_type!=11&&!trade_sessions.active(trade_handle);
                    const auto target=inventory_profile.items.find(request->target);
                    const auto reveal=target==inventory_profile.items.end()?std::optional<fw::gw::inventory::Item>{}:identification_catalog.reveal(target->second);
                    auto plan=fw::gw::identification::identify(inventory_profile,request->source,request->target,reveal,{scene,true,false});
                    bool committed=false;std::string reason=fw::gw::identification::reason_text(plan.status);
                    if(plan.accepted()){
                        auto prepared=inventory_ids.prepare(plan.profile,storage_profile);
                        const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},inventory_revision,prepared.durable.character});
                        if(saved.status==FW_OK){
                            if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed identification projection");
                            inventory_revision=saved.revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);
                            for(auto& packet:plan.packets)send_world(socket,*channel,std::move(packet),now+5000);
                            committed=true;reason="committed";refresh_energy_capacity();
                        }else reason=saved.status==FW_CONFLICT?"inventory_revision_conflict":"storage_unavailable";
                    }
                    if(!committed&&scene)outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,u"No reviewed identification result is available for this item. Your items were not changed."));
                    std::osyncstream(std::cout)<<"WORLD identification outcome="<<(committed?"committed":"rejected")<<" reason="<<reason<<std::endl;
                    pending.erase(pending.begin(),pending.begin()+frame.size);continue;
                }
                if(opcode>=125&&opcode<=128){
                    const auto now=fw::now_ms();const bool scene=joined&&loaded&&grant->map_type!=11;
                    bool committed=false,accepted=false;std::string reason="invalid_pending_upgrade";
                    const auto close_upgrade=[&](bool terminal){
                        if(terminal&&scene)send_world(socket,*channel,fw::gw::item_transactions::upgrade_cancel_2022(),now+5000);
                        if(pending_upgrade.preview.id()&&scene)send_world(socket,*channel,fw::gw::items::delete_2022(pending_upgrade.preview.id()),now+5000);
                        pending_upgrade.clear();
                    };
                    if(opcode==127){close_upgrade(false);accepted=true;reason="cancelled";}
                    else if(!scene||trade_sessions.active(trade_handle)||salvage_preview.id()){close_upgrade(scene);reason="scene_or_trade_unavailable";}
                    else if(opcode==126){
                        close_upgrade(false);
                        const auto target=inventory_profile.items.find(request->target);
                        if(request->inventory==inventory_profile.inventory&&target!=inventory_profile.items.end()&&
                           fw::gw::inventory::upgrades::armor(target->second.definition.type)&&target->second.profession&&
                           pending_upgrade.preview.reserve(inventory_profile,storage_profile)){
                            pending_upgrade.target=request->target;pending_upgrade.revision=inventory_revision;pending_upgrade.expires=now+30000;
                            auto preview=target->second;preview.definition.id=pending_upgrade.preview.id();
                            for(auto& packet:fw::gw::inventory::item_packets(preview))send_world(socket,*channel,std::move(packet),now+5000);
                            send_world(socket,*channel,fw::gw::item_transactions::upgrade_choices_2022(inventory_profile.inventory,preview.definition.id,{}),now+5000);
                            accepted=true;reason="query_ready";
                        }else{close_upgrade(true);reason="owned_armor_required";}
                    }else if(pending_upgrade.current(inventory_revision,now)){
                        const auto source=opcode==125?request->source:pending_upgrade.source;
                        auto proposed=fw::gw::inventory::upgrades::apply(inventory_profile,source,pending_upgrade.target);
                        if(proposed&&(opcode==125||pending_upgrade.selected)){
                            if(opcode==125){
                                pending_upgrade.source=source;pending_upgrade.selection=request->selection;pending_upgrade.selected=true;
                                auto preview=proposed->profile.items.at(pending_upgrade.target);preview.definition.id=pending_upgrade.preview.id();
                                for(auto& packet:fw::gw::inventory::item_packets(preview))send_world(socket,*channel,std::move(packet),now+5000);
                                send_world(socket,*channel,fw::gw::item_transactions::upgrade_ready_2022(request->selection,preview.definition.id),now+5000);
                                accepted=true;reason="confirmation_ready";
                            }else{
                                auto prepared=inventory_ids.prepare(proposed->profile,storage_profile);
                                const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},inventory_revision,prepared.durable.character});
                                if(saved.status==FW_OK){
                                    if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed upgrade projection");
                                    const auto target=pending_upgrade.target;
                                    inventory_revision=saved.revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);
                                    if(inventory_profile.items.contains(source))send_world(socket,*channel,fw::gw::items::quantity_2022(source,inventory_profile.items.at(source).definition.quantity),now+5000);
                                    else{send_world(socket,*channel,fw::gw::items::detach_2022(inventory_profile.inventory,source),now+5000);send_world(socket,*channel,fw::gw::items::delete_2022(source),now+5000);}
                                    for(auto& packet:fw::gw::inventory::item_packets(inventory_profile.items.at(target)))send_world(socket,*channel,std::move(packet),now+5000);
                                    send_world(socket,*channel,fw::gw::item_transactions::upgrade_done_2022(),now+5000);
                                    close_upgrade(false);refresh_energy_capacity();accepted=committed=true;reason="committed";
                                }else{reason=saved.status==FW_CONFLICT?"inventory_revision_conflict":"storage_unavailable";close_upgrade(true);}
                            }
                        }else{reason="unsupported_or_ineligible_component";close_upgrade(true);}
                    }else{reason="stale_or_expired_confirmation";close_upgrade(true);}
                    if(!accepted&&scene)outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,u"That armor upgrade cannot be applied. Your items were not changed."));
                    std::osyncstream(std::cout)<<"WORLD armor_upgrade opcode="<<opcode<<" outcome="<<(committed?"committed":accepted?"ready":"rejected")<<" reason="<<reason<<std::endl;
                    pending.erase(pending.begin(),pending.begin()+frame.size);continue;
                }
                if(opcode>=117&&opcode<=121){
                    const auto now=fw::now_ms();const bool scene=joined&&loaded&&grant->map_type!=11&&!trade_sessions.active(trade_handle);
                    const auto close=[&](bool cancelled){
                        if(scene)send_world(socket,*channel,cancelled?fw::gw::item_transactions::salvage_cancel_2022():fw::gw::item_transactions::salvage_done_2022(),now+5000);
                        if(salvage_preview.id())send_world(socket,*channel,fw::gw::items::delete_2022(salvage_preview.id()),now+5000);
                        pending_salvage.clear();salvage_preview.release();salvage_committed=false;
                    };
                    bool accepted=false;std::string reason="reviewed_component_recipe_unavailable";
                    if(opcode==118){close(true);accepted=true;reason="cancelled";}
                    else if(opcode==119){if(salvage_committed){close(false);accepted=true;reason="acknowledged";}else{close(true);reason="unsolicited_acknowledgment";}}
                    else if(!scene){close(true);reason="scene_or_trade_unavailable";}
                    else if(opcode==117){
                        close(true);
                        auto outcome=salvage_catalog.component(inventory_profile,request->target);
                        auto kit=inventory_profile.items.find(request->source);
                        const auto info=kit==inventory_profile.items.end()?std::optional<fw::gw::items::modifiers::KitInfo>{}:
                            fw::gw::items::modifiers::reviewed_kit(fw::gw::items::modifiers::decode_wire(kit->second.definition.modifiers));
                        if(request->inventory==inventory_profile.inventory&&outcome&&info&&info->kind==fw::gw::items::modifiers::Kit::perfect_salvage&&!pending_upgrade.preview.id()){
                            const auto plan=fw::gw::salvage::apply(inventory_profile,request->source,request->target,*outcome,{true,true,false});
                            if(plan.accepted()&&salvage_preview.reserve(inventory_profile,storage_profile)){
                                pending_salvage={request->source,request->target,inventory_revision,now+30000};
                                auto preview=outcome->outputs.front();preview.definition.id=salvage_preview.id();
                                for(auto& packet:fw::gw::inventory::item_packets(preview))send_world(socket,*channel,std::move(packet),now+5000);
                                const std::array<std::uint32_t,1> choices{salvage_preview.id()};
                                send_world(socket,*channel,fw::gw::item_transactions::salvage_choices_2022(request->inventory,request->target,choices),now+5000);
                                accepted=true;reason="component_choices_ready";
                            }else reason="ineligible_or_inventory_full";
                        }else reason="reviewed_recipe_and_perfect_kit_required";
                    }else if(opcode==121&&request->selection==0&&pending_salvage.current(inventory_revision,now)&&!salvage_committed){
                        auto outcome=salvage_catalog.component(inventory_profile,pending_salvage.target);
                        if(outcome){auto plan=fw::gw::salvage::apply(inventory_profile,pending_salvage.kit,pending_salvage.target,*outcome,{true,true,false});
                            if(plan.accepted()){
                                auto prepared=inventory_ids.prepare(plan.profile,storage_profile);
                                const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},inventory_revision,prepared.durable.character});
                                if(saved.status==FW_OK){
                                    if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed salvage projection");
                                    inventory_revision=saved.revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);
                                    for(auto& packet:plan.packets)send_world(socket,*channel,std::move(packet),now+5000);
                                    send_world(socket,*channel,fw::gw::item_transactions::salvage_success_2022(),now+5000);
                                    salvage_committed=true;accepted=true;reason="committed";refresh_energy_capacity();
                                }else reason=saved.status==FW_CONFLICT?"inventory_revision_conflict":"storage_unavailable";
                            }else reason="outcome_no_longer_valid";
                        }
                        if(!accepted)close(true);
                    }else{close(true);reason=opcode==120?"material_recipe_unavailable":"stale_or_invalid_selection";}
                    if(!accepted&&scene)outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,u"No reviewed salvage result is available for this item. Your items were not changed."));
                    std::osyncstream(std::cout)<<"WORLD salvage opcode="<<opcode<<" outcome="<<(accepted?"accepted":"rejected")<<" reason="<<reason<<std::endl;
                    pending.erase(pending.begin(),pending.begin()+frame.size);continue;
                }
                // Outcome providers must supply actual revealed properties/recipes.
                // Never debit a kit or an ingredient merely to acknowledge a request.
                // Native355/362 close their existing dialogs; they do not promise
                // a universal pending-state reset when no dialog exists.
                const bool scene=joined&&loaded&&grant->map_type!=11;
                if(scene&&(opcode==117||opcode==118||opcode==120||opcode==121))
                    send_world(socket,*channel,fw::gw::item_transactions::salvage_cancel_2022(),fw::now_ms()+5000);
                if(scene&&opcode>=125&&opcode<=128)
                    send_world(socket,*channel,fw::gw::item_transactions::upgrade_cancel_2022(),fw::now_ms()+5000);
                const bool action=opcode==104||opcode==106||opcode==117||opcode==120||opcode==121||opcode==125||opcode==126||opcode==128;
                if(scene&&action)outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,
                    u"This item operation has no authoritative outcome available yet. Your items were not changed."));
                std::osyncstream(std::cout)<<"WORLD item_transaction opcode="<<opcode
                    <<" source="<<request->source<<" target="<<request->target
                    <<" outcome="<<(action?"rejected":"closed")<<" reason="<<(scene?"outcome_provider_unavailable":"not_in_scene")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&(opcode==42||opcode==46||opcode==77||opcode==103||opcode==112||opcode==115||opcode==124)){
                fw::gw::inventory::Request request;
                const auto state=fw::gw::inventory::parse(pending,request);
                if(state==fw::gw::world::FrameState::incomplete)break;
                if(state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid inventory frame");
                bool committed=false;std::string reason;std::vector<Packet> responses;
                const bool bank_item=request.change!=fw::gw::inventory::Change::unequip&&
                    (storage_profile.items.contains(request.item)||storage_profile.bags.contains(request.bag));
                const auto previous_equipment=fw::gw::equipment::assignments(inventory_profile);
                const bool active_trade=trade_sessions.active(trade_handle);
                const bool inventory_allowed=fw::gw::inventory::mutation_allowed(
                    joined&&loaded&&grant->map_type!=11,active_trade);
                if(!inventory_allowed){reason=active_trade?"active_trade":"not_in_scene";}
                else if(request.change==fw::gw::inventory::Change::use){
                    const bool pve=!grant->arena_match&&(grant->map_type==1||grant->map_type==4);
                    const auto used=inventory_profile.items.find(request.item);
                    const auto morale_plan=used==inventory_profile.items.end()?std::optional<fw::gw::consumables::MoralePlan>{}:consumable_catalog.plan(used->second);
                    auto proposed=fw::gw::consumables::debit(inventory_profile,request.item,consumable_catalog,pve);
                    reason="reviewed_consumable_or_pve_mode_unavailable";
                    if(proposed.accepted&&morale_plan)if(const auto scope=world.effect_scope(grant->player)){
                        auto prepared=inventory_ids.prepare(proposed.profile,storage_profile);
                        committed=world.party_morale(*scope,fw::now_ms(),pve,*morale_plan,[&]{
                            const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},inventory_revision,prepared.durable.character});
                            if(saved.status!=FW_OK){reason=saved.status==FW_CONFLICT?"inventory_revision_conflict":"storage_unavailable";return false;}
                            if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed consumable projection");
                            inventory_revision=saved.revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);
                            responses=std::move(proposed.packets);reason="morale_consumable_committed";return true;
                        });
                        if(!committed&&reason=="reviewed_consumable_or_pve_mode_unavailable")reason="party_morale_capped_or_unavailable";
                    }
                }
                else if(request.change==fw::gw::inventory::Change::drop){
                    auto proposed=fw::gw::inventory::ground::debit(inventory_profile,request.item,request.quantity,joined&&loaded&&!staging&&grant->map_type!=11);
                    reason=fw::gw::inventory::ground::reason_text(proposed.status);
                    const auto source=world.effect_scope(grant->player);
                    if(proposed.accepted()&&source){
                        auto prepared=inventory_ids.prepare(proposed.profile,storage_profile);
                        const auto wire=world.reserve_ground_item_id();
                        if(wire){
                            committed=bool(world.drop_ground_item(*source,proposed.source.definition,proposed.quantity,*wire,fw::now_ms(),true,[&](const auto&){
                                const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},inventory_revision,prepared.durable.character});
                                if(saved.status!=FW_OK){reason=saved.status==FW_CONFLICT?"inventory_revision_conflict":"storage_unavailable";return false;}
                                if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed drop projection");
                                inventory_revision=saved.revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);
                                responses=std::move(proposed.packets);return true;
                            },0,proposed.source.profession,proposed.source.customized,proposed.source.reviewed_armor));
                            if(!committed&&reason=="accepted")reason="ground_world_unavailable";
                        }else reason="ground_wire_identity_exhausted";
                    }
                }else if(bank_item){
                    auto proposed=fw::gw::storage::transfer(inventory_profile,storage_profile,request,{storage_open&&joined&&loaded&&world.can_access_storage(grant->player,storage_actor,fw::now_ms()),true,true,fw::gw::character_name_to_utf16(grant->name,19)});
                    reason=fw::gw::inventory::reason_text(proposed.status);
                    if(proposed.accepted()){
                        auto prepared=inventory_ids.prepare(proposed.character,proposed.bank);
                        const auto saved=gwserver::NetworkManager::current().server().transferStorage({{{grant->account},{grant->character}},inventory_revision,storage_revision,prepared.durable.character,prepared.durable.bank});
                        if(saved.status==FW_OK){inventory_revision=saved.inventory_revision;storage_revision=saved.storage_revision;
                            if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed storage projection");
                            inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);responses=std::move(proposed.packets);committed=true;}
                        else reason=saved.status==FW_CONFLICT?"storage_revision_conflict":"storage_unavailable";
                    }
                }else{
                    auto proposed=fw::gw::inventory::propose(inventory_profile,request,{inventory_allowed,inventory_allowed,false,fw::gw::character_name_to_utf16(grant->name,19)});
                    reason=fw::gw::inventory::reason_text(proposed.status);
                    if(request.change==fw::gw::inventory::Change::equip||request.change==fw::gw::inventory::Change::unequip){
                        fw::gw::equipment::proposal::Policy policy;
                        policy.allowed=inventory_allowed;policy.primary=skill_profile.primary;policy.character=grant->name;policy.effective=effective_attributes();
                        policy.reviewed=[&](const fw::gw::items::Definition& definition){return fw::gw::equipment::metadata::reviewed(inventory_profile,definition);};
                        auto equipment=fw::gw::equipment::proposal::propose(inventory_profile,request,policy);
                        reason=fw::gw::equipment::proposal::reason_text(equipment.status);
                        if(equipment.accepted()){proposed.status=fw::gw::inventory::Reason::accepted;proposed.profile=std::move(equipment.profile);proposed.packets=std::move(equipment.packets);}
                    }
                    if(proposed.accepted()){
                        auto prepared=inventory_ids.prepare(proposed.profile,storage_profile);
                        const auto saved=gwserver::NetworkManager::current().server().saveInventory({{{grant->account},{grant->character}},inventory_revision,prepared.durable.character});
                        if(saved.status==FW_OK){
                            if(!inventory_ids.commit(std::move(prepared)))throw std::runtime_error("obsolete committed inventory projection");
                            inventory_revision=saved.revision;inventory_profile=std::move(prepared.wire.character);storage_profile=std::move(prepared.wire.bank);responses=std::move(proposed.packets);committed=true;}
                        else reason=saved.status==FW_CONFLICT?"inventory_revision_conflict":"storage_unavailable";
                    }
                }
                if(!committed){
                    // Do not replay occupied slots or publish an uncommitted proposal.
                    const auto owned_item=fw::gw::inventory::owned_request_item(inventory_profile,request);
                    responses=owned_item?fw::gw::inventory::reconcile(inventory_profile,*owned_item):std::vector<Packet>{};
                    if(storage_open){auto bank=fw::gw::inventory::reconcile(storage_profile,request.item);responses.insert(responses.end(),bank.begin(),bank.end());}
                }
                for(auto& response:responses)send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                bool equipment_rebuilt=false;
                if(committed)if(const auto scope=world.effect_scope(grant->player)){
                    constexpr std::array<unsigned,5> physical{4,2,6,3,5};
                    const auto current=fw::gw::equipment::assignments(inventory_profile);
                    for(unsigned index=0;index<physical.size();++index){
                        const auto slot=physical[index];if(current[slot]==previous_equipment[slot])continue;
                        fw::gw::CharacterProfile::EquipmentAppearance visual{};
                        if(current[slot]){
                            const auto& item=inventory_profile.items.at(current[slot]);
                            if(!item.definition.file||item.definition.file>=0xedb)continue;
                            visual={static_cast<std::uint16_t>(item.definition.file),item.definition.dye,item.definition.unknown1};
                        }
                        if(world.equipment_visual(*scope,index,visual.packed_composite(),visual.color)){
                            equipment_rebuilt=true;
                            if(grant->equipment.size()<=index)grant->equipment.resize(index+1);
                            grant->equipment[index]=visual;
                        }
                    }
                }
                if(committed){
                    // Make committed armor/resources visible before another
                    // equip-following skill request in this receive batch.
                    if(request.change==fw::gw::inventory::Change::equip||request.change==fw::gw::inventory::Change::unequip)
                        refresh_energy_capacity();
                    const auto current=fw::gw::equipment::assignments(inventory_profile);
                    auto associations=fw::gw::equipment::assignment_changes(grant->player,previous_equipment,current);
                    // Cosmetic updates rebuild the composite. Publish owned IDs
                    // only to this owner, after its queued public visual update.
                    if(equipment_rebuilt){
                        auto restored=fw::gw::equipment::restore_assignments(grant->player,current);
                        associations.insert(associations.end(),restored.begin(),restored.end());
                    }
                    outbox->enqueue(std::move(associations));
                }
                std::osyncstream(std::cout)<<"WORLD inventory_change opcode="<<opcode<<" outcome="<<(committed?"committed":"rejected")<<" reason="<<reason<<std::endl;
                pending.erase(pending.begin(),pending.begin()+request.consumed);continue;
            }
            if(build==37130&&(opcode==55||opcode==79)){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid NPC interaction frame");
                const auto actor=read32(pending,2);
                if(opcode==79)if(const auto lever=world.northlands_lever_scope(actor)){
                    if(frame.size!=7)throw std::runtime_error("invalid gadget interaction frame");
                    const bool accepted=joined&&loaded&&gameplay_ready&&grant->map_type!=11&&skill_actor_scope&&
                        world.interact_northlands(*skill_actor_scope,*lever,fw::now_ms(),pending[6]);
                    std::osyncstream(std::cout)<<"WORLD northlands_lever actor="<<actor<<" outcome="<<(accepted?"accepted":"rejected")<<std::endl;
                    pending.erase(pending.begin(),pending.begin()+frame.size);continue;
                }
                const auto approach=joined&&loaded&&grant->map_type!=11?
                    world.approach_npc(grant->player,actor,fw::now_ms()):
                    fw::gw::world::Instance::NpcApproach::unavailable;
                if(approach==fw::gw::world::Instance::NpcApproach::ready)open_npc_dialogue(actor);
                else std::osyncstream(std::cout)<<"WORLD npc_approach actor="<<actor<<" outcome="
                    <<(approach==fw::gw::world::Instance::NpcApproach::moving?"moving":"unavailable")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&opcode==98){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid chat frame");
                const auto units=read16(pending,6);std::u16string message;
                for(unsigned i=0;i<units;++i)message.push_back(static_cast<char16_t>(read16(pending,8+2*i)));
                if(message==u"/observe"||message.starts_with(u"/observe ")){
                    const auto respond=[&](std::u16string text){for(auto& p:fw::gw::chat::global(0,u"Observer",u"",text))send_world(socket,*channel,std::move(p),fw::now_ms()+5000);};
                    try{
                        const auto entries=fw::gw::observer::catalog();
                        if(message==u"/observe"){
                            respond(u"Recorded matches: use /observe ID. Playback begins at the start; log out to leave.");
                            for(auto& e:entries){std::ifstream meta(e.folder/"meta.txt");std::string a,b,title;std::getline(meta,a);std::getline(meta,b);std::getline(meta,title);respond(fw::gw::character_name_to_utf16(std::to_string(e.id)+" : "+title,255));}
                        }else if(joined&&loaded&&gameplay_ready){
                            const auto text=fw::gw::character_name_to_utf8(message.substr(9),10);const auto id=fw::gw::observer::integer(text);
                            const auto entry=std::find_if(entries.begin(),entries.end(),[&](auto& e){return e.id==id;});if(entry==entries.end())throw std::runtime_error("unknown recording ID");
                            auto match=std::make_shared<fw::gw::observer::Match>(fw::gw::observer::load(*entry));const auto ids=transfers.issue_observer(*grant,match);
                            leave_trade();skill_runtime.cancel(skill_hooks);world.steer(grant->player,0,0,fw::now_ms());flush_outbox();
                            std::array<std::uint8_t,24> host{};std::memcpy(host.data(),&world_endpoint,sizeof(world_endpoint));
                            send_world(socket,*channel,fw::gw::world::transfer_2022(host,ids.first,ids.second,match->map,
                                grant->region==0xfffffffeu?-2:static_cast<std::int32_t>(grant->region),true),fw::now_ms()+5000);
                            std::osyncstream(std::cout)<<"OBSERVER transfer match="<<id<<" map="<<match->map<<" world="<<ids.first<<std::endl;reconnect_allowed=false;return;
                        }else respond(u"Enter a map before starting recorded playback.");
                    }catch(const std::exception& e){respond(fw::gw::character_name_to_utf16(std::string("Observer: ")+e.what(),255));}
                    pending.erase(pending.begin(),pending.begin()+frame.size);continue;
                }
                std::optional<std::uint16_t> command_travel;
                bool cancel_cast=false;
                std::optional<fw::gw::emotes::Request> requested_emote;
                std::optional<fw::gw::chat::commands::External> external_request;
                fw::gw::chat::commands::Context context;
                context.roll_allowed=grant->map_type==2||grant->map_type==1||grant->map_type==4;
                context.party_changes_allowed=staging;
                const auto chat_now=fw::now_ms();
                if(world_entered_at&&chat_now>=world_entered_at)context.map_elapsed_ms=chat_now-world_entered_at;
                char address[INET_ADDRSTRLEN]{};
                if(inet_ntop(AF_INET,&world_endpoint.sin_addr,address,sizeof address))
                    context.server_address.assign(address,address+std::char_traits<char>::length(address));
                if(joined&&loaded&&grant->map_type!=11)context.travel_destination=&command_travel;
                context.cancel_cast=&cancel_cast;
                context.emote_request=&requested_emote;
                context.external=&external_request;
                const bool accepted=joined&&world.chat(grant->player,message,chat_now,context);
                if(development_die){skill_runtime.cancel(skill_hooks);skill_runtime.update(chat_now,skill_hooks);development_die=false;}
                if(cancel_cast){skill_runtime.cancel(skill_hooks);world.cancel_emote(grant->player);}
                if(requested_emote){skill_runtime.cancel(skill_hooks);world.emote(grant->player,*requested_emote,chat_now);
                    std::osyncstream(std::cout)<<"WORLD emote action="<<static_cast<unsigned>(requested_emote->action)<<" synchronized="<<requested_emote->synchronized<<std::endl;}
                if(external_request)external_chat(*external_request);
                if(development_disconnect){
                    std::osyncstream(std::cout)<<"WORLD development_disconnect reconnect_window_ms=30000"<<std::endl;
                    flush_outbox();return;
                }
                if(gm_travel){command_travel=gm_travel;gm_travel.reset();}
                if(command_travel){
                    // Feed the existing travel handler: this command cannot
                    // skip campaign, map, navigation or spawn validation.
                    pending.erase(pending.begin(),pending.begin()+frame.size);
                    Packet request;u16(request,175);u16(request,*command_travel);
                    request.push_back(static_cast<std::uint8_t>(grant->region));
                    u16(request,0);request.push_back(static_cast<std::uint8_t>(grant->language));request.push_back(0);
                    pending.insert(pending.begin(),request.begin(),request.end());
                    continue;
                }
                pending.erase(pending.begin(),pending.begin()+frame.size);
                std::osyncstream(std::cout)<<"WORLD chat_received units="<<units<<" joined="<<joined
                    <<" outcome="<<(accepted?"sent":"invalid_text_or_state")<<std::endl;
                continue;
            }
            if(build==37130&&(opcode==94||opcode==130)){
                // Both initialized descriptors are opcode + two BYTE fields
                // (RVA 62705c / 627ae0). Creation sends appearance and dye
                // requests (4cd2c0 and itcliapi equipSlot/color validation).
                if(grant->map_type!=11)throw std::runtime_error("catalog query outside creation preview");
                if(pending.size()<4)break;
                // 94 selects campaign/profession (crcontext 4cd2c0).
                // 130 selects equipSlot/color (itcliapi 7c62b0).
                if(opcode==94&&!fw::gw::creation::valid_preview_selection(pending[2],pending[3]))
                    throw std::runtime_error("invalid creation campaign/profession campaign="+std::to_string(pending[2])+" profession="+std::to_string(pending[3]));
                if(opcode==130&&(pending[2]>8||pending[3]>13))
                    throw std::runtime_error("invalid creation equipment dye");
                if(opcode==94){
                    preview_campaign=pending[2];preview_profession=pending[3];
                    if(!((preview_campaign==1&&preview_profession<=6)||
                         (preview_campaign==2&&preview_profession<=8)||
                         (preview_campaign==3&&(preview_profession<=6||preview_profession>=9))))
                        preview_profession=0;
                }
                if(opcode==130)preview_colors[pending[2]]=pending[3];
                if(preview_profession){
                    auto catalog=fw::gw::creation::campaign_armor_preview(preview_campaign,preview_profession,preview_colors);
                    for(auto& packet:preview_equipment.replace(catalog))
                        send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
                }
                if(opcode==94){
                    // Character profession update emits UI 1000004e, consumed
                    // by crcontext 4cd250 -> 4cd6b0 to finish this request.
                    // Equipment/dye updates remain a separate pending branch.
                    const auto primary=pending[3];
                    // Profession metadata alone leaves the preview's packed
                    // composite appearance unchanged. Creation serializes that
                    // appearance, so acknowledge both before clearing pending.
                    grant->appearance=fw::gw::creation::preview_appearance(grant->appearance,primary,preview_campaign);
                    send_world(socket,*channel,fw::gw::equipment::appearance_2022(grant->player,grant->appearance),fw::now_ms()+5000);
                    for(const auto response:{165,182}){
                        std::vector<std::uint8_t> packet;u16(packet,response);u32(packet,grant->player);
                        packet.push_back(primary);packet.push_back(0);
                        if(response==182)packet.push_back(0);
                        send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
                    }
                }
                std::osyncstream(std::cout)<<"WORLD creation_appearance_request opcode="<<opcode
                    <<" category="<<unsigned(pending[2])<<" option="<<unsigned(pending[3])
                    <<" preview_appearance="<<grant->appearance
                    <<" appearance_profession="<<fw::gw::creation::appearance_field(grant->appearance,3)
                    <<" appearance_catalog_group="<<fw::gw::creation::appearance_field(grant->appearance,4)
                    <<" appearance_sex="<<fw::gw::creation::appearance_field(grant->appearance,6)
                    <<" profession_response_sent="<<(opcode==94)
                    <<" armor_preview_sent="<<(preview_profession!=0)
                    <<" gameplay_inventory_complete=0"<<std::endl;
                pending.erase(pending.begin(),pending.begin()+4);
                continue;
            }
            if(build==37130&&opcode==67){
                // FUN_008767d0 / RVA 626eb0: counted opaque BYTE payload,
                // bounded to 256. FUN_00492c00 identifies this as character
                // preferences (PREF_LOCATION_CHAR), not a creation template.
                if(!player_sent)throw std::runtime_error("character preferences before player stream");
                if(pending.size()<4)break;
                const auto bytes=read16(pending,2);
                if(bytes>256)throw std::runtime_error("character preferences exceed bound");
                if(pending.size()<4+bytes)break;
                if(grant->map_type!=11){
                    const auto directory=state_directory/"character-settings"/hex_bytes(grant->account);
                    std::filesystem::create_directories(directory);
                    const auto target=directory/(hex_bytes(grant->character)+".bin");
                    auto temporary=target;temporary+="."+std::to_string(grant->world)+".tmp";
                    std::ofstream output(temporary,std::ios::binary|std::ios::trunc);
                    output.write(reinterpret_cast<const char*>(pending.data()+4),bytes);output.close();
                    if(!output||!MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
                        std::filesystem::remove(temporary);
                        throw std::runtime_error("character preferences persistence failed");
                    }
                }
                // Creation borrows the selected character's transfer token;
                // its defaults must never overwrite that saved character.
                std::osyncstream(std::cout)<<"WORLD character_preferences_received bytes="<<bytes
                    <<" persisted="<<(grant->map_type!=11)<<" creation_preview="<<(grant->map_type==11)<<std::endl;
                pending.erase(pending.begin(),pending.begin()+4+bytes);
                continue;
            }
            if(build==37130&&opcode==159){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid party name frame");
                std::u16string name;const auto count=read16(pending,2);
                for(unsigned i=0;i<count;++i)name.push_back(static_cast<char16_t>(read16(pending,4+2*i)));
                const bool accepted=joined&&grant->map_type!=11&&world.party_invite_name(grant->player,name,fw::now_ms());
                std::osyncstream(std::cout)<<"WORLD party_invite_name outcome="<<(accepted?"accepted":"rejected")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&(opcode==154||opcode==155||opcode==156||opcode==158||opcode==160||opcode==167)){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete)throw std::runtime_error("invalid party frame");
                const auto argument=opcode==160?0:read16(pending,2);
                const bool accepted=joined&&grant->map_type!=11&&world.party_action(grant->player,opcode,argument,fw::now_ms());
                std::osyncstream(std::cout)<<"WORLD party_action opcode="<<opcode<<" argument="<<argument
                    <<" outcome="<<(accepted?"accepted":"rejected")<<std::endl;
                pending.erase(pending.begin(),pending.begin()+frame.size);continue;
            }
            if(build==37130&&opcode==174){
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete)break;
                if(frame.state!=fw::gw::world::FrameState::complete||frame.size!=19)throw std::runtime_error("invalid guild hall travel frame");
                const auto requested_key=hex_bytes(std::span(pending).subspan(2,16));
                const auto mode=pending[18];pending.erase(pending.begin(),pending.begin()+19);
                const auto state=guild_state();
                if(!loaded||!state||requested_key!=gwserver::storage_hex(state->id.bytes)||mode!=1||!state->hall){
                    std::osyncstream(std::cout)<<"WORLD guild_hall_travel outcome=rejected reason=ownership_or_mode mode="<<unsigned(mode)<<std::endl;
                    continue;
                }
                const auto hall=state->hall;
                std::osyncstream(std::cout)<<"WORLD guild_hall_travel outcome=authorized map="<<hall<<std::endl;
                if(travel(hall,grant->region,0,grant->language))return;
                continue;
            }
            if(opcode==85){
                if(pending.size()<10)break;
                const auto request=fw::gw::equipment::parse_visibility_2022(std::span(pending).first(10));pending.erase(pending.begin(),pending.begin()+10);
                if(!loaded||grant->name.empty())continue;
                const auto replacement=(equipment_visibility&~request.mask)|request.value;
                const auto saved=gwserver::NetworkManager::current().server().saveEquipmentVisibility({{{grant->account},{grant->character}},equipment_visibility,replacement});
                if(saved.status==FW_OK){equipment_visibility=replacement;send_world(socket,*channel,fw::gw::equipment::visibility_2022(request.value,request.mask),fw::now_ms()+5000);}
                else if(saved.status==FW_CONFLICT){const auto current=gwserver::NetworkManager::current().server().equipmentVisibility({{grant->account},{grant->character}});if(current.status==FW_OK&&current.value){equipment_visibility=*current.value;send_world(socket,*channel,fw::gw::equipment::visibility_2022(equipment_visibility),fw::now_ms()+5000);}}
                std::osyncstream(std::cout)<<"WORLD equipment_visibility status="<<saved.status<<" value="<<equipment_visibility<<" mask="<<request.mask<<std::endl;
                continue;
            }
            switch(opcode){case 0x26:case 8:case 10:case 11:case 0x86:case 0x88:case 0x89:packet_size=2;break;case 0x3e:packet_size=10;break;case 9:packet_size=6;break;case 0x8f:case 0x91:packet_size=4;break;case 0x29:case 0x92:break;case 0x8e:packet_size=18;break;case 0x3b:packet_size=26;break;case 0x45:case 0x3c:packet_size=14;break;case 0x00af:packet_size=9;break;default:{
                if(build!=37130)throw std::runtime_error("unsupported non-2022 opcode");
                const auto frame=fw::gw::world::outgoing_frame_2022(pending);
                if(frame.state==fw::gw::world::FrameState::incomplete) {packet_size=0;break;}
                if(frame.state!=fw::gw::world::FrameState::complete)
                    throw std::runtime_error("world frame rejected opcode="+std::to_string(opcode)+" reason="+frame.reason);
                std::osyncstream(std::cout)<<"WORLD action opcode="<<opcode<<" bytes="<<frame.size
                    <<" outcome=unsupported reason=handler_not_implemented prefix="
                    <<hex_bytes(std::span(pending).first(std::min<std::size_t>(frame.size,32)))<<std::endl;
                pending.erase(pending.begin(),pending.begin()+frame.size);
                packet_size=0;break;
            }}
            if(!packet_size && opcode!=0x29 && opcode!=0x92){
                if(pending.size()<2)break;
                const auto next=fw::gw::world::outgoing_frame_2022(pending);
                if(next.state==fw::gw::world::FrameState::incomplete)break;
                continue;
            }
            if(opcode==0x29){
                // Pinned 2022 thunk_FUN_00876090: compass stroke followed by
                // at most 16 packed coordinate pairs (descriptor RVA 626d78).
                if(pending.size()<8)break;
                const auto knots=read16(pending,6);
                if(knots>16)throw std::runtime_error("invalid attribute knot count");
                packet_size=8+4*knots;
            }
            if(opcode==0x92){
                fw::gw::world::ItemsRequest request;
                const auto parsed=fw::gw::world::parse_items_request(pending,request,build==37130?0x90:0x92);
                if(parsed==fw::gw::auth::Parse::incomplete)break;
                if(parsed==fw::gw::auth::Parse::invalid)throw std::runtime_error("invalid current-client items request");
                packet_size=request.consumed;
            }
            if(pending.size()<packet_size)break;
            std::vector<std::uint8_t> packet(pending.begin(),pending.begin()+packet_size);pending.erase(pending.begin(),pending.begin()+packet_size);
            if(opcode==0x89||opcode==0x91){
                packet[0]=static_cast<std::uint8_t>(opcode);packet[1]=0;
                fw::gw::world::MapRequest request;
                if(fw::gw::world::parse_map_request(packet,request)!=fw::gw::auth::Parse::complete)
                    throw std::runtime_error("invalid map request");
                if(map_sent){
                    // The client can retry its loading request on the same
                    // encrypted connection. Never initialize agents/inventory
                    // twice, and never turn a retry into a disconnect.
                    std::osyncstream(std::cout)<<"WORLD map_request_retry ignored=1 player_sent="<<player_sent
                        <<" loaded="<<loaded<<" packet="<<hex_bytes(packet)<<std::endl;
                    continue;
                }
                std::osyncstream(std::cout)<<"WORLD map_request path="<<(request.alternate?"alternate":"ordinary")
                    <<" option0="<<unsigned(request.options[0])<<" option1="<<unsigned(request.options[1])<<std::endl;
                map_sent=true;send_map_load(socket,*channel,*grant,fw::now_ms()+5000,profile);
            }
            else if(opcode==0x8f||opcode==0x92){
                if(!map_sent)throw std::runtime_error("mission mask request before map load");
                std::osyncstream(std::cout)<<"WORLD mission_mask_request bytes="<<(packet.size()-4)
                    <<" manifest_response_source=catalog_or_explicit_diagnostic"<<std::endl;
                if(build==37130&&std::getenv("FW_GW_PROBE_EMPTY_MANIFEST")){
                    // Use reviewed catalog entries; missing maps require the explicit diagnostic option.
                    // Verified 2022 descriptors: 407 BYTE phase, 406 BYTE type/WORD mission/DWORD version.
                    send_manifest();
                }
            }
            else if(opcode==0x86){
                if(build!=37130||!map_sent)throw std::runtime_error("unexpected manifest spawn request");
                if(manifest_spawned){std::osyncstream(std::cout)<<"WORLD manifest_spawn_retry ignored=1"<<std::endl;continue;}
                if(std::getenv("FW_GW_PROBE_EMPTY_MANIFEST")){
                    manifest_spawned=true;
                    if(grant->map_type==11)
                        send_world(socket,*channel,fw::gw::equipment::local_character_2022(grant->player),fw::now_ms()+5000);
                    if(grant->map_type!=11)
                        send_world(socket,*channel,fw::gw::origin_campaign_packet_2022(grant->origin_campaign),fw::now_ms()+5000);
                    send_world(socket,*channel,fw::gw::agent::spawn_2022(grant->player,0x30000000u|grant->player,
                        grant->x,grant->y,grant->plane,std::cos(grant->facing),std::sin(grant->facing),grant->team_token),fw::now_ms()+5000);
                    // 2022 opcode 55 creates the agent's attribute state. UI
                    // construction queries this state even before any ranks exist.
                    const auto points=grant->map_type==11?0:attribute_policy().total_points;
                    const auto used=grant->map_type==11?0:fw::gw::attributes::used_points(attribute_state.ranks());
                    send_world(socket,*channel,fw::gw::attributes::initial_packet_2022(grant->player,points-used,points),fw::now_ms()+5000);
                    // Summary provides uncaptured profession metadata; an exact
                    // owned skill capture supplies learning and the saved bar.
                    const auto primary=skill_profile.primary;
                    if(primary>=1&&primary<=10){
                        // The pinned profession chapter table maps professions
                        // 1..6 to Core bit0. Without this cache, rank0 attributes
                        // vanish even when Prophecies bit1 is present.
                        send_world(socket,*channel,fw::gw::creation::campaigns_2022(
                            static_cast<std::uint8_t>(1u|skill_campaigns)),fw::now_ms()+5000);
                        std::vector<std::uint8_t> professions;u16(professions,165);u32(professions,grant->player);
                        professions.push_back(primary);professions.push_back(skill_profile.secondary);
                        send_world(socket,*channel,std::move(professions),fw::now_ms()+5000);
                        auto restored=fw::gw::skills::restore_packets(grant->player,session_skills());
                        // 55 creates attributes; 182 creates profession state.
                        // 58 evaluates ranks using that profession state. Before
                        // 182 the native getter returns sentinel11 and asserts.
                        for(std::size_t index=0;index<restored.size();++index){
                            send_world(socket,*channel,std::move(restored[index]),fw::now_ms()+5000);
                            if(index==1&&grant->map_type!=11){
                                for(auto& response:fw::gw::attributes::packets_2022(grant->player,attribute_state.ranks(),attribute_policy(),attribute_bonuses()))
                                    send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                                std::osyncstream(std::cout)<<"WORLD attributes_restored total="<<points<<" available="<<points-used
                                    <<" revision="<<attribute_revision<<std::endl;
                            }
                        }
                        std::osyncstream(std::cout)<<"WORLD character_skills_restored persisted="<<(skill_revision!=0)
                            <<" learned="<<skill_profile.learned.size()<<" primary="<<unsigned(primary)
                            <<" secondary="<<unsigned(skill_profile.secondary)<<" revision="<<skill_revision<<std::endl;
                        if(skill_actor_scope)world.configure_adrenaline(*skill_actor_scope,session_skills().bar,fw::now_ms());
                        attribute_wire_ready=true;
                        published_equipment_bonuses=attribute_bonuses();
                    }

                    if(grant->level>=1&&grant->level<=20){
                        send_world(socket,*channel,fw::gw::vitals::level_2022(grant->player,grant->level),fw::now_ms()+5000);
                    }
                    if(grant->map_type!=11&&grant->level>=1&&grant->level<=20){
                        auto initial_resources=fw::gw::vitals::base(grant->level);
                        if(primary>=1&&primary<=10){
                            initial_resources.max_health=fw::gw::consumables::capacity(initial_resources.max_health,entry_morale,equipment_health_bonus());
                            initial_resources.health=initial_resources.max_health;
                            initial_resources.max_energy=fw::gw::energy::maximum_energy(primary,effective_attributes()[12],fw::gw::energy::innate_capacity);
                            initial_resources.energy=initial_resources.max_energy;
                        }
                        for(auto& packet:world.set_vitals(grant->player,initial_resources,entry_morale))
                            send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
                    }
                    // 232 initializes the fifteen local-player stats. Callback
                    // RVA 4755b0 copies each into World+740/744 with stride 8.
                    // Level is stat 9; morale is stat 10. Uncaptured progression
                    // starts at zero in this private world, not official values.
                    const auto tournament=gwserver::NetworkManager::current().server().tournamentPoints({grant->account});
                    if(tournament.status!=FW_OK||!tournament.value)throw std::runtime_error("tournament account balance unavailable");
                    send_world(socket,*channel,fw::gw::progression::tournament_account(*tournament.value,true),fw::now_ms()+5000);
                    std::vector<std::uint8_t> stats;u16(stats,232);
                    // Uncaptured imports retain the legacy wire placeholder; party calls
                    // must not present that placeholder as captured account progress.
                    for(unsigned stat=0;stat<15;++stat)u32(stats,stat==0?experience.value_or(0):stat==9?grant->level:stat==10?entry_morale:0);
                    if(grant->map_type!=11&&grant->level>=1&&grant->level<=20&&grant->player<=65535){
                        for(auto& response:fw::gw::progression::restore(faction_profile,progression_profile,experience.value_or(0),grant->level,entry_morale,static_cast<std::uint16_t>(grant->player)))
                            send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                    }else send_world(socket,*channel,std::move(stats),fw::now_ms()+5000);
                    if(grant->map_type==11||grant->level<1||grant->level>20)
                        send_world(socket,*channel,fw::gw::vitals::morale_2022(grant->player,100),fw::now_ms()+5000);
                    // WORD inventory ID, BYTE remote flag. Remote=0 installs
                    // ItemContext's local inventory before UIGame reads it.
                    std::vector<std::uint8_t> inventory;u16(inventory,323);u16(inventory,1);inventory.push_back(0);
                    send_world(socket,*channel,std::move(inventory),fw::now_ms()+5000);
                    if(grant->map_type!=11){
                        // Original323 asserts duplicate inventory and constructs
                        // fresh zero balances. Publish each balance once here;
                        // chest reopening uses the tracked state, not additions.
                        client_gold={inventory_profile.gold,storage_profile.gold};
                        for(auto& response:fw::gw::inventory::gold::updates({},client_gold,inventory_profile.inventory))send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                    }
                    if(grant->map_type!=11)for(auto& response:fw::gw::storage::restore_character(inventory_profile,true,false))
                        send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                    // Initialize exploration on every entry, including travel:
                    // the world-map UI checks these against continent textures.
                    const auto dimensions=fw::gw::exploration_dimensions_2022(grant->map);
                    if(dimensions.width&&dimensions.height){
                        exploration.emplace(dimensions);
                        const auto* map_record=fw::gw::find_map(grant->map);
                        exploration_path=state_directory/"character-settings"/hex_bytes(grant->account)/hex_bytes(grant->character)/
                            ("exploration-"+std::to_string(map_record->continent)+".bin");
                        try{
                            if(const auto recovery=exploration->load(exploration_path))
                                std::osyncstream(std::cout)<<"WORLD exploration_save_recovered operation=load reason="<<recovery->diagnostic
                                    <<" preserved="<<recovery->preserved_path.filename().string()<<std::endl;
                        }catch(const std::exception& error){
                            // Keep the unreadable save intact. This session can
                            // still reveal fog, but cannot overwrite old progress.
                            exploration_storage_enabled=false;
                            std::osyncstream(std::cout)<<"WORLD exploration_storage_unavailable operation=load progress_persistent=0 reason="
                                <<error.what()<<std::endl;
                        }
                        if(const auto mesh=navmeshes.find(grant->map);mesh!=navmeshes.end())exploration_bounds=mesh->second->source_bounds();
                        for(auto& packet:exploration->restore_packets())
                            send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
                        std::osyncstream(std::cout)<<"WORLD exploration_restored map="<<grant->map<<" continent="<<unsigned(map_record->continent)
                            <<" source_bounds="<<bool(exploration_bounds)<<std::endl;
                    }
                    if(grant->map_type!=11){
                        grant->equipment=fw::gw::equipment::armor_visuals(inventory_profile);

                        std::vector<std::uint32_t> unlocked;
                        const auto campaigns=gwserver::NetworkManager::current().server().accountCampaigns({grant->account});
                        if(campaigns.status==FW_OK)for(const auto& [map,mesh]:navmeshes){
                            const auto* record=fw::gw::find_map(map);
                            if(record&&(record->campaign==0||campaigns.campaigns.contains(record->campaign)))
                                unlocked.push_back(map);
                        }
                        send_world(socket,*channel,fw::gw::map_unlocks_2022(unlocked),fw::now_ms()+5000);
                    }
                    std::vector<std::uint8_t> controlled;u16(controlled,34);u32(controlled,grant->player);
                    u32(controlled,grant->map_type==11?1:(std::getenv("FW_GW_PROBE_PLAYER_VIEW")?3:1));
                    send_world(socket,*channel,std::move(controlled),fw::now_ms()+5000);
                    // 2022 FUN_007cc530: BYTE player-state flags, then map-loaded
                    // UI event 0x1000008a. This is distinct from stream finish 397.
                    if(const auto* map=fw::gw::find_map(grant->map);map&&map->region_type==4){
                        const auto state=guild_state();
                        if(state&&state->hall==grant->map){
                            // Publish the actual owning guild before map-loaded
                            // creates hall labels; no separate unclaimed identity.
                            restore_guild();
                            std::osyncstream(std::cout)<<"WORLD visited_hall_restored map="<<grant->map
                                <<" world="<<grant->world<<" local_id=1 ownership=claimed"<<std::endl;
                        }else{
                            fw::gw::guild::General hall;hall.local_id=2;
                            hall.key={grant->world,grant->map,0,0};hall.name=u"Unclaimed Private Hall";
                            send_world(socket,*channel,fw::gw::guild::visited_hall(hall),fw::now_ms()+5000);
                            std::osyncstream(std::cout)<<"WORLD visited_hall_restored map="<<grant->map
                                <<" world="<<grant->world<<" local_id=2 ownership=unclaimed"<<std::endl;
                        }
                    }
                    std::vector<std::uint8_t> complete;u16(complete,365);complete.push_back(0);
                    send_world(socket,*channel,std::move(complete),fw::now_ms()+5000);
                    // Native inventory-avatar bindings are created by map completion.
                    // Publish placements once afterward so their100000ED events reach
                    // the bound preview; definitions/bags already exist from323.
                    if(grant->map_type!=11)for(auto& response:fw::gw::storage::restore_character_placements(inventory_profile))
                        send_world(socket,*channel,std::move(response),fw::now_ms()+5000);
                    // The initial roster can arrive before UIGame exists. Republish
                    // selection after map-loaded creates the UI so event 10000112
                    // can construct its Party frame (2022 FUN_004dc080).
                    if(grant->map_type!=11){
                        for(auto& packet:world.party_roster(grant->player))
                            send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
                    }
                    std::osyncstream(std::cout)<<"WORLD diagnostic_final_spawn_sent=1"<<std::endl;
                    gameplay_ready=true;
                    if(build==37130 && grant->map_type!=11){
                        if(grant->appearance)send_world(socket,*channel,
                            fw::gw::equipment::appearance_2022(grant->player,grant->appearance),fw::now_ms()+5000);
                        for(std::size_t index=0;index<grant->equipment.size();++index){
                            const auto& equipment=grant->equipment[index];
                            // Pinned client catalog count 0xedb. Preserve unsupported
                            // captured IDs in storage; do not index beyond this table.
                            if(!equipment.composite||equipment.composite>=0xedb){
                                std::osyncstream(std::cout)<<"WORLD equipment_unresolved index="<<index
                                    <<" composite="<<equipment.composite<<std::endl;
                                continue;
                            }
                            send_world(socket,*channel,fw::gw::equipment::encode_2022(
                                fw::gw::equipment::Update::composite,grant->player,index,equipment.packed_composite()),fw::now_ms()+5000);
                            send_world(socket,*channel,fw::gw::equipment::encode_2022(
                                fw::gw::equipment::Update::color,grant->player,index,equipment.color),fw::now_ms()+5000);
                        }
                        for(auto& packet:fw::gw::equipment::restore_assignments(
                            grant->player,fw::gw::equipment::assignments(inventory_profile)))
                            send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);

                        send_world(socket,*channel,fw::gw::equipment::visibility_2022(equipment_visibility),fw::now_ms()+5000);
                        std::osyncstream(std::cout)<<"WORLD equipment_appearance_sent count="<<grant->equipment.size()<<std::endl;
                        if(grant->map==146){
                            const auto state=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                            if(state.status!=FW_OK)throw std::runtime_error("starter population quest lookup failed");
                            world.set_starter_trainer_stage(grant->player,{skill_profile.primary,
                                fw::gw::quests::presearing::primary_test_rewarded(state.record?state.record->profile:fw::gw::quests::Profile{},skill_profile.primary)});
                        }
                        const auto population=world.npc_entry_packets(grant->player);
                        for(const auto& packet:population)send_world(socket,*channel,packet,fw::now_ms()+5000);
                        if(!population.empty())std::osyncstream(std::cout)<<"WORLD npc_population_sent packets="<<population.size()<<std::endl;
                    }

                    if(grant->map_type==11){
                        // Pinned 37130 descriptor RVA 60d9b4: opcode only.
                        // Callback 7ccdb0 sends CMSG 136 then emits creation
                        // UI event 10000093. Ordinary map completion does not.
                        // 37130 descriptor 60dae0, callback 7ccb10: four BYTE
                        // campaign masks. First is owned PvE; no trial/PvP grant.
                        auto campaigns=fw::gw::creation::campaigns_2022(static_cast<std::uint8_t>(1u|grant->creation_campaigns));
                        send_world(socket,*channel,std::move(campaigns),fw::now_ms()+5000);
                        std::vector<std::uint8_t> creation;u16(creation,393);
                        send_world(socket,*channel,std::move(creation),fw::now_ms()+5000);
                        std::osyncstream(std::cout)<<"WORLD creation_preview_ready_sent=1"<<std::endl;
                    }
                }else std::osyncstream(std::cout)<<"WORLD manifest_spawn_request received=1 response_implemented=0"<<std::endl;
            }
            else if(opcode==0x8e){
                if(!map_sent)throw std::runtime_error("player stream before map load");
                if(player_sent){std::osyncstream(std::cout)<<"WORLD player_stream_retry ignored=1"<<std::endl;continue;}
                player_sent=true;
                std::vector<fw::gw::world::Packet> presentation;
                if(build==37130&&grant->map_type!=11){
                    presentation.push_back(fw::gw::professions::agent_packet_2022(grant->player,skill_profile.primary,skill_profile.secondary));
                    // Persist the public level with the peer presentation so
                    // existing observers and later arrivals receive it alike.
                    if(grant->level>=1&&grant->level<=20)
                        presentation.push_back(fw::gw::vitals::level_2022(grant->player,grant->level));
                    if(grant->appearance)presentation.push_back(fw::gw::equipment::appearance_2022(grant->player,grant->appearance));
                    for(std::size_t index=0;index<grant->equipment.size();++index){
                        const auto& equipment=grant->equipment[index];
                        if(!equipment.composite||equipment.composite>=0xedb)continue;
                        presentation.push_back(fw::gw::equipment::encode_2022(fw::gw::equipment::Update::composite,
                            grant->player,index,equipment.packed_composite()));
                        presentation.push_back(fw::gw::equipment::encode_2022(fw::gw::equipment::Update::color,
                            grant->player,index,equipment.color));
                    }
                }
                // The preview avatar is not a persisted or social world player.
                std::vector<Instance::Snapshot> existing;
                if(grant->map_type!=11){
                    existing=world.join(grant->player,grant->name,grant->x,grant->y,outbox,grant->appearance,presentation,grant->plane,fw::now_ms(),grant->team_token,false);
                    joined=true;
                }
                skill_actor_scope=world.effect_scope(grant->player);
                if(skill_actor_scope&&grant->map_type!=11){
                    const auto pet_state=gwserver::NetworkManager::current().server().characterPets(skill_owner());
                    const bool restored=pet_state.status==FW_OK&&pet_state.record&&
                        world.restore_pet_profile(*skill_actor_scope,pet_state.record->revision,pet_state.record->profile,grant->level);
                    if(!restored)std::osyncstream(std::cout)<<"WORLD pet_profile_unavailable status="<<pet_state.status<<std::endl;
                    if(restored&&!world.pet_instance_mode(*skill_actor_scope,grant->map_type==6,fw::now_ms()))
                        std::osyncstream(std::cout)<<"WORLD pet_mode_unavailable"<<std::endl;
                    // Missing storage is not an empty pet profile. Pet admission
                    // remains unavailable until a successful authoritative read.
                }
                if(skill_actor_scope){
                    hex_copy_session.emplace(*skill_actor_scope,base_session_skills().bar);
                    if(!world.configure_adrenaline(*skill_actor_scope,hex_copy_session->bar(),fw::now_ms()))
                        throw std::runtime_error("initial skillbar publication rejected");
                }
                if(skill_actor_scope)hex_copy_ids.emplace(*skill_actor_scope);
                if(grant->morale&&skill_actor_scope)entry_morale=world.bind_morale(*skill_actor_scope,grant->morale,
                    fw::gw::attributes::is_outpost_type(grant->map_type)||bool(grant->arena_match)||(grant->map_type!=1&&grant->map_type!=4));
                if(grant->map_type!=11&&(!skill_actor_scope||!world.bind_character_identity(*skill_actor_scope,hex_bytes(grant->account),hex_bytes(grant->character))))throw std::runtime_error("world owned character identity rejected");
                if(grant->map_type!=11){
                    world.rotate(grant->player,grant->facing,1,fw::now_ms());
                    world.set_experience(grant->player,experience);
                }
                if(grant->map_type!=11)social_directory.character(social_owner,grant->social_lease,social_character,static_cast<std::uint16_t>(grant->map),outbox);
                send_player_state(socket,*channel,*grant,existing,fw::now_ms()+5000,profile);
                // Native397 requests CMSG144 only for outposts. Explorable
                // instances await the server's manifest directly; native406
                // completion then sends CMSG134 for the final player stream.
                if(fw::gw::skills::casting::map_allows_casts(grant->map_type))send_manifest();
                if(build==37130&&grant->map_type!=11){
                    if(grant->player>65535)throw std::runtime_error("party player ID exceeds wire range");
                    for(auto& packet:world.party_roster(grant->player))
                        send_world(socket,*channel,std::move(packet),fw::now_ms()+5000);
                    std::osyncstream(std::cout)<<"WORLD solo_party_initialized player="<<grant->player<<std::endl;
                }
                loaded=true;world_entered_at=fw::now_ms();
                if(build==37130&&grant->map_type!=11){
                    const auto quests=gwserver::NetworkManager::current().server().characterQuests({{grant->account},{grant->character}});
                    if(quests.status!=FW_OK)throw std::runtime_error("quest restoration lookup failed");
                    const auto packets=fw::gw::quests::restore_2022(quests.record?quests.record->profile:fw::gw::quests::Profile{},session_quest_catalog());
                    for(const auto& packet:packets)send_world(socket,*channel,packet,fw::now_ms()+5000);
                    reconcile_second_profession();
                    pet_quest_reconcile();
                    if(quests.record&&skill_actor_scope)for(const auto& [quest,progress]:quests.record->profile.records){
                        const auto escort=npcs.escort_encounters.find(quest);
                        if(escort!=npcs.escort_encounters.end()&&escort->second.map==grant->map&&
                           (progress.stage==fw::gw::quests::Stage::active||progress.stage==fw::gw::quests::Stage::completed)){
                            const auto actors=fw::gw::world::escort_participants(world,npcs,escort->second);
                            if(!actors)throw std::runtime_error("escort restoration lacks unique follower/recipient");
                            const bool restored=progress.stage==fw::gw::quests::Stage::active?
                                fw::gw::world::accept_escort_encounter(world,*skill_actor_scope,actors->first,actors->second,grant->map,progress.cycle,escort->second,[]{return true;}):
                                world.restore_completed_escort_objective(*skill_actor_scope,actors->first,actors->second,grant->map,progress.cycle);
                            if(!restored)throw std::runtime_error("escort quest restoration failed");
                        }
                        const auto corpse=npcs.corpse_encounters.find(quest);
                        if(corpse!=npcs.corpse_encounters.end()&&corpse->second.map==grant->map&&
                           (progress.stage==fw::gw::quests::Stage::active||progress.stage==fw::gw::quests::Stage::completed)){
                            const auto* rule=fw::gw::quests::presearing::find(quest);std::optional<fw::gw::skills::effects::Scope> trainer;
                            if(rule)for(const auto& npc:npcs.for_map(grant->map))if(npc.captured_name==rule->offer_npc){
                                const auto scope=world.effect_scope(npc.id);if(scope){if(trainer)throw std::runtime_error("ambiguous corpse quest trainer");trainer=scope;}
                            }
                            if(!trainer)throw std::runtime_error("corpse quest restoration lacks trainer");
                            const bool restored=progress.stage==fw::gw::quests::Stage::active?
                                fw::gw::world::accept_corpse_encounter(world,*skill_actor_scope,*trainer,grant->map,progress.cycle,corpse->second,[]{return true;}):
                                world.restore_completed_corpse_objective(*skill_actor_scope,*trainer,grant->map,progress.cycle);
                            if(!restored)throw std::runtime_error("corpse quest restoration failed");
                        }
                        const auto wave=npcs.wave_encounters.find(quest);
                        if(wave!=npcs.wave_encounters.end()&&wave->second.map==grant->map&&
                           (progress.stage==fw::gw::quests::Stage::active||progress.stage==fw::gw::quests::Stage::completed)){
                            const auto* rule=fw::gw::quests::presearing::find(quest);
                            std::optional<fw::gw::skills::effects::Scope> guardian;
                            if(rule)for(const auto& npc:npcs.for_map(grant->map))if(npc.captured_name==rule->offer_npc){
                                const auto scope=world.effect_scope(npc.id);
                                if(scope&&world.quest_guardian_available(*scope,grant->map)){if(guardian)throw std::runtime_error("ambiguous quest guardian");guardian=scope;}
                            }
                            if(!guardian)throw std::runtime_error("wave quest restoration lacks living guardian");
                            const bool restored=progress.stage==fw::gw::quests::Stage::active?
                                world.accept_quest_waves(*skill_actor_scope,*guardian,quest,progress.cycle,wave->second.waves,[]{return true;}):
                                world.restore_completed_wave_objective(*skill_actor_scope,*guardian,grant->map,quest,progress.cycle,
                                    static_cast<unsigned>(wave->second.waves.size()),static_cast<unsigned>(wave->second.waves.front().size()));
                            if(!restored)throw std::runtime_error("wave quest restoration failed");
                        }
                        const auto encounter=npcs.kill_encounters.find(quest);
                        const auto collection=npcs.loot_encounters.find(quest);
                        if(collection!=npcs.loot_encounters.end()&&collection->second.map==grant->map&&
                           fw::gw::quests::presearing::restore_scale_encounter(progress,inventory_profile)){
                            if(!world.spawn_loot_encounter(*skill_actor_scope,quest,progress.cycle,collection->second.actors,world_entered_at,[]{return true;}))
                                throw std::runtime_error("active collection encounter restoration failed");
                        }
                        if(progress.stage==fw::gw::quests::Stage::active&&encounter!=npcs.kill_encounters.end()&&encounter->second.map==grant->map){
                            if(quest==42){
                                if(!progress.timed)throw std::runtime_error("timed quest restore missing persisted deadline");
                                const auto utc=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                                if(utc<=0)throw std::runtime_error("timed quest restore clock unavailable");
                                if(progress.timed->expired||static_cast<std::uint64_t>(utc)>=progress.timed->deadline_utc_ms){
                                    const auto expired=gwserver::expireCharacterMarksmanship(gwserver::NetworkManager::current().server(),skill_owner(),progress.cycle,static_cast<std::uint64_t>(utc));
                                    if(expired.status!=FW_OK)throw std::runtime_error("timed quest restore expiry persistence failed");
                                    for(const auto& packet:expired.packets)send_world(socket,*channel,packet,fw::now_ms()+5000);
                                    continue;
                                }
                                const auto elapsed=fw::gw::quests::marksmanship_elapsed(progress,static_cast<std::uint64_t>(utc));
                                const auto mask=fw::gw::quests::marksmanship_credit_mask(progress);
                                if(!elapsed||!mask||!world.restore_marksmanship_encounter(*skill_actor_scope,progress.cycle,encounter->second.actors,[]{return true;},fw::now_ms(),*elapsed,*mask))
                                    throw std::runtime_error("timed quest restore rejected persisted attempt");
                                continue;
                            }
                            if(!world.spawn_kill_encounter(*skill_actor_scope,quest,progress.cycle,encounter->second.actors,[]{return true;},false,{},{},false,0,encounter->second.any_death,{},{},encounter->second.required,static_cast<unsigned>(progress.objective_credits.size())))
                                throw std::runtime_error("active quest encounter restoration failed");
                        }
                    }
                    std::osyncstream(std::cout)<<"WORLD quest_log_restored packets="<<packets.size()<<std::endl;
                }
                if(!trade_handle&&grant->map_type!=11)
                    trade_handle=trade_sessions.attach({social_owner,grant->world,grant->player,grant->map_type==6,grant->x,grant->y});
                if(portals)portal_detector.reseed(*portals,{grant->map,grant->plane,grant->world,world_entered_at,{grant->x,grant->y}});
                std::osyncstream(std::cout)<<"WORLD entry_position x="<<grant->x<<" y="<<grant->y<<" plane="<<grant->plane<<" facing="<<grant->facing<<std::endl;std::osyncstream(std::cout)<<"WORLD player_stream_sent map="<<grant->map<<" world="<<grant->world<<" player="<<grant->player<<std::endl;
            }
            else if(opcode==0x88){
                if(build!=37130||grant->map_type!=11||!player_sent)
                    throw std::runtime_error("unexpected creation preview acknowledgment");
                std::osyncstream(std::cout)<<"WORLD creation_preview_acknowledged=1"<<std::endl;
            }
            else if(opcode==11){
                const auto now=fw::now_ms();
                if(ping_challenge.begin(now)){std::vector<std::uint8_t> ping;u16(ping,12);send_world(socket,*channel,std::move(ping),now+5000);}
            }
            else if(opcode==9){
                if(const auto elapsed=ping_challenge.complete(fw::now_ms())){
                    std::vector<std::uint8_t> pong;u16(pong,13);u32(pong,*elapsed);send_world(socket,*channel,std::move(pong),fw::now_ms()+5000);
                    std::osyncstream(std::cout)<<"WORLD ping roundtrip_ms="<<*elapsed<<" client_frame_ms="<<read32(packet,2)<<std::endl;
                }
            }
            else if(opcode==0x29){
                if(!loaded)throw std::runtime_error("compass drawing before world entry");
                const bool sent=grant->map_type!=11&&world.draw_compass(grant->player,packet);
                std::osyncstream(std::cout)<<"WORLD compass_drawing stroke="<<read32(packet,2)
                    <<" points="<<read16(packet,6)<<" outcome="<<(sent?"solo_party_sent":"rejected")<<std::endl;
            }
            else if(opcode==0x26){
                if(!loaded)throw std::runtime_error("stop before world entry");
                world.steer(grant->player,0,0,fw::now_ms());
            }
            else if(opcode==0x3e){
                if(build!=37130||!loaded)throw std::runtime_error("rotation outside 2022 world");
                // Pinned sender RVA 476600: opcode, float angle, float scale.
                world.rotate(grant->player,std::bit_cast<float>(read32(packet,2)),
                    std::bit_cast<float>(read32(packet,6)),fw::now_ms());
                std::osyncstream(std::cout)<<"WORLD keyboard_rotation player="<<grant->player
                    <<" angle="<<std::bit_cast<float>(read32(packet,2))
                    <<" scale="<<std::bit_cast<float>(read32(packet,6))<<" server_ms="<<fw::now_ms()<<std::endl;
            }
            else if(opcode==0x3b){
                if(!loaded)throw std::runtime_error("movement before world entry");
                // 2022 producer RVA 476520: position/plane, velocity XY,
                // direction enum. Validate the reported path and elapsed-speed
                // budget before reconciling sparse held-turn checkpoints.
                const auto direction=read32(packet,22);
                if(direction<1||direction>8)throw std::runtime_error("invalid movement direction flags");
                if(std::hypot(std::bit_cast<float>(read32(packet,14)),std::bit_cast<float>(read32(packet,18)))>.001f)
                    skill_runtime.cancel(skill_hooks);
                const auto checkpoint=world.steer(grant->player,std::bit_cast<float>(read32(packet,14)),
                    std::bit_cast<float>(read32(packet,18)),fw::now_ms(),direction,
                    fw::gw::world::Instance::SteeringReport{std::bit_cast<float>(read32(packet,2)),
                        std::bit_cast<float>(read32(packet,6)),read32(packet,10)});
                std::osyncstream(std::cout)<<"WORLD keyboard_direction player="<<grant->player
                    <<" vx="<<std::bit_cast<float>(read32(packet,14))
                    <<" vy="<<std::bit_cast<float>(read32(packet,18))
                    <<" client_x="<<std::bit_cast<float>(read32(packet,2))
                    <<" client_y="<<std::bit_cast<float>(read32(packet,6))
                    <<" client_plane="<<read32(packet,10)<<" direction="<<direction
                    <<" checkpoint="<<static_cast<unsigned>(checkpoint.status)
                    <<" checkpoint_distance="<<checkpoint.distance<<" checkpoint_budget="<<checkpoint.budget
                    <<" checkpoint_correction="<<checkpoint.correction
                    <<" server_ms="<<fw::now_ms()<<std::endl;
            }
            else if(opcode==0x45){
                if(!loaded)throw std::runtime_error("position report before world entry");
                // Build 37130: input reset 0x51eaa0 -> 0x7986f0 stops the
                // local agent, then sends 0x45 via 0x876790. This is the
                // keyboard-stop report, not a periodic position-only update.
                // The final report uses the same collision/speed validation as
                // held input; an unchecked position can never teleport an actor.
                if(!std::isfinite(std::bit_cast<float>(read32(packet,2)))||
                   !std::isfinite(std::bit_cast<float>(read32(packet,6))))
                    throw std::runtime_error("invalid predicted position");
                const auto checkpoint=world.steer(grant->player,0,0,fw::now_ms(),1,
                    fw::gw::world::Instance::SteeringReport{std::bit_cast<float>(read32(packet,2)),
                        std::bit_cast<float>(read32(packet,6)),read32(packet,10)});
                const auto stopped=world.position(grant->player,fw::now_ms());
                std::osyncstream(std::cout)<<"WORLD keyboard_stop player="<<grant->player
                    <<" x="<<stopped.first<<" y="<<stopped.second
                    <<" correction_distance="<<std::hypot(stopped.first-std::bit_cast<float>(read32(packet,2)),stopped.second-std::bit_cast<float>(read32(packet,6)))
                    <<" server_ms="<<fw::now_ms()
                    <<" client_plane="<<read32(packet,10)<<std::endl;
                std::osyncstream(std::cout)<<"WORLD stop_checkpoint player="<<grant->player
                    <<" status="<<static_cast<unsigned>(checkpoint.status)
                    <<" distance="<<checkpoint.distance<<" budget="<<checkpoint.budget
                    <<" correction="<<checkpoint.correction<<std::endl;
            }
            else if(opcode==0x3c){
                if(!loaded)throw std::runtime_error("movement before world entry");
                const float x=std::bit_cast<float>(read32(packet,2)),y=std::bit_cast<float>(read32(packet,6));
                skill_runtime.cancel(skill_hooks);
                try{world.move(grant->player,x,y,fw::now_ms(),read32(packet,10));}
                catch(const std::runtime_error& error){
                    if(std::string_view(error.what())!="movement outside navigable route")throw;
                    // The movement planner leaves rejected replacements intact.
                    // For a blocked ground click, explicitly stop and synchronize
                    // the client rather than relying on a partially cleared route.
                    world.steer(grant->player,0,0,fw::now_ms());
                    std::osyncstream(std::cout)<<"WORLD movement_destination_blocked=1"<<std::endl;
                }
            } else if(opcode==0x00af){
                if(!loaded)throw std::runtime_error("travel before world entry");
                // Native map UI uses the fifth argument of 7da1c0; the
                // development command uses zero. Neither bypasses preflight.
                if(!fw::gw::world::valid_travel_flags_2022(packet[8])){
                    std::osyncstream(std::cout)<<"WORLD travel_rejected reason=unsupported_flags flags="
                        <<unsigned(packet[8])<<" packet="<<hex_bytes(packet)<<std::endl;
                    continue;
                }
                if(build==37130&&grant->map_type==11&&created_character&&
                   read16(packet,2)==0&&packet[4]==0xfd&&read16(packet,5)==0&&packet[7]==0xff){
                    // 400 descriptor60dbd0 -> 7ccec0 -> auth redirect.
                    // Only a committed new character may leave this preview.
                    const auto initial_map=fw::gw::creation::starting_map(preview_campaign);
                    std::ifstream points("config/creation-spawn-points.txt");
                    try{if(const auto observed=fw::gw::creation::first_arrival(points,preview_campaign,initial_map)){
                        const auto validated=map_entry_arrival(state_directory,initial_map,observed);
                        const auto* destination=fw::gw::find_map(initial_map);
                        auto created_owner=*grant;created_owner.character=created_character->character;
                        if(validated&&destination&&transfers.authorize_travel(created_owner,destination->type,
                            initial_map,0,0,0,validated))
                            std::osyncstream(std::cout)<<"WORLD creation_arrival_authorized map="<<initial_map
                                <<" x="<<validated->point.x<<" y="<<validated->point.z<<" plane="<<validated->plane<<std::endl;
                        else std::osyncstream(std::cout)<<"WORLD creation_arrival_unavailable map="<<initial_map<<std::endl;
                    }}catch(const std::exception& error){
                        std::osyncstream(std::cout)<<"WORLD creation_arrival_rejected reason="<<error.what()<<std::endl;
                    }
                    Packet redirect;u16(redirect,400);u16(redirect,initial_map);
                    send_world(socket,*channel,std::move(redirect),fw::now_ms()+5000);
                    std::osyncstream(std::cout)<<"WORLD creation_destination_redirect map="<<fw::gw::creation::starting_map(preview_campaign)<<std::endl;
                    continue;
                }
                const auto destination_id=read16(packet,2);
                const auto* destination=fw::gw::find_map(destination_id);
                const auto signed_region=static_cast<std::int8_t>(packet[4]);
                const auto region=signed_region<0?static_cast<std::uint32_t>(static_cast<std::int32_t>(signed_region)):
                    static_cast<std::uint32_t>(signed_region);
                const auto district=read16(packet,5);
                const auto language=static_cast<std::uint32_t>(packet[7]);
                const bool valid_language=language==0||language==2||language==3||language==4||language==5||language==9||language==10;
                const bool valid_region=region<=4||region==0xfffffffeu;
                std::osyncstream(std::cout)<<"WORLD travel_request map="<<destination_id
                    <<" type="<<(destination?unsigned(destination->type):255)<<" region="<<region
                    <<" district="<<district<<" language="<<language
                    <<" source_map="<<grant->map<<" creation_committed="<<bool(created_character)
                    <<" packet="<<hex_bytes(packet)<<std::endl;
                if(!destination||destination->type==255||!valid_region||!valid_language){
                    std::osyncstream(std::cout)<<"WORLD travel_rejected reason=unsupported_destination_tuple"<<std::endl;
                    continue;
                }
                if(travel(destination_id,region,district,language))return;
            } else if(opcode==8){return;}
        }
        }
        const auto now=fw::now_ms();
        if(grant->map==2&&grant->map_type==6&&joined&&loaded&&grant->arena_match){
            const auto match=grant->arena_match;
            std::optional<std::uint64_t> deadline;
            std::optional<std::pair<std::uint32_t,std::uint32_t>> ticket;
            bool dead=false;
            {
                std::lock_guard lock(match->mutex);
                fw::gw::arena::record_deaths(match->lifecycle,match->players,world.player_death_times(match->players));
                deadline=match->lifecycle.return_deadline();
                if(!match->result)match->result=match->lifecycle.poll(now);
                const unsigned index=grant->player==match->players[0]?0:1;
                if(const auto resources=world.resources(grant->player))dead=!resources->health;
                if(match->result){
                    if(!match->return_tickets[index]){
                        const auto& original=match->waiting[index];
                        const auto arrival=map_entry_arrival(state_directory,2);
                        if(!arrival)throw std::runtime_error("arena return spawn unavailable");
                        match->return_tickets[index]=transfers.issue(original.account,original.character,original.social_lease,5,2,
                            original.region,original.district,original.language,original.name,arrival->point.x,arrival->point.z,
                            original.appearance,original.level,original.equipment,original.creation_campaigns,original.secondary,
                            static_cast<std::uint16_t>(arrival->plane),arrival->facing,match->result->requeue==grant->player,original.origin_campaign);
                    }
                    ticket=match->return_tickets[index];
                }
            }
            if(ticket){
                std::array<std::uint8_t,24> host{};std::memcpy(host.data(),&world_endpoint,sizeof(world_endpoint));
                send_world(socket,*channel,fw::gw::world::transfer_2022(host,ticket->first,ticket->second,2,
                    grant->region==0xfffffffeu?-2:static_cast<std::int32_t>(grant->region),false),now+5000);
                std::osyncstream(std::cout)<<"WORLD arena_result_return player="<<grant->player<<" requeue="<<!dead<<std::endl;
                leave_trade();
                return;
            }
            if(deadline&&arena_result_deadline!=*deadline){
                const auto elapsed=now>=*deadline?10000u:static_cast<unsigned>(10000-(*deadline-now));
                send_world(socket,*channel,fw::gw::arena::countdown_plain_2022(dead?u"Returning to outpost":u"Next match",elapsed,10000),now+5000);
                arena_result_deadline=*deadline;
            }
        }
        if(grant->map==2&&grant->map_type==5&&joined&&loaded&&grant->auto_arena_queue){
            std::optional<fw::gw::arena::Queue::State> state;
            {std::lock_guard lock(arena_mutex);state=arena_queue.join(arena_lobby,arena_player,now);
                if(state)arena_participants.try_emplace(arena_player,ArenaParticipant{*grant,{}});}
            if(state){
                grant->auto_arena_queue=false;
                send_world(socket,*channel,fw::gw::arena::pending_2022(true),now+5000);
                send_world(socket,*channel,fw::gw::arena::countdown_plain_2022(u"Entering mission",30000-static_cast<unsigned>(state->deadline-now),30000),now+5000);
                arena_clock_deadline=state->deadline;
            }
        }
        if(grant->map==2&&grant->map_type==5&&arena_clock_deadline){
            std::optional<std::pair<std::uint32_t,std::uint32_t>> ticket;
            std::optional<fw::gw::arena::Queue::State> state;
            {
                std::lock_guard lock(arena_mutex);
                if(auto match=arena_queue.expire(arena_lobby,now)){
                    auto a=arena_participants.find(match->players[0]),b=arena_participants.find(match->players[1]);
                    if(a!=arena_participants.end()&&b!=arena_participants.end())try{
                        const auto first=map_entry_arrival(state_directory,2);
                        auto opposite=first;
                        if(opposite){opposite->point.x=-opposite->point.x;opposite->point.z=-opposite->point.z;opposite->facing+=3.14159265f;}
                        const auto second=opposite?map_entry_arrival(state_directory,2,opposite):std::nullopt;
                        if(!first||!second)throw std::runtime_error("arena development starts unavailable");
                        const auto keys=transfers.issue_arena_pair({a->second.grant,b->second.grant},{*first,*second});
                        a->second.ticket=keys[0];b->second.ticket=keys[1];
                        std::osyncstream(std::cout)<<"WORLD arena_match_issued world="<<keys[0].first<<" players=2 mode=6"<<std::endl;
                    }catch(const std::exception& error){
                        arena_queue.join(arena_lobby,match->players[0],now);arena_queue.join(arena_lobby,match->players[1],now);
                        std::osyncstream(std::cout)<<"WORLD arena_match_retry reason="<<error.what()<<std::endl;
                    }
                }
                auto it=arena_participants.find(arena_player);
                if(it!=arena_participants.end())ticket=it->second.ticket;
                state=arena_queue.inspect(arena_lobby,arena_player,now);
            }
            if(ticket){
                std::array<std::uint8_t,24> host{};std::memcpy(host.data(),&world_endpoint,sizeof(world_endpoint));
                send_world(socket,*channel,fw::gw::world::transfer_2022(host,ticket->first,ticket->second,2,
                    grant->region==0xfffffffeu?-2:static_cast<std::int32_t>(grant->region),true),now+5000);
                {std::lock_guard lock(arena_mutex);arena_participants.erase(arena_player);}
                std::osyncstream(std::cout)<<"WORLD arena_transfer_sent world="<<ticket->first<<" player="<<ticket->second<<std::endl;
                leave_trade();
                return;
            }
            if(state&&state->deadline!=arena_clock_deadline){
                send_world(socket,*channel,fw::gw::arena::countdown_plain_2022(u"Entering mission",30000-static_cast<std::uint32_t>(state->deadline-now),30000),now+5000);
                arena_clock_deadline=state->deadline;
            }
        }
        if(const auto delta=agent_clock.advance(loaded,now)){
                // This is a delta clock, separate from connection keepalives.
                // Advance before publishing this frame's movement changes.
                send_world(socket,*channel,fw::gw::agent::movement_tick_2022(delta),now+5000);
        }
        if(joined&&loaded)publish_cooldown_sources(now);
        world.tick(now);
        world.drive_controlled_spirits(now);
        if(joined&&loaded&&skill_actor_scope&&now>=quest_objective_poll){
            quest_objective_poll=now<=UINT64_MAX-250?now+250:UINT64_MAX;
            if(npcs.loot_encounters.contains(83)&&npcs.loot_encounters.at(83).map==grant->map){
                const auto progress=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                if(progress.status==FW_OK&&progress.record){const auto found=progress.record->profile.records.find(83);
                    if(found!=progress.record->profile.records.end()&&fw::gw::quests::presearing::scale_collection_open(found->second))
                        for(const auto& death:world.pending_quest_loot(*skill_actor_scope,83,found->second.cycle))
                            if(world.loot_npc_death(*skill_actor_scope,death.target,fw::gw::inventory::presearing::shimmering_scale_definition(),now,60000,83,found->second.cycle))
                                std::osyncstream(std::cout)<<"WORLD quest_loot_created quest=83 cycle="<<found->second.cycle<<" source="<<death.target.actor<<std::endl;
                }
            }
            if(session_quest_catalog().contains(83)&&
               fw::gw::inventory::presearing::carried_shimmering_scale(inventory_profile)){
                auto& endpoint=gwserver::NetworkManager::current().server();
                const auto progress=endpoint.characterQuests(skill_owner());
                if(progress.status==FW_OK&&progress.record){
                    const auto found=progress.record->profile.records.find(83);
                    if(found!=progress.record->profile.records.end()&&found->second.stage==fw::gw::quests::Stage::active){
                        const auto result=gwserver::completeElementalistTest(endpoint,skill_owner(),session_quest_catalog(),found->second.cycle);
                        if(result.status==FW_OK){
                            for(auto response:result.packets)send_world(socket,*channel,std::move(response),now+5000);
                            std::osyncstream(std::cout)<<"WORLD quest_objective_complete quest=83 cycle="<<found->second.cycle<<" reason=carried_scale"<<std::endl;
                        }
                    }
                }
            }
            // Quest75 uses explicitly authored melee retaliation; merely
            // entering a starter foe's proximity does not activate it.
            if(skill_actor_scope&&grant->map==160&&npcs.kill_encounters.contains(75)){
                const auto profile=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                if(profile.status==FW_OK&&profile.record){const auto progress=profile.record->profile.records.find(75);
                    if(progress!=profile.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active&&
                       world.arm_grawl_melee(*skill_actor_scope,progress->second.cycle,now))
                        world.arm_grawl_wander(*skill_actor_scope,progress->second.cycle,now);
                }
            }
            // Existing Mesmer Test roster: captured ordinary attack starts,
            // authored damage/timing, damage-triggered owner-targeted combat.
            if(skill_actor_scope&&grant->map==146&&npcs.kill_encounters.contains(81)){
                const auto profile=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                if(profile.status==FW_OK&&profile.record){const auto progress=profile.record->profile.records.find(81);
                    if(progress!=profile.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active&&
                       world.arm_mesmer_test_melee(*skill_actor_scope,progress->second.cycle,now))
                        world.arm_mesmer_test_wander(*skill_actor_scope,progress->second.cycle,now);
                }
            }
            // Existing Warrior Test roster; exact Health is captured, while
            // melee and patrol tuning are authored development policy.
            if(skill_actor_scope&&grant->map==146&&npcs.kill_encounters.contains(85)){
                const auto profile=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                if(profile.status==FW_OK&&profile.record){const auto progress=profile.record->profile.records.find(85);
                    if(progress!=profile.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active)
                        world.arm_warrior_test_melee(*skill_actor_scope,progress->second.cycle,now);
                }
            }
            // Exact captured Rogue Bull; weapon/patrol tuning is explicitly
            // authored in rogue-bull-melee-development-policy.txt.
            if(skill_actor_scope&&grant->map==146&&npcs.kill_encounters.contains(71)){
                const auto profile=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                if(profile.status==FW_OK&&profile.record){const auto progress=profile.record->profile.records.find(71);
                    if(progress!=profile.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active&&
                       world.arm_rogue_bull_melee(*skill_actor_scope,progress->second.cycle,now))
                        world.arm_rogue_bull_wander(*skill_actor_scope,progress->second.cycle,now);
                }
            }
            // Quest43 reuses paid Vile Touch and the captured owned roster.
            if(skill_actor_scope&&grant->map==145&&npcs.kill_encounters.contains(43)){
                const auto profile=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                if(profile.status==FW_OK&&profile.record){const auto progress=profile.record->profile.records.find(43);
                    if(progress!=profile.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active){
                        const auto report=world.arm_blood_fanatic_immediate(*skill_actor_scope,progress->second.cycle,now);
                        const auto key=std::tuple{progress->second.cycle,report.remaining,report.armed,report.unsupported,report.owner_valid};
                        if(reported_blood_ai!=key){reported_blood_ai=key;
                            std::osyncstream(std::cout)<<"WORLD quest_ai quest=43 cycle="<<progress->second.cycle
                                <<" remaining="<<report.remaining<<" armed="<<report.armed<<" unsupported="<<report.unsupported
                                <<" owner_valid="<<report.owner_valid<<std::endl;
                        }
                    }
                }
            }
            // Exact Domination Magic Brood: owned self Aura and real Ice Spear
            // actions make the quest's Backfire counter-casting usable.
            if(skill_actor_scope&&grant->map==161&&npcs.kill_encounters.contains(49)){
                const auto profile=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                if(profile.status==FW_OK&&profile.record){const auto progress=profile.record->profile.records.find(49);
                    if(progress!=profile.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active)
                        if(world.arm_brood_caster(*skill_actor_scope,progress->second.cycle,now))
                            world.arm_brood_wander(*skill_actor_scope,progress->second.cycle,now);
                }
            }
            for(const auto& [quest,encounter]:npcs.wave_encounters)
                if(encounter.map==grant->map){
                    const auto profile=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                    if(profile.status==FW_OK&&profile.record){const auto progress=profile.record->profile.records.find(quest);
                        if(progress!=profile.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active){
                            world.advance_quest_waves(*skill_actor_scope,quest,progress->second.cycle,encounter.waves,[]{return true;});
                            if(quest==73)world.drive_quest_guardians(*skill_actor_scope,quest,progress->second.cycle,
                                // Acquisition/follow/leash distances are authored world units; not measured retail tuning.
                                {220,128,5,2000,"data/gw/quests/experiment-ai-development-policy.txt",1000,1500,2000,fw::gw::combat::weapon::AttackDelivery::melee},now);
                            if(world.wave_objective_state(*skill_actor_scope,quest)==fw::gw::quests::WaveObjective::State::failed&&
                               reported_wave_failures.emplace(quest,progress->second.cycle).second){
                                outbox->enqueue(fw::gw::chat::local(static_cast<std::uint16_t>(grant->player),0,
                                    u"The quest guardian died. Enter a fresh map instance to restart the encounter, or abandon the quest in your quest log."));
                                std::osyncstream(std::cout)<<"WORLD quest_wave_failed quest="<<quest<<" cycle="<<progress->second.cycle
                                    <<" reason=guardian_lost retry=fresh_instance_or_abandon"<<std::endl;
                            }
                        }}
                }
            for(const auto& [quest,encounter]:npcs.escort_encounters)if(encounter.map==grant->map){
                const auto profile=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                if(profile.status==FW_OK&&profile.record){const auto progress=profile.record->profile.records.find(quest);
                    if(progress!=profile.record->profile.records.end()&&(progress->second.stage==fw::gw::quests::Stage::active||progress->second.stage==fw::gw::quests::Stage::completed))
                        fw::gw::world::advance_escort_encounter(world,*skill_actor_scope,progress->second.cycle,encounter,now);
                }
            }
            for(const auto& [quest,encounter]:npcs.corpse_encounters)if(encounter.map==grant->map){
                const auto profile=gwserver::NetworkManager::current().server().characterQuests(skill_owner());
                if(profile.status==FW_OK&&profile.record){const auto progress=profile.record->profile.records.find(quest);
                    if(progress!=profile.record->profile.records.end()&&progress->second.stage==fw::gw::quests::Stage::active)
                        fw::gw::world::advance_corpse_encounter(world,*skill_actor_scope,progress->second.cycle,encounter,now,[]{return true;});
                }
            }
            for(const auto credit:world.pending_quest_credits(*skill_actor_scope)){
                auto& endpoint=gwserver::NetworkManager::current().server();
                const auto loaded=endpoint.characterQuests(skill_owner());
                if(loaded.status!=FW_OK)continue;
                const auto* profile=loaded.record?&loaded.record->profile:nullptr;
                if(!profile||!profile->records.contains(credit.quest)||profile->records.at(credit.quest).cycle!=credit.cycle||
                   (profile->records.at(credit.quest).stage!=fw::gw::quests::Stage::active&&profile->records.at(credit.quest).stage!=fw::gw::quests::Stage::completed)){
                    world.clear_kill_objective(*skill_actor_scope,credit.quest,credit.cycle);continue;
                }
                std::optional<std::uint64_t> occurred_utc;
                if(credit.quest==42){
                    const auto sampled_mono=fw::now_ms();
                    const auto sampled_utc=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                    if(sampled_utc>0)occurred_utc=fw::gw::quests::event_utc_ms(credit.occurred_mono_ms,sampled_mono,static_cast<std::uint64_t>(sampled_utc));
                    if(!occurred_utc)continue; // Retain the unacknowledged event; never substitute processing time.
                }
                const auto result=gwserver::creditCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),credit.quest,credit.cycle,credit.credit,credit.required,occurred_utc);
                if(result.status==FW_OK){
                    world.acknowledge_quest_credit(*skill_actor_scope,credit);
                    for(auto response:result.packets)send_world(socket,*channel,std::move(response),now+5000);
                    const auto saved=endpoint.characterQuests(skill_owner());
                    if(saved.status==FW_OK&&saved.record){
                        const auto& state=saved.record->profile.records.at(credit.quest);
                        std::osyncstream(std::cout)<<"WORLD quest_objective_credit quest="<<credit.quest<<" cycle="<<credit.cycle<<" count="<<state.objective_credits.size()<<" completed="<<(state.stage==fw::gw::quests::Stage::completed)<<std::endl;
                        if(state.stage==fw::gw::quests::Stage::completed)world.clear_kill_objective(*skill_actor_scope,credit.quest,credit.cycle);
                    }
                }
            }
            {
                const auto pending=world.pending_quest_credits(*skill_actor_scope);
                if(std::none_of(pending.begin(),pending.end(),[](const auto& credit){return credit.quest==42;})&&npcs.kill_encounters.contains(42)&&now>=next_timed_quest_check){
                    next_timed_quest_check=now<=UINT64_MAX-250?now+250:UINT64_MAX;
                    auto& endpoint=gwserver::NetworkManager::current().server();
                    const auto saved=endpoint.characterQuests(skill_owner());
                    if(saved.status==FW_OK&&saved.record){
                        const auto attempt=saved.record->profile.records.find(42);
                        const auto utc=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                        if(attempt!=saved.record->profile.records.end()&&attempt->second.stage==fw::gw::quests::Stage::active&&attempt->second.timed&&!attempt->second.timed->expired&&utc>0&&static_cast<std::uint64_t>(utc)>=attempt->second.timed->deadline_utc_ms){
                            const auto expired=gwserver::expireCharacterMarksmanship(endpoint,skill_owner(),attempt->second.cycle,static_cast<std::uint64_t>(utc));
                            if(expired.status==FW_OK){
                                world.clear_kill_objective(*skill_actor_scope,42,attempt->second.cycle,true);
                                for(const auto& packet:expired.packets)send_world(socket,*channel,packet,now+5000);
                                std::osyncstream(std::cout)<<"WORLD timed_quest_expired quest=42 cycle="<<attempt->second.cycle<<std::endl;
                            }
                        }
                    }
                }
            }
            for(const auto objective:world.pending_quest_completions(*skill_actor_scope)){
                auto& endpoint=gwserver::NetworkManager::current().server();
                const auto progress=endpoint.characterQuests(skill_owner());
                if(progress.status!=FW_OK)continue;
                const auto* profile=progress.record?&progress.record->profile:nullptr;
                if((npcs.wave_encounters.contains(objective.quest)||npcs.corpse_encounters.contains(objective.quest)||npcs.escort_encounters.contains(objective.quest))&&profile&&profile->records.contains(objective.quest)&&
                   profile->records.at(objective.quest).cycle==objective.cycle&&
                   profile->records.at(objective.quest).stage==fw::gw::quests::Stage::completed)continue;
                if(!profile||!profile->records.contains(objective.quest)||
                    profile->records.at(objective.quest).cycle!=objective.cycle||
                    profile->records.at(objective.quest).stage!=fw::gw::quests::Stage::active){
                    world.clear_kill_objective(*skill_actor_scope,objective.quest,objective.cycle);continue;
                }
                const auto result=gwserver::completeCharacterQuest(endpoint,skill_owner(),session_quest_catalog(),objective.quest,objective.cycle);
                if(result.status==FW_OK){
                    if(!npcs.wave_encounters.contains(objective.quest)&&!npcs.corpse_encounters.contains(objective.quest)&&!npcs.escort_encounters.contains(objective.quest))world.clear_kill_objective(*skill_actor_scope,objective.quest,objective.cycle);
                    for(auto response:result.packets)send_world(socket,*channel,std::move(response),now+5000);
                    std::osyncstream(std::cout)<<"WORLD quest_objective_complete quest="<<objective.quest<<" cycle="<<objective.cycle<<std::endl;
                }
                // Failed CAS/storage leaves the exact completion pending for
                // retry; stale/reaccepted attempts are discarded above.
            }
        }
        if(skill_actor_scope)if(const auto identity=world.character_identity(*skill_actor_scope);identity&&identity->name!=grant->name){
            grant->name=identity->name;social_character=fw::gw::character_name_to_utf16(grant->name);
            if(grant->map_type!=11)social_directory.character(social_owner,grant->social_lease,social_character,static_cast<std::uint16_t>(grant->map),outbox);
        }
        if(salvage_preview.id()&&fw::now_ms()>=pending_salvage.expires){
            send_world(socket,*channel,fw::gw::item_transactions::salvage_cancel_2022(),fw::now_ms()+5000);
            send_world(socket,*channel,fw::gw::items::delete_2022(salvage_preview.id()),fw::now_ms()+5000);
            salvage_preview.release();pending_salvage.clear();salvage_committed=false;
        }
        if(pending_upgrade.preview.id()&&fw::now_ms()>=pending_upgrade.expires){
            if(joined&&loaded){send_world(socket,*channel,fw::gw::item_transactions::upgrade_cancel_2022(),fw::now_ms()+5000);
                send_world(socket,*channel,fw::gw::items::delete_2022(pending_upgrade.preview.id()),fw::now_ms()+5000);}
            pending_upgrade.clear();
        }
        if(joined&&loaded)refresh_energy_capacity();
        if(joined&&loaded&&gameplay_ready&&skill_actor_scope&&grant->map_type!=11){
            // Durable equipped skills determine pet presence; temporary copies
            // must not manufacture an owned companion.
            const auto equipped_pet_slots=base_session_skills().bar;
            std::array<unsigned,8> pet_bar{};
            for(std::size_t i=0;i<pet_bar.size();++i)pet_bar[i]=equipped_pet_slots[i].skill;
            const bool ready=world.reconcile_pet_bar_mode(*skill_actor_scope,grant->map,
                fw::gw::skills::casting::map_allows_casts(grant->map_type),
                grant->map_type==6,pet_bar,now);
            if(!pet_presence_ready||*pet_presence_ready!=ready){
                std::osyncstream(std::cout)<<"WORLD pet_presence ready="<<ready<<" map="<<grant->map<<std::endl;
                pet_presence_ready=ready;
            }
        }
        if(trade_handle&&joined&&loaded){
            const auto position=world.position(grant->player,now);
            trade_sessions.update(trade_handle,{social_owner,grant->world,grant->player,grant->map_type==6,position.first,position.second});
            flush_trade();
        }
        if(build==37130&&joined&&loaded&&grant->map_type!=11){
            if(const auto actor=world.take_npc_interaction(grant->player,now))open_npc_dialogue(*actor);
            if(const auto item=world.take_ground_interaction(grant->player,now))pickup_ground(*item);
        }
        if(joined&&loaded&&gameplay_ready&&grant->map_type!=11&&now-last_guild_poll>=1000){last_guild_poll=now;restore_guild();}
        if(portals&&joined&&loaded&&grant->map_type!=11){
            const auto position=world.position(grant->player,now);
            if(const auto crossing=portal_detector.observe(*portals,{grant->map,world.plane(grant->player),grant->world,now,{position.first,position.second}})){
                std::osyncstream(std::cout)<<"WORLD portal_crossed gate="<<crossing->id<<" source_map="<<crossing->source
                    <<" destination="<<crossing->destination<<" epoch="<<crossing->epoch<<std::endl;
                const bool sent=travel(crossing->destination,grant->region,0,grant->language,crossing->arrival,crossing->id);
                portal_detector.resolve(fw::now_ms());
                if(sent)return;
            }
        }
        if(joined&&loaded){
            const bool resources_settled=skill_actor_scope&&world.settle_resources(*skill_actor_scope,now);
            if(skill_actor_scope&&world.knocked_down(*skill_actor_scope,now))skill_runtime.interrupt_control(now,skill_hooks);
            if(skill_actor_scope&&world.take_morale_recharge(*skill_actor_scope))skill_runtime.morale_boost(skill_hooks);
            if(skill_actor_scope&&hex_copy_session){
                const auto base=base_session_skills();
                if(hex_copy_session->base()!=base.bar){
                    const auto policy=fw::gw::skills::policy(base,skill_campaigns,staging,grant->map_type==6,trial_area);
                    if(!fw::gw::skills::effects::replace_hex_copy_base(world,*hex_copy_session,*skill_actor_scope,base.bar,policy,now))
                        throw std::runtime_error("temporary skill base publication rejected");
                }
                const auto restored=fw::gw::skills::effects::expire_temporary_copy(world,*hex_copy_session,*skill_actor_scope,now);
                if(!restored)throw std::runtime_error("temporary skill session lifetime rejected");
                // Restore native slot identity before its recharge UI. Both
                // session membership and World authority are committed above.
                for(const auto& change:restored->changes)if(change.restored)
                    send_world(socket,*channel,fw::gw::skills::slot_packet_2022(grant->player,change.grant.index,change.grant.original),now+5000);
                if(!restored->receipt.cooldowns().empty()&&
                   !skill_runtime.consume_copy_restoration(restored->receipt,now,skill_hooks,false))
                    throw std::runtime_error("temporary skill restoration cooldown rejected");
                std::vector<fw::gw::skills::Slot> restored_slots;
                for(const auto& change:restored->changes)if(change.restored)restored_slots.push_back(change.grant.original);
                if(!restored_slots.empty()&&!skill_runtime.restored_slot_feedback(restored_slots,now,skill_hooks))
                    throw std::runtime_error("temporary skill restoration feedback rejected");
            }
            if(resources_settled){skill_runtime.update(now,skill_hooks);publish_cooldown_sources(now);}
        }
        if(loaded&&exploration&&exploration_bounds&&now-last_exploration_sample>=200){
            last_exploration_sample=now;
            const auto position=world.position(grant->player,now);
            if(const auto cell=fw::gw::exploration::locate(grant->map,*exploration_bounds,position.first,position.second);
               cell&&exploration->reveal(*cell)){
                send_world(socket,*channel,fw::gw::exploration::Progress::mark(*cell),now+5000);
                if(exploration_storage_enabled)try{
                    if(const auto recovery=exploration->save(exploration_path))
                        std::osyncstream(std::cout)<<"WORLD exploration_save_recovered operation=save reason="<<recovery->diagnostic
                            <<" preserved="<<recovery->preserved_path.filename().string()<<std::endl;
                }catch(const std::exception& error){
                    exploration_storage_enabled=false;
                    std::osyncstream(std::cout)<<"WORLD exploration_storage_unavailable operation=save progress_persistent=0 reason="
                        <<error.what()<<std::endl;
                }
                std::osyncstream(std::cout)<<"WORLD exploration_revealed map="<<grant->map<<" cell_x="<<cell->x<<" cell_y="<<cell->y<<std::endl;
            }
        }
        flush_outbox();
    }
}
}
void fw_gw_auth_prepare() { stopping = false; ready = false; }
void fw_gw_auth_stop() { stopping = true; }
bool fw_gw_auth_ready() { return ready.load(); }
uint32_t fw_gw_auth_connections() { return active_connections.load(); }
int fw_gw_auth_run(const gwserver::ServerSettings& settings) try {
    const auto port=settings.auth_port;
    const auto& world_bind_host=settings.bind;
    const auto& public_world_host=settings.public_host;
    std::map<std::uint32_t,std::shared_ptr<const Mesh>> navmeshes;
    if (!settings.navigation.empty()) {
        std::ifstream catalog(settings.navigation);
        if (catalog) navmeshes = fw::gw::navigation::load_catalog(catalog,
            std::filesystem::absolute(settings.navigation).parent_path());
        else
            throw std::invalid_argument("could not open navmesh map catalog");
    }
    const auto build=settings.build;
    if(!fw::gw::client_profile(build))throw std::invalid_argument("unsupported client build");
    std::optional<fw::gw::map_portals::Catalog> portals;
    if(build==37130&&!settings.portals.empty()){
        portals=fw::gw::map_portals::Catalog::read(settings.portals);
        portals->validate_all([&](std::uint32_t map)->const Mesh*{const auto it=navmeshes.find(map);
            return it==navmeshes.end()?nullptr:it->second.get();});
        std::osyncstream(std::cout)<<"WORLD portal_catalog_validated routes="<<portals->routes().size()
            <<" trigger_provenance=development_observed_exit"<<std::endl;
    }
    fw::gw::world::NpcCatalog npcs;
    if(!settings.npcs.empty()){
        std::ifstream population_file(settings.npcs,std::ios::binary);
        if(!population_file)throw std::invalid_argument("could not open NPC catalog");
        const std::string population_bytes((std::istreambuf_iterator<char>(population_file)),{});
        std::istringstream catalog(population_bytes);
        npcs=fw::gw::world::load_npc_catalog(catalog);
        if(!settings.npc_combat.empty()){
            std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};
            crypto_hash_sha256(digest.data(),reinterpret_cast<const unsigned char*>(population_bytes.data()),population_bytes.size());
            std::array<char,crypto_hash_sha256_BYTES*2+1> encoded{};
            sodium_bin2hex(encoded.data(),encoded.size(),digest.data(),digest.size());
            std::ifstream metadata(settings.npc_combat);
            if(!metadata)throw std::invalid_argument("could not open NPC combat metadata");
            const auto reviewed=fw::gw::combat::npcs::load_catalog(metadata,encoded.data());
            const auto observed=fw::gw::world::combat_population(npcs);
            reviewed.validate_population(observed,encoded.data());
            fw::gw::world::bind_combat_catalog(npcs,reviewed,encoded.data());
            // Exact baseline bindings only; preserve every unrelated reviewed row.
            if(settings.npcs=="data/gw/npcs.txt"){
                std::ifstream tad_file("data/gw/ai/tad-development-combat.txt");
                if(!tad_file)throw std::invalid_argument("Tad development combat metadata unavailable");
                const auto tad=fw::gw::combat::npcs::load_catalog(tad_file,encoded.data());
                const auto tad_population=fw::gw::world::combat_population(npcs);
                tad.validate_population(tad_population,encoded.data());
                for(auto& [map,actors]:npcs.maps)for(auto& actor:actors)
                    if(const auto* row=tad.find(map,actor.id))actor.combat_definition=*row;
            }
            std::osyncstream(std::cout)<<"WORLD npc_combat_catalog_validated actors="<<reviewed.size()
                <<" population_sha256="<<encoded.data()<<std::endl;
        }
        if(build!=37130&&!npcs.maps.empty())throw std::invalid_argument("NPC catalog requires 2022 client protocol");
        for(const auto& [map,population]:npcs.maps){
            const auto mesh=navmeshes.find(map);
            if(mesh==navmeshes.end())throw std::invalid_argument("NPC map has no navigation: "+std::to_string(map));
            fw::gw::world::Instance validation;validation.configure_navigation(mesh->second);validation.configure_npcs(population);
        }
    }
    const auto quest_content_hash=[](std::string_view bytes){
        std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};
        crypto_hash_sha256(digest.data(),reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size());
        std::array<char,crypto_hash_sha256_BYTES*2+1> encoded{};sodium_bin2hex(encoded.data(),encoded.size(),digest.data(),digest.size());return std::string(encoded.data());
    };
    std::set<std::string> escort_placement_names;
    if(settings.quest_escorts){
        std::ifstream escorts(*settings.quest_escorts);if(!escorts)throw std::invalid_argument("could not open escort quest catalog");
        npcs.escort_encounters=fw::gw::world::load_quest_escorts(escorts,std::filesystem::absolute(*settings.quest_escorts).parent_path(),quest_content_hash);
        for(const auto& [quest,encounter]:npcs.escort_encounters){escort_placement_names.insert(encounter.follower);escort_placement_names.insert(encounter.recipient);}
    }
    if(settings.npcs=="data/gw/npcs.txt"){
        std::ifstream input("data/gw/pets/presearing-stalker-npcs.txt",std::ios::binary);
        if(!input)throw std::invalid_argument("Stalker population unavailable");
        const std::string bytes((std::istreambuf_iterator<char>(input)),{});
        std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};
        crypto_hash_sha256(digest.data(),reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size());
        std::array<char,crypto_hash_sha256_BYTES*2+1> encoded{};
        sodium_bin2hex(encoded.data(),encoded.size(),digest.data(),digest.size());
        std::istringstream population(bytes);
        auto supplement=fw::gw::world::load_npc_catalog(population);
        std::ifstream metadata("data/gw/pets/presearing-stalker-combat.txt");
        if(!metadata)throw std::invalid_argument("Stalker combat metadata unavailable");
        const auto combat=fw::gw::combat::npcs::load_catalog(metadata,encoded.data());
        fw::gw::world::bind_combat_catalog(supplement,combat,encoded.data());
        for(const auto& [map,actors]:supplement.maps)for(const auto& actor:actors){
            for(const auto& [existing_map,existing]:npcs.maps)for(const auto& other:existing)
                if(other.id==actor.id||other.definition.id==actor.definition.id)
                    throw std::invalid_argument("Stalker supplemental actor identity collision");
            const auto mesh=navmeshes.find(map);
            if(mesh==navmeshes.end()||!mesh->second->locate_cell(actor.plane,{actor.x,actor.y}))
                throw std::invalid_argument("Stalker supplemental placement unavailable");
            npcs.maps[map].push_back(actor);
        }
    }
    if(settings.quest_giver_combat&&!settings.quest_givers)throw std::invalid_argument("quest giver combat requires giver population");
    if(settings.quest_givers){
        std::ifstream giver_file(*settings.quest_givers,std::ios::binary);
        if(!giver_file)throw std::invalid_argument("could not open quest giver catalog");
        const std::string giver_bytes((std::istreambuf_iterator<char>(giver_file)),{});
        std::istringstream giver_input(giver_bytes);auto givers=fw::gw::world::load_npc_catalog(giver_input);
        if(settings.quest_giver_combat){
            std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};
            crypto_hash_sha256(digest.data(),reinterpret_cast<const unsigned char*>(giver_bytes.data()),giver_bytes.size());
            std::array<char,crypto_hash_sha256_BYTES*2+1> encoded{};
            sodium_bin2hex(encoded.data(),encoded.size(),digest.data(),digest.size());
            std::ifstream metadata(*settings.quest_giver_combat);
            if(!metadata)throw std::invalid_argument("could not open quest giver combat catalog");
            const auto combat=fw::gw::combat::npcs::load_catalog(metadata,encoded.data());
            fw::gw::world::bind_combat_catalog(givers,combat,encoded.data());
        }
        // A primary-only escort capture must not globally replace its
        // follower's ordinary position (Gwen's shrine versus Monk-test river).
        struct ScopedFollower {std::uint32_t map;unsigned primary;fw::gw::world::Instance::Npc ordinary,quest;};
        std::vector<ScopedFollower> scoped_followers;
        for(const auto& [quest,encounter]:npcs.escort_encounters){
            const auto* rule=fw::gw::quests::presearing::find(quest);
            if(!rule||!rule->primary_only)continue;
            const auto& ordinary=npcs.for_map(encounter.map);const auto& supplemental=givers.for_map(encounter.map);
            const auto matches=[&](const auto& actors){return std::count_if(actors.begin(),actors.end(),[&](const auto& n){return n.captured_name==encounter.follower;});};
            if(matches(ordinary)!=1||matches(supplemental)!=1)throw std::invalid_argument("primary escort follower placement ambiguous");
            const auto find=[&](const auto& actors)->const auto&{return *std::find_if(actors.begin(),actors.end(),[&](const auto& n){return n.captured_name==encounter.follower;});};
            scoped_followers.push_back({encounter.map,rule->profession,find(ordinary),find(supplemental)});
        }
        fw::gw::world::merge_quest_givers(npcs,givers,escort_placement_names,true);
        for(const auto& follower:scoped_followers)for(auto& actor:npcs.maps.at(follower.map))
            if(actor.captured_name==follower.quest.captured_name){
                npcs.primary_placements.push_back({follower.map,actor.id,follower.primary,follower.quest.x,follower.quest.y,follower.quest.facing,follower.quest.plane});
                actor.x=follower.ordinary.x;actor.y=follower.ordinary.y;
                actor.plane=follower.ordinary.plane;actor.facing=follower.ordinary.facing;
            }
        for(const auto& [map,actors]:givers.maps){
            const auto mesh=navmeshes.find(map);
            if(mesh==navmeshes.end())throw std::invalid_argument("quest giver map has no navigation");
            fw::gw::world::Instance validation;validation.configure_navigation(mesh->second);validation.configure_npcs(npcs.for_map(map));
            for(unsigned primary=1;primary<=6;++primary){
                fw::gw::world::Instance scoped;scoped.configure_navigation(mesh->second);scoped.configure_npcs(npcs.for_primary(map,primary));
            }
        }
        std::osyncstream(std::cout)<<"QUEST giver_catalog_validated maps="<<givers.maps.size()<<std::endl;
    }
    if(gwserver::NetworkManager::current().quest_catalog().contains(76)){
        if(!settings.quest_catalog)throw std::invalid_argument("Ranger Companion requires its content catalog");
        const auto path=settings.quest_catalog->parent_path()/"actors"/"ranger-companion-nente.txt";
        std::ifstream input(path,std::ios::binary);if(!input)throw std::invalid_argument("Ranger Companion Nente population unavailable");
        const std::string bytes((std::istreambuf_iterator<char>(input)),{});
        std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};
        crypto_hash_sha256(digest.data(),reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size());
        std::array<char,crypto_hash_sha256_BYTES*2+1> encoded{};
        sodium_bin2hex(encoded.data(),encoded.size(),digest.data(),digest.size());
        if(std::string_view(encoded.data())!="2c7a67f2eaeef9a1ea58d1a2a55046d5e56dcd579b2e2f31e68f623c1b66142b")
            throw std::invalid_argument("Ranger Companion Nente catalog provenance mismatch");
        std::istringstream population(bytes);
        const auto supplement=fw::gw::world::load_npc_catalog(population);
        if(supplement.maps.size()!=1||supplement.for_map(162).size()!=1)
            throw std::invalid_argument("Ranger Companion requires exactly one Regent Valley Nente");
        auto nente=supplement.for_map(162).front();
        if(nente.id!=23001||nente.definition.id!=23001||nente.captured_name!="Master Ranger Nente"||
           nente.definition.model_file!=116227||nente.definition.skin_file||nente.definition.flags!=524||
           nente.definition.primary!=2||nente.definition.level!=5||nente.plane||
           nente.x!=-16961.0f||nente.y!=10431.0f)
            throw std::invalid_argument("Ranger Companion Nente identity/observed placement differs");
        fw::gw::quests::ranger_companion::assign_nente_identity(nente,npcs);
        const auto mesh=navmeshes.find(162);
        if(mesh==navmeshes.end()||!mesh->second->locate_cell(nente.plane,{nente.x,nente.y}))
            throw std::invalid_argument("Ranger Companion Nente observed point outside navigation");
        nente.literal_display_name=u"Master Ranger Nente";
        npcs.maps[162].push_back(std::move(nente));
        auto validation=std::make_unique<fw::gw::world::Instance>();
        validation->configure_navigation(mesh->second);validation->configure_npcs(npcs.for_map(162));
        std::osyncstream(std::cout)<<"QUEST ranger_companion_nente_validated actor=24001 source_identity=23001 map=162 observed_placement=1"<<std::endl;
    }
    if(settings.quest_encounters||settings.quest_waves||settings.quest_corpses||settings.quest_escorts){
        const auto load_path=settings.quest_encounters?*settings.quest_encounters:settings.quest_waves?*settings.quest_waves:settings.quest_corpses?*settings.quest_corpses:*settings.quest_escorts;
        std::ifstream encounters(load_path);
        if(!encounters)throw std::invalid_argument("could not open quest encounter catalog");
        const auto hash=[](std::string_view bytes){
            std::array<unsigned char,crypto_hash_sha256_BYTES> digest{};
            crypto_hash_sha256(digest.data(),reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size());
            std::array<char,crypto_hash_sha256_BYTES*2+1> encoded{};
            sodium_bin2hex(encoded.data(),encoded.size(),digest.data(),digest.size());return std::string(encoded.data());
        };
        if(settings.quest_encounters)
            npcs.kill_encounters=fw::gw::world::load_quest_encounters(encounters,std::filesystem::absolute(load_path).parent_path(),hash,&npcs.loot_encounters);
        if(const auto blood=npcs.kill_encounters.find(43);blood!=npcs.kill_encounters.end()){
            const auto& encounter=blood->second;
            if(encounter.map!=145||encounter.actors.size()!=4)throw std::invalid_argument("quest43 AI requires exact captured roster");
            std::set<unsigned> templates;
            for(const auto& actor:encounter.actors){
                if(actor.id<20100||actor.id>20103||!templates.insert(actor.id).second||!actor.combat_definition||
                   actor.combat_definition->binding.definition!=70||actor.combat_definition->binding.model_file!=116378||
                   actor.combat_definition->binding.level!=2)throw std::invalid_argument("quest43 AI template identity mismatch");
            }
        }
        for(const auto& [quest,encounter]:npcs.kill_encounters){
            if(!gwserver::NetworkManager::current().quest_catalog().contains(quest))throw std::invalid_argument("encounter lacks configured quest content");
            const auto mesh=navmeshes.find(encounter.map);
            if(mesh==navmeshes.end())throw std::invalid_argument("quest encounter has no navigation");
            fw::gw::world::Instance validation;validation.configure_navigation(mesh->second);validation.configure_npcs(encounter.actors);
        }
        for(const auto& [quest,encounter]:npcs.loot_encounters){
            if(!gwserver::NetworkManager::current().quest_catalog().contains(quest))throw std::invalid_argument("collection encounter lacks configured quest content");
            const auto mesh=navmeshes.find(encounter.map);if(mesh==navmeshes.end())throw std::invalid_argument("collection encounter has no navigation");
            fw::gw::world::Instance validation;validation.configure_navigation(mesh->second);validation.configure_npcs(encounter.actors);
        }
        std::osyncstream(std::cout)<<"QUEST encounter_catalog_validated encounters="<<npcs.kill_encounters.size()<<" collections="<<npcs.loot_encounters.size()<<std::endl;
        if(settings.quest_escorts){
            for(const auto& [quest,encounter]:npcs.escort_encounters){
                if(npcs.kill_encounters.contains(quest)||npcs.loot_encounters.contains(quest))throw std::invalid_argument("conflicting escort quest encounter");
                if(!gwserver::NetworkManager::current().quest_catalog().contains(quest))throw std::invalid_argument("escort encounter lacks quest content");
                const auto mesh=navmeshes.find(encounter.map);if(mesh==navmeshes.end())throw std::invalid_argument("escort encounter lacks navigation");
                fw::gw::world::Instance validation;validation.configure_navigation(mesh->second);validation.configure_npcs(npcs.for_map(encounter.map));
                if(!fw::gw::world::escort_participants(validation,npcs,encounter))throw std::invalid_argument("escort encounter lacks unique roles");
                validation.configure_npcs(encounter.blockers);
            }
            std::osyncstream(std::cout)<<"QUEST escort_catalog_validated encounters="<<npcs.escort_encounters.size()<<std::endl;
        }
        if(settings.quest_corpses){
            std::ifstream corpses(*settings.quest_corpses);if(!corpses)throw std::invalid_argument("could not open corpse quest catalog");
            npcs.corpse_encounters=fw::gw::world::load_quest_corpses(corpses,std::filesystem::absolute(*settings.quest_corpses).parent_path(),hash);
            for(const auto& [quest,encounter]:npcs.corpse_encounters){
                if(npcs.kill_encounters.contains(quest)||npcs.loot_encounters.contains(quest)||npcs.escort_encounters.contains(quest))throw std::invalid_argument("conflicting corpse quest encounter");
                if(!gwserver::NetworkManager::current().quest_catalog().contains(quest))throw std::invalid_argument("corpse encounter lacks quest content");
                const auto mesh=navmeshes.find(encounter.map);if(mesh==navmeshes.end())throw std::invalid_argument("corpse encounter lacks navigation");
                fw::gw::world::Instance validation;validation.configure_navigation(mesh->second);validation.configure_npcs(encounter.enemies);
                validation.configure_npcs({encounter.summon});
            }
            std::osyncstream(std::cout)<<"QUEST corpse_catalog_validated encounters="<<npcs.corpse_encounters.size()<<std::endl;
        }
        if(settings.quest_waves){
            std::ifstream waves(*settings.quest_waves);
            if(!waves)throw std::invalid_argument("could not open quest wave catalog");
            npcs.wave_encounters=fw::gw::world::load_quest_waves(waves,std::filesystem::absolute(*settings.quest_waves).parent_path(),hash);
            for(const auto& [quest,encounter]:npcs.wave_encounters){
                if(npcs.kill_encounters.contains(quest)||npcs.corpse_encounters.contains(quest)||npcs.escort_encounters.contains(quest))throw std::invalid_argument("quest has conflicting encounters");
                if(!gwserver::NetworkManager::current().quest_catalog().contains(quest))throw std::invalid_argument("wave encounter lacks configured quest content");
                const auto mesh=navmeshes.find(encounter.map);
                if(mesh==navmeshes.end())throw std::invalid_argument("quest wave encounter has no navigation");
                for(const auto& group:encounter.waves){
                    fw::gw::world::Instance validation;validation.configure_navigation(mesh->second);validation.configure_npcs(group);
                }
            }
            std::osyncstream(std::cout)<<"QUEST wave_catalog_validated encounters="<<npcs.wave_encounters.size()<<std::endl;
        }
    }
    // Optional explicit development supplement beside the configured base
    // catalog. Keep base capture hashes intact and reject invalid placement;
    // a missing/unsupported row never becomes a collision-free fallback NPC.
    if(build==37130&&!settings.npcs.empty()){
        const auto directory=std::filesystem::absolute(settings.npcs).parent_path()/"ai";
        for(const bool melee:{false,true}){
            const std::string name=melee?"raider-development":"firestarter-development";
            const unsigned first_actor=melee?59001:59000;
            const unsigned population_size=melee?5:1;
            const auto appearance_path=directory/(name+"-npcs.txt");
            if(std::filesystem::exists(appearance_path))try{
                std::ifstream appearance_file(appearance_path,std::ios::binary);
                if(!appearance_file)throw std::invalid_argument("appearance_unreadable");
                const std::string bytes((std::istreambuf_iterator<char>(appearance_file)),{});
                std::istringstream appearance(bytes);auto supplement=fw::gw::world::load_npc_catalog(appearance);
                std::ifstream metadata(directory/(name+"-combat.txt"));
                if(!metadata)throw std::invalid_argument("combat_unreadable");
                const auto hash=quest_content_hash(bytes);
                fw::gw::world::bind_combat_catalog(supplement,fw::gw::combat::npcs::load_catalog(metadata,hash),hash);
                if(supplement.maps.size()!=1||supplement.for_map(146).size()!=population_size)throw std::invalid_argument("unexpected_population");
                auto proposed=npcs.for_map(146);
                for(const auto& npc:supplement.for_map(146)){
                const auto& d=npc.combat_definition;
                if(npc.id<first_actor||npc.id>=first_actor+population_size||npc.definition.id!=npc.id||npc.definition.model_file!=(melee?141267u:116226u)||
                   npc.definition.level!=(melee?2:1)||npc.definition.flags!=12||npc.model_files!=std::vector<std::uint32_t>{melee?116647u:116642u}||
                   !d||d->relation!=fw::gw::combat::npcs::Relation::hostile_to_players||!d->spawn_health||
                   d->spawn_health->maximum!=(melee?120u:100u)||
                   (melee?(bool(d->spawn_energy)||bool(d->energy_pips)):
                           (d->spawn_energy!=fw::gw::combat::npcs::Energy{40,40}||d->energy_pips!=4))||
                   d->physical_armor!=0||d->earth_armor!=0||d->fire_armor!=0||d->cold_armor!=0||
                   d->lightning_armor!=0||d->general_armor!=0||d->fleshy||d->creature_type||
                   d->holy_traits.light_sensitive||d->holy_traits.summoned||d->knockdown_allowed||
                   d->healing_allowed||d->disease_immune||d->servant_of_abaddon||d->conditions_allowed)
                    throw std::invalid_argument("unexpected_identity_or_policy");
                const auto mesh=navmeshes.find(146);
                if(mesh==navmeshes.end())throw std::invalid_argument("navigation_missing");
                if(!mesh->second->locate_cell(npc.plane,{npc.x,npc.y}))throw std::invalid_argument("creation_coordinate_not_walkable");
                for(const auto& existing:proposed)if(existing.id==npc.id||existing.definition.id==npc.definition.id||
                    (existing.definition.model_file==npc.definition.model_file&&existing.x==npc.x&&existing.y==npc.y&&existing.plane==npc.plane))
                    throw std::invalid_argument("population_identity_or_placement_collision");
                proposed.push_back(npc);
                }
                const auto mesh=navmeshes.find(146);
                if(mesh==navmeshes.end())throw std::invalid_argument("navigation_missing");
                fw::gw::world::Instance validation;validation.configure_navigation(mesh->second);validation.configure_npcs(proposed);
                npcs.maps[146]=std::move(proposed);
                for(const auto& npc:supplement.for_map(146))
                std::osyncstream(std::cout)<<"WORLD population_catalog_validated map=146 actor="<<npc.id<<" policy="<<name<<" navigation=walkable population_sha256="<<hash<<std::endl;
            }catch(const std::exception& error){
                std::osyncstream(std::cout)<<"WORLD population_catalog_refused map=146 policy="<<name<<" reason="<<error.what()<<std::endl;
            }else{
                std::osyncstream(std::cout)<<"WORLD population_catalog_disabled policy="<<name<<" reason=supplement_absent"<<std::endl;
            }
        }
    }

    fw::gw::auth::AccountInfoProfile account_profile;
    const fw::gw::auth::AccountInfoProfile* account_profile_ptr=nullptr;
    if (!settings.profile.empty()) {
        std::ifstream profile(settings.profile,std::ios::binary);
        if(!profile)throw std::invalid_argument("could not open GW account profile");
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(profile)),{});
        account_profile=fw::gw::auth::AccountInfoProfile::decode(bytes);
        account_profile_ptr=&account_profile;
    }
    if (sodium_init() < 0) throw std::runtime_error("random source unavailable");
    std::ifstream file(settings.key, std::ios::binary);
    std::array<std::uint8_t, 264> blob{};
    if (!file.read(reinterpret_cast<char*>(blob.data()), blob.size()) || file.peek() != EOF)
        throw std::invalid_argument("private key file must be 264 bytes");
    fw::gw::auth::DiffieHellman key(blob); sodium_memzero(blob.data(), blob.size());
    fw::tcp::Runtime network;
    sockaddr_in world_endpoint{}; world_endpoint.sin_family=AF_INET;
    world_endpoint.sin_port=htons(static_cast<uint16_t>(port+1));
    world_endpoint.sin_addr=resolve_ipv4(public_world_host);
    // Bind auth and world on the same configured interface. The portal remains
    // loopback-only; this is the client-facing game ingress.
    fw::tcp::Listener listener(world_bind_host, static_cast<uint16_t>(port));
    fw::tcp::Listener world_listener(world_bind_host, static_cast<uint16_t>(port + 1));
    std::list<std::future<void>> workers;
    Instances worlds;
    // Reconnect tickets retain world leases, so destroy them before Instances.
    TransferRegistry transfers(npcs);
    struct Join { std::list<std::future<void>>& workers; ~Join() { stopping = true; for (auto& worker : workers) if (worker.valid()) worker.wait(); } } join{workers};
    ready = true;
    struct ClearReady { ~ClearReady() { ready = false; } } clear_ready;
    std::osyncstream(std::cout) << "READY GW auth " << world_bind_host << ':' << port << " world " << world_bind_host << ':' << (port + 1)
              << " workers=" << max_connection_workers << " navmeshes=" << navmeshes.size()
              << " npc_maps=" << npcs.maps.size()
              << " account_profile=" << (account_profile_ptr?"loaded":"unset") << '\n' << std::flush;
    while (!stopping) {
        transfers.prune();
        for (auto it = workers.begin(); it != workers.end();) {
            if (it->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) { it->get(); it = workers.erase(it); } else ++it;
        }
        std::string address; auto socket = listener.accept(address, 100);
        if (socket && workers.size() < max_connection_workers) workers.push_back(std::async(std::launch::async, [&, socket = std::move(socket)]() mutable {
            ConnectionLease connection;
            try { handle(SOCKET(socket.get()), key, world_endpoint, transfers, account_profile_ptr,build,std::filesystem::path(settings.key).parent_path()); }
            catch (const std::exception& error) {
                std::osyncstream(std::cout) << "AUTH handler error=" << error.what() << std::endl;
            }
            catch (...) { std::osyncstream(std::cout) << "AUTH handler error=unknown" << std::endl; }
        }));
        std::string world_address; auto world_socket = world_listener.accept(world_address, 10);
        if (world_socket && workers.size() < max_connection_workers) workers.push_back(std::async(std::launch::async, [&, socket = std::move(world_socket), peer = std::move(world_address)]() mutable {
            ConnectionLease connection;
            std::osyncstream(std::cout) << "WORLD connection accepted peer=" << peer << std::endl;
            try { handle_game(SOCKET(socket.get()), key, world_endpoint, transfers,worlds,npcs,navmeshes,portals,build,std::filesystem::path(settings.key).parent_path()); }
            catch (const std::exception& error) { std::osyncstream(std::cout) << "WORLD handler error=" << error.what() << std::endl; }
            catch (...) { std::osyncstream(std::cout) << "WORLD handler error=unknown" << std::endl; }
        }));
    }
    return 0;
} catch (const std::exception& error) { std::cerr << "GW auth: " << error.what() << '\n'; return 2; }









