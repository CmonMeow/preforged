#include "gw_emotes.h"
#include "gw_chat_commands.h"
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F&& operation,const char* message){
    bool rejected=false;try{operation();}catch(const std::invalid_argument&){rejected=true;}check(rejected,message);
}
std::map<std::string,fw::gw::emotes::Packet> fixtures(const char* path){
    std::ifstream input(path);check(bool(input),"cannot open native emote fixtures");
    std::map<std::string,fw::gw::emotes::Packet> result;std::string line;
    while(std::getline(input,line)){
        if(line.empty()||line[0]=='#')continue;
        std::istringstream row(line);std::string name,hex;row>>name>>hex;
        check(!name.empty()&&hex.size()>=4&&hex.size()%2==0,"invalid emote fixture row");
        fw::gw::emotes::Packet packet;
        for(std::size_t n=0;n<hex.size();n+=2)packet.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(n,2),nullptr,16)));
        packet[1]&=0x7f;check(result.emplace(name,std::move(packet)).second,"duplicate emote fixture");
    }
    check(result.size()==6,"native emote fixture count");return result;
}
}
int main(int argc,char** argv)try{
    check(argc==2,"usage: emotes_test <fixtures>");namespace e=fw::gw::emotes;
    namespace c=fw::gw::chat::commands;
    const auto native=fixtures(argv[1]);
    check(e::dance_2022(42)==native.at("dance")&&e::reset_animation_2022(42)==native.at("reset"),
        "animation packet differs from pinned retail serializer");
    check(e::effects_2022(42,e::sitting)==native.at("sit")&&e::effects_2022(42,0)==native.at("stand")&&
        e::effects_2022(42,0x2082)==native.at("sit_with_effects")&&e::effects_2022(42,0x82)==native.at("stand_with_effects"),
        "sitting packet differs from pinned retail serializer or loses effects");
    for(auto id:{0u,65536u,0xffffffffu}){
        rejects([&]{e::dance_2022(id);},"unbounded animation player accepted");
        rejects([&]{e::effects_2022(id,0);},"unbounded effect player accepted");
    }
    for(const auto& text:{u"dance",u"DANCE *",u"sit",u"SiT *",u"afk",u"AFK *",u"stand"})
        check(c::parse(text).kind==c::Kind::emote&&c::parse(text).emote.has_value(),"proven emote command rejected");
    check(c::parse(u"afk").emote->action==e::Action::sit&&c::parse(u"dance *").emote->synchronized,
        "sit alias or synchronized request lost");
    for(const auto& text:{u"dance 1",u"dance **",u"dance * extra",u"sit Bob",u"stand *",u"stand extra"})
        check(c::parse(text).kind==c::Kind::invalid&&!c::parse(text).emote,"malformed emote produced an animation request");
    for(const auto& text:{u"dancenew",u"wave",u"kneel",u"guild",u"rank",u"aion"})
        check(c::parse(text).kind==c::Kind::unsupported&&!c::parse(text).emote,"unproven/entitled animation invented");

    e::State state;auto result=state.submit(42,{e::Action::dance,false},1000,0x82);
    check(result.packets==std::vector<e::Packet>{native.at("dance")}&&!result.effects&&result.stop_movement&&
        result.announcement==u"starts dancing."&&state.dancing(),"dance did not schedule a real loop or fabricated pose bits");
    check(state.snapshot(42,0x82)==result.packets,"late observer cannot initialize current dance");
    result=state.submit(42,{e::Action::stand,false},1100,0x82);
    check(result.packets.empty()&&!result.effects&&!result.stop_movement&&state.dancing(),"stand incorrectly cancels an unsitting dance");
    result=state.submit(42,{e::Action::sit,false},1200,0x82);
    check(result.packets==std::vector<e::Packet>{native.at("reset"),native.at("sit_with_effects")}&&result.effects==0x2082u&&
        result.stop_movement&&result.announcement.empty()&&!state.dancing(),"sit lost unrelated effects, missed loop reset, or invented chat");
    check(state.snapshot(42,0x2082)==std::vector<e::Packet>{native.at("sit_with_effects")},"late observer cannot initialize seated pose");
    result=state.submit(42,{e::Action::sit,false},1300,0x2082);
    check(result.packets.empty()&&!result.effects&&result.stop_movement,"repeated sit emits redundant state replacement");
    result=state.submit(42,{e::Action::stand,false},1400,0x2082);
    check(result.packets==std::vector<e::Packet>{native.at("stand_with_effects")}&&result.effects==0x82u&&
        !result.stop_movement&&result.announcement.empty(),"stand lost unrelated effects or invented chat");
    result=state.submit(42,{e::Action::dance,false},1500,0x2082);
    check(result.packets==std::vector<e::Packet>{native.at("stand_with_effects"),native.at("dance")}&&result.effects==0x82u,
        "dance failed to leave seated pose before animation");
    result=state.cancel(42,0x82);
    check(result.packets==std::vector<e::Packet>{native.at("reset")}&&!state.dancing()&&!state.scheduled(),"movement cancellation missed loop reset");
    check(state.cancel(42,0x82).packets.empty(),"free movement emitted a redundant animation reset");
    result=state.cancel(42,0x2082);
    check(result.effects==0x82u&&result.packets==std::vector<e::Packet>{native.at("stand_with_effects")},"movement did not leave sitting pose safely");

    e::State a,b;auto first=a.submit(42,{e::Action::dance,true},1010,0);
    auto second=b.submit(43,{e::Action::dance,true},4999,0);
    check(first.packets.empty()&&first.announcement.empty()&&second.packets.empty()&&a.scheduled()&&b.scheduled(),
        "synchronized request announced or animated before shared boundary");
    check(a.tick(42,4999,0).packets.empty()&&b.tick(43,4999,0).packets.empty(),"synchronized request started early");
    check(a.tick(42,5000,0).packets==std::vector<e::Packet>{e::dance_2022(42)}&&
        b.tick(43,5000,0).packets==std::vector<e::Packet>{e::dance_2022(43)}&&!a.scheduled()&&!b.scheduled(),
        "synchronized request missed common five-second boundary");
    check(a.tick(42,5010,0).packets.empty(),"looping emote resent every world tick");
    e::State exact;check(!exact.submit(42,{e::Action::dance,true},10000,0).packets.empty(),"exact synchronization boundary delayed five seconds");
    e::State pending;pending.submit(42,{e::Action::dance,true},10001,0);pending.cancel(42,0);
    check(pending.tick(42,15000,0).packets.empty()&&!pending.scheduled(),"movement failed to cancel pending synchronized emote");
    pending.submit(42,{e::Action::dance,true},15001,0);pending.submit(42,{e::Action::sit,false},15002,0);
    check(pending.tick(42,20000,e::sitting).packets.empty()&&!pending.scheduled(),"later command failed to replace synchronized request");
    pending.submit(42,{e::Action::dance,true},20001,0);
    check(pending.tick(42,25000,0x10).packets.empty()&&!pending.scheduled(),"dead character performed a pending dance");
    pending.submit(42,{e::Action::dance,true},25001,0);
    rejects([&]{pending.submit(42,{e::Action::dance,false},25002,0x10);},"dead character emote accepted");
    rejects([&]{pending.submit(42,{static_cast<e::Action>(77),false},25002,0);},"unknown action accepted");
    rejects([&]{pending.submit(42,{e::Action::stand,true},25002,0);},"unproven stand synchronization accepted");
    rejects([&]{pending.submit(42,{e::Action::dance,true},std::numeric_limits<std::uint64_t>::max(),0);},"synchronized deadline wrapped");
    check(pending.scheduled()&&!pending.tick(42,30000,0).packets.empty(),"rejected request mutated existing schedule");
    e::State fresh;check(!fresh.dancing()&&!fresh.scheduled()&&fresh.snapshot(42,0).empty(),"new player inherited previous emote state");
    std::cout<<"PASS native emote bytes, dance/sit/stand state, effect preservation, synchronized timing, cancellation and transactional rejection\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
