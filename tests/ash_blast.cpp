#include "gw_ash_blast.h"
#include <iostream>
#include <source_location>
#include <string>
#include <stdexcept>
using namespace fw::gw;using namespace skills::effects;
void check(bool b,std::source_location l=std::source_location::current()){if(!b)throw std::runtime_error("Ash Blast line "+std::to_string(l.line()));}
int main()try{
 const Scope caster{1,25,1},target{1,30,1};Registry registry(target);combat::conditions::State burning;check(burning.apply(combat::conditions::Kind::burning,25,100,6000));
 combat::Actor actor;actor.id=30;actor.generation=1;actor.health=combat::Health{480,480};actor.earth_armor=60;actor.damageable=true;
 for(unsigned rank=0;rank<=20;++rank){const auto p=*ash_blast(rank);
  const auto hit=prepare_ash_blast_target(p,caster,target,7,100,registry,burning,actor,20,true,166);check(hit&&hit->hex&&hit->hit.damage==35+2*rank&&hit->actor.health->current==480-p.damage&&hit->registry.size()==1&&registry.size()==0&&actor.health->current==480);
  check(ash_blast_miss_chance(hit->registry,target,100)==p.miss_percent*.01);check(ash_blast_miss_chance(hit->registry,target,5099)==p.miss_percent*.01);check(ash_blast_miss_chance(hit->registry,target,5100)==0.);
  const double values[]{.5,*ash_blast_miss_chance(hit->registry,target,100)};const auto combined=combat::weapon::combined_miss_chance(values);check(combined&&std::abs(*combined-(1-.5*(1-p.miss_percent*.01)))<1e-12);
  auto removed=hit->registry;check(removed.remove(target,7).has_value());check(ash_blast_miss_chance(removed,target,101)==0.);
  combat::conditions::State clear;const auto dry=prepare_ash_blast_target(p,caster,target,7,100,registry,clear,actor,20,true,0);check(dry&&!dry->hex&&dry->hit.damage==p.damage&&dry->registry.size()==0);
  auto forged=p;++forged.damage;check(!prepare_ash_blast_target(forged,caster,target,7,100,registry,burning,actor,20,true,0));
 }
 const auto p=*ash_blast(0);check(!ash_blast(21));
 const auto followup=prepare_ash_blast_hex(p,caster,target,7,100,registry,true,true);
 check(followup&&followup->registry.lookup(target,7)&&registry.size()==0);
 check(!prepare_ash_blast_hex(p,caster,target,7,100,registry,false,true));
 check(!prepare_ash_blast_hex(p,caster,target,7,100,registry,true,false));
 check(!prepare_ash_blast_hex(p,caster,target,8,UINT64_MAX,registry,true,true));
 check(!prepare_ash_blast_target(p,caster,target,7,100,registry,burning,actor,20,true,166.01f));
 check(!prepare_ash_blast_target(p,caster,target,7,100,registry,burning,actor,20,false,0));
 check(!prepare_ash_blast_target(p,{2,25,1},target,7,100,registry,burning,actor,20,true,0));
 check(!prepare_ash_blast_target(p,caster,{1,30,2},7,100,registry,burning,actor,20,true,0));
 auto unknown=actor;unknown.earth_armor.reset();check(!prepare_ash_blast_target(p,caster,target,7,100,registry,burning,unknown,20,true,0));
 auto weak=actor;weak.health->current=10;const auto kill=prepare_ash_blast_target(p,caster,target,7,100,registry,burning,weak,20,true,0);check(kill&&kill->hit.killed&&!kill->hex&&kill->registry.size()==0);
 const auto expired=prepare_ash_blast_target(p,caster,target,7,6100,registry,burning,actor,20,true,0);check(expired&&!expired->hex);
 const auto first=*prepare_ash_blast_target(p,caster,target,7,100,registry,burning,actor,20,true,0);
 const auto replacement=prepare_ash_blast_target(p,caster,target,8,101,first.registry,burning,actor,20,true,0);check(replacement&&replacement->removed.size()==1&&!replacement->registry.lookup(target,7));
 check(!prepare_ash_blast_target(p,caster,target,7,101,first.registry,burning,actor,20,true,0));check(!ash_blast_miss_chance(first.registry,{1,30,2},100));
 std::cout<<"PASS21 Ash Blast plans and copied earth damage/burning hex proposals; accuracy consumer,boundary,expiry,replacement,death and scope guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
