#include "gw_npc_dialogue.h"
#include "gw_guild_trim.h"
#include <fstream>
#include <iostream>
#include <map>
int main(int argc,char** argv)try{
    using namespace fw::gw;
    auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
    if(argc!=2)throw std::runtime_error("dialogue fixture path required");
    std::ifstream input(argv[1]);std::map<std::string,std::string> fixtures;std::string label,hex;
    while(input>>label>>hex)fixtures.emplace(label,hex);check(fixtures.size()==3,"native fixtures missing");
    auto wire=[](dialogue::Packet packet){packet[1]|=0x80;std::string result;const char* digits="0123456789abcdef";for(auto b:packet){result+=digits[b>>4];result+=digits[b&15];}return result;};
    const auto packets=dialogue::open(10012,u"Choose a cape trim.",{{u"Silver",0x700001}});
    check(packets.size()==3&&wire(packets[0])==fixtures.at("body")&&wire(packets[1])==fixtures.at("sender")&&wire(packets[2])==fixtures.at("button"),"dialogue differs from original executable");
    std::u16string body(118,u'x');body+=u"\U0001F408";body+=u"more text";
    const auto split=dialogue::open(10012,body,{});check(split.size()==3,"long dialogue must fragment");
    std::u16string joined;for(unsigned i=0;i<2;++i){const auto& p=split[i];check(p[0]==127,"sender emitted before full body");const auto count=p[2]|unsigned(p[3])<<8;check(count<=121,"native body capacity exceeded");for(unsigned n=0;n<count;++n)joined.push_back(p[4+2*n]|unsigned(p[5+2*n])<<8);check(joined.back()<0xd800||joined.back()>0xdbff,"split surrogate pair");}
    check(joined==dialogue::literal(body,256)&&split.back()[0]==128,"body reconstruction/publication order");
    check(dialogue::selection(dialogue::Packet{57,128,1,0,112,0})==0x700001,"selection DWORD layout");
    const std::u16string resource{u'\x8101',u'\x17a8',u'\x1'};
    const auto encoded_dialog=dialogue::open_encoded(10012,resource,{{resource,0x123456}});
    check(encoded_dialog.size()==3&&encoded_dialog[0][2]==3&&encoded_dialog[0][4]==1&&encoded_dialog[0][5]==0x81&&
        encoded_dialog[2][3]==3&&encoded_dialog[2][5]==1&&encoded_dialog[2][6]==0x81,"encoded resource strings rewritten as literals");
    const auto trim_menu=guild::trim::menu(10012,10000);check(trim_menu.size()==12,"Tolkano must offer nine trims and goodbye");
    check(guild::trim::find(2)->price==15000&&!guild::trim::find(3)&&guild::trim::find(1)->price==7500&&guild::trim::find(9)->price==15000,"invalid trim award/price policy");
    for(const auto& offer:guild::trim::offers)check(guild::trim::confirmation(10012,offer).size()==4,"trim confirmation missing");
    auto rejects=[&](const auto& action){bool rejected=false;try{action();}catch(const std::invalid_argument&){rejected=true;}check(rejected,"invalid dialogue accepted");};
    rejects([&]{dialogue::open(0,u"Text",{});});rejects([&]{dialogue::open(1,u"injected\x1",{});});
    rejects([&]{dialogue::open_encoded(1,std::u16string(260,u'x'),{});});
    rejects([&]{dialogue::open_encoded(1,std::u16string{u'x',u'\0'},{});});
    rejects([&]{dialogue::open_encoded(1,resource,{{std::u16string(128,u'x'),1}});});
    rejects([&]{dialogue::open(1,u"Text",{{u"A",1},{u"B",1}});});rejects([&]{dialogue::open(1,u"Text",{{std::u16string(125,u'x'),1}});});
    rejects([&]{dialogue::selection(dialogue::Packet{57,128,1});});rejects([&]{dialogue::selection(dialogue::Packet{81,128,1,0,0,0});});
    std::cout<<"PASS native dialogue fixtures, fragmentation, UTF16, selection and bounds\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
