#include "gw_berserker_stance.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw::skills;using namespace fw::gw::skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Berserker Stance policy line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto p=berserker_stance(rank);check(p&&p->duration==1000ull*interpolate(5,11,rank)&&p->adrenaline_numerator==3&&p->adrenaline_denominator==2);}
 check(!berserker_stance(21));
 const Scope owner{1,25,1};auto stale=owner;++stale.generation;
 Registry registry(owner);check(registry.apply(owner,{123,25,370,12,Category::stance,100,10100},Replace::category).accepted);
 Slots slots{};slots[0]={370,0};slots[1]={281,0};slots[2]={340,1};slots[3]={344,0};
 CooldownBarAuthority authority(owner);const auto bar=*authority.publish(owner,slots,100);
 for(const auto used:slots){if(!used.skill)continue;
  const auto next=prepare_berserker_accepted_use(registry,owner,used,bar,200);check(next&&next->ended&&next->ended->instance==123);
  check(registry.lookup(owner,123).has_value()&&!next->registry.lookup(owner,123));
  const auto again=prepare_berserker_accepted_use(next->registry,owner,used,bar,200);check(again&&!again->ended);
 }
 check(!prepare_berserker_accepted_use(registry,owner,{340,0},bar,200));
 check(!prepare_berserker_accepted_use(registry,owner,{0,0},bar,200));
 check(!prepare_berserker_accepted_use(registry,stale,{281,0},bar,200));
 check(!prepare_berserker_accepted_use(registry,owner,{281,0},bar,99));
 for(auto now:{99ull,10100ull,11000ull}){const auto p=prepare_berserker_accepted_use(registry,owner,{281,0},bar,now);if(now==99)check(!p);else check(p&&!p->ended);}
 auto wrong=bar;wrong.published.owner=stale;check(!prepare_berserker_accepted_use(registry,owner,{281,0},wrong,200));
 wrong=bar;wrong.revision=0;check(!prepare_berserker_accepted_use(registry,owner,{281,0},wrong,200));
 std::cout<<"PASS pinned Berserker Stance rank/duration/rational gain policy and immutable accepted-skill ending proposals,copy membership,stale/future/expiry and duplicate suppression\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
