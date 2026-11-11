#pragma once
#include <optional>
#include <string_view>
namespace capture {
inline std::optional<unsigned> travel_map_id(std::wstring_view text,unsigned count){
    if(text.empty()||text.size()>10||count<2)return {};
    unsigned value=0;
    for(const auto c:text){
        if(c<L'0'||c>L'9')return {};
        const unsigned digit=c-L'0';
        if(value>(count-1)/10||(value==(count-1)/10&&digit>(count-1)%10))return {};
        value=value*10+digit;
    }
    return value?std::optional<unsigned>{value}:std::nullopt;
}
}
