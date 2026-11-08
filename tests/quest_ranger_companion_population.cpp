#include "gw_ranger_companion_population.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
namespace world=fw::gw::world;
namespace ranger=fw::gw::quests::ranger_companion;
static unsigned checks=0;
static void check(bool value,const std::source_location at=std::source_location::current()){
    ++checks;if(!value)throw std::runtime_error("Nente population invariant line "+std::to_string(at.line()));
}
static world::NpcCatalog load(const std::filesystem::path& path){std::ifstream input(path);check(bool(input));return world::load_npc_catalog(input);}
int main(int argc,char** argv)try{
    check(argc==2);const auto root=std::filesystem::path(argv[1]);
    auto population=load(root/"data/gw/npcs.txt");
    const auto stalker=load(root/"data/gw/pets/presearing-stalker-npcs.txt");
    for(const auto& [map,actors]:stalker.maps)for(const auto& actor:actors)population.maps[map].push_back(actor);
    const auto givers=load(root/"data/gw/quests/actors/presearing-givers.txt");
    world::merge_quest_givers(population,givers,{},true);
    const auto& valley=population.for_map(162);
    const auto ivor=std::find_if(valley.begin(),valley.end(),[](const auto& npc){return npc.captured_name=="Ivor Trueshot";});
    check(ivor!=valley.end()&&ivor->id==23001&&ivor->definition.id==23001);
    const auto supplement=load(root/"data/gw/quests/actors/ranger-companion-nente.txt");
    const auto original=supplement.for_map(162).front();auto nente=original;
    ranger::assign_nente_identity(nente,population);
    check(nente.id==24001&&nente.definition.id==24001&&original.id==23001);
    check(nente.x==original.x&&nente.y==original.y&&nente.definition.encoded_name==original.definition.encoded_name);
    population.maps[162].push_back(nente);
    auto validation=std::make_unique<world::Instance>();validation->configure_npcs(population.for_map(162));check(true);
    auto duplicate=original;bool rejected=false;
    try{ranger::assign_nente_identity(duplicate,population);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected&&duplicate.id==23001);
    for(unsigned kind=0;kind<3;++kind){
        world::NpcCatalog collision;auto other=original;other.captured_name="Unrelated";
        other.id=500;other.definition.id=501;
        if(kind==0)other.id=24001;if(kind==1)other.definition.id=24001;if(kind==2)other.captured_name=original.captured_name;
        collision.maps[148].push_back(other);duplicate=original;rejected=false;
        try{ranger::assign_nente_identity(duplicate,collision);}catch(const std::invalid_argument&){rejected=true;}
        check(rejected&&duplicate.id==23001&&duplicate.definition.id==23001);
    }
    std::cout<<"PASS Nente shipped population merge "<<checks<<" checks\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
