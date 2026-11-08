#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Storm Chaser line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;Scope source,target;
 Fixture(){
  for(unsigned id:{1u,2u}){w.join(id,"Ranger",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{10000,10000,id==1?40u:0u,40},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(2);
  check(w.energy_capacity(2,1,0,100,{20,-2}));
  check(w.combat_armor(target,60)&&w.elemental_armor(target,60,60)&&w.lightning_armor(target,60)&&w.physical_armor(target,60));
  w.configure_attack_miss_draws([]{return .5;});
 }
 void apply(unsigned rank=12){const auto p=*storm_chaser(rank);check(w.status_mutation(target,455,123,rank,100,100+p.duration,true));}
};
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto plan=storm_chaser(rank);check(bool(plan));
  check(plan->energy==interpolate(1,5,rank)&&plan->duration==1000ull*interpolate(8,20,rank)&&plan->movement==1.25f);
  combat::Hit hit{};hit.reason=combat::Reason::accepted;hit.damage=0;
  for(auto kind:{combat::Kind::fire,combat::Kind::cold,combat::Kind::earth,combat::Kind::lightning}){hit.kind=kind;check(storm_chaser_energy(*plan,hit)==plan->energy);}
  hit.kind=combat::Kind::armor_ignoring;check(!storm_chaser_energy(*plan,hit));
  hit.kind=combat::Kind::fire;hit.reason=combat::Reason::unknown_armor;check(!storm_chaser_energy(*plan,hit));
 }
 check(!storm_chaser(21));
 for(unsigned kind=0;kind<5;++kind){Fixture f;f.apply();
  if(kind<2)check(f.w.elemental(f.source,f.target,*elemental(kind==0?191:846,0),20,100));
  if(kind==2)check(f.w.lightning(f.source,f.target,*lightning(224,0),20,100));
  if(kind==3)check(f.w.shockwave(f.source,*caster_earth_aoe(937,0),20,100));
  if(kind==4)check(f.w.elemental(f.source,f.target,*elemental(89,0),20,100)); // Cold plus shadow; only cold pays.
  check(f.w.resources(2)->energy==(kind==3?12u:4u));
 }
 for(unsigned raw:{2u,3u,4u,5u,11u}){Fixture f;f.apply();
  combat::weapon::Owned sword{1,1,{20,20,raw},12,false,false,27};check(f.w.ordinary_weapon(f.source,sword,20));
  check(f.w.melee_attack(f.source,f.target,sword,*combat::weapon::attack_skill(322,0),100));
  check(f.w.resources(2)->energy==(raw==2?0u:4u));
 }
 {Fixture f;f.apply();check(f.w.protective_enchantment(f.target,f.target,*regeneration::reviewed(299,20),0,100));
  check(f.w.elemental(f.source,f.target,*elemental(191,0),20,100));
  check(f.w.resources(2)->health==10000&&f.w.resources(2)->energy==4);
 }
 {Fixture f;f.apply();check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::poison,100,5000));f.w.tick(1100);
  check(f.w.resources(2)->energy==0&&f.w.resources(2)->health<10000);
 }
 for(unsigned mode=0;mode<4;++mode){Fixture f;f.apply(0);
  if(mode==0)check(f.w.status_mutation(f.target,455,123,0,200,{},false));
  if(mode==1)check(f.w.status_mutation(f.target,349,124,0,200,8200,true));
  if(mode==2)check(f.w.kill(f.target,200));
  if(mode!=2)check(f.w.elemental(f.source,f.target,*elemental(191,0),20,mode==3?8100:200));
  check(f.w.resources(2)->energy==0);
 }
 {Fixture f;f.w.set_vitals(2,{10000,10000,39,40},100);f.apply();
  check(f.w.elemental(f.source,f.target,*elemental(191,0),20,100)&&f.w.resources(2)->energy==40);
 }
 {Fixture f;f.apply(0);f.w.move(2,10000,0,100);f.w.tick(1100);
  check(std::abs(f.w.position(2,1100).first-460)<.3);
  check(f.w.status_mutation(f.target,455,123,0,1100,{},false));f.w.tick(2100);
  check(std::abs(f.w.position(2,2100).first-748)<.3);
 }
 for(bool pvp:{false,true}){Fixture f;f.w.set_vitals(2,{10000,10000,40,40},100);
  Slots bar{};bar[0]={455,0};check(f.w.configure_adrenaline(f.target,bar,100));
  casting::Hooks h;std::vector<Packet> wire;h.events=[&](const auto& packets){wire.insert(wire.end(),packets.begin(),packets.end());};
  h.resources=[&](auto id){return f.w.resources(id);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.target_valid=[](auto id,float,bool self){return id==2&&self;};
  h.consume_energy_at=[&](unsigned cost,auto now){check(cost==10);return f.w.consume_energy(2,cost,f.target,now);};
  h.effect_snapshot=[&](auto now){return f.w.status_snapshot(f.target,now);};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return f.w.status_mutation(f.target,skill,effect,rank,now,until,active);};
  Policy policy;policy.primary=2;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[455]=1;attributes::Ranks ranks{};ranks[24]=12;
  const auto apply=h.status_mutation;h.status_mutation={};casting::Runtime rejected(2);
  check(rejected.start(1000,{455,0,1},bar,policy,ranks,true,h)==casting::Reason::unsupported&&f.w.resources(2)->energy==40);
  h.status_mutation=apply;
  casting::Runtime runtime(2);check(runtime.start(1000,{455,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);
  check(!runtime.active()&&f.w.resources(2)->energy==30);
  check(f.w.status_snapshot(f.target,1000)->front().expires==19000);
  check(std::count(wire.begin(),wire.end(),casting::generic(21,2,767))==1);
  check(runtime.start(20999,{455,0,1},bar,policy,ranks,true,h)==casting::Reason::recharge);
  check(runtime.start(21000,{455,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);
 }
 std::cout<<"PASS Storm Chaser instant PvE/PvP cast,canonical timers,25percent movement,all elements/zero/mixed hits,Energy cap and removal/replacement/death/expiry\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
