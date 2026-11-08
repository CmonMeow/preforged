#include "gw_item_metadata.h"
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

int main(int argc,char** argv)try{
    using namespace fw::gw::items;
    auto check=[](bool okay,const char* message){if(!okay)throw std::runtime_error(message);};
    check(argc==2,"native item-gameplay fixture path required");
    std::ifstream input(argv[1]);check(bool(input),"native fixtures missing");
    std::map<std::string,Packet> fixtures;std::string label,hex;
    while(input>>label>>hex){
        check(hex.size()%2==0,"invalid fixture hex");Packet bytes;
        for(std::size_t at=0;at<hex.size();at+=2)bytes.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(at,2),nullptr,16)));
        check(fixtures.emplace(label,std::move(bytes)).second,"duplicate fixture label");
    }
    auto matches=[&](const char* name,Packet bytes){bytes[1]|=0x80;check(bytes==fixtures.at(name),"differs from original2022 serializer");};
    matches("item_profession",profession_2022(43,7));
    matches("customization",customization_2022(43,u"Fixture Owner"));
    check(profession_2022(43,0).back()==0,"explicit no-profession restriction preserved");
    check(profession_2022(43,11).back()==11,"unknown sentinel is not fabricated profession");
    auto rejects=[&](auto make,const char* message){bool rejected=false;try{(void)make();}catch(const std::invalid_argument&){rejected=true;}check(rejected,message);};
    rejects([]{return profession_2022(0,7);},"metadata cannot target zero item");
    rejects([]{return profession_2022(43,12);},"invalid profession value accepted");
    rejects([]{return customization_2022(0,u"Fixture Owner");},"customization cannot target zero item");
    rejects([]{return customization_2022(43,u"");},"unproven empty customization clearing accepted");
    rejects([]{return customization_2022(43,std::u16string(32,u'A'));},"string exceeds native32-unit bounded descriptor");
    rejects([]{return customization_2022(43,std::u16string_view(u"A\0B",3));},"embedded terminator accepted");
    const auto longest=customization_2022(43,std::u16string(31,u'A'));
    check(longest.size()==70&&longest[6]==31&&longest[7]==0,"maximum descriptor string framing changed");
    std::cout<<"PASS native345 profession and313 customization serializer fixtures and bounded metadata\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
