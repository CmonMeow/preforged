#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Weapon facts line "+std::to_string(at.line()));}
int main()try{
 warrior_test::Fixture f(340,false);auto& w=f.world;
 check(w.weapon_attack_timing(f.source,100)->period_ms==1330);
 auto hammer=f.weapon;hammer.item_type=15;hammer.revision=2;
 check(w.ordinary_weapon(f.source,hammer,20,200));check(w.weapon_attack_timing(f.source,200)->period_ms==1750);
 check(!w.ordinary_weapon(f.source,f.weapon,20,199));check(w.weapon_attack_timing(f.source,200)->period_ms==1750);
 check(!w.ordinary_weapon(f.source,f.weapon,20,300));check(w.weapon_attack_timing(f.source,300)->period_ms==1750);
 auto stale=f.source;++stale.generation;check(!w.ordinary_weapon(stale,hammer,20,300));
 check(w.consume_energy(1,1,f.source,500));check(!w.ordinary_weapon(f.source,hammer,20,499));
 check(w.ordinary_weapon(f.source,hammer,20,500));
 check(w.ordinary_attack(f.source,2,600));check(w.ordinary_weapon(f.source,{},20,700));w.tick(1500);check(f.health()==1000);
 check(!w.weapon_attack_timing(f.source,1500));check(!w.ordinary_weapon(f.source,f.weapon,20,1500));
 auto axe=f.weapon;axe.revision=3;check(w.ordinary_weapon(f.source,axe,20,1500));check(w.weapon_attack_timing(f.source,1500)->period_ms==1330);
 check(!w.ordinary_weapon(f.source,axe,0,1500));
 // Current-boundary trusted updates remain usable without inventing a clock.
 check(w.ordinary_weapon(f.source,axe,20));
 std::cout<<"PASS timestamped weapon ownership,time/revision rejection across unequip,resource boundary and pending-attack cancellation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
