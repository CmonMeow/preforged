#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Lightning Reflexes line "+std::to_string(at.line()));}
int main()try{
 for(unsigned id:{453u,3141u})for(unsigned rank=0;rank<=20;++rank)check(lightning_reflexes(id,rank)==1000ull*interpolate(5,11,rank));
 for(unsigned mode=0;mode<8;++mode){
  warrior_test::Fixture f(383,false);auto& w=f.world;
  check(w.status_mutation(f.target,453,123,0,100,5100,true));
  check(w.attack_duration(f.target,1000,1000)==670);
  if(mode==2||mode==3)check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  if(mode==4)check(w.degeneration_hex(f.source,f.target,*degeneration_hex(137,0),100));
  if(mode==5)check(w.apply_condition(f.target,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==6)check(w.status_mutation(f.target,349,124,0,200,8200,true));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return mode==1?.75:mode==2?.874:mode==3?.875:0.;});
  const auto now=mode==7?5100:1000;
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(383,12),now));
  auto events=w.take_attack_outcomes(f.source);
check(events.size()==1);
  using Outcome=combat::weapon::AttackOutcome;
  check(events[0].outcome==(mode==0||mode==2?Outcome::block:mode==5?Outcome::miss:Outcome::hit));
  check(draws==(mode==4||mode==5||mode>=6?0u:1u));
 }
 for(bool pvp:{false,true}){
  world::Instance w;w.join(1,"Player",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);auto owner=*w.effect_scope(1);casting::Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](unsigned n){check(n==10);return w.consume_energy(1,n,owner);};h.effect_snapshot=[&](auto now){return *w.status_snapshot(owner,now);};h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return w.status_mutation(owner,skill,effect,rank,now,until,active);};Policy p;p.primary=2;p.pvp_match=pvp;p.campaigns=255;p.learned[453]=1;Slots bar{};bar[0]={453,0};attributes::Ranks ranks{};casting::Runtime runtime(1);
check(runtime.start(1000,{453,0,2},bar,p,ranks,true,h)==casting::Reason::accepted);
runtime.update(1000,h);
check(w.resources(1)->energy==90&&w.attack_duration(owner,1000,1000)==670);
  check(runtime.start(31000,{453,0,2},bar,p,ranks,true,h)==(pvp?casting::Reason::recharge:casting::Reason::accepted));
 }
 std::cout<<"PASS Lightning Reflexes player blocks,Guardian stacking,Rigor/Blind bypass,expiry/replacement,haste and PvE30s/PvP45s Runtime recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
