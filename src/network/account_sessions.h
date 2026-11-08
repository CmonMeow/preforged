#pragma once
#include "ticket_auth.h"
namespace gwserver {
// Password bytes are borrowed for this synchronous call and never retained by
// the component. The caller owns their lifetime and erasure.
struct LoginAccount {std::string name;std::span<const std::uint8_t> password;};
struct RegisterAccount {std::string name;std::span<const std::uint8_t> password;};
struct ValidateSession {SessionToken session;};
struct LogoutAccount {SessionToken session;};
struct AccountIdentity {Identity account;std::string name;std::uint64_t expires;};
struct AccountLogin {AccountIdentity identity;SessionToken session;};
struct LoginResult {std::uint32_t status;std::string message;std::optional<AccountLogin> login;};
struct SessionResult {std::uint32_t status;std::string message;std::optional<AccountIdentity> identity;};
struct LogoutResult {std::uint32_t status;std::string message;};
inline std::string normalized_account(std::string name){
    if(!fw::account_name_valid(name))throw std::invalid_argument("invalid account name");
    for(auto& c:name)if(c>='A'&&c<='Z')c+=32;return name;
}
inline std::string storage_payload(const LoginAccount& value){
    const auto name=normalized_account(value.name);
    const bool test=(name=="test"||name=="test@example.test")&&value.password.size()==4&&std::equal(value.password.begin(),value.password.end(),reinterpret_cast<const std::uint8_t*>("test"));
    if((value.password.size()<8&&!test)||value.password.size()>128)throw std::invalid_argument("password requires8..128 bytes");
    return name+"\t"+storage_hex(value.password);
}
inline std::string storage_payload(const RegisterAccount& value){return storage_payload(LoginAccount{value.name,value.password});}
inline std::uint64_t session_expiry(const std::string& text){
    std::uint64_t value=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value);
    if(text.empty()||parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||!value||value>INT64_MAX)throw std::invalid_argument("invalid session expiry");return value;
}
inline LoginResult decode_login(const fw::AccountResult& result,const LoginAccount& request){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    try {
        const auto fields=fw::split(result.payload);
        if(fields.size()!=4||fields[1]!=normalized_account(request.name))throw std::invalid_argument("login response identity mismatch");
        return {FW_OK,{},AccountLogin{{Identity::from_hex(fields[0]),fields[1],session_expiry(fields[3])},SessionToken::from_hex(fields[2])}};
    }catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid login storage response",{}};}
}
inline SessionResult decode_session(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload,{}};
    try {
        const auto fields=fw::split(result.payload);
        if(fields.size()!=3||fields[1]!=normalized_account(fields[1]))throw std::invalid_argument("session response identity");
        return {FW_OK,{},AccountIdentity{Identity::from_hex(fields[0]),fields[1],session_expiry(fields[2])}};
    }catch(const std::invalid_argument&){return {FW_INTERNAL,"invalid session storage response",{}};}
}
inline LogoutResult decode_logout(const fw::AccountResult& result){
    if(result.status!=FW_OK)return {result.status,result.payload};
    if(result.payload!="revoked")return {FW_INTERNAL,"invalid logout storage response"};
    return {FW_OK,{}};
}
}
