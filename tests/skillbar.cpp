#include "gw_skillbar.h"
#include <filesystem>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

int main(int argc,char** argv)try{
    using namespace fw::gw::skills;
    using fw::gw::world::FrameState;
    auto require=[](bool condition,const char* message){if(!condition)throw std::runtime_error(message);};
    require(argc==3,"retail skillbar and learned-copy fixture paths required");
    std::map<std::string,Packet> fixtures;std::string name,hex;
    for(unsigned file=1;file<3;++file){
        std::ifstream input(argv[file]);require(bool(input),"retail skill fixtures missing");
        unsigned count=0;
        while(input>>name>>hex){
            require(hex.size()%2==0,"invalid fixture hex");Packet packet;
            for(std::size_t at=0;at<hex.size();at+=2)packet.push_back(std::stoul(hex.substr(at,2),nullptr,16));
            require(fixtures.emplace(name,std::move(packet)).second,"duplicate fixture name");++count;
        }
        require(count==9,"missing pinned skill fixture cases");
    }
    require(fixtures.size()==18,"missing pinned skill fixture groups");
    auto fixture_matches=[&](const char* name,Packet packet){packet[1]|=0x80;
        require(packet==fixtures.at(name),"packet differs from original2022 serializer");};
    Slots example{};const std::array<unsigned,8> example_ids{1,2,5,23,44,105,152,0};
    for(unsigned n=0;n<8;++n)example[n].skill=example_ids[n];
    fixture_matches("slot",slot_packet_2022(42,7,{5,9}));
    fixture_matches("bar0",bar_packet_2022(42,example));
    fixture_matches("bar1",bar_packet_2022(42,example,true));
    fixture_matches("known",learned_packet_2022({{31,1},{32,1}}));
    fixture_matches("empty",learned_packet_2022({}));
    fixture_matches("learn_new_single",learning_packet_2022(5,1));
    fixture_matches("learn_new_second_copy",learning_packet_2022(5,2));
    fixture_matches("learn_bound_id_copy9",learning_packet_2022(3432,9));
    fixture_matches("silent_remove",learned_count_packet_2022(5,0));
    fixture_matches("silent_single",learned_count_packet_2022(5,1));
    fixture_matches("silent_second_copy",learned_count_packet_2022(5,2));
    fixture_matches("silent_capture81",learned_count_packet_2022(3,81));
    const auto professions=profession_packets_2022(42,8,7,1790);
    fixture_matches("profession_state",professions[0]);
    fixture_matches("unlocked_professions",professions[1]);
    require(professions[0][0]==182&&professions[1][0]==181,
        "unlock mask must follow profession state initialization");
    auto rejects=[&](auto make_packet,const char* message){
        bool rejected=false;try{(void)make_packet();}catch(const std::invalid_argument&){rejected=true;}
        require(rejected,message);
    };
    rejects([]{return learning_packet_2022(3,0);},"zero219 would trigger retail count assertion");
    for(const auto id:{0u,3433u,65535u,65536u,0xffffffffu}){
        rejects([&]{return learning_packet_2022(id,1);},"invalid ID encoded as new learning");
        rejects([&]{return learned_count_packet_2022(id,0);},"invalid ID encoded as silent removal");
    }
    rejects([]{return profession_packets_2022(0,8,7,1790);},"zero profession actor accepted");
    rejects([]{return profession_packets_2022(42,11,7,1790);},"invalid primary profession accepted");
    rejects([]{return profession_packets_2022(42,8,11,1790);},"invalid secondary profession accepted");

    require(skill_catalog_2022.size()==3433&&!find_skill(3433)&&!find_skill(65535),"Count sentinel or invalid ID accepted");
    const auto& healing=*find_skill(1);const auto& power=*find_skill(5);
    require(healing.profession==1&&healing.attribute==21&&healing.energy_cost()==0&&
        healing.activation==2&&healing.aftercast==.75f&&healing.recharge==4,"pinned Healing Signet metadata changed");
    require(power.profession==5&&power.attribute==2&&power.campaign==1&&power.elite()&&
        power.activation==.25f&&power.aftercast==.75f&&power.energy_cost()==15&&power.recharge==20,
        "fractional activation or encoded15energy corrupted");
    require(find_skill(218)->energy_cost()==25,"encoded25energy corrupted");
    require(find_skill(189)->aoe_range==240.f,"Rodgort must retain its skill-specific240 metadata");
    unsigned nonzero_ranges=0;
    for(const auto& row:skill_catalog_2022){
        require(std::isfinite(row.aoe_range)&&row.aoe_range>=0&&row.aoe_range<=5000,
            "native area metadata must remain finite/bounded");
        nonzero_ranges+=row.aoe_range!=0;
    }
    require(nonzero_ranges==1050,"native area metadata inventory changed");
    for(unsigned id=0;id<skill_catalog_2022.size();++id)
        require(skill_catalog_2022[id].id==id,"catalogue row does not match canonical ID");

    Request request;request.actor=999;
    for(const auto* key:{"set_request","load_request","swap_request","move_request"}){
        const auto& packet=fixtures.at(key);
        for(std::size_t n=0;n<packet.size();++n){
            Request partial;partial.actor=999;partial.consumed=123;
            require(parse_request(std::span(packet).first(n),partial)==FrameState::incomplete&&
                partial.actor==999&&partial.consumed==123,"fragment changed output or accepted early");
        }
        auto coalesced=packet;coalesced.insert(coalesced.end(),{68,128,0,0,0,0});
        require(parse_request(coalesced,request)==FrameState::complete&&request.consumed==packet.size()&&
            request.actor==42,"coalesced request consumed next frame");
    }
    require(parse_request(fixtures.at("set_request"),request)==FrameState::complete&&
        request.change==Change::set&&request.slot==7&&request.first==Slot{5,9},"set request schema");
    require(parse_request(fixtures.at("load_request"),request)==FrameState::complete&&
        request.change==Change::load&&request.count==8&&request.ids[6]==152,"load request schema");
    require(parse_request(fixtures.at("swap_request"),request)==FrameState::complete&&
        request.change==Change::swap&&request.first==Slot{3,0}&&request.second==Slot{1,0},"swap uses skill/copy identities");
    require(parse_request(fixtures.at("move_request"),request)==FrameState::complete&&
        request.change==Change::move&&request.first==Slot{3,1}&&request.slot==7,"move request schema");
    auto malformed=fixtures.at("load_request");malformed[6]=9;request.actor=999;
    require(parse_request(malformed,request)==FrameState::invalid&&request.actor==999,"oversized load count accepted");
    require(parse_request(Packet{89,128,0,0,0,0},request)==FrameState::unknown_opcode&&request.actor==999,
        "unrelated command mutated skillbar request");

    Policy policy;policy.primary=1;policy.can_change=true;policy.learned={{1,1},{2,1},{3,3}};
    SkillbarState bar(42);Request load;load.actor=42;load.change=Change::load;load.count=4;load.ids={1,3,3,2};
    require(bar.apply(load,policy).accepted()&&bar.slots()[0]==Slot{1,0}&&bar.slots()[1]==Slot{3,0}&&
        bar.slots()[2]==Slot{3,1}&&bar.slots()[3]==Slot{2,0},"explicit duplicate Capture availability not respected");
    require(parse_request(fixtures.at("swap_request"),request)==FrameState::complete,"swap fixture parse");
    auto result=bar.apply(request,policy);
    require(result.accepted()&&bar.slots()[0]==Slot{3,0}&&bar.slots()[1]==Slot{1,0}&&
        result.authoritative==bar.snapshot()&&!result.authoritative.back(),"swap or resource-preserving reconciliation");
    parse_request(fixtures.at("move_request"),request);result=bar.apply(request,policy);
    require(result.accepted()&&bar.slots()[2]==Slot{}&&bar.slots()[7]==Slot{3,1},"move into empty slot failed");
    const auto stable=bar.slots();const auto stable_packet=bar.snapshot();
    auto unchanged=[&](const Request& command,const Policy& settings,Reason reason){
        const auto result=bar.apply(command,settings);
        require(result.status==reason&&bar.slots()==stable&&result.authoritative==stable_packet,
            "denial must be atomic and return authoritative rollback");
    };
    Request set;set.actor=42;set.change=Change::set;set.slot=4;set.first={3,0};
    unchanged(set,policy,Reason::copy);
    set.first={3,3};unchanged(set,policy,Reason::copy);
    set.first={3433,0};unchanged(set,policy,Reason::unknown_skill);
    set.first={44,0};unchanged(set,policy,Reason::not_learned);
    set.first={1,1};unchanged(set,policy,Reason::copy);
    set.first={0,1};unchanged(set,policy,Reason::copy);
    set.first={0,0};set.slot=8;unchanged(set,policy,Reason::slot);
    set.slot=4;set.actor=43;unchanged(set,policy,Reason::owner);
    set.actor=42;auto forbidden=policy;forbidden.can_change=false;unchanged(set,forbidden,Reason::mode);
    Request occupied;occupied.actor=42;occupied.change=Change::move;occupied.first={3,0};occupied.slot=1;
    unchanged(occupied,policy,Reason::occupied_slot);
    occupied.first={3,2};unchanged(occupied,policy,Reason::missing_slot);

    Policy mesmer;mesmer.primary=5;mesmer.can_change=true;mesmer.campaigns=1u<<1;
    mesmer.learned={{1,1},{5,1},{13,1}};Slots mixed{};mixed[0]={5,0};mixed[1]={1,0};
    require(validate(mixed,mesmer)==Reason::profession,"primary-only profession gate missing");
    mesmer.secondary=1;require(validate(mixed,mesmer)==Reason::accepted,"known secondary profession not permitted");
    auto no_campaign=mesmer;no_campaign.campaigns=0;
    require(validate(mixed,no_campaign)==Reason::campaign,"campaign grant confused with learning");
    mixed[1]={13,0};require(validate(mixed,mesmer)==Reason::multiple_elites,"two elite bar accepted");
    SkillbarState reconnect(42);require(reconnect.restore(stable,policy)==Reason::accepted&&
        reconnect.slots()==stable&&reconnect.snapshot(true).back()==1,"explicit persisted bar restore failed");
    require(validate(Slots{},Policy{})==Reason::accepted,"empty uncaptured bar fabricated skills or denied");
    Policy capture_copies;capture_copies.primary=1;capture_copies.can_change=true;capture_copies.learned={{3,81}};
    SkillbarState copied_bar(43);Request copy_set;copy_set.actor=43;copy_set.change=Change::set;copy_set.first={3,80};
    require(copied_bar.apply(copy_set,capture_copies).accepted()&&copied_bar.slots()[0]==Slot{3,80},
        "explicit captured81 copies incorrectly limited to equipped slots");
    const auto saved_copies=copied_bar.slots();copy_set.slot=1;copy_set.first.copy=81;
    require(copied_bar.apply(copy_set,capture_copies).status==Reason::copy&&copied_bar.slots()==saved_copies,
        "copy count treated as valid zero-based copy index");
    {
        Policy pve;pve.primary=8;pve.can_change=true;pve.campaigns=0x1e;
        pve.learned={{3,81},{1239,1},{2100,1},{2110,1},{2235,1}};
        for(const auto id:{3u,2100u,2110u,2235u})
            require((find_skill(id)->special&0x80000u)!=0,"pinned PvE-only classification changed");
        Slots three{};three[0]={2100,0};three[1]={2110,0};three[2]={2235,0};three[3]={1239,0};
        require(validate(three,pve)==Reason::accepted,"three different PvE skills with ordinary skills must be allowed");
        auto four=three;four[4]={3,80};
        require(validate(four,pve)==Reason::multiple_pve_skills,
            "Signet of Capture must count alongside three other PvE skills");
        Slots three_copies{};three_copies[0]={3,0};three_copies[1]={3,1};three_copies[2]={3,80};
        three_copies[3]={1239,0};
        require(validate(three_copies,pve)==Reason::accepted,
            "three distinct equipped Capture copies from81 learned copies must be allowed");
        auto four_copies=three_copies;four_copies[4]={3,2};
        require(validate(four_copies,pve)==Reason::multiple_pve_skills,
            "maximum three PvE skills counts slots rather than distinct skill IDs");
        SkillbarState pve_bar(44);
        require(pve_bar.restore(three_copies,pve)==Reason::accepted,"valid three-copy bar restore failed");
        const auto prior=pve_bar.slots();const auto prior_packet=pve_bar.snapshot();
        Request fourth;fourth.actor=44;fourth.change=Change::set;fourth.slot=4;fourth.first={3,2};
        const auto denied_set=pve_bar.apply(fourth,pve);
        require(denied_set.status==Reason::multiple_pve_skills&&pve_bar.slots()==prior&&
            denied_set.authoritative==prior_packet&&!denied_set.authoritative.back(),
            "fourth PvE slot denial must preserve state and reconcile optimistic edits");
        Request four_load;four_load.actor=44;four_load.change=Change::load;
        four_load.count=4;four_load.ids={3,3,3,3};
        const auto denied_load=pve_bar.apply(four_load,pve);
        require(denied_load.status==Reason::multiple_pve_skills&&pve_bar.slots()==prior&&
            denied_load.authoritative==prior_packet,"four-copy load denial must be atomic");
        require(pve_bar.restore(four_copies,pve)==Reason::multiple_pve_skills&&pve_bar.slots()==prior,
            "invalid persisted four-PvE bar must not replace current state");
        require(std::string(reason_text(Reason::multiple_pve_skills))=="bar contains more than three PvE-only skills",
            "PvE limit denial needs an explicit diagnostic");
    }
    // Read-only 2026-10-03 official capture: Ritualist/Assassin, eight real
    // equipped IDs. Check compatibility with the older pinned catalogue;
    // this fixture does not grant these skills to any server character.
    const std::array<unsigned,8> captured_ids{1239,1253,2110,1232,2100,2235,1237,791};
    Policy captured;captured.primary=8;captured.secondary=7;captured.campaigns=0x1e;
    Slots captured_bar{};
    for(unsigned n=0;n<8;++n){captured_bar[n]={captured_ids[n],0};captured.learned[captured_ids[n]]=1;}
    require(validate(captured_bar,captured)==Reason::accepted,"captured official bar is incompatible with pinned2022 policy");
    Learned edge{{3431,1}};const auto high=learned_packet_2022(edge);
    require(high.size()==436&&high[2]==108&&high[3]==0&&high[432]==128,
        "highest playable-era ID or learned bitfield size off by one");
    bool invalid=false;try{learned_packet_2022({{3433,1}});}catch(const std::invalid_argument&){invalid=true;}
    require(invalid,"Count sentinel encoded as learned skill");
    std::cout<<"PASS pinned catalogue, retail skillbar/learned-copy/profession wire, ownership/learning/profession/campaign/elite/PvE-slot gates, atomic changes and rollback\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
