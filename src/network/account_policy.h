#pragma once
#include "character_state.h"
#include "gw_auth_responses.h"
#include <set>
namespace gwserver {
using AgreementResult=CharacterStateResult<bool>;
using AccountProfileResult=CharacterStateResult<fw::gw::auth::AccountInfoProfile>;
using GameAliasResult=CharacterStateResult<std::string>;
inline AccountProfileResult decode_account_profile(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    if(result.payload=="-")return {FW_OK,{},{}};
    try{return {FW_OK,{},fw::gw::auth::AccountInfoProfile::decode(profile_bytes(result.payload,239))};}
    catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid account profile storage response",{}};}
}
inline GameAliasResult decode_game_alias(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    if(!fw::account_name_valid(result.payload))return {FW_INTERNAL,"invalid game account alias",{}};
    return {FW_OK,{},result.payload};
}
struct CampaignsResult {std::uint32_t status;std::string message;std::set<std::uint32_t> campaigns;};
inline AgreementResult decode_agreement(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    if(result.payload!="0"&&result.payload!="1")return {FW_INTERNAL,"invalid agreement storage response",{}};
    return {FW_OK,{},result.payload=="1"};
}
inline CampaignsResult decode_campaigns(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    std::set<std::uint32_t> campaigns;
    std::string_view remaining=result.payload;
    while(!remaining.empty()){
        const auto comma=remaining.find(',');const auto token=remaining.substr(0,comma);
        // Storage emits one canonical digit for each of the four campaigns.
        if(token.size()!=1||token[0]<'1'||token[0]>'4'||!campaigns.insert(token[0]-'0').second)
            return {FW_INTERNAL,"invalid campaign storage response",{}};
        if(comma==remaining.npos)break;
        remaining.remove_prefix(comma+1);
        if(remaining.empty())return {FW_INTERNAL,"invalid campaign storage separator",{}};
    }
    return {FW_OK,{},std::move(campaigns)};
}
}
