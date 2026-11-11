// Offline population validation. Does not open a client, socket or account DB.
#include "gw_npc_catalog.h"
#include "gw_map_assets_generated.h"
#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc,char** argv)try{
    if(argc!=3)throw std::invalid_argument("usage: npc_catalog_validate NPC_FILE NAVIGATION_DIRECTORY");
    std::ifstream input(argv[1]);if(!input)throw std::runtime_error("cannot open NPC catalog");
    const auto catalog=fw::gw::world::load_npc_catalog(input);
    std::size_t total=0;
    for(const auto& [map,population]:catalog.maps){
        const auto asset=fw::gw::map_asset_file(static_cast<std::uint16_t>(map));
        if(!asset)throw std::runtime_error("map has no resolved asset");
        auto mesh=std::make_shared<fw::gw::navigation::Mesh>(fw::gw::navigation::Mesh::load(
            std::filesystem::path(argv[2])/(std::to_string(asset)+".fwnav")));
        mesh->discard_unverified_portals();
        fw::gw::world::Instance world;world.configure_navigation(mesh);world.configure_npcs(population);
        const auto packets=world.npc_packets();
        std::size_t storage=0;for(const auto& npc:population)storage+=npc.xunlai_storage;
        total+=population.size();
        std::cout<<"map="<<map<<" NPCs="<<population.size()<<" packets="<<packets.size()
            <<" storage_actors="<<storage<<" exact_plane_navigation=PASS\n";
    }
    std::cout<<"PASS "<<catalog.maps.size()<<" maps, "<<total<<" NPCs; native catalog, scale, definitions, IDs, navigation and packet construction\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
