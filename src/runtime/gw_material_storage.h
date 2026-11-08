#pragma once
#include "gw_item_wire.h"
#include <array>
#include <optional>
#include <algorithm>
#include <type_traits>

namespace fw::gw::inventory::materials {
// Pinned37130:318 forces material bag type5 to native index5. The public
// Inventory::bags[6] includes an unused array0; it is NOT a wire bag index.
inline constexpr unsigned bag_type=5,bag_index=5,bag_capacity=41;
inline constexpr unsigned ordinary_stack=250,default_capacity=250,maximum_capacity=2500;
struct Row {unsigned model,element,display,name_string_id;};
// Model association: local pre-Reforged GWCA ItemMgr.cpp517..557. Element
// and display columns are independently checked against native table97df08.
// Gaps7/28 and Zaishen coins38..40 are not crafting-material storage slots.
inline constexpr std::array<Row,36> rows{{
    {921,0,7,2527},{948,1,0,2554},{940,2,2,2546},{953,3,5,2559},
    {954,4,6,2560},{925,5,1,2531},{946,6,3,2551},{955,8,8,2561},
    {929,9,4,2535},{934,10,9,2540},{933,11,10,2539},{941,12,11,2547},
    {926,13,12,2532},{927,14,13,2533},{928,15,14,2534},{930,16,15,2536},
    {949,17,16,2555},{950,18,17,2556},{923,19,18,2529},{931,20,19,2537},
    {932,21,20,2538},{937,22,22,2543},{938,23,23,2544},{935,24,21,2541},
    {936,25,35,2542},{922,26,27,2528},{945,27,24,2553},{939,29,28,2545},
    {942,30,29,2548},{943,31,30,2549},{944,32,31,2550},{951,33,32,2557},
    {952,34,33,2558},{956,35,34,2562},{6532,36,25,27624},{6533,37,26,27625}
}};
inline constexpr std::optional<Row> slot_for_model(unsigned model){
    for(const auto& row:rows)if(row.model==model)return row;return {};
}
inline constexpr bool crafting_slot(unsigned element){
    for(const auto& row:rows)if(row.element==element)return true;return false;
}
// Original7c38e0 reads property0x250, default41; matching modern GWCA calls
// the exact0x2508 modifier Arg1. Require that value to agree with the reviewed
// model rather than reclassifying unknown/coin/ambiguous definitions.
inline std::optional<Row> element(const items::Definition& definition){
    if(definition.type!=11)return {};
    const auto row=slot_for_model(definition.model);if(!row)return {};
    std::optional<unsigned> found;
    for(const auto modifier:definition.modifiers){
        if(((modifier>>20)&0x3ff)!=0x250)continue;
        if(found||(modifier>>16)!=0x2508)return {};
        found=(modifier>>8)&0xff;
    }
    if(!found||*found!=row->element)return {};return row;
}
// A reviewed quota is supplied by the owning storage policy. This function
// does not infer purchase counts, charge the unlock fee, or grant expansions.
// The2021 wiki establishes increments250 and cap2500; native312 narrows to
// ushort, so every published item count must additionally remain <=65535.
inline constexpr bool valid_capacity(unsigned capacity){
    return capacity>=default_capacity&&capacity<=maximum_capacity&&capacity%default_capacity==0;
}
inline items::Packet bag_2022(std::uint16_t inventory,std::uint16_t bag){
    return items::bag_2022(inventory,bag_type,bag_index,bag,bag_capacity,0);
}
template<class Item>inline bool compatible(const Item& source,const Item& destination){
    const auto& a=source.definition;const auto& b=destination.definition;
    return source.profession==destination.profession&&source.customized==destination.customized&&
        (a.interaction&0x80000)&&a.file==b.file&&a.type==b.type&&a.unknown1==b.unknown1&&a.dye==b.dye&&a.materials==b.materials&&a.unknown2==b.unknown2&&
        a.interaction==b.interaction&&a.price==b.price&&a.model==b.model&&a.encoded_name==b.encoded_name&&a.modifiers==b.modifiers;
}
enum class Reason {accepted,definition,material,quantity,capacity,incompatible,full,identity};
struct Limits {unsigned source=ordinary_stack,destination=default_capacity;};
struct Transfer {
    Reason status;unsigned moved=0,source_after=0,destination_after=0,element=bag_capacity;
    bool accepted()const{return status==Reason::accepted;}
};
// Pure count proposal: caller owns bag/slot/chest/direction authorization,
// wire identity allocation, durable two-domain CAS and packet publication.
// Neither input mutates; quantities and exact metadata are retained until CAS
// succeeds. Native material slot is Row::element, never Row::display.
template<class Item>inline Transfer transfer(const Item& source,const std::type_identity_t<std::optional<Item>>& destination,
                         unsigned requested,Limits limits,bool allow_partial=true){
    const unsigned before=destination?destination->definition.quantity:0;
    const auto reject=[&](Reason reason){return Transfer{reason,0,source.definition.quantity,before};};
    if(!valid_capacity(limits.source)||!valid_capacity(limits.destination))return reject(Reason::capacity);
    try{
        (void)items::definition_2022(source.definition);
        if(source.profession&&*source.profession>11||source.customized&&
           (source.customized->size()>31||source.customized->find(u'\0')!=source.customized->npos))return reject(Reason::definition);
        if(destination){(void)items::definition_2022(destination->definition);
            if(destination->profession&&*destination->profession>11||destination->customized&&
               (destination->customized->size()>31||destination->customized->find(u'\0')!=destination->customized->npos))return reject(Reason::definition);}
    }catch(const std::invalid_argument&){return reject(Reason::definition);}
    const auto source_element=element(source.definition);
    if(!source_element||!(source.definition.interaction&0x80000))return reject(Reason::material);
    if(!requested||requested>source.definition.quantity||source.definition.quantity>limits.source||before>limits.destination)
        return reject(Reason::quantity);
    if(destination){
        if(destination->definition.id==source.definition.id)return reject(Reason::identity);
        const auto target_element=element(destination->definition);
        if(!target_element||target_element->element!=source_element->element||!compatible(source,*destination))
            return reject(Reason::incompatible);
    }
    const auto space=limits.destination-before;
    if(!space||!allow_partial&&requested>space)return reject(Reason::full);
    const auto moved=std::min(requested,space);
    return {Reason::accepted,moved,source.definition.quantity-moved,before+moved,source_element->element};
}
}
