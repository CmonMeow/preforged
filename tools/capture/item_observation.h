#pragma once
#include <cstdint>
#include <span>
#include <string>

namespace capture {
// Installed bags do not occupy an ordinary inventory slot. Verify both the
// item-table identity and installed-bag backlink before recording a definition.
inline bool installed_container_valid(std::uint32_t item_id,unsigned type,
        unsigned quantity,bool table_matches,bool has_slot_bag,bool backlink_matches){
    return item_id&&type==3&&quantity==1&&table_matches&&!has_slot_bag&&backlink_matches;
}
// Modern-client observations, not a 2022 wire definition. Keep raw dye bytes
// and modifiers intact until the version-specific conversion is verified.
struct ItemObservation {
    std::uint32_t model=0, file=0, interaction=0;
    unsigned type=0, value=0, quantity=0, profession=0, slot=0;
    unsigned tint=0, dye1=0, dye2=0, dye3=0, dye4=0;
};
inline std::string item_json(const ItemObservation& item,
                             std::span<const std::uint32_t> modifiers,
                             bool modifiers_complete) {
    std::string result="{";
    auto field=[&](const char* name,std::uint32_t value){
        if(result.size()>1)result+=",";
        result+='"';result+=name;result+="\":";result+=std::to_string(value);
    };
    field("model",item.model);field("model_file",item.file);
    field("type",item.type);field("base_value",item.value);
    field("quantity",item.quantity);field("interaction",item.interaction);
    field("profession",item.profession);field("slot",item.slot);
    field("dye_tint",item.tint);field("dye1",item.dye1);
    field("dye2",item.dye2);field("dye3",item.dye3);field("dye4",item.dye4);
    result+=",\"modifiers_complete\":";
    result+=modifiers_complete?"true":"false";
    result+=",\"modifiers\":[";
    bool first=true;
    for(auto mod:modifiers){if(!first)result+=",";first=false;result+=std::to_string(mod);}
    return result+"]}";
}
}
