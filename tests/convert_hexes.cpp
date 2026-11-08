#include "gw_convert_hexes.h"
#include "gw_dolyak_signet.h"
#include <iostream>
#include <stdexcept>
#include <source_location>
#include <string>
using namespace fw::gw::skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("Convert Hexes regression line "+std::to_string(at.line()));}
int main()try{
    const Scope source{1,1,1},target{1,2,1};Registry registry(target);
    check(registry.apply(target,{1,3,99,0,Category::hex,100,1000},Replace::same_skill).accepted);
    check(registry.apply(target,{2,4,100,0,Category::hex,101,1000},Replace::same_skill).accepted);
    check(registry.apply(target,{3,3,49,0,Category::hex,102,1000},Replace::same_skill).accepted);
    check(registry.apply(target,{4,3,235,0,Category::hex,103,1000},Replace::same_skill).accepted);
    check(registry.apply(target,{5,2,310,0,Category::enchantment,104,1000},Replace::same_skill).accepted);
    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=*convert_hexes(rank);const auto result=prepare_convert_hexes(plan,source,target,registry,200);check(result.has_value());
        check(result->removed.size()==4&&result->necromancer_hexes==2&&result->armor==10&&result->armor_until==200+plan.duration);
        for(unsigned index=0;index<4;++index)check(result->removed[index].instance==4-index);
        check(registry.size()==5&&registry.lookup(target,1)&&registry.lookup(target,5));
        const auto expired=prepare_convert_hexes(plan,source,target,registry,1000);check(expired&&expired->removed.empty()&&!expired->armor);
        auto forged=plan;forged.armor_bonus=20;check(!prepare_convert_hexes(forged,source,target,registry,200));
    }
    const auto plan=*convert_hexes(12);check(plan.duration==18000&&plan.energy==15&&plan.recharge==12000&&!convert_hexes(21));
    check(!prepare_convert_hexes(plan,target,target,registry,200));
    check(!prepare_convert_hexes(plan,{2,1,1},target,registry,200));
    check(!prepare_convert_hexes(plan,source,{1,2,2},registry,200));
    check(!prepare_convert_hexes(plan,source,target,registry,UINT64_MAX));
    Registry empty(target);const auto none=prepare_convert_hexes(plan,source,target,empty,200);check(none&&none->removed.empty()&&!none->armor);
    Registry many(target);unsigned effect=0;for(unsigned skill:{99u,100u,101u,103u})check(many.apply(target,{++effect,3,skill,0,Category::hex,100,1000},Replace::same_skill).accepted);
    const auto stronger=prepare_convert_hexes(plan,source,target,many,200);check(stronger&&stronger->necromancer_hexes==4&&stronger->armor==10);
    check(stacked_skill_armor(stronger->armor,10)==20); // Retail303 contributes only10;ordinary shared stacking still applies.
    std::cout<<"PASS21 Convert Hexes ranks,newest-first all-hex selection,Necromancer-only armor,zero/expired sets,source/scope/overflow guards and documented retail flat10armor despite multiple Necromancer hexes\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

