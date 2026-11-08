#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include "gw_map_catalog_generated.h"
namespace fw::gw::quests::presearing {
// Native IDs are joined by decoded names in the archived W/E quest snapshots.
// Published grant phases/prerequisites are from the named local wiki pages.
// This is content policy, not a mapping of unobserved native dialogue IDs.
struct Rule {
    std::uint32_t id;std::string_view name,offer_npc,reward_npc;
    unsigned profession;bool primary_only,requires_primary_test;
    std::array<std::uint32_t,3> acceptance_skills,reward_skills;
    unsigned experience;std::string_view source_sha256;
};
inline constexpr std::array<Rule,22> rules{{
    {78,"Opposition to the King","Devona","Devona",0,false,false,{},{},500,
     "f6aaadf57e84c80639b72b8c6b29a5f348fb79a81f41fde9eb533f258d2b3b7d"},
    {62,"Unsettling Rumors","Devona","Armin Saberlin",0,false,true,{},{},250,
     "8ba56c0b300e2adcb51e7186421813e3b299490996c48d397c1d885fb6bfcc7e"},
    {76,"The Ranger's Companion","Master Ranger Nente","Master Ranger Nente",2,false,true,{436,446,411},{},250,
     "c344264fd0e8dd7ff9df93ba8ff33ef10b2324a9ea116adb897b846ab759a1bb"},
    {89,"The Path to Glory","Armin Saberlin","Sir Tydus",0,false,true,{},{},100,
     "b960ef4adbbce5d76999b9d38ae8dca4ce03170a291add2528e884a22ace2cf6"},
    {80,"Message from a Friend","Town Crier","Sir Tydus",0,false,false,{},{},100,
     "d8e598973c21ee4f3c566a154de856435b95651852731d59a59b6600e039e896"},
    {79,"A Second Profession","Armin Saberlin","Armin Saberlin",0,false,true,{},{},250,
     "fdd610f3fd643e6679b9ed367796a7e2ba4fd29cb0e394078b95e09b3b9897bf"},
    {54,"Further Adventures","Haversdan","Devona",0,false,true,{},{},250,
     "10adf5ea4394f062438601fc6b819a1fca08133e361782b64ee9e74e96de276b"},
    // Quest65 has a separate primary/prerequisite gate in the menu/controller;
    // primary_only also controls starter retirement and must remain false here.
    {65,"A New Elementalist Trainer","Haversdan","Elementalist Aziure",6,false,false,{},{},250,
     "bb0ef5a0fe553387e2b9bb6340a766e466c1ba6b5627c3ad28145695c4bf51b0"},
    // Captured grants220 at lines20010/20014; content remains disabled until
    // the encounter and acceptance inventory are installed together.
    {42,"A Test of Marksmanship","Ivor Trueshot","Ivor Trueshot",2,false,false,{407,432,0},{},250,
     "c12481c380a89bdce5ae14d96adb344cf8ca35795e30fc8b8f62f5bc06bff447"},
    {55,"The Wayward Wizard","Sandre Elek","Sandre Elek",0,false,false,{},{},250,
     "56987929fccd4253ca2e75ae41dff3e2db8329131ab5b8780cf96ad90db6f61d"},
    {43,"The Power of Blood","Kasha Blackblood","Kasha Blackblood",4,false,false,{115,156,0},{},250,
     "cc2875cc3e62bd0bcb5ab76512b7d1850f2e22143f3856cc836dd4485e8afb31"},
    {49,"Domination Magic","Vassar","Vassar",5,false,false,{28,27,0},{},250,
     "242b7ea58f1127b20409719c53389d731d7bb569ae198a999e3c5c5af6ca81ca"},
    {71,"A Mesmer's Burden","Lady Althea","Lady Althea",5,false,true,{31,40,76},{},250,
     "2c10e65c372892a831c87842c5f71b42d667c4540ce57c6993b6be9b35b43663"},
    // Names/IDs/NPCs from archived four-profession snapshots; rewards from the
    // corresponding wiki pages. Catalog/objective activation remains separate.
    {86,"Ranger Test","Artemis the Ranger","Artemis the Ranger",2,true,false,{}, {394,446,0},500,
     "848c105e5e7fdc2472eaa327651294e620a813641aae13d98013b76112252c5f"},
    {81,"Mesmer Test","Sebedoh the Mesmer","Sebedoh the Mesmer",5,true,false,{}, {26,40,0},500,
     "7125152805d209feb268f3a555447d81a1f24f93dfa081d4ead8a36998361f8e"},
    {84,"Monk Test","Ciglo the Monk","Ciglo the Monk",3,true,false,{}, {288,252,0},500,
     "9b8bf2c2aebfd889e7ef9c6dcd3bd1ab957168efe578c3e45141b3dac8d2fe5a"},
    {82,"Necromancer Test","Verata the Necromancer","Verata the Necromancer",4,true,false,{}, {153,105,0},500,
     "ac686f70d30cb221063b9ca23cba5d9cd056918a1f91f50715cb273ff76e708d"},
    // Installed development policy: captured Howland dialogue and scale metadata,
    // with explicitly authored River Skale encounter placements.
    {83,"Elementalist Test","Howland the Elementalist","Howland the Elementalist",6,true,false,{}, {194,180,0},500,
     "dcbc7b708a266f9f2dd10919c8d0eb3e88606f60313910fde8671f28bbcec5cd"},
    {85,"Warrior Test","Van the Warrior","Van the Warrior",1,true,false,{}, {346,1,0},500,
     "4340806fce5a5de0ab57aabe1077d93b8c1a7222753f21a57fa9233e61179a88"},
    {75,"Grawl Invasion","Warmaster Grast","Warmaster Grast",1,false,true,{382,384,1},{},250,
     "6250821f757dfa613aef1917e598662d544b122bbd5d9ce709da76a55f61a2e7"},
    {73,"The Elementalist Experiment","Elementalist Aziure","Elementalist Aziure",6,false,true,{197,180,200},{},250,
     "c1c821cd609b0bf83d0d69b8c1d419041595afa35450b0d7b614225b6b60b9e5"},
    {37,"The Supremacy of Air","Ralena Stormbringer","Elementalist Aziure",6,false,false,{230,220,0},{},250,
     "bca9408ddfd1b110508861f00967c204c93f798279da87b53ce550413ef45f09"}
}};
inline const Rule* find(std::uint32_t id){for(const auto& rule:rules)if(rule.id==id)return &rule;return nullptr;}
// Population policy supplied by the development playthrough. A completed
// objective still needs its trainer for turn-in; only reward retires the trainer.
// Party callers union eligible members instead of guessing retail leader priority.
struct TrainerStage {unsigned primary=0;bool rewarded=false;};
inline bool starter_trainer_visible(std::string_view name,const TrainerStage* members,std::size_t count){
    if(name=="Haversdan"){
        for(std::size_t i=0;i<count;++i)
            if(members[i].primary>=1&&members[i].primary<=6&&members[i].rewarded)return true;
        return false;
    }
    const Rule* trainer=nullptr;
    for(const auto& rule:rules)if(rule.primary_only&&rule.offer_npc==name){trainer=&rule;break;}
    if(!trainer)return true; // unrelated population remains unchanged
    for(std::size_t i=0;i<count;++i)
        if(members[i].primary==trainer->profession&&!members[i].rewarded)return true;
    return false;
}
inline bool next_guide_visible(const TrainerStage* members,std::size_t count){
    for(std::size_t i=0;i<count;++i)
        if(members[i].primary>=1&&members[i].primary<=6&&members[i].rewarded)return true;
    return false;
}
inline bool trial_area(unsigned map){const auto* row=fw::gw::find_map(map);
    return row&&row->campaign==1&&row->continent==1&&row->region==7&&(row->type==1||row->type==3);}
struct Eligibility {unsigned primary,secondary;bool prophecies_character,in_presearing,primary_test_completed;};
inline bool eligible(const Rule& rule,const Eligibility& state){
    return state.prophecies_character&&state.in_presearing&&
        (!rule.profession||state.primary==rule.profession||(!rule.primary_only&&(state.secondary==rule.profession||state.secondary==0)))&&
        (!rule.requires_primary_test||state.primary_test_completed);
}
// Callers must preserve the quest-specific mechanics: Grawl's acceptance sword,
// Experiment's three scoped waves and living Aziure, and Supremacy's distinct
// offer/reward NPCs. Never treat an arbitrary model kill as completion.
}
