#include "gw_wild_blow.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;using namespace combat::weapon;
void check(bool b){if(!b)throw std::runtime_error("Wild Blow policy");}
int main()try{
 auto p=wild_blow();check(p&&p->energy==5&&p->recharge_ms==8000&&p->unblockable&&p->lose_all_adrenaline);
 Strike s;s.level=20;s.mastery=12;s.minimum=10;s.maximum=20;s.location_armor=60;s.base_roll=.1;s.miss_roll=.5;
 auto hit=wild_blow_damage(*p,s);check(hit&&!hit->missed&&std::abs(hit->damage-20*std::sqrt(2.))<1e-8);
 s.critical_immune=true;check(std::abs(wild_blow_damage(*p,s)->damage-11)<1e-8);
 s.blind=true;check(wild_blow_damage(*p,s)->missed);s.blind=false;s.caster_weapon=true;check(!wild_blow_damage(*p,s));
 auto forged=*p;forged.energy=0;check(!wild_blow_damage(forged,s));
 const AttackScope target{1,2,3};std::optional<skills::effects::Entry> stance=skills::effects::Entry{123,2,371,0,skills::effects::Category::stance,100,1100};
 check(wild_blow_contact(*p,target,stance,AttackOutcome::hit,100)->remove_stance==123);
 check(!wild_blow_contact(*p,target,stance,AttackOutcome::hit,1100)->remove_stance);
 for(auto outcome:{AttackOutcome::miss,AttackOutcome::block,AttackOutcome::dodge,AttackOutcome::obstruction,AttackOutcome::target_retired,AttackOutcome::cancelled,AttackOutcome::replaced})check(!wild_blow_contact(*p,target,stance,outcome,100)->remove_stance);
 check(!wild_blow_contact(*p,target,{},AttackOutcome::hit,100)->remove_stance);
 check(!wild_blow_contact(*p,{},stance,AttackOutcome::hit,100));
 check(!wild_blow_contact(*p,target,stance,AttackOutcome::hit,99));
 stance->kind=skills::effects::Category::enchantment;check(!wild_blow_contact(*p,target,stance,AttackOutcome::hit,100));
 std::cout<<"PASS staged Wild Blow pinned policy,critical immunity,Blind/caster rejection and exact hit-only stance removal proposal;World/Runtime pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
