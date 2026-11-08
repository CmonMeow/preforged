#include "gw_battle_rage.h"
#include <array>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw::skills;
using namespace fw::gw::skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Battle Rage policy line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){
  const auto p=battle_rage(rank);
  check(p&&p->duration==1000ull*interpolate(5,20,rank)&&
    p->adrenaline_cost==80&&p->movement_bonus_percent==33&&
    p->adrenaline_gain_multiplier==2);
 }
 check(!battle_rage(21));
 const Scope owner{1,25,1};auto stale=owner;++stale.generation;
 Registry r(owner);
 check(r.apply(owner,{123,25,317,12,Category::stance,100,17100},Replace::category).accepted);
 const auto active=r.active(owner,100);
 check(battle_rage_active(active,100).value_or(false));
 check(!battle_rage_active(active,17100).value_or(true));
 Slots slots{};slots[0]={317,0};slots[1]={1,0};slots[2]={281,0};
 CooldownBarAuthority authority(owner);const auto bar=*authority.publish(owner,slots,100);
 const auto adrenal=prepare_battle_rage_accepted_use(r,owner,slots[0],bar,200);
 check(adrenal&&!adrenal->ended&&adrenal->registry.lookup(owner,123).has_value());
 const auto nonadrenal=prepare_battle_rage_accepted_use(r,owner,slots[1],bar,200);
 check(nonadrenal&&nonadrenal->ended&&nonadrenal->ended->instance==123&&
       !nonadrenal->registry.lookup(owner,123));
 check(!prepare_battle_rage_accepted_use(r,owner,{1,1},bar,200));
 check(!prepare_battle_rage_accepted_use(r,stale,slots[1],bar,200));
 check(!prepare_battle_rage_accepted_use(r,owner,slots[1],bar,99));
 auto bad=bar;bad.revision=0;
 check(!prepare_battle_rage_accepted_use(r,owner,slots[1],bad,200));
 const std::array<Entry,1> corrupt{{Entry{123,25,317,21,Category::stance,100,17100}}};
 check(!battle_rage_active(corrupt,200));
 std::cout<<"PASS Battle Rage pinned metadata, stance lifetime and accepted non-adrenal ending policy\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
