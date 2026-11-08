#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Wild Blow World line "+std::to_string(at.line()));}
bool critical_notice(const std::vector<Packet>& packets){return std::any_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162&&p[1]==0&&p[2]==17&&p[3]==0&&p[4]==0&&p[5]==0;});}
int main()try{
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={345,0};bar[1]={340,0};check(w.configure_adrenaline(f.target,bar,100));
  const auto skill=mode==2?371u:345u;check(w.status_mutation(f.target,skill,123,mode==2?0u:15u,100,mode==2?8100:5100,true));
  w.configure_attack_block_draws([]{return 0.;});if(mode==1)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  auto gear=f.weapon;if(mode==3){gear.item_type=35;gear.revision=2;check(w.ordinary_weapon(f.source,gear,20,100));}
  f.box->drain();check(w.melee_attack(f.source,f.target,gear,*combat::weapon::attack_skill(321,0),mode==4?5100:300)==(mode!=3));
  check(critical_notice(f.box->drain())==(mode==0||mode==2||mode==4));
  if(mode==1||mode==3){check(f.health()==1000&&w.status_snapshot(f.target,300)->front().skill==skill);}
  else {check(w.status_snapshot(f.target,mode==4?5100:300)->empty());check(mode==2?f.health()==980:f.health()<980);}
 }
 for(unsigned type:{2u,15u,27u}){warrior_test::Fixture f(340,false);auto gear=f.weapon;gear.item_type=type;gear.revision=2;check(f.world.ordinary_weapon(f.source,gear,20,100));
  check(f.world.melee_attack(f.source,f.target,gear,*combat::weapon::attack_skill(321,0),300));check(f.health()<980);
 }
 for(bool blind:{false,true}){warrior_test::Fixture f(340,true);auto& w=f.world;check(w.npc_interrupt_stance(f.target,16,0,1000,100));
  if(blind)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));f.box->drain();
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(321,0),300));
  const auto wire=f.box->drain();const Packet removed{67,0,100,0,0,0,1,0,0,0};
  check(critical_notice(wire)==!blind);check(std::count(wire.begin(),wire.end(),removed)==unsigned(!blind));check(blind?f.health()==1000:f.health()<980);
 }
 for(bool npc:{false,true}){warrior_test::Fixture f(340,npc);auto& w=f.world;Slots bar{};bar[0]={33,0};bar[1]={321,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,33,123,12,100,30100,true));
  if(npc)check(w.npc_interrupt_stance(f.target,16,0,1000,100));else check(w.status_mutation(f.target,453,124,12,100,10100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(321,0),300));
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes[0].outcome==combat::weapon::AttackOutcome::replaced);
  check(f.health()==966);if(npc)check(w.end_stance(f.target,1,301));else check(w.status_snapshot(f.target,300)->front().skill==453);
 }
 for(bool npc:{false,true}){warrior_test::Fixture f(352,npc);auto& w=f.world;check(w.status_mutation(f.source,375,123,0,100,5100,true));
  if(npc){check(w.npc_interrupt_stance(f.target,16,0,1000,100));}
  else {check(w.status_mutation(f.target,16,124,0,100,1100,true));check(w.cast_activity(f.target,281,100,3000));}
  if(npc){const auto action=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,100,3000,Slot{281,0});check(action.has_value());
   check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(321,0),300));check(w.complete_npc_action(*action,3000));}
  else {check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(321,0),300));check(w.casting_skill(f.target,300)==281&&w.status_snapshot(f.target,300)->empty());}
 }
 for(unsigned mode=0;mode<5;++mode){warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={321,0};bar[1]={335,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200));check(w.adrenaline(f.source)->front().units==25);
  const auto original=f.health();casting::Hooks h;unsigned commits=0;std::vector<Packet> packets;
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto n,auto at){return mode!=3&&w.consume_energy(1,n,f.source,at);};h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.weapon_snapshot=[&](){return std::optional(f.weapon);};h.melee_target=[&](auto id,auto at){return w.melee_target(f.source,id,at,321);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){++commits;return w.melee_attack(f.source,target,weapon,plan,at);};
  h.attack_activity=[&](auto until){w.attack_activity(f.source,until,1000);};h.attack_interrupt_epoch=[&](){return w.attack_interrupt_epoch(f.source);};h.weapon_attack_timing=[&](auto at){return w.weapon_attack_timing(f.source,at);};
  if(mode!=4)h.begin_action=[&](auto slot,auto at){check(w.resources(1)->energy==25);check(w.accepted_skill_start(f.source,slot,at));};
  Policy p;p.primary=1;p.campaigns=255;p.learned[321]=p.learned[335]=1;attributes::Ranks ranks{};casting::Runtime runtime(1);
  const auto result=runtime.start(1000,{321,0,2},bar,p,ranks,true,h);check((result==casting::Reason::accepted)==(mode<3));
  if(mode>=3){check(w.adrenaline(f.source)->front().units==25&&w.resources(1)->energy==30);continue;}
  check(w.adrenaline(f.source)->front().units==0&&w.resources(1)->energy==25);
  check(std::count(packets.begin(),packets.end(),casting::activation(1,2,321,true))==1);
  check(std::count(packets.begin(),packets.end(),casting::generic(21,1,2077))==0);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1200,h);runtime.update(1499,h);check(!commits);runtime.update(1500,h);
  check(commits==unsigned(mode==0));check(mode==0?f.health()<original: f.health()==original);
  if(mode==0)check(runtime.start(1501,{321,0,2},bar,p,ranks,true,h)!=casting::Reason::accepted);
 }
 std::cout<<"PASS Wild Blow unblockable critical/immunity,Blind/expired/unsupported scythe,player/NPC exact stance end and Runtime payment/adrenaline/cancel/interrupt/recharge/required callback\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
