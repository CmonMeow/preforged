#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Guardian casting regression line "+std::to_string(at.line()));
}
int main()try{using namespace skills;using namespace casting;
 for(unsigned mode=0;mode<9;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
  w.join(2,"Ally",60,0,box,0,{}, {},100,1);w.join(3,"Enemy",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  for(unsigned id:{1u,2u,3u})w.set_vitals(id,{100,1000,40,100},100);
  const auto source=*w.effect_scope(1);Hooks hooks;unsigned commits=0;std::vector<world::Packet> sent;
  hooks.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};
  hooks.resources=[&](unsigned id){return w.resources(id);};
  hooks.consume_energy=[&](unsigned amount){return w.consume_energy(1,amount,source);};
  hooks.target_valid=[](unsigned id,float range,bool self){return id==2&&range==1248&&self;};
  hooks.target_scope=[&](unsigned id){return w.effect_scope(id);};
  hooks.control_epoch=[&]{return w.control_epoch(source);};
  hooks.protective_enchantment=[&](auto target,const auto& plan,unsigned divine,auto now){
   ++commits;check(plan.skill==258&&plan.rank==12&&plan.duration==6000&&divine==12);
   return w.protective_enchantment(source,target,plan,divine,now);
  };
  if(mode==4)hooks.protective_enchantment={};
  if(mode==7)hooks.target_scope={};
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[258]=1;
  Slots slots{};slots[0]={258,0};attributes::Ranks ranks{};ranks[15]=12;ranks[16]=12;Runtime cast(1);
  check(cast.start(100,{258,0,2},slots,policy,ranks,true,hooks)==(mode==4||mode==7?casting::Reason::unsupported:casting::Reason::accepted));
  check(w.resources(1)->energy==(mode==4||mode==7?40u:35u));
  if(mode==4||mode==7)continue;
  cast.update(1099,hooks);check(!commits&&w.resources(2)->health==100);
  if(mode==1)cast.cancel(hooks);
  if(mode==2)cast.interrupt(200,hooks);
  if(mode==3){w.leave(2);w.join(2,"Replacement",60,0,box,0,{}, {},200,1);w.set_vitals(2,{100,1000,40,100},100);}
  if(mode==5)check(w.kill(source,200));
  if(mode==6)check(w.kill(*w.effect_scope(2),200));
  if(mode==8){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{world::Packet{165,0,1,0,0,0,3,0}}, {},200,1);w.set_vitals(1,{100,1000,40,100},100);}
  box->drain();cast.update(1100,hooks);cast.update(1101,hooks);
  check(w.resources(2)->health==(mode==0?138u:mode==6?0u:100u));
  if(commits!=(mode==0?1u:0u))throw std::runtime_error("Guardian callback count mode="+std::to_string(mode)+" actual="+std::to_string(commits));
  if(mode==0){
   const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::target_body_2022(2,258))==1);
   check(std::count(sent.begin(),sent.end(),slot_event(228,1,{258,0},4))==1);
   check(cast.start(2000,{258,0,2},slots,policy,ranks,true,hooks)==casting::Reason::recharge);
   const auto enemy=*w.effect_scope(3),target=*w.effect_scope(2);check(w.physical_armor(target,60));
   combat::weapon::Owned sword{1,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(enemy,sword,20));
   w.configure_attack_block_draws([]{return 0.;});check(w.melee_attack(enemy,target,sword,*combat::weapon::attack_skill(322,0),2100));check(w.resources(2)->health==138);
   const auto outcomes=w.take_attack_outcomes(enemy);check(outcomes.size()==1&&outcomes[0].outcome==combat::weapon::AttackOutcome::block);
  }
  if(mode==1)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{258,0},4))==0);
  if(mode==2)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{258,0},4))==1);
 }
 std::cout<<"PASS Guardian actual casting5Energy/1second/4recharge,DivineFavor,kind21body,block,cancel/interrupt and actor lifetime guards\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

