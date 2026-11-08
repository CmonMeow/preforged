#include "gw_creation_request.h"
#include "gw_instance_admission.h"
#include <iostream>
#include <stdexcept>
#include <vector>
int main()try{
    using namespace fw::gw;
    constexpr std::array<unsigned,8> shifts{15,10,24,20,30,1,0,5};
    constexpr std::array<unsigned,8> widths{5,5,6,4,2,4,1,5};
    for(unsigned field=0;field<8;++field)for(unsigned value=0;value<(1u<<widths[field]);++value){
        const auto mask=((1u<<widths[field])-1)<<shifts[field];
        const auto appearance=(0xa5c39e71u&~mask)|(value<<shifts[field]);
        if(creation::appearance_field(appearance,field)!=value)throw std::runtime_error("native packed appearance extraction");
    }
    if(creation::appearance_field(0x4283000a,3)!=8||creation::appearance_field(0x4283000a,4)!=1)
        throw std::runtime_error("live Factions Ritualist packed fields");
    if(!instance_tuple_failure(1,148,3,0,0,0,true).empty()||
       !instance_tuple_failure(1,490,1,0,0,0,true).empty()||
       instance_tuple_failure(1,148,3,0,0,0,false)!="campaign_not_owned"||
       instance_tuple_failure(1,148,1,0,0,0,true)!="map_type_mismatch"||
       instance_tuple_failure(0,148,3,0,0,0,true)!="missing_transaction"||
       instance_tuple_failure(1,148,3,5,0,0,true)!="invalid_region"||
       instance_tuple_failure(1,148,3,0,65536,0,true)!="invalid_district"||
       instance_tuple_failure(1,148,3,0,0,1,true)!="invalid_language"||
       !instance_tuple_failure(1,2,6,0xfffffffeu,0,10,true).empty())
        throw std::runtime_error("instance admission failure classification");
    for(unsigned campaign=0;campaign<=3;++campaign)for(unsigned profession=1;profession<=10;++profession)
        if(!creation::valid_preview_selection(campaign,profession))throw std::runtime_error("native campaign/profession preview rejected");
    if(creation::valid_preview_selection(4,1)||creation::valid_preview_selection(255,1)||creation::valid_preview_selection(1,0)||creation::valid_preview_selection(1,11))throw std::runtime_error("invalid preview selection admitted");
        if(creation::starting_map(1)!=148||creation::starting_map(2)!=212||creation::starting_map(3)!=490||creation::starting_map(0))throw std::runtime_error("creation destination policy");
    for(unsigned c=1;c<=3;++c)for(unsigned p=1;p<=10;++p){
        const bool expected=(c==1&&p<=6)||(c==2&&p<=8)||(c==3&&(p<=6||p>=9));
        if(creation::authorized_preview(c,p,14)!=expected||creation::authorized_preview(c,p,14&~(1u<<c)))throw std::runtime_error("creation ownership or profession policy");
    }
    std::vector<std::uint8_t> packet{137,128,3,0,'A',0,' ',0,'B',0,1,2,3,4,5,6,7,8};
    for(unsigned campaign=0;campaign<=3;++campaign)for(unsigned profession=1;profession<=10;++profession){
        constexpr std::uint32_t appearance=0xab1cdef0;
        const auto changed=creation::preview_appearance(appearance,profession,campaign);
        const auto mask=(15u<<20)|(campaign?(3u<<30):0u);
        if(((changed>>20)&15)!=profession||(changed&~mask)!=(appearance&~mask)||
           (campaign&&creation::appearance_field(changed,4)!=campaign-1))
            throw std::runtime_error("preview profession must update composite without overwriting other appearance fields");
    }
    creation::Request request;request.name=u"unchanged";
    for(unsigned campaign=1;campaign<=3;++campaign)for(unsigned profession=1;profession<=10;++profession){
        creation::Request candidate;candidate.appearance[2]=static_cast<std::uint8_t>(profession<<4);
        if(creation::authorized_request(candidate,campaign,profession,14)!=creation::authorized_preview(campaign,profession,14))
            throw std::runtime_error("creation request disagrees with campaign authorization");
        if(creation::authorized_request(candidate,campaign,profession%10+1,14)||creation::authorized_request(candidate,campaign,profession,0))
            throw std::runtime_error("mismatched preview or unowned campaign accepted");
        for(unsigned reserved=4;reserved<8;++reserved){candidate.appearance[reserved]=1;
            if(creation::authorized_request(candidate,campaign,profession,14))throw std::runtime_error("reserved creation bytes accepted");
            candidate.appearance[reserved]=0;}
    }
    for(std::size_t n=0;n<packet.size();++n)
        if(creation::parse_request(std::span(packet).first(n),request)!=world::FrameState::incomplete||request.name!=u"unchanged")
            throw std::runtime_error("fragment changed output or framed as map request");
    packet.insert(packet.end(),{0,128,0,0,0,0});
    if(creation::parse_request(packet,request)!=world::FrameState::complete||request.consumed!=18||request.name!=u"A B"||request.appearance[7]!=8)
        throw std::runtime_error("creation fields or coalesced frame boundary");
    packet[2]=20;
    if(creation::parse_request(packet,request)!=world::FrameState::invalid)throw std::runtime_error("name bound");
    std::cout<<"PASS creation request fragmentation, appearance bytes, coalescing and bounds\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}

