#include "gw_npc_catalog.h"
#include <iostream>
int main()try{
    using namespace fw::gw::world;
    const std::string row="npc 148 1 17 1234 5678 1686175488 0 8 6 20 0 10 -20 0 810112340001 100,101\n";
    std::istringstream input("# reviewed synthetic test\ngw-npcs 1\n"+row);
    const auto catalog=load_npc_catalog(input);
    const auto& population=catalog.for_map(148);
    if(population.size()!=1||!catalog.for_map(248).empty()||population[0].definition.encoded_name!=u"\x8101\x1234\x1"||
       population[0].model_files!=std::vector<std::uint32_t>{100,101}||population[0].y!=-20||
       population[0].definition.visual_adjustment!=std::array<std::uint8_t,4>{0,255,128,100})
        throw std::runtime_error("catalog content mismatch");
    std::uint32_t next=1;
    if(!population[0].captured_name.empty())throw std::runtime_error("header comment became NPC identity");
    for(const auto name:{"Xunlai Agent [Storage]","Xunlai Chest","Xunlai Gift-Giver Ruxx","Kazuma [Xunlai Bakery Agent]"}){
      for(const std::string ending:{"\n","\r\n"}){
        auto storage_row=row;storage_row.pop_back();storage_row+=ending;
        std::istringstream storage_input(std::string("gw-npcs 1")+ending+"# "+name+ending+storage_row);
        auto storage_catalog=load_npc_catalog(storage_input);const bool expected=std::string(name)=="Xunlai Chest"||std::string(name)=="Xunlai Agent [Storage]";
        if(storage_catalog.for_map(148)[0].xunlai_storage!=expected)throw std::runtime_error("Xunlai classification included a gift/bakery NPC");
        Instance storage_world;storage_world.configure_npcs(storage_catalog.for_map(148));
        auto box=std::make_shared<Outbox>();storage_world.join(2,"Storage Visitor",10,-20,box);
        if(storage_world.nearby_npc_name(2,1,0)!=std::string(name)||storage_world.nearby_npc_name(2,999,0))throw std::runtime_error("nearby NPC identity mismatch");
        if(storage_world.can_access_storage(2,1,0)!=expected||storage_world.can_access_storage(2,999,0))throw std::runtime_error("storage interaction ownership gate");
        storage_world.move(2,1000,-20,0);
        if(storage_world.can_access_storage(2,1,10000))throw std::runtime_error("storage accessible after leaving interaction range");
        if(storage_world.nearby_npc_name(2,1,10000))throw std::runtime_error("quest NPC identity exposed outside range");
      }
    }
    for(const auto name:{"Tolkano [Tournament]","Tolkano","Other [Tournament]"}){
        std::istringstream source(std::string("gw-npcs 1\r\n# ")+name+"\r\n"+row);
        const auto parsed=load_npc_catalog(source);const bool expected=std::string(name)=="Tolkano [Tournament]";
        if(parsed.for_map(148)[0].guild_trim_vendor!=expected)throw std::runtime_error("trim vendor name classification");
        Instance world;world.configure_npcs(parsed.for_map(148));auto box=std::make_shared<Outbox>();world.join(2,"Trim Visitor",10,-20,box);
        if(world.can_access_trim_vendor(2,1,0)!=expected||world.can_access_trim_vendor(2,999,0))throw std::runtime_error("trim NPC authority gate");
        world.move(2,1000,-20,0);if(world.can_access_trim_vendor(2,1,10000))throw std::runtime_error("trim purchase accessible outside NPC range");
    }
    if(catalog.allocate_player(148,next)!=2||next!=3)throw std::runtime_error("player ID collided with NPC");
    next=65535;if(catalog.allocate_player(148,next)!=65535||next!=65536)throw std::runtime_error("last player ID lost");
    bool exhausted=false;try{catalog.allocate_player(148,next);}catch(const std::exception&){exhausted=true;}
    if(!exhausted)throw std::runtime_error("player ID overflow accepted");
    auto rejects=[](std::string text){
        std::istringstream in(text);bool rejected=false;
        try{load_npc_catalog(in);}catch(const std::exception&){rejected=true;}
        if(!rejected)throw std::runtime_error("invalid catalog accepted");
    };
    rejects("");rejects("gw-npcs 2\n");rejects("gw-npcs 1\n"+row+row);
    for(const auto& [from,to]:std::vector<std::pair<std::string,std::string>>{
            {"npc 148","npc -1"},{"10 -20","nan -20"},{"810112340001","810100001234"},
            {"100,101","100,"},{"100,101","1,2,3,4,5,6,7,8,9"},{" 6 20 "," 256 20 "}}){
        auto invalid=row;invalid.replace(invalid.find(from),from.size(),to);rejects("gw-npcs 1\n"+invalid);
    }
    for(unsigned scale=0;scale<=5;++scale){
        auto invalid=row;
        const std::string visual="1686175488";
        invalid.replace(invalid.find(visual),visual.size(),std::to_string((scale<<24)|0x80ff00u));
        rejects("gw-npcs 1\n"+invalid);
    }
    Instances worlds;unsigned initialized=0;
    bool rejected=false;
    try{worlds.acquire(5,[](auto&){throw std::runtime_error("failed setup");});}catch(const std::exception&){rejected=true;}
    if(!rejected)throw std::runtime_error("failed setup published");
    {
        auto first=worlds.acquire(5,[&](auto& world){++initialized;world.configure_npcs(population);});
        auto second=worlds.acquire(5,[&](auto&){++initialized;});
        if(initialized!=1||&first.instance()!=&second.instance()||second.instance().npc_packets().size()!=3)
            throw std::runtime_error("shared instance initialized more than once");
    }
    auto recreated=worlds.acquire(5,[&](auto&){++initialized;});
    if(initialized!=2||!recreated.instance().npc_packets().empty())throw std::runtime_error("released instance reused stale population");
    std::cout<<"PASS NPC catalog validation, ID reservation and instance initialization rollback\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
