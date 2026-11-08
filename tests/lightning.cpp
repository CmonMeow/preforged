#include "gw_lightning.h"
#include "gw_lightning_javelin.h"
#include <stdexcept>
#include <cmath>
#include <fstream>
#include <string>
#include <source_location>
#include <iostream>
void check(bool ok,std::source_location source=std::source_location::current()){if(!ok)throw std::runtime_error("lightning family regression line"+std::to_string(source.line()));}
int main(int argc,char** argv)try{
    using namespace fw::gw;using namespace skills::effects;using combat::conditions::Kind;
    check(lightning_javelin(0)->damage==15&&lightning_javelin(12)->damage==43&&lightning_javelin(15)->damage==50);
    check(lightning_javelin(12)->penetration==25&&lightning_javelin(12)->interrupts_attacks&&!lightning_javelin(21));
    check(!lightning(230,12)); // Partial plan must not advertise a working effect.
    check(javelin_path_hit(0,0,100,0,50,5,5)==.5);
    check(javelin_path_hit(0,0,100,0,100,0,0)==1);
    check(!javelin_path_hit(0,0,100,0,50,6,5)&&!javelin_path_hit(0,0,100,0,-1,0,5)&&!javelin_path_hit(0,0,100,0,101,0,5));
    check(javelin_path_hit(100,100,0,0,50,50,0)==.5);
    check(!javelin_path_hit(0,0,0,0,0,0,5)&&!javelin_path_hit(0,0,100,0,50,0,-1));
    {
        Scope caster{1,1,1},selected{1,2,1};
        std::vector<JavelinCandidate> candidates{
            {selected,50,0,true,true}, // Evaded towards caster: misses aim.
            {{1,3,1},50,4,true,true},{{1,4,1},105,0,true,true},
            {{1,5,1},-1,0,true,true},{{1,6,1},50,0,true,false},
            {{1,7,1},50,0,false,true}};
        auto hits=javelin_hits(caster,selected,0,0,100,0,5,2,candidates);
        check(hits&&hits->size()==2&&hits->at(0).scope.actor==3&&hits->at(1).scope.actor==4);
        candidates[0].x=100;hits=javelin_hits(caster,selected,0,0,100,0,5,2,candidates);
        check(hits&&hits->size()==3&&hits->at(1).scope==selected);
        candidates.push_back(candidates[0]);check(!javelin_hits(caster,selected,0,0,100,0,5,2,candidates));
        candidates.pop_back();JavelinFlight flight;const auto plan=*lightning_javelin(12);
        check(flight.launch(caster,selected,plan,{100,2},5,0,0,100,0,0,0,100));
        check(!flight.impact(caster,true,1099,candidates)&&flight.pending());
        const auto impact=flight.impact(caster,true,1100,candidates);check(impact&&impact->size()==3&&!flight.pending());
        check(!flight.impact(caster,true,1101,candidates));
        check(flight.launch(caster,selected,plan,{100,2},5,0,0,100,0,0,0,2000));
        candidates[0].scope.generation=2;
        const auto recycled=flight.impact(caster,true,3000,candidates);check(recycled&&recycled->size()==2);
        check(flight.launch(caster,selected,plan,{100,2},5,0,0,100,0,0,0,4000));
        check(!flight.impact({1,1,2},true,5000,candidates)&&!flight.pending());
        auto forged=plan;forged.damage=900;check(!flight.launch(caster,selected,forged,{100,2},5,0,0,100,0,0,0,6000));
        check(!flight.launch(caster,selected,plan,{100,2},5,0,0,100,0,0,0,UINT64_MAX));
    }
    combat::Actor target{2,3,combat::Health{480,480},std::nullopt,0,true,80};
    const auto charge=*lightning(224,12),orb=*lightning(229,12),touch=*lightning(232,12);
    check(charge.damage==45&&charge.weakness_ms==17000&&!charge.projectile&&charge.penetration==25);
    check(orb.damage==82&&orb.projectile&&orb.range==1248);
    check(touch.damage==50&&touch.range==144&&touch.radius==166&&touch.blind_ms==3000&&touch.cracked_ms==8000);
    const auto hammer=*lightning(865,12);
    check(hammer.damage==82&&!hammer.projectile&&hammer.penetration==25&&hammer.cracked_ms==17000);
    check(lightning(865,0)->damage==10&&lightning(865,15)->damage==100&&lightning(865,20)->damage==130&&lightning(865,20)->cracked_ms==25000);
    const auto hammered=lightning_target(hammer,target,{},20,1,100);
    check(hammered.accepted&&hammered.hit.damage==82&&hammered.actor.health->current==398&&hammered.conditions.has(Kind::cracked_armor,17099)&&!hammered.conditions.has(Kind::cracked_armor,17100));
    const auto hammer_follow=lightning_target(hammer,hammered.actor,hammered.conditions,20,1,200);
    check(hammer_follow.accepted&&hammer_follow.hit.damage>82);
    const auto pvp_hammer=*lightning(3396,12);check(pvp_hammer.damage==82&&pvp_hammer.cracked_ms==0&&!pvp_hammer.projectile);
    const auto pvp_hit=lightning_target(pvp_hammer,target,{},20,1,100);
    check(pvp_hit.accepted&&pvp_hit.hit.damage==82&&pvp_hit.applied_conditions.empty());
    auto result=lightning_target(charge,target,{},20,1,100);
    check(result.accepted&&result.hit.damage==45&&result.actor.health->current==435&&result.conditions.has(Kind::weakness,17099)&&!result.conditions.has(Kind::weakness,17100));
    auto struck=lightning_target(touch,target,{},20,1,100);
    check(struck.accepted&&struck.hit.damage==50&&struck.conditions.has(Kind::blind,3099)&&!struck.conditions.has(Kind::blind,3100)&&struck.conditions.has(Kind::cracked_armor,8099));
    auto follow=lightning_target(orb,struck.actor,struck.conditions,20,1,200);
    check(follow.accepted&&std::abs(follow.hit.damage-82*std::exp2(15.0/40))<1e-8);
    // Initial touch damage uses prior armor, then adds CrackedArmor, never
    // reducing armor before that same spell's damage resolves.
    check(struck.hit.damage==50&&follow.hit.damage>82);
    target.lightning_armor.reset();check(!lightning_target(orb,target,{},20,1,100).accepted);
    target.lightning_armor=80;auto altered=orb;altered.penetration=100;check(!lightning_target(altered,target,{},20,1,100).accepted);
    target.health->current=1;auto killed=lightning_target(charge,target,{},20,1,100);check(killed.accepted&&killed.hit.killed&&!killed.conditions.has(Kind::weakness,100));
    target.health->current=0;check(!lightning_target(touch,target,{},20,1,100).accepted);
    check(!lightning(138,12)&&!lightning(229,21));
    check(argc==2);std::ifstream fixture(argv[1]);check(bool(fixture));std::vector<std::string> expected;for(std::string row;std::getline(fixture,row);)if(!row.empty())expected.push_back(row);check(expected.size()==9);
    target={2,3,combat::Health{480,480},std::nullopt,0,true,80};unsigned n=0;
    for(const auto plan:{charge,orb,touch}){
        const auto hit=lightning_target(plan,target,{},20,1,100).hit;
        for(const auto& packet:combat::damage_packets_2022(1,hit)){
            auto native=packet;native[1]|=0x80; // Original serializer's transport marker.
            constexpr char hex[]="0123456789abcdef";std::string encoded;for(auto byte:native){encoded+=hex[byte>>4];encoded+=hex[byte&15];}
            check(encoded==expected.at(n++));
        }
    }
    check(n==expected.size());
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
