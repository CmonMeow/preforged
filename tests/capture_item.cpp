#include "../tools/capture/item_observation.h"
#include <array>
#include <iostream>
#include <stdexcept>
int main()try{
    if(!capture::installed_container_valid(42,3,1,true,false,true))throw std::runtime_error("installed bag rejected");
    if(capture::installed_container_valid(0,3,1,true,false,true)||
       capture::installed_container_valid(42,9,1,true,false,true)||
       capture::installed_container_valid(42,3,2,true,false,true)||
       capture::installed_container_valid(42,3,1,false,false,true)||
       capture::installed_container_valid(42,3,1,true,true,true)||
       capture::installed_container_valid(42,3,1,true,false,false))
        throw std::runtime_error("unverified installed container accepted");
    capture::ItemObservation item;
    item.model=123;item.file=456;item.tint=255;item.dye1=2;item.dye2=13;
    item.interaction=0x80000000u;item.slot=8;item.quantity=1;
    std::array<std::uint32_t,2> mods{0xffffffffu,0x12345678u};
    const auto json=capture::item_json(item,mods,true);
    for(const char* expected:{"\"model_file\":456","\"interaction\":2147483648",
        "\"dye_tint\":255","\"dye1\":2","\"dye2\":13","\"slot\":8",
        "\"modifiers_complete\":true","\"modifiers\":[4294967295,305419896]"})
        if(json.find(expected)==std::string::npos)throw std::runtime_error(expected);
    const auto missing=capture::item_json(item,{},false);
    const auto empty=capture::item_json(item,{},true);
    if(missing==empty||missing.find("\"modifiers_complete\":false")==std::string::npos)
        throw std::runtime_error("unreadable modifiers must differ from known empty modifiers");
    std::cout<<"PASS unsigned item fields, dye channels, modifier fidelity and incomplete distinction\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
