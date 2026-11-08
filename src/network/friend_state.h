#pragma once
#include "account_policy.h"
#include <sodium.h>
namespace gwserver {
struct FriendEntry {Identity account;unsigned type;std::string alias,character;};
struct FriendsResult {std::uint32_t status;std::string message;std::vector<FriendEntry> entries;};
struct FriendTarget {Identity owner,target;};
struct AddFriend {Identity owner;unsigned type;std::string name,alias;};
struct FriendAddition {Identity target;std::string alias;};
using FriendAddResult=CharacterStateResult<FriendAddition>;
struct FriendRemoveResult {std::uint32_t status;std::string message;};
inline std::string friend_hex(const std::string& text){return storage_hex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()));}
inline std::string storage_payload(const AddFriend& v){
    if(v.type!=1&&v.type!=2)throw std::invalid_argument("invalid friend list type");
    fw::gw::character_name_to_utf16(v.name,19);
    if(!v.alias.empty())fw::gw::character_name_to_utf16(v.alias,19);
    return storage_hex(v.owner.bytes)+"\t"+std::to_string(v.type)+"\t"+friend_hex(v.name)+"\t"+friend_hex(v.alias);
}
inline std::string storage_payload(const FriendTarget& v){return storage_hex(v.owner.bytes)+"\t"+storage_hex(v.target.bytes);}
inline std::string friend_text(const std::string& hex,bool empty=false){
    if(hex.empty()&&empty)return {};
    if(hex.empty()||hex.size()%2||hex.size()>152)throw std::invalid_argument("friend text bounds");
    std::string text(hex.size()/2,'\0');
    if(sodium_hex2bin(reinterpret_cast<unsigned char*>(text.data()),text.size(),hex.data(),hex.size(),nullptr,nullptr,nullptr))throw std::invalid_argument("friend text encoding");
    fw::gw::character_name_to_utf16(text,19);return text;
}
inline FriendsResult decode_friends(const fw::AccountResult& response){
    if(response.status!=FW_OK)return {response.status,response.payload,{}};
    try{
        const auto fields=fw::split(response.payload);unsigned count=0;
        if(fields.empty())throw std::invalid_argument("friend framing");
        const auto parsed=std::from_chars(fields[0].data(),fields[0].data()+fields[0].size(),count);
        if(fields[0].empty()||parsed.ec!=std::errc{}||parsed.ptr!=fields[0].data()+fields[0].size()||count>110||fields.size()!=1+4*count)throw std::invalid_argument("friend framing");
        FriendsResult result{FW_OK,{},{}};std::set<std::string> seen;unsigned friends=0,ignored=0;
        for(unsigned i=0;i<count;++i){const auto at=1+4*i;
            auto id=Identity::from_hex(fields[at]);
            if(!seen.insert(fields[at]).second||(fields[at+1]!="1"&&fields[at+1]!="2"))throw std::invalid_argument("friend identity/type");
            unsigned type=fields[at+1]=="1"?1:2;
            if((type==1&&++friends>100)||(type==2&&++ignored>10))throw std::invalid_argument("friend list capacity");
            result.entries.push_back({id,type,friend_text(fields[at+2]),friend_text(fields[at+3],true)});
        }return result;
    }catch(const std::exception&){return {FW_INTERNAL,"invalid friend storage response",{}};}
}
inline FriendAddResult decode_friend_add(const fw::AccountResult& r,const AddFriend& request){
    if(r.status!=FW_OK)return {r.status,r.payload,{}};
    try{
        const auto fields=fw::split(r.payload);
        if(fields.size()!=2)throw std::invalid_argument("friend addition framing");
        auto id=Identity::from_hex(fields[0]);auto alias=friend_text(fields[1]);
        if(id.bytes==request.owner.bytes||alias!=(request.alias.empty()?request.name:request.alias))throw std::invalid_argument("friend addition identity");
        return {FW_OK,{},FriendAddition{id,std::move(alias)}};
    }catch(const std::exception&){return {FW_INTERNAL,"invalid friend addition response",{}};}
}
inline FriendRemoveResult decode_friend_remove(const fw::AccountResult& r){
    if(r.status!=FW_OK)return {r.status,r.payload};
    if(r.payload!="removed")return {FW_INTERNAL,"invalid friend removal response"};
    return {FW_OK,{}};
}
using FriendBlockedResult=CharacterStateResult<bool>;
inline FriendBlockedResult decode_friend_blocked(const fw::AccountResult& r){
    if(r.status!=FW_OK)return {r.status,r.payload,{}};
    if(r.payload=="0"||r.payload=="1")return {FW_OK,{},r.payload=="1"};
    return {FW_INTERNAL,"invalid ignore storage response",{}};
}
}
