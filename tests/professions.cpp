#include "gw_profession.h"
#include <fstream>
#include <iostream>
#include <map>
#include <string>

int main(int argc,char** argv)try{
    using namespace fw::gw::professions;
    using fw::gw::world::FrameState;
    auto require=[](bool condition,const char* message){if(!condition)throw std::runtime_error(message);};
    require(argc==2,"original2022 profession fixture path required");
    std::map<std::string,Packet> fixtures;std::ifstream input(argv[1]);
    require(bool(input),"original profession fixtures missing");std::string name,hex;
    while(input>>name>>hex){require(hex.size()%2==0,"invalid profession fixture hex");Packet p;
        for(std::size_t at=0;at<hex.size();at+=2)p.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(at,2),nullptr,16)));
        require(fixtures.emplace(name,std::move(p)).second,"duplicate profession fixture");}
    require(fixtures.size()==9,"missing original profession fixture cases");
    auto fixture_matches=[&](const char* name,Packet p){p[1]|=0x80;
        require(p==fixtures.at(name),"profession packet differs from original2022 serializer");};
    for(const auto* key:{"request_secondary6","request_secondary0","request_secondary8","request_secondary11"}){
        const auto& p=fixtures.at(key);
        for(std::size_t n=0;n<p.size();++n){Request r;r.actor=999;r.consumed=123;
            require(parse_request(std::span(p).first(n),r)==FrameState::incomplete&&r.actor==999&&r.consumed==123,
                "partial profession request accepted or mutated caller output");}
        auto joined=p;joined.insert(joined.end(),{68,128,0,0,0,0});Request r;
        require(parse_request(joined,r)==FrameState::complete&&r.actor==42&&r.consumed==p.size(),
            "profession request consumed next coalesced packet");}
    Request request;request.actor=999;
    require(parse_request(Packet{68,128,0,0,0,0},request)==FrameState::unknown_opcode&&request.actor==999,
        "unrelated packet changed profession request");
    require(parse_request(fixtures.at("request_secondary6"),request)==FrameState::complete&&request.secondary==6,
        "secondary byte interpreted as DWORD or primary");
    fw::gw::skills::Profile skills;skills.primary=8;skills.secondary=7;skills.profession_mask=0x6fe;
    skills.learned={{2,1},{3,81},{160,1},{571,1},{772,1}};
    skills.bar[0]={772,0};skills.bar[1]={571,0};skills.bar[2]={2,0};skills.bar[3]={3,5};
    fw::gw::attributes::Profile attributes;attributes.quest_bonus=30;
    attributes.source=fw::gw::attributes::PointsSource::minimum_inferred;
    attributes.ranks[29]=3;attributes.ranks[34]=10;attributes.ranks[36]=12;
    const auto original_skills=fw::gw::skills::encode_profile(skills);
    const auto original_attributes=attributes.encode();
    Policy policy{42,3,0x1e,20,false};fw::gw::attributes::Bonuses bonuses{};bonuses[36]=3;
    const auto update=propose(request,skills,attributes,policy,bonuses);
    require(update.accepted()&&update.changed&&update.skill_profile.secondary==6&&update.skill_profile.primary==8,
        "valid unlocked secondary change failed");
    require(!(skills.profession_mask&(1u<<8)),"fixture should retain missing primary bit");
    require(update.skill_profile.profession_mask==skills.profession_mask&&update.skill_profile.learned==skills.learned,
        "secondary change fabricated learning or profession unlocks");
    require(update.skill_profile.bar[0]==skills.bar[0]&&!update.skill_profile.bar[1].skill&&
        update.skill_profile.bar[2]==skills.bar[2]&&update.skill_profile.bar[3]==skills.bar[3],
        "secondary change must only clear incompatible slots and preserve copy identities");
    require(update.attribute_profile.ranks[29]==0&&update.attribute_profile.ranks[34]==10&&
        update.attribute_profile.ranks[36]==12&&update.attribute_profile.quest_bonus==30&&
        update.attribute_profile.source==attributes.source,"secondary change must refund old attributes without modifying earned points");
    require(fw::gw::skills::encode_profile(skills)==original_skills&&attributes.encode()==original_attributes,
        "proposal mutated state before atomic persistence");
    const std::array<unsigned,7> opcodes{165,182,181,58,56,57,217};
    require(update.authoritative.size()==opcodes.size(),"missing profession reconciliation packet");
    for(unsigned n=0;n<opcodes.size();++n)require((update.authoritative[n][0]|(unsigned(update.authoritative[n][1])<<8))==opcodes[n],
        "profession state must precede attribute snapshot; never recreate55 on edits");
    require(update.authoritative[4][6]==42&&update.authoritative[5][6]==200,"old secondary allocated points not refunded");
    fixture_matches("agent_profession",update.authoritative[0]);
    fixture_matches("profession_state",update.authoritative[1]);
    fixture_matches("unlocked_professions",update.authoritative[2]);
    fixture_matches("attributes_snapshot",update.authoritative[3]);
    fixture_matches("reconciled_bar",update.authoritative[6]);
    const auto unchanged=packets_2022(skills,attributes,policy,bonuses);
    request.actor=43;auto denied=propose(request,skills,attributes,policy,bonuses);
    require(denied.status==Reason::owner&&!denied.changed&&denied.authoritative==unchanged,
        "unowned actor changed professions or reconciled to proposed state");
    request.actor=42;request.secondary=8;
    require(propose(request,skills,attributes,policy).status==Reason::profession,"own primary accepted as secondary");
    request.secondary=0;require(propose(request,skills,attributes,policy).status==Reason::profession,"existing secondary removed toNone");
    request.secondary=11;require(propose(request,skills,attributes,policy).status==Reason::profession,"nonplayer profession11 accepted");
    request.secondary=6;auto locked=skills;locked.profession_mask&=~(1u<<6);
    require(propose(request,locked,attributes,policy).status==Reason::locked,"locked profession accepted");
    request.secondary=7;locked.profession_mask&=~(1u<<7);
    auto same=propose(request,locked,attributes,policy);require(same.accepted()&&!same.changed,
        "current secondary incorrectly requires mask bit or unnecessary save");
    request.secondary=6;
    for(unsigned type:{1u,4u,6u,8u,10u,11u,16u,99u}){auto combat=policy;combat.map_type=type;
        require(propose(request,skills,attributes,combat).status==Reason::mode,"active/unknown instance permits profession change");}
    for(unsigned type:{0u,2u,3u,5u,7u,15u}){auto staging=policy;staging.map_type=type;
        require(propose(request,skills,attributes,staging).accepted(),"valid staging instance denies profession change");}
    auto novice=skills;novice.secondary=0;novice.bar[1]={};novice.profession_mask=0;
    auto novice_attrs=attributes;novice_attrs.ranks[29]=0;request.secondary=0;
    require(propose(request,novice,novice_attrs,policy).accepted(),"currentNone no-op rejected");
    request.secondary=6;require(propose(request,novice,novice_attrs,policy).status==Reason::locked,
        "uncaptured character granted a secondary profession");
    std::cout<<"Profession retail packets, framing, unlock semantics, mode gates and atomic proposals passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
