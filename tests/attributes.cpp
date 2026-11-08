#include "gw_attributes.h"
#include "gw_compass_wire.h"
#include <fstream>
#include <iostream>
#include <map>
#include <string>

int main(int argc,char** argv)try{
    using namespace fw::gw::attributes;
    using fw::gw::world::FrameState;
    auto require=[](bool condition,const char* message){if(!condition)throw std::runtime_error(message);};
    require(argc==2,"original2022 attribute fixture path required");
    std::map<std::string,Packet> fixtures;std::ifstream input(argv[1]);
    require(bool(input),"original2022 attribute fixtures missing");
    std::string name,hex;
    while(input>>name>>hex){
        require(hex.size()%2==0,"invalid attribute fixture hex");Packet packet;
        for(std::size_t at=0;at<hex.size();at+=2)packet.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(at,2),nullptr,16)));
        require(fixtures.emplace(name,std::move(packet)).second,"duplicate attribute fixture");
    }
    require(fixtures.size()==9,"missing original attribute fixture cases");
    auto fixture_matches=[&](const char* name,Packet packet){packet[1]|=0x80;
        require(packet==fixtures.at(name),"attribute packet differs from original2022 serializer");};
    auto rejects=[&](auto action,const char* message){bool rejected=false;
        try{action();}catch(const std::invalid_argument&){rejected=true;}require(rejected,message);};
    Policy policy{1,3,200,true,true};Ranks example{};example[17]=12;example[20]=10;
    Bonuses bonuses{};bonuses[17]=1;bonuses[20]=2;
    fixture_matches("initial_empty",initial_packet_2022(42,200,200));
    fixture_matches("initial_allocated",initial_packet_2022(42,42,200));
    fixture_matches("available_points",points_packet_2022(42,42,200,true));
    fixture_matches("total_points",points_packet_2022(42,42,200,false));
    fixture_matches("full_snapshot",snapshot_packet_2022(42,example,policy,bonuses));
    const auto updates=packets_2022(42,example,policy,bonuses);
    require(updates.size()==3&&updates[0][0]==58&&updates[1][0]==56&&updates[2][0]==57,
        "allocation reconciliation must clear optimistic state before updating points");
    require(updates[1][6]==42&&updates[2][6]==200,"available/total point order reversed");
    for(const auto* key:{"decrease_request","increase_request","parallel_load_request"}){
        const auto& packet=fixtures.at(key);
        for(std::size_t n=0;n<packet.size();++n){Request partial;partial.actor=999;partial.consumed=123;
            require(parse_request(std::span(packet).first(n),partial)==FrameState::incomplete&&
                partial.actor==999&&partial.consumed==123,"fragment accepted early or changed caller output");}
        auto joined=packet;joined.insert(joined.end(),{68,128,0,0,0,0});Request request;
        require(parse_request(joined,request)==FrameState::complete&&request.actor==42&&request.consumed==packet.size(),
            "attribute request consumed coalesced next frame");
    }
    Request request;
    const auto& compass_fixture=fixtures.at("compact_load_request");
    request.actor=999;
    require(parse_request(compass_fixture,request)==FrameState::unknown_opcode&&request.actor==999,
        "compass drawing was interpreted as an attribute edit");
    require(fw::gw::compass::response_2022(42,compass_fixture).has_value(),
        "original opcode41 serializer fixture rejected as compass drawing");
    require(parse_request(fixtures.at("decrease_request"),request)==FrameState::complete&&request.change==Change::decrease&&
        request.token==7&&request.attribute==17,"refund token mistaken for desired rank");
    require(parse_request(fixtures.at("increase_request"),request)==FrameState::complete&&request.change==Change::increase&&
        request.token==7&&request.attribute==17,"increment wire layout changed");
    for(const auto* key:{"parallel_load_request"}){
        require(parse_request(fixtures.at(key),request)==FrameState::complete&&request.change==Change::load&&
            request.count==2&&request.allocations[0].id==17&&request.allocations[0].rank==12&&
            request.allocations[1].id==20&&request.allocations[1].rank==10,"bulk load ID/rank arrays decoded incorrectly");
    }
    auto malformed=fixtures.at("parallel_load_request");malformed[16]=1;request.actor=999;
    require(parse_request(malformed,request)==FrameState::invalid&&request.actor==999,"parallel array counts disagree");
    malformed=fixtures.at("parallel_load_request");malformed[6]=17;
    require(parse_request(malformed,request)==FrameState::invalid&&request.actor==999,"attribute array exceeds original count16");
    require(parse_request(Packet{68,128,0,0,0,0},request)==FrameState::unknown_opcode&&request.actor==999,
        "unrelated command interpreted as attributes");

    for(unsigned level=1;level<=20;++level)require(level_points(level)==level_point_table[level-1],"level point baseline");
    require(level_points(1)==0&&level_points(10)==45&&level_points(15)==95&&level_points(20)==170,
        "uncaptured level20 baseline must not fabricate quest awards");
    require(used_points(example)==158,"nonlinear cumulative allocation cost");
    rejects([]{level_points(0);},"level0 accepted");rejects([]{level_points(21);},"level21 accepted");
    for(unsigned profession=1;profession<=10;++profession){Policy p{profession,0,170,true,true};
        unsigned primaries=0;for(unsigned id=0;id<51;++id)if(available(id,p)&&catalog[id].primary_only)++primaries;
        require(primaries==1,"exact primary attribute catalogue");}
    require(available(17,policy)&&available(13,policy)&&!available(16,policy)&&!available(26,policy),
        "secondary primary-only or engine attributes exposed to players");
    for(unsigned type:{0u,2u,3u,5u,7u,15u})require(is_outpost_type(type)&&can_increase_type(type),"staging type denied refunds");
    for(unsigned type:{1u,4u})require(!is_outpost_type(type)&&can_increase_type(type),"PvE unspent points cannot increase");
    for(unsigned type:{6u,8u,10u,11u,16u,99u})require(!is_outpost_type(type)&&!can_increase_type(type),"PvP/preview/unknown allows edits");

    AttributeState state(42);require(state.restore(example,policy)==Reason::accepted,"restore valid captured allocations");
    parse_request(fixtures.at("decrease_request"),request);
    const auto decrease=state.propose(request,policy,bonuses);
    require(decrease.accepted()&&decrease.proposed[17]==11&&state.ranks()==example,
        "proposal mutated live state before persistence");
    require(decrease.authoritative[1][6]==62,"refund must restore rank12 last cost20");
    Policy explorable=policy;explorable.can_decrease=false;
    const auto denied=state.propose(request,explorable,bonuses);
    require(denied.status==Reason::decrease_mode&&denied.authoritative==updates&&state.ranks()==example,
        "denied refund must reconcile optimistic client to unchanged authoritative state");
    rejects([&]{state.commit(denied,policy);},"denied allocation committed");
    state.commit(decrease,policy);require(state.ranks()[17]==11,"accepted persisted refund not committed");
    rejects([&]{state.commit(decrease,policy);},"stale proposal committed twice");
    AttributeState foreign(43);require(foreign.restore(example,policy)==Reason::accepted,"foreign restore");
    rejects([&]{foreign.commit(decrease,policy);},"another actor's allocation committed");
    parse_request(fixtures.at("increase_request"),request);
    const auto increase=state.propose(request,explorable);require(increase.accepted(),"PvE spending unused points denied");
    state.commit(increase,explorable);require(state.ranks()==example,"increase does not invert refund");
    require(state.propose(request,policy).status==Reason::rank,"base rank13 accepted");
    request.attribute=18;Policy pvp=policy;pvp.can_increase=false;pvp.can_decrease=false;
    require(state.propose(request,pvp).status==Reason::increase_mode,"active PvP allows increase");
    request.actor=43;require(state.propose(request,policy).status==Reason::owner,"foreign attribute actor accepted");
    request.actor=42;request.attribute=16;require(state.propose(request,policy).status==Reason::attribute,"secondary primary attribute accepted");
    request.attribute=26;require(state.propose(request,policy).status==Reason::attribute,"engine attribute accepted");
    request.attribute=0xffffffff;require(state.propose(request,policy).status==Reason::attribute,"attribute ID overflow accepted");
    require(state.restore(example,Policy{1,0,100,true,true})==Reason::points&&state.ranks()==example,
        "overbudget restore changed live state");
    parse_request(fixtures.at("parallel_load_request"),request);request.allocations[1].id=17;
    require(state.propose(request,policy).status==Reason::duplicate,"duplicate attribute load accepted");
    request.allocations[1]={20,13};require(state.propose(request,policy).status==Reason::rank,"bulk rank13 accepted");
    request.allocations[1]={20,12};require(state.propose(request,Policy{1,3,170,true,true}).status==Reason::points,
        "bulk overspending accepted");
    request.allocations[1]={20,10};request.allocations[0].rank=11;
    require(state.propose(request,explorable).status==Reason::decrease_mode,"template refund bypassed PvE permission");
    Request clear;clear.actor=42;clear.change=Change::load;
    auto cleared=state.propose(clear,policy);require(cleared.accepted()&&used_points(cleared.proposed)==0,
        "empty template failed to refund complete allocation");

    Ranks captured{};captured[32]=3;captured[34]=12;captured[36]=12;
    auto profile=Profile::from_captured(captured,20);
    require(used_points(captured)==200&&profile.quest_bonus==30&&profile.total_points(20)==200&&
        profile.source==PointsSource::minimum_inferred,"captured spent200 fails to prove earned quest bonus30");
    const auto encoded=profile.encode();const auto decoded=Profile::decode(encoded);
    require(decoded.ranks==captured&&decoded.quest_bonus==30&&decoded.source==profile.source,"attribute profile round trip");
    require(Profile::from_captured({},20).total_points(20)==170,"empty capture invented unused quest bonus");
    require(Profile::from_captured(example,20).quest_bonus==0,"spent below baseline invented quests");
    Ranks partial{};partial[17]=12;partial[20]=12;
    require(Profile::from_captured(partial,20).quest_bonus==30,"spent194 requires both15point awards");
    partial[20]=11;require(Profile::from_captured(partial,20).quest_bonus==15,"spent174 requires one15point award");
    captured[32]=12;rejects([&]{Profile::from_captured(captured,20);},"impossible allocation imported");
    auto corrupt=encoded;corrupt[0]=2;rejects([&]{Profile::decode(corrupt);},"unknown profile schema accepted");
    corrupt=encoded;corrupt[2]=14;rejects([&]{Profile::decode(corrupt);},"nonquest award14 accepted");
    corrupt=encoded;corrupt[1]=4;rejects([&]{Profile::decode(corrupt);},"unknown provenance accepted");
    corrupt=encoded;corrupt[1]=3;
    if(Profile::decode(corrupt).source!=PointsSource::development)throw std::runtime_error("development entitlement provenance lost");
    corrupt=encoded;corrupt[3+26]=1;rejects([&]{Profile::decode(corrupt);},"engine/nonplayer rank persisted");
    corrupt=encoded;corrupt[3+34]=16;rejects([&]{Profile::decode(corrupt);},"effective item bonus persisted as base rank");
    rejects([&]{Profile::decode(std::span(encoded).first(53));},"truncated profile accepted");
    rejects([]{initial_packet_2022(42,201,200);},"available points exceed total");
    rejects([]{initial_packet_2022(0,0,0);},"zero actor points encoded");
    rejects([&]{snapshot_packet_2022(42,example,Policy{1,3,100,true,true});},"overbudget snapshot encoded");
    std::cout<<"Attribute cost, captured provenance, permissions, transactional state, retail packets and framing passed\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
