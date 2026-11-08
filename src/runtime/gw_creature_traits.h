#pragma once
#include <string_view>
namespace fw::gw::creatures {
// Server content keys, not numeric retail model IDs. A key must be assigned
// from reviewed creature-type evidence; appearance never establishes it.
inline bool valid_type(std::string_view key){
 if(key.empty()||key.size()>64)return false;
 for(const auto c:key)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;
 return key!="-";
}
}
