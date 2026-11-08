#pragma once
#include "gw_world_simulation.h"
#include "gw_npc_combat_catalog.h"
#include <charconv>
#include <istream>
#include <sstream>
namespace fw::gw::world {
struct NpcCatalog {
    struct KillEncounter {std::uint32_t map;std::vector<Instance::Npc> actors;bool any_death=false;unsigned required=0;};
    struct WaveEncounter {std::uint32_t map=0;std::vector<std::vector<Instance::Npc>> waves;};
    struct CorpseEncounter {std::uint32_t map=0;std::vector<Instance::Npc> enemies;Instance::Npc summon;float approach_speed=0,claim_range=0;};
    struct EscortEncounter {std::uint32_t map=0;std::vector<Instance::Npc> blockers;std::string follower,recipient;float speed=0,discover_range=0,follow_range=0,arrival_range=0;};
    std::map<std::uint32_t,std::vector<Instance::Npc>> maps;
    std::map<std::uint32_t,KillEncounter> kill_encounters;
    std::map<std::uint32_t,KillEncounter> loot_encounters;
    std::map<std::uint32_t,WaveEncounter> wave_encounters;
    std::map<std::uint32_t,CorpseEncounter> corpse_encounters;
    std::map<std::uint32_t,EscortEncounter> escort_encounters;
    // Alternate initial positions preserve one identity. The room creator's
    // primary profession selects its initial population; escort motion owns it
    // afterwards. Mixed-profession leader priority is development policy.
    struct PrimaryPlacement {std::uint32_t map=0,actor=0;unsigned primary=0;float x=0,y=0,facing=0;std::uint32_t plane=0;};
    std::vector<PrimaryPlacement> primary_placements;
    std::vector<Instance::Npc> for_primary(std::uint32_t map,unsigned primary)const{
        auto actors=for_map(map);
        for(const auto& place:primary_placements)if(place.map==map&&place.primary==primary)
            for(auto& actor:actors)if(actor.id==place.actor){
                actor.x=place.x;actor.y=place.y;actor.plane=place.plane;actor.facing=place.facing;
            }
        return actors;
    }
    const std::vector<Instance::Npc>& for_map(std::uint32_t map)const{
        static const std::vector<Instance::Npc> empty;
        const auto it=maps.find(map);return it==maps.end()?empty:it->second;
    }
    std::uint32_t allocate_player(std::uint32_t map,std::uint32_t& next)const{
        const auto& population=for_map(map);
        while(next&&next<=65535&&std::any_of(population.begin(),population.end(),
              [&](const auto& npc){return npc.id==next;}))++next;
        if(!next||next>65535)throw std::runtime_error("instance player ID capacity");
        return next++;
    }
};
inline std::vector<combat::npcs::Observed> combat_population(const NpcCatalog& catalog){
    std::vector<combat::npcs::Observed> result;
    for(const auto& [map,actors]:catalog.maps)for(const auto& actor:actors){
        const auto& d=actor.definition;
        result.push_back({{map,actor.id,d.id,d.model_file,d.skin_file,d.appearance,d.flags,d.level},actor.xunlai_storage||actor.guild_emblemer||actor.guild_trim_vendor});
    }
    return result;
}
inline void bind_combat_catalog(NpcCatalog& population,const combat::npcs::Catalog& combat,
                                std::string_view population_sha256){
    // Validate the whole replacement before attaching any metadata.
    const auto observed=combat_population(population);
    combat.validate_population(observed,population_sha256);
    for(auto& [map,actors]:population.maps)for(auto& actor:actors){
        const auto* row=combat.find(map,actor.id);
        actor.combat_definition=row?std::optional(*row):std::nullopt;
    }
}
// Supplemental captured quest-giver appearances leave base combat hashes intact.
// Isolated capture IDs are remapped before publishing the replacement population.
inline void merge_quest_givers(NpcCatalog& base,const NpcCatalog& supplement,const std::set<std::string>& replace_names={},bool coalesce_same_placement=false){
    auto proposed=base;
    for(const auto& [map,actors]:supplement.maps){
        auto& destination=proposed.maps[map];std::uint32_t actor_id=0,definition_id=0;
        for(const auto& npc:destination){actor_id=std::max(actor_id,npc.id);definition_id=std::max(definition_id,npc.definition.id);}
        for(auto npc:actors){
            if(npc.xunlai_storage||npc.guild_emblemer||npc.guild_trim_vendor||npc.captured_name.empty())
                throw std::invalid_argument("quest giver supplement must contain named nonservice actors");
            if(npc.combat_definition){
                const combat::npcs::Binding expected{map,npc.id,npc.definition.id,npc.definition.model_file,
                    npc.definition.skin_file,npc.definition.appearance,npc.definition.flags,npc.definition.level};
                const auto& d=*npc.combat_definition;
                if(d.binding!=expected||d.relation!=combat::npcs::Relation::neutral||
                   (d.damageable&&(!d.spawn_health||d.spawn_health->current<=0)))
                    throw std::invalid_argument("quest guardian supplement lacks reviewed neutral living binding");
            }
            const bool same_placement=coalesce_same_placement&&std::any_of(destination.begin(),destination.end(),[&](const auto& old){
                return old.captured_name==npc.captured_name&&old.x==npc.x&&old.y==npc.y&&old.plane==npc.plane;
            });
            if(replace_names.contains(npc.captured_name)||same_placement){
                unsigned matches=0;
                for(const auto& old:destination)if(old.captured_name==npc.captured_name){
                    if(++matches>1||old.xunlai_storage||old.guild_emblemer||old.guild_trim_vendor||
                       old.definition.model_file!=npc.definition.model_file||old.definition.skin_file!=npc.definition.skin_file||
                       old.definition.encoded_name!=npc.definition.encoded_name||old.model_files!=npc.model_files)
                        throw std::invalid_argument("quest placement override has ambiguous or mismatched identity");
                }
                std::erase_if(destination,[&](const auto& old){return old.captured_name==npc.captured_name;});
            }
            if(actor_id>=59999||definition_id>=65535)throw std::invalid_argument("quest giver identity capacity");
            npc.id=++actor_id;npc.definition.id=++definition_id;
            npc.literal_display_name=fw::gw::character_name_to_utf16(npc.captured_name,28);
            if(npc.combat_definition){npc.combat_definition->binding.actor=npc.id;npc.combat_definition->binding.definition=npc.definition.id;}
            destination.push_back(std::move(npc));
        }
        Instance validation;validation.configure_npcs(destination);
    }
    base=std::move(proposed);
}
inline NpcCatalog load_npc_catalog(std::istream& input){
    NpcCatalog out;std::string line;std::size_t line_number=0;bool header=false,xunlai=false,emblemer=false,trim_vendor=false;
    std::string captured_name;
    auto number=[](std::string_view text,std::uint32_t maximum=0xffffffffu,unsigned base=10){
        std::uint32_t value=0;auto result=std::from_chars(text.data(),text.data()+text.size(),value,base);
        if(result.ec!=std::errc{}||result.ptr!=text.data()+text.size()||value>maximum)
            throw std::invalid_argument("invalid unsigned field");
        return value;
    };
    auto real=[](std::string_view text){
        float value=0;auto result=std::from_chars(text.data(),text.data()+text.size(),value);
        if(result.ec!=std::errc{}||result.ptr!=text.data()+text.size()||!std::isfinite(value))
            throw std::invalid_argument("invalid coordinate or facing");
        return value;
    };
    while(std::getline(input,line)){
        ++line_number;
        try{
            // The catalog is read as binary for provenance hashing. Normalize
            // CRLF here too, before exact service-name classification.
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.size()>4096)throw std::invalid_argument("line too long");
            if(const auto comment=line.find('#');comment!=line.npos){
                const auto name=line.substr(comment+1);
                captured_name=name.starts_with(" ")?name.substr(1):name;
                xunlai=name==" Xunlai Agent [Storage]"||name==" Xunlai Chest";
                emblemer=name==" Guild Emblemer"||name.ends_with(" [Guild Emblemer]");
                trim_vendor=name==" Tolkano [Tournament]";
                line.resize(comment);
            }
            std::istringstream row(line);std::vector<std::string> fields;std::string field;
            while(row>>field)fields.push_back(field);
            if(fields.empty())continue;
            if(!header){
                if(fields!=std::vector<std::string>{"gw-npcs","1"})throw std::invalid_argument("expected gw-npcs 1 header");
                header=true;captured_name.clear();continue;
            }
            if(fields.size()!=17||fields[0]!="npc")throw std::invalid_argument("expected 17 NPC fields");
            const auto map=number(fields[1],65535);if(!map)throw std::invalid_argument("zero map ID");
            Instance::Npc npc;npc.xunlai_storage=std::exchange(xunlai,false);npc.guild_emblemer=std::exchange(emblemer,false);npc.guild_trim_vendor=std::exchange(trim_vendor,false);npc.id=number(fields[2],65535);npc.definition.id=number(fields[3],65535);
            npc.captured_name=std::exchange(captured_name,{});
            auto& d=npc.definition;d.model_file=number(fields[4]);d.skin_file=number(fields[5]);
            auto visual=number(fields[6]);for(unsigned i=0;i<4;++i)d.visual_adjustment[i]=static_cast<std::uint8_t>(visual>>(8*i));
            d.appearance=number(fields[7]);d.flags=number(fields[8]);d.primary=static_cast<std::uint8_t>(number(fields[9],255));
            d.level=static_cast<std::uint8_t>(number(fields[10],255));npc.plane=static_cast<std::uint16_t>(number(fields[11],65535));
            npc.x=real(fields[12]);npc.y=real(fields[13]);npc.facing=real(fields[14]);
            if(fields[15]!="-"){
                const auto& encoded=fields[15];if(encoded.size()%4||encoded.size()>28)throw std::invalid_argument("invalid encoded name length");
                for(std::size_t i=0;i<encoded.size();i+=4)d.encoded_name.push_back(static_cast<char16_t>(number(std::string_view(encoded).substr(i,4),65535,16)));
            }
            if(fields[16]!="-"){
                std::string_view files=fields[16];
                for(;;){
                    const auto comma=files.find(',');npc.model_files.push_back(number(files.substr(0,comma)));
                    if(npc.model_files.size()>8)throw std::invalid_argument("NPC model-file capacity");
                    if(comma==files.npos)break;files.remove_prefix(comma+1);
                }
            }
            auto& population=out.maps[map];population.push_back(std::move(npc));
            if(out.maps.size()>4096||population.size()>1024)throw std::invalid_argument("NPC catalog capacity");
        }catch(const std::exception& error){throw std::invalid_argument("NPC catalog line "+std::to_string(line_number)+": "+error.what());}
    }
    if(input.bad()||!header)throw std::invalid_argument("incomplete NPC catalog");
    for(const auto& [map,population]:out.maps){Instance validation;validation.configure_npcs(population);}
    return out;
}
}
