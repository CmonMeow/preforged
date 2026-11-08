#include "../src/runtime/gw_world_simulation.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;
static void check(bool v,std::source_location w=std::source_location::current()){if(!v)throw std::runtime_error("world adrenaline line"+std::to_string(w.line()));}
int main()try{
 world::Instance arena;arena.configure_attack_miss_draws([]{return .5;});auto a=std::make_shared<world::Outbox>(),b=std::make_shared<world::Outbox>();arena.join(1,"Axe",0,0,a,0,{}, {},100,1);arena.join(2,"Foe",60,0,b,0,{}, {},100,2);
 arena.set_vitals(1,{480,480,20,20},100);arena.set_vitals(2,{480,480,20,20},100);auto source=*arena.effect_scope(1),target=*arena.effect_scope(2);
 a->drain();check(arena.consume_energy(1,0));check(a->drain().empty());check(arena.resources(1)->energy==20);
 check(!arena.consume_energy(999,0));check(!arena.consume_energy(1,21));check(a->drain().empty());
 check(arena.consume_energy(1,5));check(arena.resources(1)->energy==15);check(!a->drain().empty());check(arena.restore_energy(1,5));
 skills::Slots bar{};bar[0]={335,0};bar[1]={336,0};bar[2]={335,1};check(arena.configure_adrenaline(source,bar,100));check(arena.configure_adrenaline(target,bar,100));
 combat::weapon::Owned axe{1,1,{15,22,2},12,false,false,2};check(arena.ordinary_weapon(source,axe,20));check(arena.physical_armor(target,60));check(arena.physical_absorption(target,3));
 const auto power=*combat::weapon::attack_skill(322,12);combat::weapon::Strike strike;strike.level=20;strike.mastery=12;strike.minimum=15;strike.maximum=22;strike.location_armor=60;strike.base_roll=.5;strike.miss_roll=.5;
 const auto raw=combat::weapon::resolve_attack_skill(strike,power,false)->damage;const auto gain=static_cast<unsigned>(std::floor(100.*raw/480));
 for(unsigned n=0;n<4;++n)check(arena.melee_attack(source,target,axe,power,101+n));
 auto pools=*arena.adrenaline(source),incoming=*arena.adrenaline(target);check(pools[0].units==80&&pools[1].units==100&&pools[2].units==80);check(incoming[0].units==4*gain); // pre-Absorption damage, not integer HP loss.
 check(!arena.consume_adrenaline({source.instance,source.actor,source.generation+1},{335,0},105));check(arena.consume_adrenaline(source,{335,1},105));pools=*arena.adrenaline(source);check(pools[0].units==55&&pools[1].units==75&&pools[2].units==0);
 check(arena.melee_attack(source,target,axe,*combat::weapon::attack_skill(335,12,0),106));pools=*arena.adrenaline(source);check(pools[0].units==80&&pools[1].units==100&&pools[2].units==25);
 auto sword=axe;sword.item_type=27;check(!arena.melee_attack(source,target,sword,*combat::weapon::attack_skill(335,12),107));
 check(arena.apply_condition(target,source,combat::conditions::Kind::blind,108,1000));auto before=*arena.adrenaline(source);check(arena.melee_attack(source,target,axe,power,109));check(arena.adrenaline(source)->at(1).units==before[1].units);
 const auto before_sacrifice=arena.adrenaline(source)->at(1).units;check(bool(arena.health_sacrifice(source,118,110)));check(arena.adrenaline(source)->at(1).units==before_sacrifice);
 arena.tick(25109);check(arena.adrenaline(source)->at(1).units==0);check(arena.adrenaline(target)->at(0).units==0);
 check(arena.melee_attack(source,target,axe,power,25200));check(arena.adrenaline(source)->at(1).units==25);check(arena.kill(source,25201));check(arena.adrenaline(source)->at(1).units==0);
 arena.leave(1);arena.join(1,"Recycled",0,0,a,0,{}, {},25300,1);const auto recycled=*arena.effect_scope(1);check(recycled!=source);check(!arena.configure_adrenaline(source,bar,25300));check(arena.adrenaline(recycled)->empty());
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

