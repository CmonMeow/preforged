#include "gw_healing_seed.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <limits>

using namespace fw::gw::skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Healing Seed line "+std::to_string(at.line()));
}
int main()try{
    check(healing_seed(0)->healing==3);
    check(healing_seed(12)->healing==25);
    check(healing_seed(15)->healing==30);
    check(!healing_seed(21));
    const Scope caster{1,1,1},damaged{1,2,1},near{1,3,1},edge{1,4,1},far{1,5,1};
    check(healing_seed_target(*healing_seed(12),caster,{damaged,true,true,true,1248}));
    check(!healing_seed_target(*healing_seed(12),caster,{caster,true,true,true,0}));
    check(!healing_seed_target(*healing_seed(12),caster,{damaged,true,true,true,1249}));
    check(!healing_seed_target(*healing_seed(12),caster,{damaged,false,true,true,10}));
    check(!healing_seed_target(*healing_seed(12),caster,{damaged,true,true,false,10}));
    Registry registry(damaged);
    check(registry.apply(damaged,{10,caster.actor,274,12,Category::enchantment,100,10100},Replace::same_skill).accepted);
    std::vector<HealingSeedRecipient> candidates{
        {damaged,true,true,true,0},{near,true,true,true,50},
        {edge,true,true,true,166},{far,true,true,true,167},
        {{1,6,1},false,true,true,5},{{1,7,1},true,false,true,5},
        {{1,8,1},true,true,false,5}};
    const auto healed=healing_seed_reaction(registry,damaged,1000,17,candidates);
    check(healed&&healed->effect==10&&healed->cause==1&&healed->healing==25);
    check(healed->recipients==std::vector<Scope>({damaged,near,edge}));
    check(!healing_seed_reaction(registry,damaged,1000,-1,candidates));
    check(!healing_seed_reaction(registry,damaged,1000,std::numeric_limits<double>::infinity(),candidates));
    check(healing_seed_reaction(registry,damaged,1000,0,candidates)->recipients.empty());
    check(healing_seed_reaction(registry,damaged,10100,17,candidates)->recipients.empty());
    auto missing=candidates;missing.erase(missing.begin());
    check(!healing_seed_reaction(registry,damaged,1000,17,missing));
    auto duplicate=candidates;duplicate.push_back(candidates[1]);
    check(!healing_seed_reaction(registry,damaged,1000,17,duplicate));
    auto stale=damaged;++stale.generation;
    check(!healing_seed_reaction(registry,stale,1000,17,candidates));
    std::cout<<"PASS Healing Seed activation plan and scoped group-heal reaction\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
