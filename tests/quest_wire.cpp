#include "gw_quest_wire.h"
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
int main(int argc,char** argv)try{
    using namespace fw::gw;
    if(argc!=5)throw std::runtime_error("native quest fixtures required");
    std::ifstream input(argv[1]);if(!input)throw std::runtime_error("quest fixture unavailable");
    std::uint32_t state;std::string expected;unsigned count=0;
    while(input>>state>>expected){
        quests::Entry entry{101,state,12.5f,-4.25f,2,72,72,u"UW",u"Chamber",u"Soul"};
        auto bytes=quests::add_2022(entry);bytes[1]|=0x80;std::string actual;
        for(auto byte:bytes){actual+="0123456789abcdef"[byte>>4];actual+="0123456789abcdef"[byte&15];}
        if(actual!=expected)throw std::runtime_error("quest bytes differ from original serializer");++count;
    }
    if(count!=4||!input.eof())throw std::runtime_error("incomplete quest fixture");
    std::ifstream updates(argv[3]);count=0;
    while(updates>>state>>expected){
        quests::Entry entry{101,state,0,0,0,0,72,u"UW",u"Chamber",u"Soul"};
        auto bytes=quests::update_2022(entry);bytes[1]|=0x80;std::string actual;
        for(auto byte:bytes){actual+="0123456789abcdef"[byte>>4];actual+="0123456789abcdef"[byte&15];}
        if(actual!=expected)throw std::runtime_error("quest update differs from original serializer");++count;
    }
    if(count!=4||!updates.eof())throw std::runtime_error("incomplete native update fixture");
    bool rejected=false;try{quests::Entry entry;entry.id=101;entry.state=3;(void)quests::update_2022(entry);}catch(const std::invalid_argument&){rejected=true;}
    if(!rejected)throw std::runtime_error("native update loaded-bit assertion not fenced");
    std::ifstream detail_input(argv[2]);std::map<std::string,std::string> fixtures;std::string label,hex;
    while(detail_input>>label>>hex)if(!fixtures.emplace(label,hex).second)throw std::runtime_error("duplicate quest fixture");
    if(fixtures.size()!=5||!detail_input.eof())throw std::runtime_error("quest detail fixtures missing");
    auto native=[&](const char* label,items::Packet packet){packet[1]|=0x80;std::string actual;
        for(auto byte:packet){actual+="0123456789abcdef"[byte>>4];actual+="0123456789abcdef"[byte&15];}
        if(actual!=fixtures.at(label))throw std::runtime_error("quest detail bytes differ from original serializer");};
    native("empty",quests::details_2022(101,u"",u""));
    native("text",quests::details_2022(101,u"Reward",u"Kill foes"));
    native("maximum",quests::details_2022(101,std::u16string(127,u'D'),std::u16string(127,u'O')));
    native("remove",quests::remove_2022(101));
    native("marker",quests::marker_2022({101,0,12.5f,-4.25f,2,72}));
    std::ifstream objectives_input(argv[4]);fixtures.clear();
    while(objectives_input>>label>>hex)fixtures.emplace(label,hex);
    if(fixtures.size()!=3||!objectives_input.eof())throw std::runtime_error("objectives fixtures missing");
    native("empty",quests::objectives_2022(101,u""));
    native("text",quests::objectives_2022(101,u"Return for reward"));
    native("maximum",quests::objectives_2022(101,std::u16string(127,u'O')));
    auto refuses=[](auto action){bool failed=false;try{action();}catch(const std::invalid_argument&){failed=true;}
        if(!failed)throw std::runtime_error("invalid quest detail accepted");};
    refuses([]{quests::details_2022(0,u"",u"");});refuses([]{quests::remove_2022(0);});
    refuses([]{quests::details_2022(101,std::u16string(128,u'x'),u"");});
    refuses([]{quests::details_2022(101,u"",std::u16string(128,u'x'));});
    refuses([]{quests::details_2022(101,std::u16string{u'A',u'\0'},u"");});
    refuses([]{quests::marker_2022({101,0,std::numeric_limits<float>::infinity()});});
    auto rejects=[](quests::Entry entry){bool failed=false;try{quests::add_2022(entry);}catch(const std::invalid_argument&){failed=true;}if(!failed)throw std::runtime_error("invalid quest entry accepted");};
    quests::Entry entry;entry.id=101;
    auto bad=entry;bad.id=0;rejects(bad);
    bad=entry;bad.x=std::numeric_limits<float>::quiet_NaN();rejects(bad);
    bad=entry;bad.y=std::numeric_limits<float>::infinity();rejects(bad);
    for(unsigned field=0;field<3;++field){
        bad=entry;auto* name=field==0?&bad.location:field==1?&bad.name:&bad.npc;
        *name=std::u16string(8,u'x');rejects(bad);
        *name=std::u16string{u'x',u'\0',u'y'};rejects(bad);
    }
    if(quests::add_2022(entry).size()!=30)throw std::runtime_error("empty encoded names lost wire framing");
    std::cout<<"PASS native quest fixtures and marker/string bounds\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
