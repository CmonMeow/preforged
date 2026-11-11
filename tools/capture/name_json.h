#pragma once
#include <string>
#include <string_view>
namespace capture {
// ASCII JSON preserves UTF-16 names without depending on the system code page.
inline std::string name_json(std::wstring_view text) {
    constexpr char hex[]="0123456789abcdef";
    std::string out="\"";
    for(size_t i=0;i<text.size();++i){
        unsigned c=static_cast<unsigned>(text[i]);
        if(c>=0xd800&&c<=0xdbff){
            if(i+1==text.size()||text[i+1]<0xdc00||text[i+1]>0xdfff)c=0xfffd;
        }else if(c>=0xdc00&&c<=0xdfff){
            if(i==0||text[i-1]<0xd800||text[i-1]>0xdbff)c=0xfffd;
        }
        out+="\\u";
        for(int shift=12;shift>=0;shift-=4)out+=hex[(c>>shift)&15];
    }
    return out+'"';
}
inline std::string encoded_name_json(std::wstring_view text){
    std::string out="[";
    for(auto c:text){if(out.size()>1)out+=",";out+=std::to_string(static_cast<unsigned>(c));}
    return out+"]";
}
}
