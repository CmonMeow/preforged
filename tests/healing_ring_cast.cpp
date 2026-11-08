#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool x,std::source_location at=std::source_location::current()){if(!x)throw std::runtime_error("Healing Ring cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;
 for(unsigned mode=0;mode<9;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Monk",0,0,box,0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{mode==5?1u:100u,500,30,30},100);w.set_vitals(2,{100,500,30,30},100);
  const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);check(w.energy_capacity(1,3,0,100,{10,-2}));
  if(mode==5)check(w.backfire(foe,owner,*effects::backfire(28,12),20,100));
  casting::Hooks h;unsigned commits=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy_at=[&](unsigned cost,auto at){check(cost==5);return w.consume_energy(1,cost,owner,at);};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[](unsigned id,float,bool self){return id==1&&self;};
  h.before_spell_effect=[&](unsigned skill,auto event,auto at){return w.before_spell_effect(owner,skill,event,at);};
  h.healing_ring=[&](auto scope,const auto& plan,auto at){++commits;check(plan.rank==12&&plan.divine_rank==12);return w.healing_ring(scope,plan,at);};
  if(mode==4)h.healing_ring={};if(mode==6)h.consume_energy_at={};if(mode==7)h.before_spell_effect={};if(mode==8)h.target_scope={};
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[1262]=1;Slots bar{};bar[0]={1262,0};attributes::Ranks ranks{};ranks[13]=12;ranks[16]=12;casting::Runtime cast(1);
  const bool missing=mode==4||mode>=6;
  check(cast.start(100,{1262,0,2},bar,policy,ranks,true,h)==(missing?casting::Reason::unsupported:casting::Reason::accepted));
  if(missing){check(w.resources(1)->energy==30);continue;}
  check(w.resources(1)->energy==25);cast.update(1099,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1099,h);
  if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1099,1);w.set_vitals(1,{100,500,25,30},100);}
  box->drain();cast.update(1100,h);check(!cast.active()&&commits==(mode==0?1u:0u));
  if(mode==0){check(w.resources(1)->health==138&&w.resources(2)->health==250);
   const auto effects=box->drain();const auto body=casting::generic(21,1,499);check(std::count(effects.begin(),effects.end(),body)==1&&std::count(sent.begin(),sent.end(),body)==0);
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{1262,0},10))==1);
   check(cast.start(11099,{1262,0,2},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }else check(w.resources(2)->health==100);
 }
 std::cout<<"PASS Healing Ring1s/5Energy/10s cast, self targeting, Divine Favor, single body499, cancel/interrupt/retired/Backfire and missing-hook rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
