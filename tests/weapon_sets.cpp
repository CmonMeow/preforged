#include "gw_weapon_sets.h"
#include <fstream>
#include <iostream>
#include <map>
#include <string>

int main(int argc,char** argv)try{
    using namespace fw::gw::weapon_sets;
    auto check=[](bool okay,const char* why){if(!okay)throw std::runtime_error(why);};
    auto rejects=[&](auto operation,const char* why){bool threw=false;try{operation();}catch(const std::invalid_argument&){threw=true;}check(threw,why);};
    check(argc==2,"original2022 weapon-set fixture required");std::ifstream input(argv[1]);check(bool(input),"fixture missing");
    std::map<std::string,fw::gw::items::Packet> fixture;std::string label,hex;
    while(input>>label>>hex){check(hex.size()%2==0,"invalid hex fixture");fw::gw::items::Packet packet;
        for(std::size_t at=0;at<hex.size();at+=2)packet.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(at,2),nullptr,16)));
        check(fixture.emplace(label,std::move(packet)).second,"duplicate fixture");}
    auto match=[&](const char* name,fw::gw::items::Packet packet){packet[1]|=0x80;check(packet==fixture.at(name),"original serializer mismatch");};
    match("set",set_2022(1,2,{43,44}));match("active",active_2022(1,2));match("cancel",cancel_2022());
    check(switch_request_2022(fixture.at("switch"))==2,"original switch sender fixture not parsed");
    check(switch_request_2022({48,0,3})==3,"unframed switch rejected");
    for(auto packet:{fw::gw::items::Packet{48,0},fw::gw::items::Packet{48,0,0,0},fw::gw::items::Packet{48,0,4},fw::gw::items::Packet{49,0,0},fw::gw::items::Packet{48,1,0}})
        check(!switch_request_2022(packet),"malformed or different opcode accepted");
    std::map<std::uint32_t,Item> items{{42,{27,1,0}},{43,{26,1,0x200000}},{44,{24,1,0}},{45,{12,1,0}}};
    Lookup owned=[&](std::uint32_t id)->std::optional<Item>{const auto found=items.find(id);if(found==items.end())return {};return found->second;};
    State state{{Set{42,44},Set{43,44},Set{42,45},Set{}},1};validate(state,owned);
    check(effective(state,owned)==Set{43,0},"two-handed hides effective offhand");
    check(state.sets[1]==Set{43,44},"two-handed must preserve stored offhand");
    check(effective(switched(state,0),owned)==Set{42,44},"one-handed active pair restored");
    const auto packets=restore_2022(1,state,owned);check(packets.size()==5,"complete four-set restoration missing");
    for(unsigned at=0;at<4;++at)check(packets[at]==set_2022(1,at,state.sets[at]),"restoration order or set identity changed");
    check(packets.back()==active_2022(1,1),"active selection must follow all memberships");
    auto edited=equipped(state,Hand::weapon,42);check(edited.sets[1]==Set{42,44}&&edited.sets[0]==state.sets[0],"edit only active set");validate(edited,owned);
    auto removed=unequipped(edited,Hand::weapon,42);check(removed.sets[1].weapon==0&&removed.sets[0].weapon==42&&removed.sets[2].weapon==42,"unequip must preserve other stored references");
    check(unequipped(edited,Hand::weapon,43)==edited,"unrelated physical item must not clear active reference");
    auto deleted=erased(state,44);check(deleted.sets[0].offhand==0&&deleted.sets[1].offhand==0&&deleted.sets[2].offhand==45&&deleted.active==1,"erase all and only deleted references");
    items.erase(44);validate(deleted,owned);rejects([&]{validate(state,owned);},"stale destroyed item accepted");
    items.emplace(44,Item{24,1,0});
    auto bad=state;bad.active=4;rejects([&]{validate(bad,owned);},"invalid active state accepted");
    bad=state;bad.sets[3]={44,0};rejects([&]{validate(bad,owned);},"offhand item used as mainhand");
    bad=state;bad.sets[3]={0,42};rejects([&]{validate(bad,owned);},"weapon used as offhand");
    bad=state;bad.sets[3]={42,42};rejects([&]{validate(bad,owned);},"same item in both hands");
    bad=state;bad.sets[3]={99,0};rejects([&]{validate(bad,owned);},"unowned item accepted");
    items.emplace(99,Item{0,1,0});rejects([&]{validate(bad,owned);},"reserved/salvage type default native slot accepted");
    items[99]={27,2,0};rejects([&]{validate(bad,owned);},"stackable equipment accepted");
    rejects([&]{validate(state,{});},"populated state accepted without ownership lookup");validate(State{},{});
    rejects([]{(void)set_2022(0,0,{});},"zero inventory accepted");rejects([]{(void)set_2022(1,4,{});},"out-of-range membership accepted");
    rejects([]{(void)set_2022(1,0,{42,42});},"duplicate raw pair accepted");rejects([]{(void)active_2022(1,4);},"out-of-range selection accepted");
    rejects([&]{(void)switched(state,4);},"invalid switch accepted");rejects([&]{(void)equipped(state,Hand::weapon,0);},"zero equip accepted");
    rejects([&]{(void)unequipped(state,Hand::weapon,0);},"zero unequip accepted");rejects([&]{(void)erased(state,0);},"zero erase accepted");
    std::cout<<"PASS weapon-set native fixtures, ownership, shared references, two-handed, restoration and removal\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
