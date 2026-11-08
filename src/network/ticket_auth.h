#pragma once
#include "character_requests.h"
#include "accounts.h"
#include "common.h"
#include "gw_token.h"
#include "framework/module.h"
#include <charconv>
namespace gwserver {
struct SessionToken {
    std::array<std::uint8_t,32> bytes{};
    static SessionToken from_hex(std::string_view text){
        if(text.size()!=64)throw std::invalid_argument("session requires32 bytes");
        SessionToken token;
        const auto first=Identity::from_hex(text.substr(0,32)),second=Identity::from_hex(text.substr(32));
        std::copy(first.bytes.begin(),first.bytes.end(),token.bytes.begin());
        std::copy(second.bytes.begin(),second.bytes.end(),token.bytes.begin()+16);return token;
    }
};
struct IssueTicket {SessionToken session;};
struct IssuedTicket {fw::gw::GameToken token;std::uint64_t expires;};
struct TicketIssuanceResult {std::uint32_t status;std::string message;std::optional<IssuedTicket> ticket;};
inline TicketIssuanceResult decode_ticket_issuance(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    const auto fields=fw::split(result.payload);
    if(fields.size()!=2)return {FW_INTERNAL,"invalid ticket issuance storage response",{}};
    const auto token=fw::gw::parse_token(fields[0]);std::uint64_t expires=0;
    const auto parsed=std::from_chars(fields[1].data(),fields[1].data()+fields[1].size(),expires);
    if(!token||fields[1].empty()||parsed.ec!=std::errc{}||parsed.ptr!=fields[1].data()+fields[1].size()||!expires||expires>INT64_MAX)
        return {FW_INTERNAL,"invalid ticket issuance storage response",{}};
    return {FW_OK,{},IssuedTicket{*token,expires}};
}
struct AuthenticateTicket { fw::gw::GameToken ticket;Identity account;std::string alias,character; };
// Internal ownership handle, never sent to the game client or usable as a
// bearer login token. Authentication transfers cleanup to the auth connection.
struct GameSessionLease {Identity account;SessionToken session_hash;};
struct AuthenticatedGameAccount {
    Identity account;std::string name;std::uint64_t session_expires;
    std::optional<Identity> character;std::string character_name;std::uint16_t map;unsigned roster_size;
    std::optional<GameSessionLease> lease;
};
struct TicketAuthenticationResult {std::uint32_t status;std::string message;std::optional<AuthenticatedGameAccount> identity;};
inline std::string storage_payload(const AuthenticateTicket& value) {
    if(!value.alias.empty()&&!fw::account_name_valid(value.alias))throw std::invalid_argument("invalid game alias");
    if(!value.character.empty())(void)fw::gw::character_name_to_utf16(value.character,19);
    return fw::gw::format_token(value.ticket)+"\t"+fw::gw::format_token(value.account.bytes)+"\t"+value.alias+"\t"+value.character;
}
inline TicketAuthenticationResult decode_ticket_authentication(const fw::AccountResult& result,const AuthenticateTicket& request) {
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    try {
        const auto fields=fw::split(result.payload);
        if(fields.size()!=8)throw std::invalid_argument("game identity framing");
        const auto decimal=[](const std::string& text,std::uint64_t max){
            std::uint64_t value=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
            if(text.empty()||parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||value>max)throw std::invalid_argument("game identity number");return value;
        };
        AuthenticatedGameAccount identity{Identity::from_hex(fields[0]),fields[1],decimal(fields[2],INT64_MAX),{},fields[4],static_cast<std::uint16_t>(decimal(fields[5],65535)),static_cast<unsigned>(decimal(fields[6],8))};
        if(identity.account.bytes!=request.account.bytes||!fw::account_name_valid(identity.name)||!identity.session_expires||!identity.map)
            throw std::invalid_argument("game identity claim");
        identity.lease=GameSessionLease{identity.account,SessionToken::from_hex(fields[7])};
        if(fields[3].empty()!=fields[4].empty()||fields[3].empty()!=request.character.empty())throw std::invalid_argument("game selection shape");
        if(!fields[3].empty()){
            identity.character=Identity::from_hex(fields[3]);
            (void)fw::gw::character_name_to_utf16(identity.character_name,19);
            if(!identity.roster_size||identity.character_name.size()!=request.character.size()||!std::equal(identity.character_name.begin(),identity.character_name.end(),request.character.begin(),[](unsigned char a,unsigned char b){if(a>='A'&&a<='Z')a+=32;if(b>='A'&&b<='Z')b+=32;return a==b;}))throw std::invalid_argument("game selection mismatch");
        }
        return {FW_OK,{},std::move(identity)};
    } catch(const std::invalid_argument&) {return {FW_INTERNAL,"invalid ticket authentication storage response",{}};}
}
}
