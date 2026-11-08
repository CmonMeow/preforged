#include "../tools/capture/npc_observation.h"
#include <iostream>
int main()try{
    capture::NpcDefinition value;
    value.model_file=123;value.skin_file=456;value.appearance=0xffffffff;
    value.flags=8;value.primary=1;value.secondary=2;value.level=20;
    value.visual_adjustment={0,127,128,255};value.model_files={0,456,0xffffffff};
    const std::string expected="{\"model_file\":123,\"skin_file\":456,\"appearance\":4294967295,\"flags\":8,\"primary\":1,\"secondary\":2,\"default_level\":20,\"visual_adjustment_bytes\":[0,127,128,255],\"model_files\":[0,456,4294967295]}";
    if(capture::npc_definition_json(value)!=expected)throw std::runtime_error("raw NPC fields changed");
    value.model_files.clear();
    if(!capture::npc_definition_json(value).ends_with("\"model_files\":[]}"))throw std::runtime_error("empty file list lost");
    value.model_files.resize(64,1);capture::npc_definition_json(value);
    value.model_files.push_back(2);bool rejected=false;
    try{capture::npc_definition_json(value);}catch(const std::invalid_argument&){rejected=true;}
    if(!rejected)throw std::runtime_error("oversized file list accepted");
    std::cout<<"PASS raw NPC definition fields and bounds\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
