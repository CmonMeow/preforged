#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Rigor casting regression line "+std::to_string(at.line()));
}
int main()try{using namespace skills;using namespace casting;
 for(unsigned mode=0;mode<8;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.join(2,"Defender",60,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto source=*w.effect_scope(1);Hooks hooks;unsigned commits=0;std::vector<world::Packet> sent;
  hooks.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};
  hooks.resources=[&](unsigned id){return w.resources(id);};hooks.consume_energy=[&](unsigned amount){return w.consume_energy(1,amount,source);};
  hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};hooks.control_epoch=[&]{return w.control_epoch(source);};
  hooks.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};
  hooks.degeneration_hex=[&](auto target,const auto& plan,auto now){++commits;check(plan.skill==137&&plan.rank==12&&plan.duration==18000&&!plan.pips);return w.degeneration_hex(source,target,plan,now);};
  if(mode==4)hooks.degeneration_hex={};
  Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[137]=1;Slots slots{};slots[0]={137,0};attributes::Ranks ranks{};ranks[7]=12;Runtime cast(1);
  check(cast.start(100,{137,0,2},slots,policy,ranks,true,hooks)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(w.resources(1)->energy==(mode==4?100u:90u));if(mode==4)continue;
  cast.update(199,hooks);check(!commits);
  if(mode==1)cast.cancel(hooks);if(mode==2)cast.interrupt(200,hooks);
  if(mode==3){w.leave(2);w.join(2,"Replacement",60,0,box,0,{}, {},200,2);w.set_vitals(2,{500,500,100,100},100);}
  if(mode==5)check(w.kill(source,200));if(mode==6)check(w.kill(*w.effect_scope(2),200));
  if(mode==7){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(1,{500,500,100,100},100);}
  cast.update(1099,hooks);check(!commits);box->drain();cast.update(1100,hooks);cast.update(1101,hooks);
  check(commits==(mode==0?1u:0u));
  const auto status=w.status_snapshot(*w.effect_scope(2),1101);check(bool(status));
  check(std::count_if(status->begin(),status->end(),[](const auto& entry){return entry.skill==137;})==(mode==0?1:0));
  if(mode==0){
   const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,137))==1);
   check(std::count(sent.begin(),sent.end(),slot_event(228,1,{137,0},20))==1);
   check(cast.start(2000,{137,0,2},slots,policy,ranks,true,hooks)==casting::Reason::recharge);
   const auto target=*w.effect_scope(2);const auto guardian=*regeneration::reviewed(258,12);
   check(w.apply_health_regeneration(target,2,guardian,1200,7200,1248,true,target)&&w.physical_armor(target,60));
   combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(source,sword,20));
   unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});
   check(w.melee_attack(source,target,sword,*combat::weapon::attack_skill(322,0),1300));check(w.resources(2)->health==470&&!draws);
  }
  if(mode==1)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{137,0},20))==0);
  if(mode==2)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{137,0},20))==1);
 }
 std::cout<<"PASS Rigor actual casting10Energy/1second/20recharge,target248,Guardian bypass,cancel/interrupt and original actor lifetime\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
