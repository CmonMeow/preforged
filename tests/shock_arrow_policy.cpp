#include "gw_shock_arrow.h"
#include "gw_lightning.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Shock Arrow policy line "+std::to_string(at.line()));
}
int main()try{
 check(!shock_arrow(21,0)&&!shock_arrow(0,21));
 combat::Actor target;target.id=2;target.health=combat::Health{1000,1000};target.lightning_armor=80;target.damageable=true;
 for(unsigned rank=0;rank<=20;++rank)for(unsigned storage=0;storage<=20;++storage){const auto p=shock_arrow(rank,storage);
  check(p&&p->damage==damage_scale(5,50,rank)&&p->penetration==25&&p->energy_if_cracked==5+storage/2);
  combat::conditions::State conditions;check(conditions.apply(combat::conditions::Kind::cracked_armor,1,1000,1000));
  const auto plain=shock_arrow_target(*p,target,{},20,1000),cracked=shock_arrow_target(*p,target,conditions,20,1000);
  check(plain&&cracked&&plain->energy==0&&cracked->energy==5+storage/2&&cracked->hit.applied>=plain->hit.applied);
  const auto expired=shock_arrow_target(*p,target,conditions,20,2000);check(expired&&expired->energy==0);
 }
 const auto p=*shock_arrow(12,13);auto forged=p;++forged.energy_if_cracked;check(!shock_arrow_target(forged,target,{},20,1000));
 const auto live=lightning(1082,12,13);check(live&&live->projectile&&live->storage==13&&live->energy_if_cracked==11&&valid_lightning(*live));
 auto changed=*live;++changed.energy_if_cracked;check(!valid_lightning(changed));
 const auto impact=skills::particles::shock_arrow_impact_2022(2,1);check(impact.size()==18&&impact[0]==159&&impact[2]==20&&impact[14]==0x9d&&impact[15]==4);
 const auto flight=skills::particles::spell_projectile_launch_2022(1,200,0,0,.084f,1082);check(flight.size()==29&&flight[0]==163&&flight[20]==0x9b&&flight[21]==4);
 auto unknown=target;unknown.lightning_armor.reset();check(!shock_arrow_target(p,unknown,{},20,1000));
 auto dead=target;dead.health->current=0;check(!shock_arrow_target(p,dead,{},20,1000));
 combat::conditions::State conditions;check(conditions.apply(combat::conditions::Kind::cracked_armor,1,1000,1000));
 auto lethal=target;lethal.health->current=1;const auto hit=shock_arrow_target(p,lethal,conditions,20,1000);
 check(hit&&hit->hit.killed&&hit->energy==11);
 std::cout<<"PASS staged Shock Arrow all441Air/EnergyStorage rank pairs,Cracked Armor at impact/expiry,25percent penetration,lethal Energy qualification and plan/armor gates\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
