#include "gw_bonetti_defense.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills;using namespace fw::gw::skills::effects;
void check(bool b){if(!b)throw std::runtime_error("Bonetti policy");}
int main()try{
 for(unsigned r=0;r<=20;++r){auto p=bonetti_defense(r);check(p&&p->energy==5&&p->duration==1000ull*interpolate(5,11,r));}check(!bonetti_defense(21));
 const Scope owner{1,25,1},attacker{1,26,2};Registry registry(owner);
 check(registry.apply(owner,{123,25,380,12,Category::stance,100,10100},Replace::category).accepted);
 const AttributedBlock contact{fw::gw::combat::weapon::AttackOutcome::block,BlockSource{380,123,.75},1};
 auto gain=bonetti_block_reward(registry,owner,attacker,contact,true,200);check(gain&&gain->energy==5&&gain->owner==owner);
 check(!bonetti_block_reward(registry,owner,attacker,contact,false,200));
 auto other=contact;other.source->skill=258;check(!bonetti_block_reward(registry,owner,attacker,other,true,200));
 other=contact;other.source->instance=124;check(!bonetti_block_reward(registry,owner,attacker,other,true,200));
 check(!bonetti_block_reward(registry,owner,{2,26,2},contact,true,200));
 check(!bonetti_block_reward(registry,owner,attacker,contact,true,10100));
 Slots slots{};slots[0]={380,0};slots[1]={340,1};CooldownBarAuthority authority(owner);const auto bar=*authority.publish(owner,slots,100);
 auto end=prepare_bonetti_accepted_use(registry,owner,{340,1},bar,200);check(end&&end->ended&&registry.lookup(owner,123)&&!end->registry.lookup(owner,123));
 check(!prepare_bonetti_accepted_use(registry,owner,{340,0},bar,200));
 check(!prepare_bonetti_accepted_use(registry,owner,{340,1},bar,99));
 const auto twice=prepare_bonetti_accepted_use(end->registry,owner,{340,1},bar,200);check(twice&&!twice->ended);
 std::cout<<"PASS Bonetti pinned policy,exact attributed melee reward and accepted-use ownership/copy/expiry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
