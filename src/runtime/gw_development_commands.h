#pragma once
#include "gw_character_name.h"
#include <array>
#include <algorithm>
#include <optional>
#include <string_view>
namespace fw::gw::development {
enum class Tier {gm,developer,staging,staging_unknown,undocumented,private_extension,ordinary};
enum class Kind {unknown,invalid,gm,die,kill,setname,namepet,resurrect,map,experience,faction,guild_trim,title_points,generic};
struct Declaration {std::u16string_view keyword,syntax;Tier tier;Kind kind;};
// Mirrored2021 Game_master.html and Commands.html. Declaration is evidence of
// syntax, NOT a claim that an operation is implemented. Unknown effects remain
// explicitly undocumented; private extensions are labelled separately.
inline constexpr auto declarations=std::to_array<Declaration>({
 {u".gm",u"/.gm",Tier::gm,Kind::gm},{u".die",u"/.die",Tier::gm,Kind::die},
 {u".examine",u"/.examine",Tier::gm,Kind::generic},{u".fireworks",u"/.fireworks",Tier::gm,Kind::generic},
 {u".mute",u"/.mute",Tier::gm,Kind::generic},{u".resurrect",u"/.resurrect",Tier::gm,Kind::resurrect},{u".teleport",u"/.teleport",Tier::gm,Kind::generic},
 {u"%",u"/% <text>",Tier::developer,Kind::generic},{u".dev",u"/.dev",Tier::developer,Kind::generic},
 {u".s",u"/.s <map ID>",Tier::developer,Kind::map},{u".unspawn",u"/.unspawn",Tier::developer,Kind::generic},
 {u".joinguild",u"/.joinguild <guild> <tag>",Tier::developer,Kind::generic},{u".guildhall",u"/.guildhall",Tier::developer,Kind::generic},
 {u".",u"/.",Tier::developer,Kind::generic},{u".unmute",u"/.unmute",Tier::developer,Kind::generic},
 {u".who",u"/.who",Tier::developer,Kind::generic},{u".write",u"/.write",Tier::developer,Kind::generic},
 {u".guildtrim",u"/.guildtrim <0|1|2|3|9>",Tier::developer,Kind::guild_trim},
 {u".allheroes",u"/.allheroes",Tier::staging,Kind::generic},{u".allitems",u"/.allitems",Tier::staging,Kind::generic},
 {u".allmissions",u"/.allmissions",Tier::staging,Kind::generic},{u".allskills",u"/.allskills",Tier::staging,Kind::generic},
 {u".exp",u"/.exp <0..50000>",Tier::staging,Kind::experience},{u".faction",u"/.faction <0 Kurzick|1 Luxon> <points>",Tier::staging,Kind::faction},
 {u".gold",u"/.gold <amount>",Tier::staging,Kind::generic},{u".loot",u"/.loot <item alias>",Tier::staging,Kind::generic},
 {u".p",u"/.p",Tier::staging,Kind::generic},{u".rp",u"/.rp",Tier::staging,Kind::generic},{u".run",u"/.run <1..9>",Tier::staging,Kind::generic},
 {u".titlepoints",u"/.titlepoints <0..42> <points>",Tier::staging,Kind::title_points},
 {u".ascend",u"/.ascend",Tier::staging_unknown,Kind::generic},{u".chest",u"/.chest",Tier::staging_unknown,Kind::generic},
 {u".crash",u"/.crash",Tier::staging_unknown,Kind::generic},{u".damage",u"/.damage",Tier::staging_unknown,Kind::generic},
 {u".disconnect",u"/.disconnect",Tier::staging_unknown,Kind::generic},{u".fame",u"/.fame <number>",Tier::staging_unknown,Kind::generic},
 {u".heal",u"/.heal",Tier::staging_unknown,Kind::generic},{u".hotkey",u"/.hotkey <number> <number>",Tier::staging_unknown,Kind::generic},
 {u".item",u"/.item <string>",Tier::staging_unknown,Kind::generic},{u".lockup",u"/.lockup",Tier::staging_unknown,Kind::generic},
 {u".mission",u"/.mission",Tier::staging_unknown,Kind::generic},{u".playtime",u"/.playtime",Tier::staging_unknown,Kind::generic},
 {u".quest",u"/.quest",Tier::staging_unknown,Kind::generic},{u".respawn",u"/.respawn",Tier::staging_unknown,Kind::generic},
 {u".ressurect",u"/.ressurect",Tier::staging_unknown,Kind::generic},{u".spawn",u"/.spawn",Tier::staging_unknown,Kind::generic},
 {u".t",u"/.t",Tier::staging_unknown,Kind::generic},{u".trigger",u"/.trigger",Tier::staging_unknown,Kind::generic},
 {u".unlock",u"/.unlock",Tier::staging_unknown,Kind::generic},
 {u".dp",u"/.dp",Tier::undocumented,Kind::generic},{u".event",u"/.event",Tier::undocumented,Kind::generic},
 {u".ip",u"/.ip",Tier::undocumented,Kind::generic},{u".m",u"/.m",Tier::undocumented,Kind::generic},
 {u".mark",u"/.mark <event> <player>",Tier::undocumented,Kind::generic},{u".monitor",u"/.monitor",Tier::undocumented,Kind::generic},
 {u".partysize",u"/.partysize",Tier::undocumented,Kind::generic},{u".time",u"/.time",Tier::undocumented,Kind::generic},
 {u".w",u"/.w",Tier::undocumented,Kind::generic},{u"^",u"/^",Tier::undocumented,Kind::generic},
 {u"kill",u"/kill <player name or ID>",Tier::private_extension,Kind::kill},
 {u"setname",u"/setname <player name or ID> = <new name>",Tier::private_extension,Kind::setname},
 {u"namepet",u"/namepet [name]",Tier::ordinary,Kind::namepet},{u"petname",u"/petname [name]",Tier::ordinary,Kind::namepet}
});
struct Request {Kind kind=Kind::unknown;const Declaration* declaration=nullptr;std::u16string keyword,target,text;std::array<std::uint32_t,2> numbers{};bool requires_authorization()const{return declaration&&declaration->tier!=Tier::ordinary;}};
inline std::u16string_view trim(std::u16string_view s){while(!s.empty()&&s.front()==u' ')s.remove_prefix(1);while(!s.empty()&&s.back()==u' ')s.remove_suffix(1);return s;}
inline std::optional<std::uint32_t> number(std::u16string_view s,std::uint32_t limit){if(s.empty())return {};std::uint32_t n=0;for(auto c:s){if(c<u'0'||c>u'9'||n>limit/10||(n==limit/10&&c-u'0'>limit%10))return {};n=n*10+c-u'0';}return n;}
inline bool valid_name(std::u16string_view name,std::size_t maximum=19){try{(void)character_name_to_utf8(name,maximum);return true;}catch(const std::invalid_argument&){return false;}}
inline Request parse(std::u16string_view input){
 Request r;input=trim(input);if(input.empty()||input.front()!=u'/')return r;input.remove_prefix(1);auto split=input.find(u' ');auto keyword=input.substr(0,split);auto arg=split==input.npos?std::u16string_view{}:trim(input.substr(split+1));
 r.keyword=keyword;for(auto& c:r.keyword)if(c>=u'A'&&c<=u'Z')c+=u'a'-u'A';
 for(const auto& declaration:declarations)if(declaration.keyword==r.keyword){r.declaration=&declaration;break;}if(!r.declaration)return r;r.kind=r.declaration->kind;r.text=arg;
 if(input.size()>256||std::any_of(input.begin(),input.end(),[](char16_t c){return c<32||c==127;})){r.kind=Kind::invalid;return r;}
 auto invalid=[&]{r.kind=Kind::invalid;return r;};
 if(r.kind==Kind::gm||r.kind==Kind::die||r.kind==Kind::resurrect){if(!arg.empty())return invalid();}
 else if(r.kind==Kind::kill){if(!valid_name(arg))return invalid();r.target=arg;r.text.clear();}
 else if(r.kind==Kind::setname){auto delimiter=arg.find(u'=');if(delimiter==arg.npos||arg.find(u'=',delimiter+1)!=arg.npos)return invalid();r.target=trim(arg.substr(0,delimiter));r.text=trim(arg.substr(delimiter+1));if(!valid_name(r.target)||!valid_name(r.text))return invalid();}
 else if(r.kind==Kind::namepet){if(!arg.empty()&&!valid_name(arg,19))return invalid();}
 else if(r.kind==Kind::map||r.kind==Kind::experience||r.kind==Kind::guild_trim){auto value=number(arg,r.kind==Kind::map?65535:r.kind==Kind::experience?50000:9);if(!value||r.kind==Kind::map&&!*value||r.kind==Kind::guild_trim&&*value!=0&&*value!=1&&*value!=2&&*value!=3&&*value!=9)return invalid();r.numbers[0]=*value;}
 else if(r.kind==Kind::faction||r.kind==Kind::title_points){auto separator=arg.find(u' ');if(separator==arg.npos)return invalid();auto first=number(arg.substr(0,separator),r.kind==Kind::faction?1:42),second=number(trim(arg.substr(separator+1)),UINT32_MAX);if(!first||!second)return invalid();r.numbers={*first,*second};}
 return r;
}
}

