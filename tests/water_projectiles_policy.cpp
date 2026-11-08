#include "gw_water_projectiles.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <limits>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Water projectile line "+std::to_string(at.line()));
}
int main()try{
 using namespace skills::effects;
 for(unsigned rank=0;rank<=20;++rank)for(unsigned id:{213u,214u,237u}){
  const auto p=water_projectile(id,rank);check(p&&valid_water_projectile(*p));
  check(p->damage==damage_scale(10,id==213?85:id==214?60:90,rank));
  const auto flight=projectiles::intercept(p->flight,0,0,p->flight.speed,0,0,0);
  check(flight&&flight->flight_ms==1000&&flight->x==p->flight.speed&&flight->y==0);
  auto forged=*p;forged.damage+=1;check(!valid_water_projectile(forged));
  forged=*p;forged.flight.speed+=1;check(!valid_water_projectile(forged));
  forged=*p;forged.maximum_targets+=1;check(!valid_water_projectile(forged));
 }
 check(!water_projectile(213,21)&&!water_projectile(229,12));
 const auto shard=*water_projectile(213,15),spear=*water_projectile(214,15),trident=*water_projectile(237,15);
 check(shard.snare_ms==6000&&shard.movement_multiplier==.34f&&shard.maximum_targets==1);
 check(spear.overcast_regeneration_pips==4&&spear.overcast_regeneration_ms==5000&&spear.snare_ms==0);
 check(water_projectile(214,20)->overcast_regeneration_pips==5);
 check(trident.maximum_targets==3&&trident.secondary_radius==166&&trident.moving_knockdown_ms==2000);
 const combat::Actor base{2,1,combat::Health{1000,1000},{},0,true,{},{},60};
 const combat::conditions::State clean;
 for(unsigned rank=0;rank<=20;++rank)for(unsigned id:{213u,214u,237u}){
  WaterProjectileImpact impact{{1,1,1},{1,2,1},*water_projectile(id,rank),true,100};
  const auto r=water_projectile_target(impact,base,clean,20,false,true);
  check(r.accepted&&r.hit.kind==combat::Kind::cold&&r.hit.skill==id&&r.hit.applied==impact.plan.damage);
  check(r.actor.health->current==1000-impact.plan.damage);
  check(r.snare_ms==(id==213?impact.plan.snare_ms:0));
  check(r.knockdown_ms==(id==237?2000:0));
  auto dead=base;dead.health->current=1;
  const auto lethal=water_projectile_target(impact,dead,clean,20);
  check(lethal.accepted&&lethal.hit.killed&&!lethal.snare_ms&&!lethal.knockdown_ms);
 }
 {
  WaterProjectileImpact impact{{1,1,1},{1,2,1},shard,false,100};
  auto r=water_projectile_target(impact,base,clean,20,true);
  check(r.accepted&&r.hit.applied==85&&!r.snare_ms);
  auto unknown=base;unknown.cold_armor.reset();check(!water_projectile_target(impact,unknown,clean,20).accepted);
  unknown=base;unknown.generation=2;check(!water_projectile_target(impact,unknown,clean,20).accepted);
  auto protected_actor=base;protected_actor.incoming_damage_multiplier=2;protected_actor.incoming_reduction=5;
  protected_actor.elemental_protection=combat::Kind::cold;protected_actor.elemental_multiplier=.5;
  r=water_projectile_target(impact,protected_actor,clean,20);check(r.accepted&&r.hit.applied==80);
  auto cracked=clean;check(cracked.apply(combat::conditions::Kind::cracked_armor,1,100,1000));
  auto armor=base;armor.cold_armor=80;
  r=water_projectile_target(impact,armor,cracked,20);check(r.accepted&&r.hit.applied==85);
  impact.plan=trident;impact.moving=true;
  check(!water_projectile_target(impact,base,clean,20).accepted);
  r=water_projectile_target(impact,base,clean,20,false,false);check(r.accepted&&!r.knockdown_ms);
  impact.moving=false;r=water_projectile_target(impact,base,clean,20);check(r.accepted&&!r.knockdown_ms);
  impact.plan=shard;impact.at=UINT64_MAX;
  check(!water_projectile_target(impact,base,clean,20).accepted);
  check(water_projectile_target(impact,base,clean,20,true).accepted);
 }
 for(unsigned id:{213u,214u,237u})for(unsigned mode=0;mode<12;++mode){
  WaterProjectileFlight shot;const Scope caster{1,1,1};
  WaterProjectileTarget target{{1,2,1},600,0,0,true,true,false};
  const auto plan=*water_projectile(id,12);
  check(shot.launch(caster,target,plan,0,0,0,0,0,100));
  check(!shot.launch(caster,target,plan,0,0,0,0,0,100));
  const auto due=shot.arrives();check(due>100);
  check(!shot.impact(caster,target,due-1)&&shot.pending());
  auto source=caster;
  if(mode==1)++source.generation;
  if(mode==2)++target.scope.generation;
  if(mode==3)target.x+=33;
  if(mode==4)target.plane=1;
  if(mode==5)target.alive=false;
  if(mode==6)target.hostile=false;
  if(mode==7)target.moving=true;
  if(mode==8)target.x+=32;
  if(mode==9)target.x=std::numeric_limits<double>::quiet_NaN();
  if(mode==10)shot.cancel();
  if(mode==11){check(!shot.impact(source,target,99)&&!shot.pending());continue;}
  const auto hit=shot.impact(source,target,due+5000);
  check(bool(hit)==(mode==0||mode==7||mode==8));
  if(hit)check(hit->at==due&&hit->moving==(mode==7)&&hit->target==target.scope&&hit->caster==caster);
  check(!shot.pending()&&!shot.impact(source,target,due+5001));
 }
 {
  WaterProjectileFlight shot;const Scope caster{1,1,1};
  WaterProjectileTarget target{{1,2,1},600,0,0,true,true,false};
  check(!shot.launch(caster,target,shard,0,0,1,0,0,100));
  check(!shot.launch(caster,target,shard,0,0,0,0,0,UINT64_MAX));
  target.x=1249;check(!shot.launch(caster,target,shard,0,0,0,0,0,100));
  check(!shot.pending());
 }
 for(unsigned id:{213u,214u,237u}){
  WaterProjectileFlight shot;const Scope caster{1,1,1};
  WaterProjectileTarget target{{1,2,1},600,0,0,true,true,false};
  const auto plan=*water_projectile(id,12);
  check(shot.launch(caster,target,plan,0,0,0,0,200,100));
  check(shot.aim().y>0&&shot.aim().x==600);
  // Fixed launch aim cannot follow a subsequent turn. Hitting the predicted
  // endpoint succeeds; remaining at the old position dodges this shot.
  auto endpoint=target;endpoint.x=shot.aim().x;endpoint.y=shot.aim().y;endpoint.moving=true;
  const auto hit=shot.impact(caster,endpoint,shot.arrives());check(hit&&hit->moving);
  WaterProjectileFlight dodge;check(dodge.launch(caster,target,plan,0,0,0,0,200,100));
  check(!dodge.impact(caster,target,dodge.arrives())&&!dodge.pending());
 }
 {
  const Scope caster{1,1,1};WaterProjectileTarget target{{1,2,1},1414,0,0,true,true,false};
  WaterProjectileFlight secondary,primary,invalid;
  check(secondary.launch(caster,target,trident,0,0,0,0,0,100,true));
  check(!primary.launch(caster,target,trident,0,0,0,0,0,100));
  target.x=1415;check(!invalid.launch(caster,target,trident,0,0,0,0,0,100,true));
  target.x=600;check(!invalid.launch(caster,target,spear,0,0,0,0,0,100,true));
 }
 // Literal pinned-2022 serialization of the frozen modern launch receipts.
 const combat::Packet shard_wire{163,128,25,0,0,0,0,64,180,197,0,64,247,68,0,0,204,71,95,61,122,1,0,0,1,0,0,0,0};
 const combat::Packet spear_wire{163,128,25,0,0,0,0,64,180,197,0,64,247,68,0,0,217,117,167,61,123,1,0,0,1,0,0,0,0};
 const combat::Packet trident_wire{163,128,25,0,0,0,0,64,180,197,0,64,247,68,0,0,85,224,248,60,161,1,0,0,1,0,0,0,0};
 check(skills::particles::spell_projectile_launch_2022(25,-5768,1978,0,std::bit_cast<float>(0x3d5f47ccu),213)==shard_wire);
 check(skills::particles::spell_projectile_launch_2022(25,-5768,1978,0,std::bit_cast<float>(0x3da775d9u),214)==spear_wire);
 check(skills::particles::spell_projectile_launch_2022(25,-5768,1978,0,std::bit_cast<float>(0x3cf8e055u),237)==trident_wire);
 for(unsigned id:{0u,215u}){bool rejected=false;try{skills::particles::spell_projectile_launch_2022(25,0,0,0,1,id);}catch(const std::invalid_argument&){rejected=true;}check(rejected);}
 std::cout<<"PASS Water projectile reviewed plans,flight durations and three captured wire fixtures\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
