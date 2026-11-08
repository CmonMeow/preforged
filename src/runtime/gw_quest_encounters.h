#pragma once
#include "gw_npc_catalog.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <functional>

namespace fw::gw::world {
using WaveEncounter=NpcCatalog::WaveEncounter;
inline bool valid_escort_encounter(const NpcCatalog::EscortEncounter& encounter){
    if(!encounter.map||encounter.map>65535||encounter.blockers.size()!=3||encounter.follower.empty()||encounter.recipient.empty()||encounter.follower==encounter.recipient)return false;
    if(!std::isfinite(encounter.speed)||encounter.speed<=0||encounter.speed>1000)return false;
    for(auto range:{encounter.discover_range,encounter.follow_range,encounter.arrival_range})if(!std::isfinite(range)||range<=0||range>1248)return false;
    for(const auto& npc:encounter.blockers){const auto& d=npc.combat_definition;
        if(!d||d->binding.map!=encounter.map||d->relation!=combat::npcs::Relation::hostile_to_players||!d->damageable||!d->spawn_health||d->spawn_health->current<=0||npc.xunlai_storage||npc.guild_emblemer||npc.guild_trim_vendor)return false;
    }
    return true;
}
inline std::optional<std::pair<skills::effects::Scope,skills::effects::Scope>> escort_participants(Instance& world,const NpcCatalog& population,const NpcCatalog::EscortEncounter& encounter){
    std::optional<skills::effects::Scope> follower,recipient;
    for(const auto& npc:population.for_map(encounter.map)){
        if(npc.captured_name!=encounter.follower&&npc.captured_name!=encounter.recipient)continue;
        const auto scope=world.effect_scope(npc.id);if(!scope)return {};
        auto& selected=npc.captured_name==encounter.follower?follower:recipient;if(selected)return {};selected=scope;
    }
    if(!follower||!recipient)return {};return std::pair{*follower,*recipient};
}
inline bool accept_escort_encounter(Instance& world,skills::effects::Scope owner,skills::effects::Scope follower,skills::effects::Scope recipient,unsigned map,unsigned cycle,
    const NpcCatalog::EscortEncounter& encounter,const Instance::EncounterCommit& commit){
    if(map!=encounter.map||!valid_escort_encounter(encounter))return false;
    return world.spawn_escort_encounter(owner,follower,recipient,cycle,encounter.blockers,commit);
}
inline void advance_escort_encounter(Instance& world,skills::effects::Scope owner,unsigned cycle,const NpcCatalog::EscortEncounter& encounter,std::uint64_t now){
    if(!valid_escort_encounter(encounter))return;
    world.discover_escort(owner,cycle,encounter.discover_range,now);
    world.follow_escort(owner,cycle,encounter.speed,encounter.follow_range,now);
    world.arrive_escort(owner,cycle,encounter.arrival_range,now);
}
inline bool valid_corpse_encounter(const NpcCatalog::CorpseEncounter& encounter){
    if(!encounter.map||encounter.map>65535||encounter.enemies.empty()||encounter.enemies.size()>64||
       !std::isfinite(encounter.approach_speed)||encounter.approach_speed<=0||encounter.approach_speed>1000||
       !std::isfinite(encounter.claim_range)||encounter.claim_range<=0||encounter.claim_range>1248)return false;
    const auto& summon=encounter.summon;const auto& d=summon.combat_definition;
    if(!d||d->binding.map!=encounter.map||summon.xunlai_storage||summon.guild_emblemer||summon.guild_trim_vendor||
       (d->relation!=combat::npcs::Relation::protected_ally&&d->relation!=combat::npcs::Relation::neutral))return false;
    for(const auto& enemy:encounter.enemies){const auto& metadata=enemy.combat_definition;
        if(!metadata||metadata->binding.map!=encounter.map||metadata->relation!=combat::npcs::Relation::hostile_to_players||
           !metadata->damageable||!metadata->spawn_health||metadata->spawn_health->current<=0||enemy.xunlai_storage||enemy.guild_emblemer||enemy.guild_trim_vendor)return false;
    }
    try{Instance validation;validation.configure_npcs(encounter.enemies);validation.configure_npcs({summon});}catch(const std::exception&){return false;}
    return true;
}
inline bool accept_corpse_encounter(Instance& world,skills::effects::Scope owner,skills::effects::Scope trainer,
    unsigned current_map,unsigned cycle,const NpcCatalog::CorpseEncounter& encounter,const Instance::EncounterCommit& commit){
    if(!current_map||!cycle||!commit||!valid_corpse_encounter(encounter)||world.effect_scope(owner.actor)!=owner)return false;
    if(current_map!=encounter.map)return commit();
    return world.spawn_corpse_encounter(owner,trainer,cycle,encounter.enemies,commit);
}
inline Instance::CorpseAdvance advance_corpse_encounter(Instance& world,skills::effects::Scope owner,unsigned cycle,
    const NpcCatalog::CorpseEncounter& encounter,std::uint64_t now,const Instance::EncounterCommit& commit){
    if(!valid_corpse_encounter(encounter))return Instance::CorpseAdvance::unavailable;
    world.follow_corpse_trainer(owner,cycle,encounter.approach_speed,now);
    return world.advance_corpse_encounter(owner,cycle,encounter.summon,encounter.approach_speed,encounter.claim_range,now,commit);
}
// A summon appearance is independently revision-bound, just like enemies.
// This does not infer health, armor or damage from its visible level/model.
inline Instance::Npc load_quest_summon(const std::filesystem::path& appearance,const std::filesystem::path& combat_file,
    unsigned map,std::string_view expected,const std::function<std::string(std::string_view)>& hash){
    std::ifstream file(appearance,std::ios::binary);if(!file||!hash)throw std::invalid_argument("summon appearance unavailable");
    const std::string bytes((std::istreambuf_iterator<char>(file)),{});
    if(hash(bytes)!=expected)throw std::invalid_argument("summon population hash mismatch");
    std::istringstream input(bytes);auto population=load_npc_catalog(input);
    if(population.maps.size()!=1||population.for_map(map).size()!=1)throw std::invalid_argument("summon map/count mismatch");
    std::ifstream metadata_file(combat_file);if(!metadata_file)throw std::invalid_argument("summon combat metadata unavailable");
    const auto metadata=combat::npcs::load_catalog(metadata_file,expected);bind_combat_catalog(population,metadata,expected);
    auto summon=population.for_map(map).front();
    if(!summon.combat_definition||summon.xunlai_storage||summon.guild_emblemer||summon.guild_trim_vendor||
       (summon.combat_definition->relation!=combat::npcs::Relation::protected_ally&&summon.combat_definition->relation!=combat::npcs::Relation::neutral))
        throw std::invalid_argument("summon lacks reviewed friendly relation");
    return summon;
}
// Acceptance may occur in a different zone from the objective. Commit the
// durable quest there; world entry restores its encounter only in its own map.
inline bool accept_kill_encounter(Instance& world,skills::effects::Scope owner,
    std::uint32_t current_map,std::uint32_t quest,std::uint32_t cycle,
    const NpcCatalog::KillEncounter& encounter,const Instance::EncounterCommit& commit){
    if(!current_map||!quest||!cycle||!encounter.map||encounter.actors.empty()||!commit||
       world.effect_scope(owner.actor)!=owner)return false;
    if(current_map==encounter.map)
        return world.spawn_kill_encounter(owner,quest,cycle,encounter.actors,commit,false,{},{},false,0,encounter.any_death,{},{},encounter.required);
    return commit();
}
// Host-owned content: kill QUEST MAP APPEARANCE_FILE COMBAT_FILE POPULATION_SHA256.
// Numeric actor IDs in these files are templates and are remapped at activation.
inline std::map<std::uint32_t,NpcCatalog::KillEncounter> load_quest_encounters(
    std::istream& input,const std::filesystem::path& directory,
    const std::function<std::string(std::string_view)>& hash,
    std::map<std::uint32_t,NpcCatalog::KillEncounter>* loot_output=nullptr){
    std::map<std::uint32_t,NpcCatalog::KillEncounter> result;
    std::map<std::uint32_t,NpcCatalog::KillEncounter> loot;bool version2=false,version3=false,version4=false;
    std::string line;bool header=false;std::size_t line_number=0;
    while(std::getline(input,line)){
        ++line_number;
        try{
            if(line.size()>4096)throw std::invalid_argument("encounter row too long");
            const auto first=line.find_first_not_of(" \t\r");
            if(first==line.npos||line[first]=='#')continue;
            std::istringstream row(line);
            if(!header){std::string magic,version,extra;row>>magic>>version;
                if(magic!="gw-quest-encounters"||(version!="1"&&version!="2"&&version!="3"&&version!="4")||(row>>extra))throw std::invalid_argument("invalid encounter header");
                version4=version=="4";version3=version=="3"||version4;version2=version=="2"||version3;
                header=true;continue;}
            std::string kind,appearance,combat_file,expected,extra;std::uint32_t quest=0,map=0;
            if(!(row>>kind>>quest>>map>>std::quoted(appearance)>>std::quoted(combat_file)>>expected)||
               (kind!="kill"&&!(kind=="kill-any"&&version3)&&!(kind=="kill-count"&&version4)&&!(kind=="scale"&&version2&&loot_output&&quest==83))||!quest||!map||map>65535||result.contains(quest)||loot.contains(quest)||result.size()+loot.size()>=64)
                throw std::invalid_argument("invalid or duplicate encounter");
            unsigned required=0;
            if(kind=="kill-count"&&(!(row>>required)||!required||required>64))throw std::invalid_argument("invalid counted encounter target");
            if(row>>extra)throw std::invalid_argument("trailing encounter fields");
            std::ifstream population_file(directory/appearance,std::ios::binary);
            if(!population_file)throw std::invalid_argument("encounter appearance file unavailable");
            const std::string bytes((std::istreambuf_iterator<char>(population_file)),{});
            if(!hash||hash(bytes)!=expected)throw std::invalid_argument("encounter population hash mismatch");
            std::istringstream population_input(bytes);auto population=load_npc_catalog(population_input);
            if(population.maps.size()!=1||!population.maps.contains(map)||population.for_map(map).empty()||population.for_map(map).size()>64)
                throw std::invalid_argument("encounter population map/count mismatch");
            if(required>population.for_map(map).size())throw std::invalid_argument("insufficient counted encounter actors");
            std::ifstream combat_input(directory/combat_file);
            if(!combat_input)throw std::invalid_argument("encounter combat file unavailable");
            const auto metadata=combat::npcs::load_catalog(combat_input,expected);
            bind_combat_catalog(population,metadata,expected);
            for(const auto& actor:population.for_map(map)){
                const auto& d=actor.combat_definition;
                if(!d||d->relation!=combat::npcs::Relation::hostile_to_players||!d->damageable||!d->spawn_health)
                    throw std::invalid_argument("kill encounter lacks explicit hostile living state");
            }
            Instance validation;validation.configure_npcs(population.for_map(map));
            (kind=="scale"?loot:result).emplace(quest,NpcCatalog::KillEncounter{map,population.for_map(map),kind=="kill-any",required});
        }catch(const std::exception& e){throw std::invalid_argument("encounter line "+std::to_string(line_number)+": "+e.what());}
    }
    if(!header||input.bad())throw std::invalid_argument("incomplete encounter catalog");
    if(loot_output)*loot_output=std::move(loot);
    return result;
}
// Separate reviewed content manifest. Contiguous one-based wave numbers are
// required; each population uses the same identity/evidence checks as kills.
inline std::map<std::uint32_t,NpcCatalog::EscortEncounter> load_quest_escorts(std::istream& input,const std::filesystem::path& directory,
    const std::function<std::string(std::string_view)>& hash){
    std::map<std::uint32_t,NpcCatalog::EscortEncounter> result;std::string line;bool header=false;unsigned number=0;
    while(std::getline(input,line)){
        ++number;try{
            if(line.size()>4096)throw std::invalid_argument("escort row too long");
            const auto first=line.find_first_not_of(" \t\r");if(first==line.npos||line[first]=='#')continue;
            std::istringstream row(line);std::string kind,extra;
            if(!header){unsigned version=0;if(!(row>>kind>>version)||kind!="gw-quest-escorts"||version!=1||(row>>extra))throw std::invalid_argument("invalid escort header");header=true;continue;}
            unsigned quest=0,map=0;std::string appearance,combat,expected,follower,recipient;float speed=0,discovery=0,follow=0,arrival=0;
            if(!(row>>kind>>quest>>map>>std::quoted(appearance)>>std::quoted(combat)>>expected>>std::quoted(follower)>>std::quoted(recipient)>>speed>>discovery>>follow>>arrival)||
               kind!="escort"||quest!=84||(row>>extra)||result.contains(quest))throw std::invalid_argument("invalid or duplicate escort row");
            std::ostringstream single;single<<"gw-quest-encounters 1\nkill "<<quest<<' '<<map<<' '<<std::quoted(appearance)<<' '<<std::quoted(combat)<<' '<<expected<<'\n';
            std::istringstream blockers(single.str());auto loaded=load_quest_encounters(blockers,directory,hash);
            NpcCatalog::EscortEncounter encounter{map,std::move(loaded.at(quest).actors),follower,recipient,speed,discovery,follow,arrival};
            if(!valid_escort_encounter(encounter))throw std::invalid_argument("invalid escort content/policy");result.emplace(quest,std::move(encounter));
        }catch(const std::exception& e){throw std::invalid_argument("escort encounter line "+std::to_string(number)+": "+e.what());}
    }
    if(!header||input.bad()||result.empty())throw std::invalid_argument("incomplete escort catalog");return result;
}
inline std::map<std::uint32_t,NpcCatalog::CorpseEncounter> load_quest_corpses(std::istream& input,const std::filesystem::path& directory,
    const std::function<std::string(std::string_view)>& hash){
    std::map<std::uint32_t,NpcCatalog::CorpseEncounter> result;std::string line;bool header=false;unsigned number=0;
    while(std::getline(input,line)){
        ++number;try{
            if(line.size()>4096)throw std::invalid_argument("corpse row too long");
            const auto first=line.find_first_not_of(" \t\r");if(first==line.npos||line[first]=='#')continue;
            std::istringstream row(line);std::string kind,extra;
            if(!header){unsigned version=0;if(!(row>>kind>>version)||kind!="gw-quest-corpses"||version!=1||(row>>extra))throw std::invalid_argument("invalid corpse header");header=true;continue;}
            unsigned quest=0,map=0;float speed=0,range=0;std::string enemies,combat,expected,summon,summon_combat,summon_hash;
            if(!(row>>kind>>quest>>map>>std::quoted(enemies)>>std::quoted(combat)>>expected>>std::quoted(summon)>>std::quoted(summon_combat)>>summon_hash>>speed>>range)||
               kind!="corpse"||quest!=82||(row>>extra)||result.contains(quest))throw std::invalid_argument("invalid or duplicate corpse row");
            std::ostringstream single;single<<"gw-quest-encounters 1\nkill "<<quest<<' '<<map<<' '<<std::quoted(enemies)<<' '<<std::quoted(combat)<<' '<<expected<<'\n';
            std::istringstream enemy_input(single.str());auto loaded=load_quest_encounters(enemy_input,directory,hash);
            NpcCatalog::CorpseEncounter encounter{map,std::move(loaded.at(quest).actors),load_quest_summon(directory/summon,directory/summon_combat,map,summon_hash,hash),speed,range};
            if(!valid_corpse_encounter(encounter))throw std::invalid_argument("invalid corpse encounter policy");result.emplace(quest,std::move(encounter));
        }catch(const std::exception& e){throw std::invalid_argument("corpse encounter line "+std::to_string(number)+": "+e.what());}
    }
    if(!header||input.bad()||result.empty())throw std::invalid_argument("incomplete corpse catalog");return result;
}
inline std::map<std::uint32_t,WaveEncounter> load_quest_waves(
    std::istream& input,const std::filesystem::path& directory,
    const std::function<std::string(std::string_view)>& hash){
    std::map<std::uint32_t,WaveEncounter> result;std::string line;bool header=false;
    std::size_t number=0;
    while(std::getline(input,line)){
        ++number;
        try{
            if(line.size()>4096)throw std::invalid_argument("wave row too long");
            const auto first=line.find_first_not_of(" \t\r");
            if(first==line.npos||line[first]=='#')continue;
            std::istringstream row(line);std::string kind,extra;
            if(!header){unsigned version=0;row>>kind>>version;
                if(kind!="gw-quest-waves"||version!=1||(row>>extra))throw std::invalid_argument("invalid wave header");
                header=true;continue;}
            unsigned quest=0,map=0,index=0;std::string appearance,combat,expected;
            if(!(row>>kind>>quest>>map>>index>>std::quoted(appearance)>>std::quoted(combat)>>expected)||
               kind!="wave"||!quest||!index||index>16||(row>>extra))throw std::invalid_argument("invalid wave row");
            auto found=result.find(quest);
            if(found==result.end()&&result.size()>=64)throw std::invalid_argument("too many wave quests");
            const auto count=found==result.end()?0:found->second.waves.size();
            if(index!=count+1||(found!=result.end()&&found->second.map!=map))
                throw std::invalid_argument("wave order/map mismatch");
            std::ostringstream single;single<<"gw-quest-encounters 1\nkill "<<quest<<' '<<map<<' '
                <<std::quoted(appearance)<<' '<<std::quoted(combat)<<' '<<expected<<'\n';
            std::istringstream record(single.str());auto loaded=load_quest_encounters(record,directory,hash);
            auto actors=std::move(loaded.at(quest).actors);
            if(found!=result.end()&&actors.size()!=found->second.waves.front().size())
                throw std::invalid_argument("wave group size mismatch");
            auto& encounter=result[quest];encounter.map=map;encounter.waves.push_back(std::move(actors));
        }catch(const std::exception& e){throw std::invalid_argument("wave line "+std::to_string(number)+": "+e.what());}
    }
    if(!header||input.bad()||result.empty())throw std::invalid_argument("incomplete wave catalog");
    return result;
}
}
