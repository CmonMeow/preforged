#include "gw_chat_commands.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <limits>

namespace {
void require(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
using fw::gw::world::Packet;
struct Message {std::uint16_t sender;std::uint8_t channel;std::u16string text;};
std::vector<Message> messages(const std::vector<Packet>& packets){
    std::vector<Message> out;std::u16string encoded;
    for(const auto& packet:packets){
        if(packet.size()<2)throw std::runtime_error("short response packet");
        const auto opcode=packet[0]|(unsigned(packet[1])<<8);
        if(opcode==92){
            require(packet.size()>=4,"short chat core");
            const auto count=packet[2]|(unsigned(packet[3])<<8);
            require(packet.size()==4+count*2&&count<=121,"chat chunk outside retail bound");
            for(std::size_t at=4;at<packet.size();at+=2)encoded.push_back(char16_t(packet[at]|(unsigned(packet[at+1])<<8)));
        }else if(opcode==96){
            require(packet.size()==5&&encoded.size()>=4&&encoded[0]==0x108&&encoded[1]==0x107&&encoded.back()==1,
                "chat message lacks literal wrapper or terminator");
            std::u16string body(encoded.begin()+2,encoded.end()-1);
            require(fw::gw::chat::valid_text(body),"response exceeds text bound or corrupts UTF-16");
            out.push_back({std::uint16_t(packet[2]|(unsigned(packet[3])<<8)),packet[4],std::move(body)});encoded.clear();
        }
    }
    require(encoded.empty(),"unfinished chat core in response");return out;
}
}
int main()try{
    namespace commands=fw::gw::chat::commands;using commands::Kind;
    for(const auto& body:{u"RoLl 2",u"roll 100",u"roll 002"})
        require(commands::parse(body).kind==Kind::roll,"valid bounded roll command rejected");
    for(const auto& body:{u"roll",u"roll 0",u"roll 1",u"roll 101",u"roll -2",u"roll +2",u"roll 2x",u"roll 2 3",u"roll 999999999999999"})
        require(commands::parse(body).kind==Kind::invalid,"invalid/overflow roll command accepted");
    for(const auto& body:{u"tp 1",u"TrAvEl 65535 ",u" tp 00055"})
        require(commands::parse(body).kind==Kind::travel,"valid travel command not classified");
    for(const auto& body:{u"tp",u"tp 0",u"tp 65536",u"tp -1",u"travel 1 junk"})
        require(commands::parse(body).kind==Kind::invalid,"malformed map command must give syntax feedback");
    const auto invitation=commands::parse(u"  InViTe  MiXeD Name  ");
    require(invitation.kind==Kind::invite&&invitation.target==u"MiXeD Name","command parser changed target spelling/case");
    const auto whisper=commands::parse(u"TeLl  MiXeD Name , Case sensitive \u6797 \U0001f600");
    require(whisper.kind==Kind::whisper&&whisper.target==u"MiXeD Name"&&whisper.message==u"Case sensitive \u6797 \U0001f600",
        "whisper parsing changed UTF-16 payload or recipient");
    require(commands::same_name(u"MiXeD Name",u"mixed name")&&!commands::same_name(u"\u6797",u"\u6798"),
        "name comparison is not case-safe or loses Unicode");
    for(const auto& body:{u"whisper Bob",u"whisper , message",u"whisper Bob,",u"invite",u"kick",u"leave extra",u"hp extra",u"a"})
        require(commands::parse(body).kind==Kind::invalid,"missing/malformed arguments were accepted");
    require(commands::parse(u"COMMANDS CHAT").kind==Kind::help,"help section is not case-insensitive");
    require(commands::parse(u"dance *").kind==Kind::emote&&commands::parse(u"guild").kind==Kind::unsupported&&
        commands::parse(u"deaths").kind==Kind::unsupported&&commands::parse(u"not_a_command").kind==Kind::unknown,
        "unimplemented behavior was misrepresented as supported");
    for(const auto& text:std::vector<std::u16string>{u"hp\n",std::u16string(257,u'x'),{u'h',0xd800}})
        require(commands::parse(text).kind==Kind::invalid,"malformed command text accepted");
    const auto max_whisper=std::u16string(u"Bob,")+std::u16string(252,u'x');
    require(commands::whisper(max_whisper).kind==Kind::whisper,"valid256-unit native whisper acquired an artificial prefix limit");
    commands::Session rate;
    for(unsigned n=0;n<8;++n)require(rate.admit(1000),"rate burst is too small");
    require(!rate.admit(1000)&&!rate.admit(900)&&rate.flood_feedback(1000)&&!rate.flood_feedback(1100),
        "burst cap, clock regression or feedback cooldown failed");
    require(rate.admit(1250)&&!rate.admit(1250)&&rate.flood_feedback(2000),"rate refill/cooldown failed");
    require(rate.admit(std::numeric_limits<std::uint64_t>::max()),"rate elapsed time overflow");
    commands::Session random;random.random.emplace(12345);
    for(const auto sides:{2u,3u,17u,100u})for(unsigned n=0;n<1000;++n){const auto value=random.roll(sides);
        require(value>=1&&value<=sides,"uniform dice result out of range");}

    using namespace fw::gw::world;
    Instance world;auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>(),c=std::make_shared<Outbox>();
    world.join(1,"Alice",0,0,a);world.join(2,"Bob",0,0,b);world.join(3,"Observer",0,0,c);
    auto clear=[&]{a->drain();b->drain();c->drain();};clear();
    std::uint64_t now=1000;
    auto send=[&](std::uint32_t player,const std::u16string& text,const commands::Context& context=commands::Context{}){
        now+=1000;require(world.chat(player,text,now,context),"valid command rejected or failed world session");
    };
    send(1,u"/Hp");auto feedback=messages(a->drain());
    require(feedback.size()==1&&feedback[0].channel==0&&feedback[0].text==u"Health and energy are not initialized yet."&&
        b->drain().empty()&&c->drain().empty(),"uncaptured vitals invented or error leaked to peers");
    world.set_vitals(1,{75,100,7,20},100);clear();send(1,u"/health");
    require(a->drain()==fw::gw::chat::local(1,0,u"Health 75/100, Energy 7/20")&&b->drain().empty()&&c->drain().empty(),
        "health command did not use current owner vitals");
    world.set_experience(1,0);send(1,u"/Xp");
    require(a->drain()==fw::gw::chat::local(1,0,u"Experience: 0."),"known zero experience confused with uncaptured");
    world.set_experience(1,std::nullopt);send(1,u"/xp");
    require(messages(a->drain())[0].text==u"Experience has not been captured for this character.","missing XP was invented");
    commands::Context context;context.party_changes_allowed=true;context.map_elapsed_ms=90001;context.server_address=u"127.0.0.1:8112";
    send(1,u"/age",context);require(messages(a->drain())[0].text==u"You have been in this private map for 1 minute(s). Account and character ages are unavailable.",
        "map elapsed time invented account/character ages");
    send(1,u"/ip",context);require(messages(a->drain())[0].text==u"Server: 127.0.0.1:8112","server address ignored supplied context");
    send(1,u"/a MiXeD \u6797 \U0001f600");const auto local=fw::gw::chat::local(1,3,u"MiXeD \u6797 \U0001f600");
    require(a->drain()==local&&b->drain()==local&&c->drain()==local,"local alias changed text/channel/delivery");
    send(1,u"/tr Trade");const auto trade=fw::gw::chat::local(1,12,u"Trade");
    require(a->drain()==trade&&b->drain()==trade&&c->drain()==trade,"trade alias channel/delivery changed");
    send(1,u"/p Solo");require(a->drain()==fw::gw::chat::local(1,11,u"Solo")&&b->drain().empty()&&c->drain().empty(),
        "solo party alias leaked to others");
    send(1,u"/w bOB, Private \u6797");
    require(b->drain()==fw::gw::chat::local(1,14,u"Private \u6797")&&messages(a->drain())[0].text==u"Whisper sent to Bob."&&c->drain().empty(),
        "whisper lost sender/body, changed name case, or leaked to bystander");
    send(1,u"\"Bob,"+std::u16string(252,u'x'));
    require(b->drain()==fw::gw::chat::local(1,14,std::u16string(252,u'x'))&&messages(a->drain()).size()==1&&c->drain().empty(),
        "native quote whisper lost maximum-length valid payload");
    send(1,u"/whisper Missing, Secret");
    require(messages(a->drain())[0].text.find(u"not in this map")!=std::u16string::npos&&b->drain().empty()&&c->drain().empty(),
        "missing whisper recipient leaked body or fabricated delivery");
    send(1,u"/invite bOB");require(messages(a->drain())[0].text.find(u"town or outpost")!=std::u16string::npos&&
        b->drain().empty()&&c->drain().empty(),"party invite ignored authoritative staging permission");
    send(1,u"/invite bOB",context);require(!a->drain().empty()&&!b->drain().empty()&&c->drain().empty(),"invite alias failed name/consent handling");
    require(world.party_action(2,154,1,now),"explicit invitation consent failed");clear();
    send(1,u"/p Team");const auto team=fw::gw::chat::local(1,11,u"Team");
    require(a->drain()==team&&b->drain()==team&&c->drain().empty(),"party alias escaped merged party");
    for(const auto& restricted:{u"/kick Bob",u"/leave"}){send(1,restricted);
        require(messages(a->drain())[0].text.find(u"town or outpost")!=std::u16string::npos&&b->drain().empty()&&c->drain().empty(),
            "party kick/leave ignored authoritative staging permission");}
    send(1,u"/p Still team");const auto still_team=fw::gw::chat::local(1,11,u"Still team");
    require(a->drain()==still_team&&b->drain()==still_team&&c->drain().empty(),"denied party change mutated membership");
    send(2,u"/kick Alice",context);require(messages(b->drain())[0].text.find(u"Unable")!=std::u16string::npos&&a->drain().empty()&&c->drain().empty(),
        "nonleader kick changed party or spammed peers");
    send(1,u"/kick bOb",context);clear();send(1,u"/p Alone");
    require(a->drain()==fw::gw::chat::local(1,11,u"Alone")&&b->drain().empty()&&c->drain().empty(),"kick failed to separate parties");
    send(1,u"/invite Bob",context);clear();require(world.party_action(2,154,1,now),"second invitation consent failed");clear();
    send(2,u"/leave",context);clear();send(1,u"/p Alone again");
    require(a->drain()==fw::gw::chat::local(1,11,u"Alone again")&&b->drain().empty()&&c->drain().empty(),"leave failed to separate parties");
    send(1,u"/roll 100");require(messages(a->drain())[0].text.find(u"not available")!=std::u16string::npos&&b->drain().empty()&&c->drain().empty(),
        "roll enabled without authoritative area permission");
    context.roll_allowed=true;send(1,u"/RoLl 100",context);const auto own_roll=a->drain();
    require(b->drain()==own_roll&&c->drain()==own_roll,"dice result inconsistent between recipients");
    const auto rolled=messages(own_roll);require(rolled.size()==1&&rolled[0].sender==1&&rolled[0].channel==6&&
        rolled[0].text.starts_with(u"Alice rolls ")&&rolled[0].text.ends_with(u" on a 100-sided die."),"roll channel/text wrong");
    for(const auto& command:{u"/roll 101",u"/deaths",u"/dance",u"/unknown",u"/tp -1"}){
        send(1,command);require(messages(a->drain()).size()==1&&b->drain().empty()&&c->drain().empty(),"command error not private");
    }
    send(1,u"/commands");require(messages(a->drain()).size()==4&&b->drain().empty()&&c->drain().empty(),"help missing sections or leaked to peers");
    std::optional<std::uint16_t> destination;context.travel_destination=&destination;
    send(1,u"/TrAvEl 00055 ",context);
    require(destination==55&&a->drain().empty()&&b->drain().empty()&&c->drain().empty(),"admitted travel did not request authoritative transfer");
    send(1,u"/tp 65536",context);
    require(!destination&&messages(a->drain()).size()==1&&b->drain().empty()&&c->drain().empty(),"invalid travel retained old destination");
    context.travel_destination=nullptr;send(1,u"/tp 55");
    require(messages(a->drain())[0].text==u"Map travel is not available in this session.","unwired travel fabricated a transfer");
    require(!world.chat(999,u"/commands",now)&&!world.chat(1,u"/roll\n",now)&&a->drain().empty()&&b->drain().empty()&&c->drain().empty(),
        "invalid sender/control text consumed response or threw");
    world.steer(1,10000,0,now);clear();const auto stopping_at=now+50;
    bool cancel_cast=false;context.cancel_cast=&cancel_cast;
    require(world.chat(1,u"/stuck",stopping_at,context)&&cancel_cast&&std::abs(world.position(1,stopping_at).first-14.4f)<.01f,
        "stuck discarded final partial held-input interval");clear();
    world.tick(stopping_at+1000);require(std::abs(world.position(1,stopping_at+1000).first-14.4f)<.01f,"stuck resumed movement");
    send(1,u"/hp",context);require(!cancel_cast&&messages(a->drain()).size()==1,"non-stuck command retained stale cast cancellation");

    Instance flood;auto owner=std::make_shared<Outbox>(),peer=std::make_shared<Outbox>();
    flood.join(1,"Owner",0,0,owner);flood.join(2,"Peer",0,0,peer);owner->drain();peer->drain();
    for(unsigned n=0;n<100;++n)require(flood.chat(1,u"/unknown",1000),"flood handling failed session");
    const auto limited=messages(owner->drain());require(limited.size()==9&&limited.back().text.find(u"too quickly")!=std::u16string::npos&&peer->drain().empty(),
        "unknown command flood exceeded bounded owner-only feedback");
    commands::Context travel;travel.travel_destination=&destination;destination=std::uint16_t{148};
    travel.cancel_cast=&cancel_cast;cancel_cast=true;
    require(flood.chat(1,u"/tp 55",1000,travel)&&!destination&&owner->drain().empty()&&peer->drain().empty(),
        "travel command bypassed rate admission or retained stale destination");
    require(!cancel_cast,"rate drop retained stale cast cancellation");cancel_cast=true;
    require(flood.chat(1,u"/stuck",1000,travel)&&!cancel_cast&&owner->drain().empty()&&peer->drain().empty(),
        "stuck bypassed rate admission or canceled a cast while rejected");
    require(flood.chat(1,u"!Recovered",1250)&&owner->drain()==fw::gw::chat::local(1,3,u"Recovered")&&
        peer->drain()==fw::gw::chat::local(1,3,u"Recovered"),"rate-limited session failed to recover");
    flood.leave(1);peer->drain();flood.join(1,"New Owner",0,0,owner);owner->drain();peer->drain();
    require(flood.chat(1,u"!Fresh",1250)&&peer->drain()==fw::gw::chat::local(1,3,u"Fresh"),"reused actor inherited exhausted chat state");
    std::cout<<"PASS bounded native commands, real vitals, private feedback/whispers, consent-based party changes, roll permission and flood recovery\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
