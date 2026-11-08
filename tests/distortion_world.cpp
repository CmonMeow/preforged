#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Distortion World line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<8;++mode){
  warrior_test::Fixture f(383,false);auto& w=f.world;
  check(w.energy_capacity(2,1,0,100,{10,-2})); // Explicit zero regeneration.
  const auto energy=mode==1?1u:mode==2?2u:30u;
  w.set_vitals(2,{1000,1000,energy,40},100);
  check(w.status_mutation(f.target,11,123,12,100,4100,true));
  if(mode==3)check(w.apply_health_regeneration(f.target,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  if(mode==4)check(w.degeneration_hex(f.source,f.target,*skills::effects::degeneration_hex(137,0),100));
  if(mode==5)check(w.apply_condition(f.target,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==6)check(w.status_mutation(f.target,349,124,0,200,8200,true));
  w.configure_attack_block_draws([]{return 0.;});
  const auto now=mode==7?4100:1000;
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(383,12),now));
  const auto events=w.take_attack_outcomes(f.source);check(events.size()==1);
  check(events[0].outcome==(mode<=3?combat::weapon::AttackOutcome::block:mode==5?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::hit));
  check(w.resources(2)->energy==(mode==0?28u:mode==2?0u:energy));
  const auto active=w.status_snapshot(f.target,now);check(bool(active));
  const bool present=std::any_of(active->begin(),active->end(),[](const auto& e){return e.skill==11;});
  check(present==(mode!=1&&mode!=6&&mode!=7));
 }
 {world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,30,40},100);
  const auto owner=*w.effect_scope(1);skills::casting::Hooks h;std::vector<combat::Packet> packets;
  h.events=[&](const auto& p){packets.insert(packets.end(),p.begin(),p.end());};h.resources=[&](auto id){return w.resources(id);};
  h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy=[&](unsigned n){return w.consume_energy(1,n,owner);};
  h.effect_snapshot=[&](auto now){return *w.status_snapshot(owner,now);};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return w.status_mutation(owner,skill,effect,rank,now,until,active);};
  skills::Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[11]=1;skills::Slots slots{};slots[0]={11,0};attributes::Ranks ranks{};
  skills::casting::Runtime runtime(1);check(runtime.start(1000,{11,0,2},slots,policy,ranks,true,h)==skills::casting::Reason::accepted);runtime.update(1000,h);
  check(w.resources(1)->energy==25);const auto active=w.status_snapshot(owner,1000);check(active&&active->size()==1&&active->front().skill==11);
  const auto body=skills::casting::success_body_effect(1,11);check(body&&std::find(packets.begin(),packets.end(),*body)!=packets.end());
 }
 std::cout<<"PASS Distortion actual stance cast,Energy,attributed blocks,insufficient end,Guardian,Rigor,blind,replacement,expiry and captured body\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
