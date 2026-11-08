#include "gw_world_simulation.h"
#include "gw_npc_catalog.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Owned NPC return clock line "+std::to_string(at.line()));
}
int main()try{
    for(bool brood:{false,true}){
        const auto root=std::filesystem::path(__FILE__).parent_path().parent_path()/"data/gw/quests";
        std::ifstream appearance(root/(brood?"actors/domination-magic-brood.txt":"actors/grawl-invaders.txt"));check(bool(appearance));
        auto population=world::load_npc_catalog(appearance);
        const auto metadata_path=root/(brood?"domination-magic-development-combat.txt":"grawl-development-combat.txt");
        std::ifstream header(metadata_path);std::string magic,hash;unsigned version=0;header>>magic>>version>>hash;
        check(magic=="gw-npc-combat"&&version>0);std::ifstream metadata(metadata_path);
        world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);
        const auto templates=population.for_map(brood?161:160);check(!templates.empty());auto sample=templates.front();
        // Translate the whole encounter for this no-navigation movement
        // fixture; canonical appearance/combat bindings remain unchanged.
        sample.x=0;sample.y=0;
        world::Instance w;w.join(1,"Quest owner",sample.x+100,sample.y,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,40,40},100);const auto owner=*w.effect_scope(1);
        check(w.elemental_armor(owner,60,60));check(w.physical_armor(owner,0));
        const unsigned quest=brood?49:75;
        check(w.spawn_kill_encounter(owner,quest,1,{sample},[]{return true;}));const auto source=*w.effect_scope(60000);
        if(brood)check(w.arm_brood_caster(owner,1,1000));else check(w.arm_grawl_melee(owner,1,1000));
        check(w.damage_npc(owner,source,{combat::Kind::armor_ignoring,1,20,194},1248,1100).reason==combat::Reason::accepted);
        w.tick(1100);w.move(1,sample.x+5000,sample.y,1200);
        const auto returning=[&]{return brood?w.npc_elemental_ai_snapshot(source)->returning:w.npc_flare_ai_snapshot(source)->returning;};
        unsigned returned_at=0;
        for(unsigned at=1200;at<=40000;at+=100){w.tick(at);if(returning()){returned_at=at;break;}}
        check(returned_at!=0);
        check(w.settle_resources(owner,returned_at+20000));
        // Source-owned home motion must finish before any authoritative
        // observation of the old foe at this future timestamp is available.
        for(unsigned at=returned_at+100;at<=returned_at+15000;at+=100)w.tick(at);
        check(!returning());
    }
    std::cout<<"Production Grawl/Brood return-home independent of former-owner combat clocks\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
