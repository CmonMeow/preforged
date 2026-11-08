#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace fw::gw::pets {
// Durable character state. Actor IDs, World generations, action state, Health
// and commands belong to the current instance and must never be encoded here.
enum class Species:std::uint32_t {melandrus_stalker=1};
enum class Evolution:std::uint32_t {unevolved=0,playful=1,hearty=2,aggressive=3,dire=4,elder=5};
struct Pet {
    Species species=Species::melandrus_stalker;
    unsigned level=5;
    std::uint32_t experience=0;
    Evolution evolution=Evolution::unevolved;
    std::string name; // Empty means the species/evolution default localized name.
    bool operator==(const Pet&)const=default;
};
struct Profile {std::optional<Pet> pet;bool operator==(const Profile&)const=default;};
inline bool valid_name(const std::string& name){
    if(name.empty())return true;
    if(name.size()<3||name.size()>12||name.front()==' '||name.back()==' ')return false;
    bool space=false;
    for(unsigned char c:name){
        if(c==' '){if(space)return false;space=true;}
        else{if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')))return false;space=false;}
    }
    return true;
}
inline bool valid(const Pet& pet){
    return pet.species==Species::melandrus_stalker&&pet.level>=5&&pet.level<=20&&
        unsigned(pet.evolution)<=unsigned(Evolution::elder)&&valid_name(pet.name);
}
inline void validate(const Profile& profile){
    if(profile.pet&&!valid(*profile.pet))throw std::invalid_argument("invalid durable pet profile");
}
inline std::vector<std::uint8_t> encode(const Profile& profile){
    validate(profile);std::vector<std::uint8_t> bytes{'P','E','T',1};
    const auto put=[&](std::uint32_t value){for(unsigned n=0;n<4;++n)bytes.push_back(std::uint8_t(value>>(8*n)));};
    put(bool(profile.pet));
    if(profile.pet){const auto& p=*profile.pet;put(unsigned(p.species));put(p.level);put(p.experience);put(unsigned(p.evolution));put(unsigned(p.name.size()));bytes.insert(bytes.end(),p.name.begin(),p.name.end());}
    return bytes;
}
inline Profile decode(std::span<const std::uint8_t> bytes){
    if(bytes.size()<8||bytes[0]!='P'||bytes[1]!='E'||bytes[2]!='T'||bytes[3]!=1)throw std::invalid_argument("pet profile header");
    std::size_t offset=4;
    const auto get=[&](){if(bytes.size()-offset<4)throw std::invalid_argument("truncated pet profile");
        std::uint32_t value=0;for(unsigned n=0;n<4;++n)value|=std::uint32_t(bytes[offset++])<<(8*n);return value;};
    const auto present=get();if(present>1)throw std::invalid_argument("pet profile presence");
    Profile profile;
    if(present){Pet pet;pet.species=Species(get());pet.level=get();pet.experience=get();pet.evolution=Evolution(get());
        const auto length=get();if(length>12||length>bytes.size()-offset)throw std::invalid_argument("pet name length");
        pet.name.assign(bytes.begin()+offset,bytes.begin()+offset+length);offset+=length;profile.pet=std::move(pet);}
    if(offset!=bytes.size())throw std::invalid_argument("pet profile trailing bytes");validate(profile);return profile;
}
inline bool brings_pet(std::span<const unsigned> skills){
    for(const auto skill:skills)if(skill==411||skill==436||skill==1195)return true;
    return false;
}
}
