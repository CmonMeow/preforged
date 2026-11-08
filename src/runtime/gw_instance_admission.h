#pragma once
#include "gw_map_catalog_generated.h"
#include "gw_map_assets_generated.h"
#include <string_view>
namespace fw::gw {
// Preserve native request admission while making failures actionable. Ownership
// of a directed transfer is checked separately after this tuple is valid.
inline std::string_view instance_tuple_failure(unsigned transaction,unsigned map,unsigned type,
    unsigned region,unsigned district,unsigned language,bool campaign_owned){
    if(!transaction)return "missing_transaction";
    if(map>65535)return "map_id_out_of_range";
    const auto* destination=find_map(map);
    if(!destination||destination->type==255)return "unknown_or_disabled_map";
    if(destination->type!=type&&!(map==2&&type==6))return "map_type_mismatch";
    if(!map_asset_file(static_cast<std::uint16_t>(map)))return "missing_map_asset";
    if(destination->campaign&&!campaign_owned)return "campaign_not_owned";
    if(region>4&&region!=0xfffffffeu)return "invalid_region";
    if(district>65535)return "invalid_district";
    if(language!=0&&language!=2&&language!=3&&language!=4&&language!=5&&language!=9&&language!=10)return "invalid_language";
    return {};
}
}
