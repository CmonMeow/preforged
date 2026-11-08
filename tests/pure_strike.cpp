#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){
 if(!x)throw std::runtime_error("Pure Strike line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){
  const auto p=combat::weapon::attack_skill(328,rank,0);
  check(p&&p->required_type==27&&p->bonus==unsigned(std::lround(1.+29.*rank/15.)));
 }
 check(!combat::weapon::attack_skill(328,21));
 for(bool npc:{false,true})for(unsigned stance=0;stance<3;++stance)for(bool miss:{false,true}){
  warrior_test::Fixture f(328,npc);unsigned draws=0;
  if(!npc)check(f.world.apply_health_regeneration(f.target,2,*skills::regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  f.world.configure_attack_block_draws([&]{++draws;return 0.;});
  if(stance)check(f.world.status_mutation(f.source,346,10,0,200,stance==1?2000:500,true));
  if(miss)check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,300,2000));
  check(f.world.melee_target(f.source,f.target.actor,500,328));
  check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(328,12,0),500));
  const bool blocked=!npc&&stance==1&&!miss;
  check(f.health()==1000-(!miss&&!blocked?44:0));
  check(draws==unsigned(blocked));
  const auto outcomes=f.world.take_attack_outcomes(f.source);check(outcomes.size()==1);
  check(outcomes[0].outcome==(miss?combat::weapon::AttackOutcome::miss:blocked?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit));
 }
 for(unsigned mode=0;mode<8;++mode){
  using namespace skills;
  warrior_test::Fixture f(328,false);unsigned commits=0,draws=0;
  check(f.world.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));
  f.world.configure_attack_block_draws([&]{++draws;return 0.;});
  // 0:no stance;1:added during windup;2:expires at contact;3:removed
  // during windup;4:persists;5:Blind;6:cancel;7:interrupt.
  if(mode>=2&&mode<=4)check(f.world.status_mutation(f.source,349,10,0,900,mode==2?1500:3000,true));
  Slots bar{};bar[0]={328,0};Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[328]=1;
  attributes::Ranks ranks{};ranks[20]=12;
  std::uint64_t clock=1000;std::vector<world::Packet> packets;
  casting::Hooks h;
  h.resources=[&](unsigned id){return f.world.resources(id);};
  h.events=[&](const auto& values){packets.insert(packets.end(),values.begin(),values.end());};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.world.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){return f.world.effect_scope(id);};
  h.weapon_snapshot=[&](){return std::optional{f.weapon};};
  h.melee_target=[&](unsigned id,auto at){return f.world.melee_target(f.source,id,at,328);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){++commits;return f.world.melee_attack(f.source,target,weapon,plan,at);};
  h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,clock);};
  h.attack_interrupt_epoch=[&](){return f.world.attack_interrupt_epoch(f.source);};
  h.weapon_attack_timing=[&](auto at){return f.world.weapon_attack_timing(f.source,at);};
  h.attack_duration=[&](auto base,auto at){return f.world.attack_duration(f.source,base,at);};
  casting::Runtime cast(1);
  check(cast.start(clock,{328,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  check(f.world.resources(1)->energy==25&&!commits);
  check(std::count(packets.begin(),packets.end(),casting::activation(1,f.target.actor,328,true))==1);
  clock=1200;
  if(mode==1)check(f.world.status_mutation(f.source,349,10,0,clock,3000,true));
  if(mode==3)check(f.world.status_mutation(f.source,349,10,0,clock,{},false));
  if(mode==5)check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,clock,5000));
  if(mode==6)cast.cancel(h);
  if(mode==7)cast.interrupt(clock,h);
  clock=1499;cast.update(clock,h);check(!commits);
  clock=1500;cast.update(clock,h);check(!cast.active());
  check(commits==unsigned(mode<6));
  const bool hit=mode==0||mode==2||mode==3;
  check(f.health()==(hit?956:1000));check(draws==unsigned(mode==1||mode==4));
  const auto result=cast.start(1600,{328,0,f.target.actor},bar,policy,ranks,true,h);
  if(mode!=6)check(result==casting::Reason::recharge);
 }
 std::cout<<"PASS Pure Strike policy,World contact and real cast stance transitions,miss,cancel,interrupt and recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
