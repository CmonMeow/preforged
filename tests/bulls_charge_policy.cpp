#include "gw_bulls_charge.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Bull's Charge line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned r=0;r<=20;++r){auto p=bulls_charge(r);check(p&&p->rank==r&&p->duration==1000ull*interpolate(5,11,r)&&p->recharge==20000&&p->energy==5&&p->movement==1.33f);}check(!bulls_charge(21));
 const Scope owner{1,25,1},foe{1,26,1};Registry registry(owner);
 check(registry.apply(owner,{100,25,379,0,Category::stance,100,5100},Replace::category).accepted);
 combat::weapon::ResolvedAttack a{owner,foe,1,90,100,combat::weapon::AttackOutcome::hit,100,combat::weapon::AttackDelivery::melee};
 check(bulls_charge_knockdown(registry,owner,a,true)==true);
 check(bulls_charge_knockdown(registry,owner,a,false)==false);
 auto b=a;b.delivery=combat::weapon::AttackDelivery::ranged;check(bulls_charge_knockdown(registry,owner,b,true)==false);
 for(auto outcome:{combat::weapon::AttackOutcome::miss,combat::weapon::AttackOutcome::block,combat::weapon::AttackOutcome::dodge,combat::weapon::AttackOutcome::obstruction,combat::weapon::AttackOutcome::cancelled}){b=a;b.outcome=outcome;check(bulls_charge_knockdown(registry,owner,b,true)==false);}
 b=a;b.at=5100;check(bulls_charge_knockdown(registry,owner,b,true)==false);
 b=a;b.event=0;check(!bulls_charge_knockdown(registry,owner,b,true));
 b=a;b.attacker.generation++;check(!bulls_charge_knockdown(registry,owner,b,true));
 b=a;b.target=owner;check(!bulls_charge_knockdown(registry,owner,b,true));
 b=a;b.delivery=combat::weapon::AttackDelivery::unknown;check(!bulls_charge_knockdown(registry,owner,b,true));
 Slots bar{};bar[0]={344,1};CooldownBarAuthority authority(owner);auto snapshot=*authority.publish(owner,bar,100);
 check(!prepare_bulls_charge_accepted_use(registry,owner,{344,2},snapshot,200));
 check(!prepare_bulls_charge_accepted_use(registry,{1,25,2},{344,1},snapshot,200));
 check(!prepare_bulls_charge_accepted_use(registry,owner,{344,1},snapshot,99));
 const auto end=prepare_bulls_charge_accepted_use(registry,owner,{344,1},snapshot,200);
 check(end&&end->ended&&registry.lookup(owner,100)&&!end->registry.lookup(owner,100));
 check(bulls_charge_knockdown(end->registry,owner,a,true)==false);
 std::cout<<"PASS detached Bull's Charge pinned policy,moving melee contact,expiry and exact accepted-use ending;World/Runtime pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
