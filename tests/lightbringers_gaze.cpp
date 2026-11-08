#include "gw_lightbringers_gaze.h"
#include "gw_progression.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace fw::gw::skills::effects;
static void check(bool yes){if(!yes)throw std::runtime_error("Lightbringer's Gaze plan");}
int main()try{
    check(lightbringers_gaze(0)->holy_damage==100);
    check(lightbringers_gaze(8)->additional_foes==8);
    check(!lightbringers_gaze(9));
    check(lightbringer_servant_trait(474,{})==true);
    check(lightbringer_servant_trait(445,{})==true);
    check(lightbringer_servant_trait(474,false)==false);
    check(!lightbringer_servant_trait(146,{}));
    const auto gaze=*lightbringers_gaze(8);
    check(lightbringers_gaze_disable_duration(gaze,{},1000)==15000);
    const Entry serpents{1,1,456,12,Category::stance,500,20000};
    check(lightbringers_gaze_disable_duration(gaze,{serpents},1000)==10000);
    check(lightbringers_gaze_disable_duration(gaze,{},1000,true)==8000);
    check(lightbringers_gaze_disable_duration(gaze,{serpents},1000,true)==8000);
    check(lightbringers_gaze_disable_duration(gaze,{serpents},20000)==15000);
    auto invalid=gaze;++invalid.holy_damage;
    check(!lightbringers_gaze_disable_duration(invalid,{},1000));
    const Scope primary{1,20,1},near{1,21,1},far{1,22,1},outsider{1,23,1};
    std::vector<GazeCandidate> candidates{
        {primary,0,true,true,true},
        {far,200,true,true,false},
        {near,100,true,true,std::nullopt},
        {outsider,313,true,true,true},
    };
    check(*select_lightbringers_gaze(*lightbringers_gaze(0),primary,candidates)==
          std::vector<Scope>{primary});
    check(*select_lightbringers_gaze(*lightbringers_gaze(2),primary,candidates)==
          (std::vector<Scope>{primary,near,far}));
    check(*lightbringers_gaze_impacts(*lightbringers_gaze(2),primary,candidates)==
          (std::vector<GazeImpact>{{primary,100,true},{near,100,false},
                                   {far,100,false}}));
    candidates.push_back({{1,24,1},std::numeric_limits<float>::quiet_NaN(),true,true,false});
    check(*select_lightbringers_gaze(*lightbringers_gaze(2),primary,candidates)==
          (std::vector<Scope>{primary,near,far}));
    candidates[0].servant_of_abaddon.reset();
    check(!select_lightbringers_gaze(*lightbringers_gaze(2),primary,candidates));
    candidates[0].servant_of_abaddon=false;
    check(!select_lightbringers_gaze(*lightbringers_gaze(2),primary,candidates));
    candidates[0].servant_of_abaddon=true;candidates[0].hostile=false;
    check(!select_lightbringers_gaze(*lightbringers_gaze(2),primary,candidates));
    auto forged=*lightbringers_gaze(2);++forged.holy_damage;
    check(!select_lightbringers_gaze(forged,primary,candidates));
    fw::gw::progression::Character owner;
    check(fw::gw::progression::lightbringer_rank(owner)==0);
    owner.tracks[20].fields[2]=123;
    owner.tracks[17].fields[2]=124;owner.ranks[124].number=3;
    owner.ranks[123].number=8;
    check(fw::gw::progression::lightbringer_rank(owner)==8);
    check(fw::gw::progression::sunspear_rank(owner)==3);
    owner.ranks[123].number=11;
    check(!fw::gw::progression::lightbringer_rank(owner));
    std::cout<<"PASS Lightbringer's Gaze rank, target selection and recharge-linked disable duration\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
