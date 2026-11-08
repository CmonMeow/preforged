#include "gw_npc_friendly_heal.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool value){if(!value)throw std::runtime_error("friendly heal authority failed");}
int main()try{
    namespace f=combat::ai::friendly;
    const skills::effects::Scope monk{1,36,1},rurik{1,35,1};
    f::HealProfile p{286,0,0,"captured Monk36 HealOther286; synthetic rank/resource fixture"};
    f::Healer source{monk,{100,100},10,1000,1000,true};
    f::Recipient target{rurik,{50,100},1000,100,true,true,true,true};
    const auto proposal=f::prepare(p,source,target,1000);check(proposal.has_value());
    combat::npcs::control::Token token{monk,0,1,combat::npcs::control::Action::spell,1000,1750,skills::Slot{286,0}};
    const auto quote=f::bind(*proposal,token);check(quote.has_value());
    source.energy=0;check(!f::prepare(p,source,target,1000));
    check(f::completion(*quote,token,source,target,1750,true,p)==35); //paid cost is not charged twice
    check(!f::completion(*quote,token,source,target,1749,true,p));check(!f::completion(*quote,token,source,target,1750,false,p));
    auto changed=target;++changed.scope.generation;check(!f::completion(*quote,token,source,changed,1750,true,p));
    changed=target;changed.health.current=100;check(f::completion(*quote,token,source,changed,1750,true,p)==0);
    auto rank=p;++rank.rank;check(!f::completion(*quote,token,source,target,1750,true,rank));
    auto stale=token;++stale.sequence;check(!f::completion(*quote,stale,source,target,1750,true,p));
    changed=target;changed.scope=monk;source.energy=10;check(!f::prepare(p,source,changed,1000));
    changed=target;changed.allied=false;check(!f::prepare(p,source,changed,1000));
    changed=target;changed.health_clock=2000;check(!f::prepare(p,source,changed,1000));
    changed=target;changed.healing_allowed=false;check(!f::prepare(p,source,changed,1000));
    {const f::HealProfile word{282,0,0,"authored fractional Word threshold"};auto victim=target;
        victim.health={499.5,1000};source.energy=10;
        const auto prepared=f::prepare(word,source,victim,1000);check(bool(prepared));
        const combat::npcs::control::Token cast{monk,0,2,combat::npcs::control::Action::spell,1000,1750,skills::Slot{282,0}};
        const auto bound=f::bind(*prepared,cast);check(bool(bound));
        const auto* row=skills::find_skill(282);
        check(f::completion(*bound,cast,source,victim,1750,true,word)==row->scale0+row->bonus0);}
    source.health.current=0;check(!f::completion(*quote,token,source,target,1750,true,p));
    std::cout<<"Friendly NPC direct-heal admission and scoped completion authority passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
