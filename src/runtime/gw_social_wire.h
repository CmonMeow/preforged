#pragma once
#include "gw_auth_responses.h"
#include "gw_character_name.h"
#include <optional>
namespace fw::gw::social {
using Packet=std::vector<std::uint8_t>;
struct Friend {GameToken account{};unsigned type=1,status=0;std::u16string alias,character;};
inline void string(Packet& out,std::u16string_view text,unsigned capacity){
    if(text.size()>=capacity)throw std::invalid_argument("social string bound");
    if(!text.empty())character_name_to_utf8(std::u16string(text),capacity-1);
    auth::encoding::integer(out,static_cast<std::uint32_t>(text.size()),2);
    for(auto c:text)auth::encoding::integer(out,c,2);
}
// Pinned37130 descriptor613508: presence, UUID, alias20, playing20.
// 48c8d0 ->4867a0 ->event2c ->7bc380 updates an EXISTING friend by alias.
// It cannot initialize a missing roster entry.16/18 are different cached
// records: their second DWORD is a packed date, not a presence enum.
inline Packet friend_presence(const Friend& value){
    if(value.type<1||value.type>2||value.status>3||value.alias.empty())throw std::invalid_argument("friend enum/name bound");
    Packet out;auth::encoding::integer(out,11,2);
    auth::encoding::integer(out,value.status,4);
    auth::encoding::raw(out,value.account);string(out,value.alias,20);
    string(out,value.status?value.character:u"",20);return out;
}
// Source compatibility for current auth polling; the bool never selects an
// initial-roster protocol. Both calls now emit only the proven presence delta.
inline Packet friend_record(const Friend& value,bool=false){return friend_presence(value);}
// Descriptor6134fc: transaction, list type, UUID, alias20. The first
// DWORD is a pending request ID, not presence (48c870 ->486670).
inline Packet friend_change(std::uint32_t transaction,const Friend& value,unsigned type){
    if(!transaction||type>2||value.alias.empty())throw std::invalid_argument("friend reply fields");
    Packet out;auth::encoding::integer(out,10,2);auth::encoding::integer(out,transaction,4);
    auth::encoding::integer(out,type,4);auth::encoding::raw(out,value.account);string(out,value.alias,20);return out;
}
inline Packet friend_added(std::uint32_t transaction,const Friend& value){
    if(value.type<1||value.type>2)throw std::invalid_argument("friend add category");
    return friend_change(transaction,value,value.type);
}
// Event26 type0 removes an existing alias. A presence delta with status0 only
// marks it offline and cannot acknowledge the client's pending removal.
inline Packet friend_removed(std::uint32_t transaction,const Friend& value){return friend_change(transaction,value,0);}
struct Request {enum class Result{incomplete,invalid,complete};unsigned opcode=0,type=0;std::uint32_t transaction=0;GameToken account{};std::u16string name,alias;std::size_t consumed=0;};
inline Packet whisper(std::u16string_view sender,std::u16string_view message){
    if(sender.empty()||message.empty())throw std::invalid_argument("empty whisper");
    Packet out;auth::encoding::integer(out,12,2);auth::encoding::integer(out,0,4);
    string(out,sender,20);string(out,message,256);return out;
}
inline Request::Result parse(std::span<const std::uint8_t> input,Request& output){
    using Result=Request::Result;if(input.size()<2)return Result::incomplete;
    auto word=[&](std::size_t at){return unsigned(input[at])|(unsigned(input[at+1])<<8);};
    const auto opcode=word(0)&0x7fff;if(opcode!=26&&opcode!=27)return Result::invalid;
    std::size_t at=2;Request request;request.opcode=opcode;
    auto integer=[&](std::uint32_t& value){if(input.size()-at<4)return false;for(unsigned i=0;i<4;++i)value|=std::uint32_t(input[at++])<<(i*8);return true;};
    if(!integer(request.transaction))return Result::incomplete;if(!request.transaction)return Result::invalid;
    if(opcode==26){std::uint32_t type=0;if(!integer(type))return Result::incomplete;if(type<1||type>2)return Result::invalid;request.type=type;}
    else {if(input.size()-at<16)return Result::incomplete;std::copy_n(input.begin()+at,16,request.account.begin());at+=16;}
    auto text=[&](std::u16string& value,bool empty){
        if(input.size()-at<2)return Result::incomplete;const auto count=word(at);at+=2;
        if(count>19||(!count&&!empty))return Result::invalid;if(input.size()-at<count*2)return Result::incomplete;
        for(unsigned i=0;i<count;++i){value.push_back(static_cast<char16_t>(word(at)));at+=2;}
        try{if(count)character_name_to_utf8(value,19);}catch(const std::invalid_argument&){return Result::invalid;}return Result::complete;
    };
    const auto name=text(request.name,opcode==27);if(name!=Result::complete)return name;
    if(opcode==26){const auto alias=text(request.alias,true);if(alias!=Result::complete)return alias;}
    request.consumed=at;output=std::move(request);return Result::complete;
}
}
