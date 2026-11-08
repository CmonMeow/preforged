#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <fstream>
#include <filesystem>
using namespace fw::gw;
void check(bool yes,std::source_location at=std::source_location::current()){
 if(!yes)throw std::runtime_error("NPC Flare World line "+std::to_string(at.line()));
}
std::shared_ptr<const navigation::Mesh> flare_corner_mesh(){
 const auto path=std::filesystem::temp_directory_path()/"npc-flare-release-wall.fwnav";
 std::ofstream out(path,std::ios::binary|std::ios::trunc);
 auto word=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
 auto real=[&](float n){word(std::bit_cast<std::uint32_t>(n));};
 out.write("FWNAV001",8);for(auto n:{1u,7u,0xEEFE704cu,12u,1u,1u})word(n);
 for(float n:{0.f,0.f,1000.f,1000.f})real(n);word(1);word(3);
 for(unsigned i=0;i<3;++i){word(i?i-1:0xffffffff);word(i<2?i+1:0xffffffff);for(unsigned j=0;j<3;++j)word(0xffffffff);
  const float left=i==0?0.f:i==1?100.f:200.f,right=i==0?100.f:i==1?200.f:1000.f;
  for(float n:{0.f,i==0?10.f:1000.f,left,right,left,right})real(n);
 }
 out.close();return std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path));
}
struct Fixture {
 world::Instance w;skills::effects::Scope source,target;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(float distance=1600,bool energy=true,bool armor=true,bool wall=false){
  if(wall)w.configure_navigation(flare_corner_mesh());
  world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  if(wall){n.x=5;n.y=9;}
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
  d.conditions_allowed=true;d.knockdown_allowed=true;d.knockdown_evidence="authored fixture";
  if(energy){d.spawn_energy=combat::npcs::Energy{30,30};d.energy_pips=0;}n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Target",distance,wall?9:0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  source=*w.effect_scope(100);target=*w.effect_scope(1);if(armor)check(w.elemental_armor(target,60,60));
 }
 bool start(float leash=2000,double flight_speed=1800,std::size_t capacity=8,float acquire=2000,float follow=2500){combat::ai::FlarePolicy p;p.source=source;p.fire_magic=12;p.level=20;p.speed=300;p.acquire=acquire;p.follow=follow;p.leash=leash;
  p.flight={flight_speed,32};p.max_flights=capacity;p.evidence="authored offline caster fixture, native Flare row194";return w.configure_npc_flare_ai(p,1000);}
 void tick(std::uint64_t until){for(std::uint64_t t=1000;t<=until;t+=100)w.tick(t);}
};
int main()try{
 for(bool reversal:{false,true}){
  Fixture f(600);check(f.start());
  check(f.w.status_mutation(f.target,1399,80,12,100,6100,true));
  if(reversal)check(f.w.status_mutation(f.target,307,81,12,100,8100,true));
  f.w.tick(1000);f.w.tick(2000);f.w.tick(2400);
  const auto damage=unsigned(skills::effects::elemental(194,12)->damage);
  check(f.w.resources(1)->health==(reversal?1000u:1000u-(damage-5)));
  f.w.tick(2750);f.w.tick(3750);f.w.tick(4200);
  check(f.w.resources(1)->health==1000u-(reversal?0u:damage-5)-(damage-10));
 }

 {Fixture f(1000);check(!f.start(2000,100,0)&&!f.start(2000,100,65));}
 {Fixture f(1000);check(f.start(2000,100,2));
  combat::ai::ordinary::Reviewed ordinary{10,10,0,5,true,combat::ai::ordinary::Delivery::ranged,500,1500,1248,{100,32},"authored capacity fallback fixture"};
  check(f.w.configure_npc_ordinary_ai(f.source,ordinary,1000));
  f.w.tick(1000);f.w.tick(2000);auto state=*f.w.npc_flare_ai_snapshot(f.source);
  check(state.shots.size()==1&&state.shots[0].target==f.target&&state.shots[0].plane==0&&state.shots[0].arrives==12000);
  f.w.tick(2650);check(!f.w.npc_flare_ai_snapshot(f.source)->cast);
  f.w.tick(2750);check(f.w.npc_flare_ai_snapshot(f.source)->cast.has_value()&&f.w.resources(1)->health==1000);
  f.w.tick(3750);state=*f.w.npc_flare_ai_snapshot(f.source);
  check(state.shots.size()==2&&state.shots[0].sequence!=state.shots[1].sequence&&state.shots[1].arrives==13750);
  f.w.tick(4500);check(!f.w.npc_flare_ai_snapshot(f.source)->cast&&f.w.npc_energy_snapshot(f.source)->current==20);
  check(f.w.npc_ordinary_pending(f.source).has_value()); // full spell queue does not stall ordinary fallback
 }
 {Fixture f(1000);check(f.start(2000,100,2,1000,1000));f.w.tick(1000);f.w.tick(2000);
  f.w.join(2,"Alternative",200,0,std::make_shared<world::Outbox>(),0,{}, {},2000,2);f.w.set_vitals(2,{1000,1000,100,100},100);
  const auto other=*f.w.effect_scope(2);check(f.w.elemental_armor(other,60,60));
  f.w.move(1,1300,0,2050);f.w.tick(2750);check(f.w.npc_flare_ai_snapshot(f.source)->policy.target==other);
  f.w.move(1,1000,0,2800);f.w.tick(3750);
  const auto state=*f.w.npc_flare_ai_snapshot(f.source);
  check(state.shots.size()==2&&state.shots[0].target==f.target&&state.shots[1].target==other);
  check(f.w.damage_npc(f.target,f.source,{combat::Kind::armor_ignoring,1000,20,194},1248,3800).killed);
  check(f.w.settle_resources(f.target,13000));const auto original_health=f.w.resources(1)->health;
  check(original_health<1000&&f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
  f.w.tick(12000); // Both original-target deliveries preceded the13000 resource event.
  check(f.w.resources(1)->health==original_health&&f.w.resources(2)->health<1000);
  check(f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
  f.w.tick(13000);check(f.w.resources(1)->health==original_health&&f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
  const auto health=f.w.resources(1)->health;f.w.tick(13100);check(f.w.resources(1)->health==health);
 }
 {Fixture f(1000);check(f.start(2000,100));f.w.tick(1000);f.w.tick(2000);
  f.w.join(2,"Observer",200,0,std::make_shared<world::Outbox>(),0,{}, {},2000,2);f.w.set_vitals(2,{1000,1000,100,100},100);
  check(f.w.damage_npc(*f.w.effect_scope(2),f.source,{combat::Kind::armor_ignoring,1000,20,194},1248,2100).killed);
  const auto original=f.target;f.w.leave(1);f.w.join(1,"Replacement",1000,0,f.box,0,{}, {},2200,1);f.w.set_vitals(1,{1000,1000,100,100},100);
  f.target=*f.w.effect_scope(1);check(f.target!=original&&f.w.elemental_armor(f.target,60,60));
  f.w.tick(12000);check(f.w.resources(1)->health==1000&&f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
 }
 {Fixture f(1000);check(f.start(2000,100));
  check(f.w.spiteful_spirit(f.target,f.source,*skills::effects::spiteful_spirit(12),20,1000));
  f.w.tick(1000);f.w.tick(2000);check(f.w.npc_combat_snapshot(f.source)->health->current==471);
  f.w.tick(2750);f.w.tick(3750);check(f.w.npc_combat_snapshot(f.source)->health->current==442);
  check(f.w.npc_flare_ai_snapshot(f.source)->shots.size()==2);
  f.w.tick(3850);check(f.w.npc_combat_snapshot(f.source)->health->current==442); // no reaction per retained flight/tick
 }
 {Fixture f(1000);check(f.start(2000,100));f.w.tick(1000);f.w.tick(2000);f.w.tick(2750);
  check(f.w.interrupt_skill(f.target,f.source,*skills::effects::interrupt_skill(5,12),20,2800));
  f.w.tick(3750);check(!f.w.npc_flare_ai_snapshot(f.source)->cast&&f.w.npc_flare_ai_snapshot(f.source)->shots.size()==1);
  check(f.w.damage_npc(f.target,f.source,{combat::Kind::armor_ignoring,1000,20,194},1248,3800).killed);
  f.w.tick(12000);check(f.w.resources(1)->health<1000&&f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
 }
 for(unsigned mode=0;mode<4;++mode){Fixture f(1600);check(f.start(mode==3?10:2000));
  f.w.tick(1000);f.w.tick(1100); // pursue away from home, still outside spell range
  const auto pursuing=*f.w.npc_flare_ai_snapshot(f.source);
  check(!pursuing.cast);
  if(mode==3)check(!pursuing.engaged&&pursuing.returning); // clipped at the ten-unit home boundary
  else check(pursuing.engaged&&!pursuing.returning);
  f.w.join(2,"Alternative",300,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);
  f.w.set_vitals(2,{1000,1000,100,100},100);const auto other=*f.w.effect_scope(2);check(f.w.elemental_armor(other,60,60));
  if(mode==1)f.w.leave(1);else if(mode!=3)check(f.w.kill(f.target,1150));
  if(mode==2)check(f.w.settle_resources(other,4000));
  f.w.tick(1200);const auto ai=*f.w.npc_flare_ai_snapshot(f.source);
  if(mode<2){check(ai.engaged&&!ai.returning&&ai.policy.target==other&&ai.cast.has_value());
   check(f.w.npc_energy_snapshot(f.source)->current==25);}
  else {check(!ai.engaged&&!ai.cast&&f.w.npc_energy_snapshot(f.source)->current==30);
   if(mode==2)check(ai.returning);
   else {check(!ai.returning); // return reaches home before any reacquisition
    f.w.tick(1300);const auto reacquired=*f.w.npc_flare_ai_snapshot(f.source);
    check(reacquired.engaged&&!reacquired.returning&&reacquired.policy.target==other&&reacquired.cast.has_value());
    check(f.w.npc_energy_snapshot(f.source)->current==25);}}
 }
 {Fixture f(100);check(f.w.settle_resources(f.target,4000)&&f.start());f.w.tick(1000);
  check(!f.w.npc_flare_ai_snapshot(f.source)->engaged&&f.w.npc_energy_snapshot(f.source)->current==30);}
 {Fixture f(100);check(f.w.spiteful_spirit(f.target,f.source,*skills::effects::spiteful_spirit(12),20,100));
  check(f.start());f.w.tick(1000);const auto end=f.w.npc_flare_ai_snapshot(f.source)->cast->completes;
  check(f.w.npc_combat_snapshot(f.source)->health->current==500);f.w.tick(end);
  check(f.w.npc_combat_snapshot(f.source)->health->current==471&&f.w.resources(1)->health==1000);
  f.w.tick(end);f.w.tick(end+100);check(f.w.npc_combat_snapshot(f.source)->health->current==471);
  check(f.w.resources(1)->health<1000); // Hostile NPC's human target is not an allied area recipient.
 }
 {Fixture f(100);check(f.start());f.w.tick(1000);const auto end=f.w.npc_flare_ai_snapshot(f.source)->cast->completes;
  f.w.join(2,"Other",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{1000,1000,100,100},100);
  const auto other=*f.w.effect_scope(2);check(f.w.elemental_armor(other,60,60));
  check(f.w.backfire(other,f.source,*skills::effects::backfire(28,12),20,1100));
  check(f.w.kill(f.target,1200));f.w.tick(end);
  check(!f.w.npc_flare_ai_snapshot(f.source)->cast&&f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
  check(f.w.npc_energy_snapshot(f.source)->current==25&&f.w.npc_combat_snapshot(f.source)->health->current==500);
  f.w.tick(end+100);f.w.tick(end+200);check(f.w.npc_flare_ai_snapshot(f.source)->cast.has_value());
  check(f.w.npc_energy_snapshot(f.source)->current==20); // paid again before successful aftercast would expire
 }
 {Fixture f;check(f.start());f.tick(6000);check(f.w.resources(1)->health<1000);
  const auto ai=*f.w.npc_flare_ai_snapshot(f.source);check(ai.engaged&&ai.policy.target==f.target);
  check(f.w.npc_energy_snapshot(f.source)->current<30);
  const auto packets=f.box->drain();check(std::any_of(packets.begin(),packets.end(),[](const auto& p){return p.size()>=2&&p[0]==159&&p[1]==0;}));
  const auto health=f.w.resources(1)->health;f.w.tick(6000);check(f.w.resources(1)->health==health);
  for(std::uint64_t t=6100;t<=16000;t+=100)f.w.tick(t);
  check(f.w.npc_energy_snapshot(f.source)->current==0&&!f.w.npc_flare_ai_snapshot(f.source)->cast);
 }
 {Fixture f;check(f.start());for(std::uint64_t t=1000;t<=2000;t+=100)f.w.tick(t);
  check(f.w.npc_flare_ai_snapshot(f.source)->engaged);check(f.w.kill(f.target,2100));
  for(std::uint64_t t=2100;t<=4000;t+=100)f.w.tick(t);
  const auto ai=*f.w.npc_flare_ai_snapshot(f.source);check(!ai.engaged&&!ai.returning&&!ai.cast);
 }
 {Fixture f(100);check(f.start());f.w.tick(1000);const auto first=*f.w.npc_flare_ai_snapshot(f.source);check(first.cast.has_value());
  check(f.w.interrupt_skill(f.target,f.source,*skills::effects::interrupt_skill(5,12),20,1100));
  for(std::uint64_t t=1100;t<=3000;t+=100)f.w.tick(t);
  check(f.w.resources(1)->health==1000&&f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
 }
 {Fixture f(100);check(f.start());f.w.tick(1000);check(f.w.kill(f.target,1100));
  for(std::uint64_t t=1100;t<=6000;t+=100)f.w.tick(t);
  check(f.w.resources(1)->health==0&&!f.w.npc_flare_ai_snapshot(f.source)->engaged);
 }
 {Fixture f(100,false);check(!f.start());f.tick(3000);check(f.w.resources(1)->health==1000);}
 {Fixture f(100,true,false);check(f.start());f.tick(3000);check(!f.w.npc_flare_ai_snapshot(f.source)->cast&&f.w.resources(1)->health==1000);}
 {Fixture f(3000);check(f.start());f.tick(3000);check(!f.w.npc_flare_ai_snapshot(f.source)->engaged&&f.w.resources(1)->health==1000);}
 {Fixture f(105,true,true,true);check(f.start());f.w.tick(1000);check(f.w.npc_flare_ai_snapshot(f.source)->cast.has_value());
  f.w.steer(1,0,1,1100);f.w.tick(2000);
  check(f.w.npc_flare_ai_snapshot(f.source)->shots.empty()&&f.w.resources(1)->health==1000);
  check(f.w.npc_energy_snapshot(f.source)->current==25); // obstructed release is paid, not refunded
 }
 for(unsigned clock=0;clock<3;++clock){Fixture f(100);check(f.start());f.w.tick(1000);f.w.tick(2000);
  check(!f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
  if(clock==0)check(f.w.settle_resources(f.target,4000)); // Health and Energy advance together
  if(clock==1)f.w.move(1,100,0,4000); // identical pose, later movement authority
  if(clock==2)f.w.steer(1,0,1,4000); // active steering authority ahead of sampled tick
  const auto health=f.w.resources(1)->health;check(health<1000&&f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
  f.w.tick(2100);check(f.w.npc_flare_ai_snapshot(f.source)->shots.empty()&&f.w.resources(1)->health==health);
  f.w.tick(4000);check(f.w.npc_flare_ai_snapshot(f.source)->shots.empty()&&f.w.resources(1)->health==health);
 }
 {Fixture f(100);check(f.start());f.w.tick(1000);
  // A separate player's future Weakness advances the NPC's condition clock
  // without advancing the selected target or interrupting the paid cast.
  f.w.join(2,"Other",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{1000,1000,100,100},100);
  check(f.w.condition_spell(*f.w.effect_scope(2),f.source,*skills::effects::condition_spell(117,0),4000));
  f.w.tick(2000);check(f.w.npc_flare_ai_snapshot(f.source)->cast.has_value()&&f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
  f.w.tick(4000);check(!f.w.npc_flare_ai_snapshot(f.source)->cast&&!f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
  f.w.tick(4100);check(f.w.resources(1)->health<1000);
 }
 {Fixture f(100);check(f.start());f.w.tick(1000);f.w.tick(2000);check(!f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
  check(f.w.damage_npc(f.target,f.source,{combat::Kind::armor_ignoring,1000,20,194},1248,2010).killed);
  f.w.tick(2100);check(f.w.resources(1)->health<1000&&f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
 }
 for(bool sparse:{false,true}){
  // A released NPC spell survives its caster's death and shares the same
  // chronology as a player's field. A late resource request must settle both.
  Fixture f(100);check(f.start(2000,100));f.w.tick(1000);f.w.tick(2000);
  check(f.w.npc_flare_ai_snapshot(f.source)->shots.size()==1);
  f.w.join(2,"Field caster",100,0,std::make_shared<world::Outbox>(),0,{}, {},2000,2);
  f.w.set_vitals(2,{1000,1000,100,100},100);
  const auto other=*f.w.effect_scope(2);
  check(f.w.damage_npc(other,f.source,{combat::Kind::armor_ignoring,1000,20,194},1248,2100).killed);
  check(f.w.fire_storm(other,f.target,*skills::effects::fire_storm(0,197),20,2100));
  if(!sparse)for(unsigned at=2200;at<6100;at+=100)f.w.tick(at);
  check(f.w.settle_resources(f.target,6100));
  check(f.w.resources(1)->health==1000-skills::effects::elemental(194,12)->damage-20);
  check(f.w.npc_flare_ai_snapshot(f.source)->shots.empty());
  const auto hp=f.w.resources(1)->health;f.w.tick(6100);check(f.w.resources(1)->health==hp);
 }
 std::cout<<"PASS autonomous NPC acquisition,approach,paid casting,projectile damage,interrupt,fail-closed inputs and mixed field/delivery chronology\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
