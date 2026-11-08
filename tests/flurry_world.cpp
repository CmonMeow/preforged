#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Flurry line "+std::to_string(at.line()));}
int main()try{
 check(flurry_plan());
 for(bool npc:{false,true})for(unsigned mode=0;mode<4;++mode){
  warrior_test::Fixture f(383,npc),baseline(383,npc);
  check(!f.world.status_mutation(f.source,344,122,0,100,5099,true));
  check(f.world.status_mutation(f.source,344,123,0,100,5100,true));
  if(mode==1)check(f.world.status_mutation(f.source,349,124,0,200,8200,true));
  if(mode==2)check(f.world.status_mutation(f.source,344,123,0,200,{},false));
  const auto now=mode==3?5100:1000;
  check(f.world.attack_duration(f.source,1000,now)==(mode==0?670:1000));
  const auto plan=*combat::weapon::attack_skill(383,12);
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,now));
  check(baseline.world.melee_attack(baseline.source,baseline.target,baseline.weapon,plan,now));
  check(std::abs(f.health()-baseline.health()-(mode==0?5.:0.))<.001);
 }
 {const Entry entries[]={{10,1,344,0,Category::stance,100,5100},{11,2,135,12,Category::hex,100,8100}};
 const auto timing=reviewed_attack_timing({500,1000,128},entries,1000);
 check(timing&&timing->windup_ms==503&&timing->period_ms==1005);
 check(reviewed_attack_timing({500,1000,128},entries,5100)->period_ms==1500);}
 combat::weapon::Strike s;s.level=20;s.mastery=12;s.minimum=s.maximum=20;s.location_armor=60;s.base_roll=s.miss_roll=.5;
 auto a=*combat::weapon::resolve(s);s.weapon_base_multiplier=.75;check(combat::weapon::resolve(s)->damage==a.damage*.75);
 s.weapon_base_multiplier=.5;check(!combat::weapon::resolve(s));
 {world::Instance w;w.join(1,"Player",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);auto owner=*w.effect_scope(1);casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](unsigned n){check(n==5);return w.consume_energy(1,n,owner);};h.effect_snapshot=[&](auto now){return *w.status_snapshot(owner,now);};h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return w.status_mutation(owner,skill,effect,rank,now,until,active);};Policy p;p.primary=1;p.campaigns=255;p.learned[344]=1;Slots bar{};bar[0]={344,0};attributes::Ranks ranks{};casting::Runtime runtime(1);check(runtime.start(1000,{344,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);runtime.update(1000,h);check(w.resources(1)->energy==95&&w.attack_duration(owner,1000,1000)==670);check(w.kill(owner,1100));check(w.status_snapshot(owner,1100)->empty());}
 std::cout<<"PASS Flurry player/NPC base-only damage, timing, replacement/removal/expiry, canonical duration, Runtime cost/self-target and death\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
