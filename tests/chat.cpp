#include "gw_world_simulation.h"
#include "gw_callout.h"
#include "gw_party_wire.h"
#include <iostream>
int main()try{
    auto retail_hex=[](fw::gw::chat::Packet packet){packet[1]|=0x80;std::string hex;const char* digits="0123456789abcdef";for(auto b:packet){hex+=digits[b>>4];hex+=digits[b&15];}return hex;};
    // Original pinned serializer fixtures, tools/protocol/global_chat_2022.py.
    if(retail_hex(fw::gw::chat::global(9,u"Fixture Sender",u"",u"fixture").back())!=
        "5f80090e0046006900780074007500720065002000530065006e006400650072000000")
        throw std::runtime_error("guild global sender packet differs from original serializer");
    if(retail_hex(fw::gw::chat::global(0,u"Fixture Sender",u"TEST",u"fixture").back())!=
        "5f80000e0046006900780074007500720065002000530065006e0064006500720004005400450053005400")
        throw std::runtime_error("alliance global tag packet differs from original serializer");
    const auto party=fw::gw::party::solo(7,9);
    if(party!=std::vector<std::vector<std::uint8_t>>{{0xd1,1,7,0},{0xca,1,7,0,9,0,1},{0xd2,1,7,0},{0xb1,1,7,0,1}})
        throw std::runtime_error("party stream field order");
    const std::vector<std::uint16_t> members{9,12};
    auto leader=fw::gw::party::roster(7,members,9),follower=fw::gw::party::roster(7,members,12);
    if(leader.size()!=5||leader[2]!=std::vector<std::uint8_t>{0xca,1,7,0,12,0,1}||leader.back().back()!=1||follower.back().back()!=0)
        throw std::runtime_error("party roster member order and recipient leader flag");
    leader.back().back()=0;
    if(leader!=follower)throw std::runtime_error("party members receive inconsistent roster");
    for(const auto& invalid:std::vector<std::vector<std::uint16_t>>{{},{9,9},{9,0},{12}}){
        bool rejected=false;try{fw::gw::party::roster(7,invalid,9);}catch(const std::invalid_argument&){rejected=true;}
        if(!rejected)throw std::runtime_error("invalid party roster accepted");
    }
    if(fw::gw::callout::target_response(0x1234,0x12345678)!=std::vector<std::uint8_t>{52,0,0x34,0x12,0x78,0x56,0x34,0x12})
        throw std::runtime_error("target call field order");
    for(std::uint8_t kind:{1,2,5,7,9,10}){
        const std::vector<std::uint8_t> wire{33,128,kind,1,0,0,0};
        const auto call=fw::gw::callout::parse(wire);
        if(!call||std::uint8_t(call->kind)!=kind||call->argument!=1)throw std::runtime_error("captured callout decode");
        for(std::size_t n=0;n<wire.size();++n)if(fw::gw::callout::parse(std::span(wire).first(n)))
            throw std::runtime_error("truncated callout accepted");
    }
    if(fw::gw::callout::parse(std::vector<std::uint8_t>{33,128,255,1,0,0,0}))throw std::runtime_error("unknown callout accepted");
    using namespace fw::gw::world;
    Instance world;auto a=std::make_shared<Outbox>(),b=std::make_shared<Outbox>();
    world.join(1,"Alice",0,0,a);world.join(2,"Bob",0,0,b);a->drain();b->drain();
    const Packet drawing{41,0,123,0,0,0,2,0,0xe7,0xff,40,0,50,0,0xc4,0xff};
    if(!world.draw_compass(1,drawing)||a->drain()!=std::vector<Packet>{{144,0,1,0,123,0,0,0,2,0,0xe7,0xff,40,0,50,0,0xc4,0xff}}||!b->drain().empty())
        throw std::runtime_error("compass preserves packed signed points and solo-party scope");
    for(std::size_t size=0;size<drawing.size();++size)
        if(world.draw_compass(1,std::span(drawing).first(size)))throw std::runtime_error("truncated compass accepted");
    if(world.draw_compass(99,drawing)||!a->drain().empty()||!b->drain().empty())
        throw std::runtime_error("unknown compass sender or rejected drawing emitted traffic");
    using fw::gw::callout::Kind;
    if(world.call_status(1,{Kind::health,1}))throw std::runtime_error("callout invented missing vitals");
    const auto vitals=world.set_vitals(1,{75,100,7,20},80);
    if(vitals.size()!=6||vitals[5]!=fw::gw::vitals::energy_regeneration_2022(1,2,20)||!a->drain().empty()||b->drain()!=std::vector<Packet>{vitals[1],vitals[3]})
        throw std::runtime_error("only health is broadcast; owner receives full state through caller");
    for(const auto& [kind,text]:std::vector<std::pair<Kind,std::u16string>>{
            {Kind::health,u"My Health is 75 of 100."},{Kind::energy,u"My Energy is 7 of 20."},
            {Kind::morale,u"My morale is 80%."}}){
        if(!world.call_status(1,{kind,1})||a->drain()!=fw::gw::chat::local(1,11,text)||!b->drain().empty())
            throw std::runtime_error("status call must use server values and remain in party");
    }
    if(world.call_status(1,{Kind::health,2})||world.call_status(99,{Kind::health,99})||
       world.call_status(1,{Kind::experience,1})||!a->drain().empty()||!b->drain().empty())
        throw std::runtime_error("invalid or unsupported status call sent");
    world.set_vitals(1,{50,100,3,20},100);
    if(b->drain()!=fw::gw::vitals::public_health_2022(1,{50,100,3,20}))
        throw std::runtime_error("health changes must reach existing observers");
    for(const std::uint32_t xp:{0u,123456u,4294967295u}){
        world.set_experience(1,xp);
        const auto text="I have "+std::to_string(xp)+" experience.";
        if(!world.call_status(1,{Kind::experience,1})||
           a->drain()!=fw::gw::chat::local(1,11,std::u16string(text.begin(),text.end()))||!b->drain().empty())
            throw std::runtime_error("experience must use stored progress and remain in party");
    }
    world.set_experience(1,std::nullopt);
    if(world.call_status(1,{Kind::experience,1})||!a->drain().empty())
        throw std::runtime_error("uncaptured experience must not become zero");
    if(!world.call_status(1,{Kind::energy,1})||a->drain()!=fw::gw::chat::local(1,11,u"My Energy is 3 of 20."))
        throw std::runtime_error("status call used stale values");
    for(const auto& invalid:std::vector<std::u16string>{u"!",u"!\n",u"!\x7f",
            {u'!',0},{u'!',0xd800},{u'!',0xdc00},{u'!',0xd800,u'A'},u"!"+std::u16string(257,u'x')}){
        if(world.chat(1,invalid)||!a->drain().empty()||!b->drain().empty())
            throw std::runtime_error("malformed chat must be rejected without delivery or throwing");
    }
    if(!world.chat(1,u"!Still connected")||a->drain().empty()||b->drain().empty())
        throw std::runtime_error("valid chat after rejected message");
    const auto unicode=std::u16string(118,u'x')+u"\U0001f600";
    const auto unicode_packets=fw::gw::chat::local(1,3,unicode);
    std::u16string assembled;
    for(std::size_t i=0;i+1<unicode_packets.size();++i){
        const auto& p=unicode_packets[i];
        for(std::size_t j=4;j<p.size();j+=2)assembled.push_back(char16_t(p[j]|(p[j+1]<<8)));
        const auto last=char16_t(p[p.size()-2]|(p.back()<<8));
        if(last>=0xd800&&last<=0xdbff)throw std::runtime_error("chat chunk split surrogate pair");
    }
    if(assembled!=std::u16string{u'\x108',u'\x107'}+unicode+u'\x1')
        throw std::runtime_error("Unicode chat changed during chunking");
    for(bool ready:{true,false}){
        if(!world.set_ready(1,ready)||world.is_ready(1)!=ready||world.set_ready(999,ready)||
           a->drain()!=std::vector<Packet>{Packet{0xd0,1,1,0,1,0,static_cast<std::uint8_t>(ready)}}||
           !b->drain().empty())throw std::runtime_error("ready state and party-scoped response");
    }
    if(!world.select_target(1,2,1)||world.selection(1)!=std::pair<std::uint32_t,std::uint32_t>{2,1})
        throw std::runtime_error("selection state");
    if(world.select_target(1,999,0)||world.select_target(1,0,999)||world.select_target(999,0,0)||
       world.selection(1)!=std::pair<std::uint32_t,std::uint32_t>{2,1})throw std::runtime_error("invalid selection changed state");
    if(!world.select_target(1,0,0)||world.selection(1)!=std::pair<std::uint32_t,std::uint32_t>{0,0}||
       !a->drain().empty()||!b->drain().empty())throw std::runtime_error("selection clear must not broadcast");
    if(!world.call_target(1,2)||world.call_target(1,999))throw std::runtime_error("target validation");
    if(a->drain()!=std::vector<Packet>{fw::gw::callout::target_response(1,2)}||!b->drain().empty())
        throw std::runtime_error("target call party isolation");
    world.chat(1,u"!Hello");auto own=a->drain(),peer=b->drain();
    if(own!=peer||own.size()!=2||own.back()!=Packet{96,0,1,0,3})throw std::runtime_error("local delivery");
    world.chat(1,u"$Trade");if(b->drain().back().back()!=12)throw std::runtime_error("trade channel");a->drain();
    world.chat(1,u"#Party");
    auto party_chat=a->drain();
    if(party_chat.size()!=2||party_chat.back()!=Packet{96,0,1,0,11}||!b->drain().empty())
        throw std::runtime_error("party channel isolation");
    world.chat(1,u"@Private");if(a->drain().empty()||!b->drain().empty())throw std::runtime_error("private-channel isolation");
    world.chat(1,u"/help");if(a->drain().empty()||!b->drain().empty())throw std::runtime_error("command response isolation");
    world.steer(1,1,0,1000);world.tick(1100);
    auto before=world.position(1,1100);a->drain();b->drain();
    world.chat(1,u"/Stuck ",1100);world.tick(2100);
    if(world.position(1,2100)!=before)throw std::runtime_error("stuck must stop steering without teleporting");
    auto recovery=b->drain();if(recovery.size()!=2||recovery[0][0]!=44||recovery[1][0]!=40)
        throw std::runtime_error("stuck broadcasts position and stop only");
    const auto large=fw::gw::chat::local(1,3,std::u16string(256,u'x'));
    if(large.size()!=4)throw std::runtime_error("chat chunking");
    world.select_target(1,2,2);world.leave(2);
    if(world.selection(1)!=std::pair<std::uint32_t,std::uint32_t>{0,0})throw std::runtime_error("departed target retained");
    std::cout<<"PASS chat delivery, channels, command feedback and chunking\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
