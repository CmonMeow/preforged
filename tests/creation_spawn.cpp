#include "gw_creation_spawn.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv)try{
    if(argc!=3)throw std::runtime_error("creation spawn configuration and navigation required");
    std::ifstream input(argv[1]);if(!input)throw std::runtime_error("configuration missing");
    const auto arrival=fw::gw::creation::first_arrival(input,1,148);
    const auto mesh=fw::gw::navigation::Mesh::load(argv[2]);
    if(!arrival||arrival->plane!=0||!mesh.walkable(arrival->plane,arrival->point)||
       std::abs(arrival->facing+2.2758f)>0.00001f)
        throw std::runtime_error("captured first arrival not navigable with recorded facing");
    std::ifstream nightfall_input(argv[1]);
    const auto nightfall=fw::gw::creation::first_arrival(nightfall_input,3,490);
    const auto nightfall_mesh=fw::gw::navigation::Mesh::load(std::filesystem::path(argv[2]).parent_path()/"249289.fwnav");
    if(!nightfall||nightfall->point.x!=10215||nightfall->point.z!=6479||nightfall->plane!=0||
       !nightfall_mesh.walkable(nightfall->plane,nightfall->point))
        throw std::runtime_error("captured Nightfall creation arrival not navigable");
    for(const auto [campaign,map]:{std::pair{1u,55u},std::pair{2u,148u},std::pair{3u,514u}}){
        std::ifstream other(argv[1]);if(fw::gw::creation::first_arrival(other,campaign,map))
            throw std::runtime_error("unrelated campaign/map inherited creation spawn");
    }
    for(const auto text:{"1 148 0 0 65536 0", "1 148 0 0 0 0 trailing",
        "0 148 0 0 0 0", "1 148 0 0 0 0\n1 148 1 1 0 0"}){
        std::istringstream malformed(text);bool rejected=false;
        try{(void)fw::gw::creation::first_arrival(malformed,1,148);}
        catch(const std::runtime_error&){rejected=true;}
        if(!rejected)throw std::runtime_error("malformed creation spawn accepted");
    }
    std::cout<<"PASS creation arrival capture navigation, scope and invalid configuration\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
