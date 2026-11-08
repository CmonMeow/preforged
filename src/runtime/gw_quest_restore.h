#pragma once
#include "gw_quest_state.h"
#include "gw_quest_wire.h"
#include "gw_quest_request.h"
#include <memory>
namespace fw::gw::quests {
struct Content {
    std::string source_sha256;
    Entry entry;
    // Unloaded details differ from a captured, legitimately empty string.
    std::optional<std::u16string> description,objectives;
    std::optional<std::u16string> captured_character_argument;
    std::shared_ptr<Content> completed_variant;
    std::map<unsigned,std::shared_ptr<Content>> objective_variants;
};
using Catalog=std::map<std::uint32_t,Content>;
using Batch=std::vector<items::Packet>;
inline const Content& stage_content(const Content& content,Stage stage){
    return stage==Stage::completed&&content.completed_variant?*content.completed_variant:content;
}
inline const Content& progress_content(const Content& content,const Progress& progress){
    if(progress.stage==Stage::completed)return stage_content(content,progress.stage);
    if(progress.stage==Stage::active&&!progress.objective_credits.empty()){
        const auto found=content.objective_variants.find(static_cast<unsigned>(progress.objective_credits.size()));
        if(found==content.objective_variants.end()||!found->second)
            throw std::invalid_argument("counted quest lacks captured objective variant");
        return *found->second;
    }
    return content;
}
// Build the entire batch before sending anything. Missing content is a data
// error, never permission to substitute a synthetic title or omit active quests.
inline Batch restore_2022(const Profile& profile,const Catalog& catalog){
    validate(profile);Batch packets;
    for(const auto& [id,progress]:profile.records){
        if(progress.stage==Stage::rewarded||progress.stage==Stage::abandoned)continue;
        const auto found=catalog.find(id);
        if(found==catalog.end()||found->second.entry.id!=id)
            throw std::invalid_argument("active quest lacks matching content");
        const auto& content=progress_content(found->second,progress);
        auto entry=content.entry;
        // Completion bit2 is established by the native2022 quest callback.
        // Bit1 is set by Details, rather than copied from a captured UI cache.
        entry.state&=~3u;
        if(progress.stage==Stage::completed)entry.state|=2;
        if(content.description.has_value()!=content.objectives.has_value())
            throw std::invalid_argument("partial quest detail content");
        packets.push_back(add_2022(entry));
        if(content.description)packets.push_back(details_2022(id,*content.description,*content.objectives));
    }
    return packets;
}
enum class MarkerOutcome {available,not_owned,content_missing,unresolved};
struct MarkerReply {MarkerOutcome outcome;std::optional<items::Packet> packet;};
inline MarkerReply marker_reply_2022(const Profile& profile,const Catalog& catalog,const MarkerRequest& request){
    validate(profile);const auto owned=profile.records.find(request.id);
    if(owned==profile.records.end()||(owned->second.stage!=Stage::active&&owned->second.stage!=Stage::completed))
        return {MarkerOutcome::not_owned,{}};
    const auto found=catalog.find(request.id);
    if(found==catalog.end()||found->second.entry.id!=request.id)return {MarkerOutcome::content_missing,{}};
    const auto& entry=progress_content(found->second,owned->second).entry;
    if(!valid_marker(entry.x,entry.y))throw std::invalid_argument("invalid quest marker content");
    if(!std::isfinite(entry.x))return {MarkerOutcome::unresolved,{}};
    return {MarkerOutcome::available,marker_2022(entry)};
}
}
