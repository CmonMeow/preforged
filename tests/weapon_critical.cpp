#include "../src/runtime/gw_weapon_critical.h"
#include "../src/runtime/gw_weapon_attack_skills.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::combat::weapon;
void check(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
int main(){
 CriticalProfile warrior{1,0,20},assassin{7,0,20};
 check(warrior.valid()&&!CriticalProfile{1,1,20}.valid(),"primary-only effective rank");
 const auto chance=critical_chance(warrior,20,12,0);
 check(chance&&std::abs(*chance-.1255)<1e-12,"independent level20 mastery12 example");
 check(std::abs(*critical_chance(CriticalProfile{1,0,19},20,12,0)-.12454547674984816)<1e-12,"fractional oddlevel threshold");
 check(!critical_chance(warrior,0,12,0)&&!critical_chance(warrior,20,21,0),"unknown level/invalid mastery");
 check(!critical_chance(warrior,20,12,NAN),"unknown item penalty");
 check(*critical_chance(warrior,20,12,.05)<*chance,"known item critical penalty");
 for(unsigned rank=0;rank<=20;++rank){assassin.critical_strikes=rank;check(critical_energy(assassin)==(rank<3?0u:rank<8?1u:rank<13?2u:rank<18?3u:4u),"independent Energy threshold table");}
 check(critical_sample(*chance,0)==true&&critical_sample(*chance,*chance)==false&&!critical_sample(*chance,1),"half-open independent draw");
 Strike s;s.level=20;s.mastery=12;s.minimum=10;s.maximum=20;s.base_roll=.5;s.miss_roll=.5;s.location_armor=60;
 check(resolve(s)&&std::abs(resolve(s)->damage-15)<1e-12&&!resolve(s)->critical,"normal midpoint");
 s.critical=true;check(resolve(s)->critical&&std::abs(resolve(s)->damage-20*std::sqrt(2.))<1e-12,"critical maximum and20armor");
 s.critical_damage_suppressed=true;check(resolve(s)->critical&&resolve(s)->damage==15,"damage suppression preserves critical event");
 check(resolve(s)->critical_energy,"damage suppression alone does not deny attribute Energy");
 s.critical_energy_suppressed=true;check(resolve(s)->critical&&!resolve(s)->critical_energy,"Stoneflesh explicit attribute Energy denial preserves event");s.critical_energy_suppressed=false;
 s.critical_immune=true;check(!resolve(s)->critical&&resolve(s)->damage==15,"event immunity suppresses entitlement");
 s.critical_immune=false;s.critical_damage_suppressed=false;s.item_type=35;
 check(std::abs(resolve(s)->damage-20*std::exp2(.125))<1e-12,"scythe exact5armor increment");
 s.blind=true;s.miss_roll=0;check(resolve(s)->missed&&!resolve(s)->critical,"miss cannot become critical event");
 s.blind=false;s.critical=false;s.item_type=2;
 const auto forced=resolve_attack_skill(s,*attack_skill(321,0),false);
 check(forced&&forced->critical,"forced critical survives typed skill result");
 std::cout<<"weapon critical policy passed\n";
}
