#pragma once
#include <optional>
#include <string>
#include <string_view>
namespace capture::loot {
struct Report { unsigned template_id; std::wstring source_atom,item_atom,encoded; };
// Toolbox ChatFilter identifies 7f0/7f1 first/second arguments as dropper/item.
// Arguments can contain multiple encoded words and nested10A..10F arguments.
// Outer delimiter1 is omitted; nested delimiter1 remains part of the name.
inline std::optional<Report> parse(std::wstring_view text){
 if(text.empty()||text.size()>8192||(text[0]!=0x7f0&&text[0]!=0x7f1))return {};
 const auto a=text.find(wchar_t(0x10a)),b=text.find(wchar_t(0x10b));
 if(a==text.npos||b==text.npos||a>=b||a+1>=text.size()||b+1>=text.size())return {};
 if(text[a+1]==0xba9&&a+2<text.size()&&text[a+2]==0x107)return {}; // player drop
 size_t item_end=0,source_end=0;
 auto atom=[&](size_t p,bool item)->std::wstring{
  ++p;if(p>=text.size()||text[p]<0x100)return {};
  const auto first=p;unsigned depth=1;
  for(;p<text.size()&&p-first<1024;++p){
   const auto c=unsigned(text[p]);
   if(c==1){if(!--depth){if(item)item_end=p+1;else source_end=p+1;return std::wstring(text.substr(first,p-first));}}
   else if(c>=0x10a&&c<=0x10f){if(++depth>16)return {};}
  }return {};
 };
 auto source=atom(a,false),item=atom(b,true);if(source.empty()||item.empty()||source_end!=b)return {};
 // Never retain10C reservation recipient/player-name arguments or tail.
 return Report{unsigned(text[0]),std::move(source),std::move(item),std::wstring(text.substr(0,item_end))};
}
}
