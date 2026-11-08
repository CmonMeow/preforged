#pragma once
#include "gw_armor_attribute_runes.h"
#include "gw_inventory_identity.h"
namespace fw::gw::inventory::upgrades {
enum class Class {rune,insignia};
struct Component {Class kind;std::uint32_t word;};
inline std::optional<Component> component(std::uint32_t word,unsigned profession){
    if(word==0xa53003ce)return profession>=1&&profession<=10?std::optional(Component{Class::insignia,word}):std::nullopt;
    if(word==0xa53003f4)return profession==1?std::optional(Component{Class::insignia,word}):std::nullopt;
    if(word==0x253003d8)return profession==4?std::optional(Component{Class::insignia,word}):std::nullopt;
    if(word==0x253003ca||word==0x253003cc)return Component{Class::insignia,word};
    if((word&0x3ff00000u)==0x21e00000u&&(word&0xffff0000u)!=0x21e80000u)return {};
    auto decoded=equipment::attribute_runes::decode(std::span(&word,1),profession);
    if(decoded.valid&&decoded.rune)return Component{Class::rune,word};return {};
}
// Named upgrade identities from pinned Toolbox PriceCheckerModule.cpp. Used
// only to recognize occupied slot families; unknown upgrade effects are not
// thereby implemented. Removing an old rune must not leave a second opaque rune.
inline std::optional<Class> occupied_component(std::uint32_t word){
    switch(word){
    case 0x21e80001:
    case 0x21e80002:
    case 0x21e80003:
    case 0x21e80101:
    case 0x21e80102:
    case 0x21e80103:
    case 0x21e80201:
    case 0x21e80202:
    case 0x21e80203:
    case 0x21e80301:
    case 0x21e80302:
    case 0x21e80303:
    case 0x21e80401:
    case 0x21e80402:
    case 0x21e80403:
    case 0x21e80501:
    case 0x21e80502:
    case 0x21e80503:
    case 0x21e80601:
    case 0x21e80602:
    case 0x21e80603:
    case 0x21e80701:
    case 0x21e80702:
    case 0x21e80703:
    case 0x21e80801:
    case 0x21e80802:
    case 0x21e80803:
    case 0x21e80901:
    case 0x21e80902:
    case 0x21e80903:
    case 0x21e80a01:
    case 0x21e80a02:
    case 0x21e80a03:
    case 0x21e80b01:
    case 0x21e80b02:
    case 0x21e80b03:
    case 0x21e80c01:
    case 0x21e80c02:
    case 0x21e80c03:
    case 0x21e80d01:
    case 0x21e80d02:
    case 0x21e80d03:
    case 0x21e80e01:
    case 0x21e80e02:
    case 0x21e80e03:
    case 0x21e80f01:
    case 0x21e80f02:
    case 0x21e80f03:
    case 0x21e81001:
    case 0x21e81002:
    case 0x21e81003:
    case 0x21e81101:
    case 0x21e81102:
    case 0x21e81103:
    case 0x21e81201:
    case 0x21e81202:
    case 0x21e81203:
    case 0x21e81301:
    case 0x21e81302:
    case 0x21e81303:
    case 0x21e81401:
    case 0x21e81402:
    case 0x21e81403:
    case 0x21e81501:
    case 0x21e81502:
    case 0x21e81503:
    case 0x21e81601:
    case 0x21e81602:
    case 0x21e81603:
    case 0x21e81701:
    case 0x21e81702:
    case 0x21e81703:
    case 0x21e81801:
    case 0x21e81802:
    case 0x21e81803:
    case 0x21e81901:
    case 0x21e81902:
    case 0x21e81903:
    case 0x21e81d01:
    case 0x21e81d02:
    case 0x21e81d03:
    case 0x21e81e01:
    case 0x21e81e02:
    case 0x21e81e03:
    case 0x21e81f01:
    case 0x21e81f02:
    case 0x21e81f03:
    case 0x21e82001:
    case 0x21e82002:
    case 0x21e82003:
    case 0x21e82101:
    case 0x21e82102:
    case 0x21e82103:
    case 0x21e82201:
    case 0x21e82202:
    case 0x21e82203:
    case 0x21e82301:
    case 0x21e82302:
    case 0x21e82303:
    case 0x21e82401:
    case 0x21e82402:
    case 0x21e82403:
    case 0x21e82501:
    case 0x21e82502:
    case 0x21e82503:
    case 0x21e82601:
    case 0x21e82602:
    case 0x21e82603:
    case 0x21e82701:
    case 0x21e82702:
    case 0x21e82703:
    case 0x21e82801:
    case 0x21e82802:
    case 0x21e82803:
    case 0x21e82901:
    case 0x21e82902:
    case 0x21e82903:
    case 0x21e82a01:
    case 0x21e82a02:
    case 0x21e82a03:
    case 0x21e82b01:
    case 0x21e82b02:
    case 0x21e82b03:
    case 0x21e82c01:
    case 0x21e82c02:
    case 0x21e82c03:
    case 0x25300423:
    case 0x25300425:
    case 0x25300427:
    case 0x25300429:
    case 0x2530042b:
    case 0x2530042d:
    case 0x27e802c2:
    case 0x27e802ea:
    case 0x27e902c2:
    case 0x27e902ea:
    case 0x27ea02c2:
    case 0x27ea02ea:
        return Class::rune;
    case 0x253003ca:
    case 0x253003cc:
    case 0x253003d8:
    case 0x27e802a9:
    case 0x27e802b6:
    case 0x27e802b7:
    case 0xa53003bc:
    case 0xa53003be:
    case 0xa53003c0:
    case 0xa53003c2:
    case 0xa53003c4:
    case 0xa53003c6:
    case 0xa53003c8:
    case 0xa53003ce:
    case 0xa53003d0:
    case 0xa53003d2:
    case 0xa53003d4:
    case 0xa53003d6:
    case 0xa53003da:
    case 0xa53003dc:
    case 0xa53003de:
    case 0xa53003e0:
    case 0xa53003e2:
    case 0xa53003e4:
    case 0xa53003e6:
    case 0xa53003e8:
    case 0xa53003ea:
    case 0xa53003ec:
    case 0xa53003ee:
    case 0xa53003f0:
    case 0xa53003f2:
    case 0xa53003f4:
    case 0xa53003f6:
    case 0xa53003f8:
    case 0xa53003fa:
    case 0xa53003fc:
    case 0xa53003fe:
    case 0xa5300400:
    case 0xa5300402:
    case 0xa5300404:
    case 0xa5300406:
    case 0xa5300408:
    case 0xa530040a:
    case 0xa530040c:
    case 0xa530040e:
        return Class::insignia;
    default:return {};
    }
}
inline bool armor(unsigned type){return type==7||type==19||type==16||type==4||type==13;}
// Original 2022 7c6690 / 8784a0 / 8783c0. These armor types have
// native type flags 0x182; the source's 25b marker grants applicability.
inline bool eligible(const Item& source,const Item& target){
    if(source.definition.type!=8||!armor(target.definition.type)||
       (source.profession.value_or(11)!=11&&source.profession!=target.profession))return false;
    const auto category=source.definition.interaction&0x04110000u;
    const auto flags=target.definition.interaction;
    if(category==0x10000u){
        if((flags&0x442u)==0x42u||(flags&0x4000u))return false;
    }else if(category==0x100000u){
        if(flags&0x8000u)return false;
    }else return false;
    for(auto word:source.definition.modifiers)if((word&0x3ff00000u)==0x25b00000u){
        const auto marker=(word>>8)&0x3ffu;
        if(marker==target.definition.type||marker==0)return true;
    }
    return false;
}
struct Plan {Profile profile;Item source,target;};
inline std::optional<Plan> apply(const Profile& profile,std::uint32_t source,std::uint32_t target){
    const auto si=profile.items.find(source),ti=profile.items.find(target);
    if(source==target||si==profile.items.end()||ti==profile.items.end())return {};
    const auto& s=si->second;const auto& t=ti->second;auto sb=profile.bags.find(s.bag),tb=profile.bags.find(t.bag);
    if(sb==profile.bags.end()||sb->second.type!=1||tb==profile.bags.end()||
       (tb->second.type!=1&&tb->second.type!=2)||s.definition.type!=8||!s.definition.quantity||
       !armor(t.definition.type)||t.definition.quantity!=1||!t.profession||!*t.profession||*t.profession>10||!eligible(s,t))return {};
    std::optional<Component> upgrade;
    for(auto word:s.definition.modifiers)if(auto value=component(word,*t.profession)){
        if(upgrade)return {};upgrade=value;
    }
    if(!upgrade)return {};
    const auto current=equipment::attribute_runes::decode(t.definition.modifiers,*t.profession);
    if(!current.valid)return {};
    unsigned rune_count=0,insignia_count=0;
    for(auto word:t.definition.modifiers)if(auto kind=occupied_component(word)){
        rune_count+=*kind==Class::rune;insignia_count+=*kind==Class::insignia;
    }
    if(rune_count>1||insignia_count>1)return {};
    Plan plan{profile,s,t};auto& modified=plan.profile.items.at(target).definition.modifiers;
    modified.erase(std::remove_if(modified.begin(),modified.end(),[&](auto word){
        auto value=occupied_component(word);return value&&*value==upgrade->kind;
    }),modified.end());
    modified.push_back(upgrade->word);
    if(modified.size()>64||!equipment::attribute_runes::decode(modified,*t.profession).valid)return {};
    auto& debit=plan.profile.items.at(source).definition.quantity;
    if(--debit==0)plan.profile.items.erase(source);
    validate_character(plan.profile);return plan;
}
// Private presentation identities are disjoint from owned, floor and original
// equipment placeholders. Preview definitions have no bag and never enter a
// Profile/identity projection, so they cannot be persisted, traded or equipped.
class PreviewReservation {
    std::uint32_t id_=0;
public:
    static constexpr std::uint32_t first=identity::placeholder_last+1,last=first+31;
    static_assert(first>identity::owned_last&&first>identity::ground_last);
    bool reserve(const Profile& character,const Profile& bank){
        if(id_)return false;
        for(auto id=first;id<=last;++id)if(!character.items.contains(id)&&!bank.items.contains(id)){id_=id;return true;}
        return false;
    }
    std::uint32_t id()const{return id_;}
    void release(){id_=0;}
};
struct Pending {
    std::uint32_t target=0,source=0;std::uint64_t revision=0,expires=0;std::uint8_t selection=0;
    bool selected=false;PreviewReservation preview;
    bool current(std::uint64_t rev,std::uint64_t now)const{return target&&preview.id()&&revision==rev&&now<expires;}
    void clear(){target=source=0;revision=expires=0;selected=false;preview.release();}
};
}
