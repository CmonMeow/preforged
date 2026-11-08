#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){
 if(!x)throw std::runtime_error("Counterattack NPC line "+std::to_string(at.line()));
}
int main()try{
 using Action=combat::npcs::control::Action;
 // Contact-time activity, not selected target or an earlier attack request.
 for(unsigned mode=0;mode<5;++mode)for(bool miss:{false,true})for(unsigned rank:{0u,12u,20u}){
  warrior_test::Fixture f(1693,true);
  f.world.set_vitals(1,{1000,1000,0,30},100);
  const auto plan=*combat::weapon::attack_skill(1693,rank,0);
  if(mode){
   const auto action=f.world.begin_npc_action(f.target,mode==4?Action::spell:Action::attack,200,mode==2?300:mode==3?400:2000);
   check(bool(action));
   if(mode==3)check(f.world.complete_npc_action(*action,400));
  }
  if(miss)check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,400,1000));
  const auto before=f.health();
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,500));
  check(f.health()==before-(miss?0:20+plan.bonus));
  check(f.world.resources(1)->energy==(!miss&&mode==1?plan.refund:0));
 }
 // A killing hit still earns Energy; full Energy clamps without overflow.
 for(unsigned energy:{0u,29u,30u}){
  warrior_test::Fixture f(1693,true,1);f.world.set_vitals(1,{1000,1000,energy,30},100);
  check(bool(f.world.begin_npc_action(f.target,Action::attack,200,2000)));
  check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1693,12,0),500));
  check(f.health()==0&&f.world.resources(1)->energy==std::min(30u,energy+5));
  check(f.world.take_npc_deaths(f.source).size()==1);
 }
 // Real casting dispatcher: charge before impact, qualify at contact, and
 // never refund a cancelled strike or a target that merely attacked earlier.
 for(unsigned mode=0;mode<6;++mode){
  using namespace skills;
  warrior_test::Fixture f(1693,true);
  f.world.set_vitals(1,{1000,1000,10,30},100);
  Slots bar{};bar[0]={1693,0};Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[1693]=1;
  attributes::Ranks ranks{};ranks[17]=12;
  std::uint64_t clock=1000;unsigned commits=0;std::vector<world::Packet> packets;
  casting::Hooks h;
  h.resources=[&](unsigned id){return f.world.resources(id);};
  h.events=[&](const auto& values){packets.insert(packets.end(),values.begin(),values.end());};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.world.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){return f.world.effect_scope(id);};
  h.weapon_snapshot=[&](){return std::optional{f.weapon};};
  h.melee_target=[&](unsigned id,auto at){return f.world.melee_target(f.source,id,at,1693);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){++commits;return f.world.melee_attack(f.source,target,weapon,plan,at);};
  h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,clock);};
  h.attack_interrupt_epoch=[&](){return f.world.attack_interrupt_epoch(f.source);};
  h.weapon_attack_timing=[&](auto at){return f.world.weapon_attack_timing(f.source,at);};
  h.attack_duration=[&](auto base,auto at){return f.world.attack_duration(f.source,base,at);};
  if(mode==2)check(bool(f.world.begin_npc_action(f.target,Action::attack,900,1200)));
  casting::Runtime cast(1);
  check(cast.start(clock,{1693,0,f.target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  check(f.world.resources(1)->energy==5&&!commits);
  check(std::count(packets.begin(),packets.end(),casting::activation(1,f.target.actor,1693,true))==1);
  clock=1200;
  if(mode==1||mode>=3)check(bool(f.world.begin_npc_action(f.target,Action::attack,clock,2500)));
  if(mode==3)cast.cancel(h);
  if(mode==4)cast.interrupt(clock,h);
  if(mode==5)check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,clock,5000));
  clock=1499;cast.update(clock,h);check(!commits);
  clock=1500;cast.update(clock,h);
  check(!cast.active()&&commits==unsigned(mode!=3&&mode!=4));
  check(f.world.resources(1)->energy==(mode==1?10u:5u));
  if(mode==3||mode==4||mode==5)check(f.health()==1000);
  else check(f.health()<1000); // Real Strength rank also penetrates weapon armor.
 }
 std::cout<<"PASS Counterattack NPC contact activity,expiry,completion,spell exclusion,miss,lethal,Energy cap and real cast lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
