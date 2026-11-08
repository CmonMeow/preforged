#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("hammer control cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;using enum combat::conditions::Kind;
 for(unsigned skill:{354u,355u,357u,358u,359u,360u})for(bool npc:{false,true})for(unsigned mode=0;mode<=15;++mode){
  if((mode>=12&&skill!=359)||(npc&&mode==8))continue;
  warrior_test::Fixture f(skill,npc);const auto* row=find_skill(skill);Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};bar[1]={336,0};
  if(mode==14)bar[2]={1043,0};check(f.world.configure_adrenaline(f.source,bar,100));
  if(mode!=5)for(unsigned n=0;n<10;++n)check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(322,0),101+n));
  if(skill==359&&mode!=6)check(f.condition(weakness,1000,mode==7?150:5000));
  auto attacking=[&](std::uint64_t at,std::uint64_t until){
   return npc?bool(f.world.begin_npc_action(f.target,combat::npcs::control::Action::attack,at,until)):f.world.attack_activity(f.target,until,at);
  };
  if(skill==357&&mode!=6)check(attacking(1000,mode==7?1150:6000));
  std::uint64_t clock=1000;unsigned commits=0;std::string phase;std::vector<world::Packet> events;combat::weapon::AttackPermit saved;
  casting::Hooks h;
  h.resources=[&](unsigned id){return f.world.resources(id);};h.events=[&](const auto& packets){events.insert(events.end(),packets.begin(),packets.end());};
  h.diagnostic=[&](unsigned,const char* value){phase=value;};
  h.consume_energy_at=[&](unsigned cost,auto at){check(cost==0||cost==5);return f.world.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){return f.world.effect_scope(id);};
  h.weapon_snapshot=[&](){auto weapon=f.weapon;if(mode==4)weapon.item_type=27;return std::optional{weapon};};
  h.melee_target=[&](unsigned id,auto at){return f.world.melee_target(f.source,id,at,skill);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){++commits;return f.world.melee_attack(f.source,target,weapon,plan,at);};
  h.prepare_melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto at){saved=f.world.prepare_melee_attack(f.source,target,weapon,plan,at);return saved;};
  h.prepared_melee_attack=[&](auto target,const auto& weapon,const auto& plan,const auto& permit,auto at){++commits;return f.world.melee_attack(f.source,target,weapon,plan,at,permit);};
  h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,clock);};
  h.attack_interrupt_epoch=[&](){return f.world.attack_interrupt_epoch(f.source);};
  h.consume_adrenaline=[&](auto slot,auto at){return f.world.consume_adrenaline(f.source,slot,at);};
  h.reconcile_adrenaline=[&](auto at){f.world.reconcile_adrenaline(f.source,at);};
  h.weapon_attack_timing=[&](auto at){return f.world.weapon_attack_timing(f.source,at);};
  h.attack_duration=[&](auto base,auto at){return f.world.attack_duration(f.source,base,at);};
  h.target_valid=[&](unsigned id,float,bool self){return self&&id==1;};
  h.speed_multiplier=[&](double value){return f.world.movement_multiplier(1,float(value),clock);};
  h.status_mutation=[&](auto id,auto effect,auto rank,auto at,auto until,bool active){return f.world.status_mutation(f.source,id,effect,rank,at,until,active);};
  if(mode==3)h.melee_attack={};if(mode==12)h.prepare_melee_attack={};if(mode==13)h.prepared_melee_attack={};
  Policy policy;policy.primary=1;policy.secondary=mode==14?7:0;policy.campaigns=255;policy.learned[skill]=policy.learned[336]=1;if(mode==14)policy.learned[1043]=1;
  attributes::Ranks ranks{};ranks[19]=12;ranks[17]=mode==11?8:0;
  std::optional<casting::Runtime> cast;cast.emplace(1);
  const auto admitted=cast->start(clock,{skill,0,f.target.actor},bar,policy,ranks,true,h);
  const auto expected=mode==3||mode==12||mode==13?casting::Reason::unsupported:mode==4?casting::Reason::weapon:mode==5?casting::Reason::adrenaline:casting::Reason::accepted;
  if(admitted!=expected)std::cerr<<"skill="<<skill<<" npc="<<npc<<" mode="<<mode<<" admission="<<casting::reason_text(admitted)<<'\n';check(admitted==expected);
  if(expected!=casting::Reason::accepted){check(!cast->active()&&!commits);if(saved)check(!saved->open());continue;}
  if(skill==359)check(f.world.adrenaline(f.source)->at(0).units==0&&f.world.adrenaline(f.source)->at(1).units==0);
  check(std::count(events.begin(),events.end(),casting::activation(1,f.target.actor,skill,true))==1);
  if(mode==15){cast.reset();check(saved&&!saved->open());check(!f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(359,12),1500,saved));continue;}
  clock=1200;
  if(mode==6&&skill==359)check(f.condition(weakness,clock,5000));
  if(mode==6&&skill==357)check(attacking(clock,6000));
  if(mode==8){f.world.leave(2);f.world.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},clock,2);f.world.set_vitals(2,{1000,1000,30,30},100);check(f.world.physical_armor(*f.world.effect_scope(2),60));}
  if(mode==9)check(f.world.kill(f.source,clock));
  if(mode==10)check(f.world.apply_condition(f.source,f.source,blind,clock,5000));
  if(mode==14){check(cast->start(clock,{1043,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);check(saved&&saved->open()&&cast->active());}
  const auto end=skill==360?2000u:1500u;
  clock=end-1;cast->update(clock,h);check(!commits);
  if(mode==1)cast->cancel(h);if(mode==2)cast->interrupt(clock,h);
  clock=end;f.world.tick(clock);const auto before=f.health();f.box->drain();cast->update(clock,h);check(!cast->active());
  const bool committed=mode!=1&&mode!=2&&mode!=8&&mode!=9;
  if(commits!=unsigned(committed))std::cerr<<"skill="<<skill<<" npc="<<npc<<" mode="<<mode<<" commits="<<commits<<" phase="<<phase<<'\n';check(commits==unsigned(committed));
  const bool hit=committed&&mode!=10;
  if(hit)check(f.health()<before);else check(f.health()==before);
  if(skill==359&&hit&&mode!=11)check(before-f.health()==(mode==6?20:44));
  const bool down=hit&&skill!=360&&!(mode==7&&(skill==357||skill==359));
  check(f.world.knocked_down(f.target,clock)==down);
  if(down){const auto until=clock+(skill==358&&mode==11?4000u:2000u);check(f.world.knocked_down(f.target,until-1)&&!f.world.knocked_down(f.target,until));}
  if(hit&&(skill==355||skill==360))check(f.world.has_condition(f.target,weakness,clock+16999));
  if(skill==355){const auto packets=f.box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(f.target.actor,1,355))==unsigned(hit));}
  check(std::count(events.begin(),events.end(),casting::generic(46,1,0))==unsigned(committed));
  check(std::count(events.begin(),events.end(),casting::generic(58,1,0))==0);
  if(saved)check(!saved->open());
 }
 std::cout<<"PASS five hammer Runtime/World paths,paid costs,all-pool Heavy spend,start/contact anomaly,knockdown timing,Staggering1s,Devastating effect,cancel/interrupt/recycle/death and instant stance/lease cleanup\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

