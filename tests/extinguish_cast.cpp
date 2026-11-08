#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Extinguish cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;using combat::conditions::Kind;
 for(unsigned mode=0;mode<13;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
  for(unsigned id:{2u,3u})w.join(id,"Other",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?1:2);
  for(unsigned id:{1u,2u,3u})w.set_vitals(id,{id==1&&mode==5?1u:100u,500,30,30},100);
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(2),foe=*w.effect_scope(3);
  if(mode!=12)check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
  check(w.apply_condition(foe,ally,Kind::burning,100,10000));
  check(w.energy_capacity(1,3,0,100,{10,-2}));
  if(mode==9)w.set_vitals(1,{100,500,14,30},100);
  if(mode==5)check(w.backfire(foe,owner,*effects::backfire(28,12),20,100));
  casting::Hooks h;unsigned commits=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy_at=[&](unsigned cost,auto at){check(cost==15);return w.consume_energy(1,cost,owner,at);};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[](unsigned id,float,bool self){return id==1&&self;};
  h.before_spell_effect=[&](unsigned skill,auto event,auto at){return w.before_spell_effect(owner,skill,event,at);};
  h.extinguish=[&](auto scope,const auto& plan,auto at){++commits;check(scope==owner&&plan.rank==12&&plan.divine_rank==12);return w.extinguish(scope,plan,at);};
  if(mode==4)h.extinguish={};if(mode==6)h.consume_energy_at={};if(mode==7)h.before_spell_effect={};if(mode==8)h.target_scope={};if(mode==10)h.target_valid={};
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[943]=1;Slots bar{};bar[0]={943,0};attributes::Ranks ranks{};ranks[15]=12;ranks[16]=12;casting::Runtime cast(1);
  const bool missing=mode==4||(mode>=6&&mode<=8)||mode==10;
  check(cast.start(100,{943,0,3},bar,policy,ranks,true,h)==(missing?casting::Reason::unsupported:mode==9?casting::Reason::energy:casting::Reason::accepted));
  if(missing||mode==9){check(w.resources(1)->energy==(mode==9?14u:30u)&&!cast.active()&&!commits);check(w.has_condition(ally,Kind::burning,100));check(std::count(sent.begin(),sent.end(),casting::generic(21,1,1014))==0);continue;}
  check(w.resources(1)->energy==15);cast.update(1099,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1099,h);
  if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1099,1);w.set_vitals(1,{100,500,15,30},100);}
  if(mode==11)check(w.party_action(2,160,0,1099));
  if(mode==12)check(w.party_action(1,158,2,1099)&&w.party_action(2,154,1,1099));
  const bool succeeds=mode==0||mode==11||mode==12;
  cast.update(1100,h);check(!cast.active()&&commits==(succeeds?1u:0u));
  if(succeeds){check(w.resources(1)->health==138);
   if(mode==11)check(w.resources(2)->health==100&&w.has_condition(ally,Kind::burning,1100));
   else check(w.resources(2)->health==168&&!w.has_condition(ally,Kind::burning,1100));
   check(std::count(sent.begin(),sent.end(),casting::generic(21,1,1014))==1);
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{943,0},12))==1);
   check(cast.start(13099,{943,0,3},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }else {check(w.has_condition(ally,Kind::burning,1100));check(std::count(sent.begin(),sent.end(),casting::generic(21,1,1014))==0);}
 }
 std::cout<<"PASS Extinguish1s/15Energy/12s scoped party cleanse,Divine Favor,native body1014,cancel/interrupt/retired/Backfire and missing-hook rejection;duplicate effect provenance pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
