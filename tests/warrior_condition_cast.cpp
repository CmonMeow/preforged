#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("warrior condition cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;
 for(unsigned skill:{320u,334u,342u,352u,849u,850u,1416u})for(bool npc:{false,true})for(unsigned mode=0;mode<10;++mode){
  if(npc&&mode==8)continue;
  warrior_test::Fixture f(skill,npc);const auto* row=find_skill(skill);Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};
  check(f.world.configure_adrenaline(f.source,bar,100));
  if(row->adrenaline&&mode!=5){
   for(unsigned n=0;n<6;++n)check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(322,0),101+n));
   check(f.world.adrenaline(f.source)->at(0).units>=row->adrenaline);
  }
  if(mode!=6&&mode!=7)check(f.prerequisite(skill,true,1000));
  // Mode7 specifically removes a condition before release. Knockdown/stance
  // boundary cases are covered by the World suite, not fabricated here.
  if(mode==7&&(skill==334||skill==342||skill==850))check(f.prerequisite(skill,true,1000,50));
  if(mode==5&&!row->adrenaline)f.world.set_vitals(1,{1000,1000,row->energy_cost()-1,30},100);
  std::uint64_t clock=1000;unsigned commits=0;std::vector<world::Packet> events;std::string phase;
  casting::Hooks h;
  h.resources=[&](unsigned id){return f.world.resources(id);};h.events=[&](const auto& packets){events.insert(events.end(),packets.begin(),packets.end());};
  h.diagnostic=[&](unsigned,const char* value){phase=value;};
  h.consume_energy_at=[&](unsigned cost,auto at){check(cost==row->energy_cost());return f.world.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){return f.world.effect_scope(id);};
  h.weapon_snapshot=[&](){auto gear=f.weapon;if(mode==4)gear.item_type=5;return std::optional{gear};};
  h.melee_target=[&](unsigned id,auto at){return f.world.melee_target(f.source,id,at,skill);};
  h.melee_attack=[&](auto target,const auto& gear,const auto& plan,auto at){++commits;check(plan.skill==skill&&plan.rank==12&&plan.strength==0);return f.world.melee_attack(f.source,target,gear,plan,at);};
  h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,clock);};
  h.consume_adrenaline=[&](auto slot,auto at){return f.world.consume_adrenaline(f.source,slot,at);};
  h.reconcile_adrenaline=[&](auto at){f.world.reconcile_adrenaline(f.source,at);};
  h.weapon_attack_timing=[&](auto at){return f.world.weapon_attack_timing(f.source,at);};
  if(mode==3)h.melee_attack={};
  Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[skill]=1;attributes::Ranks ranks{};ranks[row->attribute]=12;
  casting::Runtime cast(1);const auto admitted=cast.start(clock,{skill,0,f.target.actor},bar,policy,ranks,true,h);
  check(admitted==(mode==3?casting::Reason::unsupported:mode==4?casting::Reason::weapon:mode==5?(row->adrenaline?casting::Reason::adrenaline:casting::Reason::energy):casting::Reason::accepted));
  if(mode>=3&&mode<=5){check(!cast.active()&&!commits);continue;}
  check(std::count(events.begin(),events.end(),casting::activation(1,f.target.actor,skill,true))==1);
  clock=1200;
  if(mode==6)check(f.prerequisite(skill,true,clock));
  if(mode==8){f.world.leave(2);f.world.join(2,"Replacement",60,0,std::make_shared<world::Outbox>(),0,{}, {},clock,2);f.world.set_vitals(2,{1000,1000,30,30},100);check(f.world.physical_armor(*f.world.effect_scope(2),60));}
  if(mode==9)check(f.world.kill(f.source,clock));
  clock=1499;cast.update(clock,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(clock,h);
  // Settle expiry first: losing Deep Wound restores Health capacity/current
  // proportionally, so pre-expiry Health is not a valid damage baseline.
  clock=1500;f.world.tick(clock);const auto before=f.health();cast.update(clock,h);check(!cast.active());
  const bool hit=mode==0||mode==6||mode==7;
  if(commits!=unsigned(hit))std::cerr<<"skill="<<skill<<" npc="<<npc<<" mode="<<mode<<" commits="<<commits<<" phase="<<phase<<'\n';
  if((hit&&f.health()>=before)||(!hit&&f.health()!=before))std::cerr<<"skill="<<skill<<" npc="<<npc<<" mode="<<mode<<" before="<<before<<" after="<<f.health()<<" phase="<<phase<<'\n';
  check(commits==unsigned(hit));if(hit)check(f.health()<before);else check(f.health()==before);
  const auto kind=skill==320||skill==334?combat::conditions::Kind::crippled:skill==342?combat::conditions::Kind::weakness:skill==849||skill==1416?combat::conditions::Kind::bleeding:combat::conditions::Kind::deep_wound;
  const bool conditioned=hit&&(mode!=7||skill==320||skill==1416);
  check(f.world.has_condition(f.target,kind,clock)==conditioned);
  check(std::count(events.begin(),events.end(),casting::generic(46,1,0))==unsigned(hit));
  check(std::count(events.begin(),events.end(),casting::generic(58,1,0))==0);
  if(hit)check(std::count(events.begin(),events.end(),casting::slot_event(228,1,{static_cast<std::uint16_t>(skill),0},row->recharge))==1);
 }
 std::cout<<"PASS seven warrior attacks through actual Runtime/player-NPC World:paid Energy/adrenaline,weapon animation,500ms contact,dynamic conditions,cancel,interrupt,recycled target and death guards\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
