#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Screaming Shot line "+std::to_string(at.line())+context);}
struct Fixture:warrior_test::Fixture{
 Fixture(bool npc,float distance=1000,unsigned health=1000,bool conditions=true):warrior_test::Fixture(1719,npc,health,true,conditions){
  world.leave(1);world.join(1,"Archer",60-distance,0,box,0,{}, {},100,1);world.set_vitals(1,{1000,1000,30,30},100);source=*world.effect_scope(1);
  check(world.energy_capacity(1,1,0,100,{10,-2})&&world.ordinary_weapon(source,weapon,20));
 }
 bool bleeding(std::uint64_t at){return world.has_condition(target,combat::conditions::Kind::bleeding,at);}
};
int main()try{
 for(unsigned r=0;r<=20;++r){const auto p=combat::weapon::screaming_shot(r);check(p&&p->bonus==10+r&&p->bleeding_ms==1000*(5+r));check(combat::weapon::attack_skill(1719,r)->required_type==5);
  check(combat::weapon::screaming_earshot(*p,0,0,1012,0,true)==true);check(combat::weapon::screaming_earshot(*p,0,0,1012.01,0,true)==false);check(combat::weapon::screaming_earshot(*p,0,0,1,0,false)==false);
  auto forged=*p;++forged.bleeding_ms;check(!combat::weapon::screaming_earshot(forged,0,0,0,0,true));}
 check(!combat::weapon::screaming_shot(21));
 for(bool npc:{false,true})for(float distance:{1011.f,1012.f,1012.25f,1100.f})for(unsigned mode=0;mode<5;++mode){
  context=" npc="+std::to_string(npc)+" distance="+std::to_string(distance)+" mode="+std::to_string(mode);Fixture f(npc,distance,mode==3?1:1000,mode!=4);auto& w=f.world;
  if(mode==1)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==2){w.configure_attack_block_draws([]{return 0.;});if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),100));else check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));}
  const auto arrival=1000+projectiles::intercept({2500,32},60-distance,0,60,0,0,0)->flight_ms;
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1719,12,0),1000));w.tick(arrival);
  const bool hit=mode!=1&&mode!=2;check(f.health()==(hit?(mode==3?0:958):1000));
  const bool bleeds=hit&&mode!=3&&(!npc||mode!=4)&&distance<=1012;check(f.bleeding(arrival)==bleeds);
  if(bleeds){check(f.bleeding(arrival+16999));check(!f.bleeding(arrival+17000));}
 }
 // Only source movement changes earshot;target remains on the arrow aim point.
 for(bool npc:{false,true})for(bool enter:{false,true}){Fixture f(npc,enter?1100:1000);auto& w=f.world;const auto arrival=enter?1440u:1400u;
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1719,12,0),1000));w.move(1,enter?0.f:-1300.f,0,1000);w.tick(arrival);
  check(f.health()==958&&f.bleeding(arrival)==enter);
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){Fixture f(npc,1000);auto& w=f.world;casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet> wire;unsigned releases=0;
  h.events=[&](const auto& p){wire.insert(wire.end(),p.begin(),p.end());};h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};h.consume_energy_at=[&](auto n,auto at){check(n==10);return w.consume_energy(1,n,f.source,at);};
  h.weapon_snapshot=[&](){return std::optional(f.weapon);};h.melee_target=[&](auto id,auto at){return w.melee_target(f.source,id,at,1719);};h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){++releases;return w.melee_attack(f.source,target,gear,plan,at);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};
  Policy p;p.primary=2;p.campaigns=255;p.learned[1719]=1;Slots bar{};bar[0]={1719,0};attributes::Ranks ranks{};ranks[25]=12;
  if(mode==3)h.melee_attack={};check(runtime.start(1000,{1719,0,f.target.actor},bar,p,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));if(mode==3){check(w.resources(1)->energy==30);continue;}
  check(w.resources(1)->energy==20);if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1200,h);runtime.update(1500,h);if(mode==4)check(w.kill(f.source,1600));w.tick(1900);
  const bool hit=mode==0;check(f.health()==(hit?958:1000)&&f.bleeding(1900)==hit);check(releases==unsigned(mode!=1&&mode!=2));
  check(std::count(wire.begin(),wire.end(),casting::activation(1,f.target.actor,1719,true))==1&&std::count(wire.begin(),wire.end(),casting::terminal(46,1))==0);
  check(std::count(wire.begin(),wire.end(),combat::detail::integer(21,1,2077))==0&&!particles::target_body_2022(f.target.actor,1719));
  if(mode==0){
   check(std::count(wire.begin(),wire.end(),casting::slot_event(228,1,{1719,0},8))==1);
   check(runtime.start(9499,{1719,0,f.target.actor},bar,p,ranks,true,h)==casting::Reason::recharge);
   check(runtime.start(9500,{1719,0,f.target.actor},bar,p,ranks,true,h)==casting::Reason::accepted);runtime.cancel(h);
  }
 }
 // Movement may observe a newer real clock after an earlier tick has been
 // sampled. Retain the arrow rather than cancel it or rewind that observation.
 {context=" future source movement";world::Instance w;std::uint64_t movement_now=100;w.configure_movement_clock([&]{return movement_now;});
  auto box=std::make_shared<world::Outbox>();w.join(1,"Archer",-940,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,30,30},100);
  w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,30,30},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.physical_armor(target,60));
  combat::weapon::Owned bow{12,1,{20,20,2},12,false,false,5};check(w.ordinary_weapon(source,bow,20));check(w.energy_capacity(1,1,0,100,{10,-2}));
  check(w.melee_attack(source,target,bow,*combat::weapon::attack_skill(1719,12,0),1000));movement_now=2000;w.move(1,-1300,0,1000);
  w.tick(1400);check(w.resources(2)->health==1000&&w.take_attack_outcomes(source).empty());
  w.tick(2000);check(w.resources(2)->health==958&&w.has_condition(target,combat::conditions::Kind::bleeding,2000));
  const auto events=w.take_attack_outcomes(source);check(events.size()==1&&events[0].at==2000);w.tick(2100);check(w.take_attack_outcomes(source).empty());
 }
 {context=" future recipient resources";Fixture f(false);auto& w=f.world;
  check(w.energy_capacity(2,1,0,2000,{10,-2}));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1719,12,0),1000));
  w.tick(1400);check(f.health()==1000&&w.take_attack_outcomes(f.source).empty());
  w.tick(2000);check(f.health()==958&&f.bleeding(2000));const auto events=w.take_attack_outcomes(f.source);check(events.size()==1&&events[0].at==2000);
 }
 std::cout<<"PASS Screaming Shot ranks/playerNPC/earshot boundary/contact movement/miss/block/lethal/immunity/expiry/Runtime cost and cancellation\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
