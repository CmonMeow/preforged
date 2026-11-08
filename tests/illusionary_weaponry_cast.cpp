#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("IW cast line "+std::to_string(at.line()));
}
int main()try{using namespace skills;
 for(unsigned mode=0;mode<7;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{mode==5?1u:500u,500,50,100},100);w.set_vitals(2,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);check(w.energy_capacity(1,5,0,100,{80,-2}));
  Slots bar{};bar[0]={33,0};bar[1]={11,0};check(w.configure_adrenaline(owner,bar,100));
  if(mode==5)check(w.backfire(foe,owner,*effects::backfire(28,12),20,100));
  casting::Hooks h;std::vector<combat::Packet> sent;
  h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](auto id){return w.resources(id);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.target_valid=[](unsigned id,float range,bool self){return id==1&&self&&range==1248;};
  h.consume_energy=[&](unsigned cost){check(cost==5);return w.consume_energy(1,cost,owner);};
  h.before_spell_effect=[&](unsigned skill,auto event,auto now){return w.before_spell_effect(owner,skill,event,now);};
  h.effect_snapshot=[&](auto now){return w.status_snapshot(owner,now).value_or(std::vector<effects::Entry>{});};
  h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto until,bool active){return w.status_mutation(owner,skill,effect,rank,now,until,active);};
  if(mode==4)h.status_mutation={};
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[33]=1;policy.learned[11]=1;policy.pvp_match=mode==3;attributes::Ranks ranks{};ranks[1]=12;
  casting::Runtime cast(1);check(cast.start(100,{33,0,2},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==4){check(w.resources(1)->energy==50);continue;}
  check(w.resources(1)->energy==45);cast.update(1099,h);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1099,h);
  if(mode==6){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1099,1);w.set_vitals(1,{500,500,45,100},100);}
  cast.update(1100,h);check(!cast.active());
  const auto status=w.status_snapshot(*w.effect_scope(1),1100);check(bool(status));
  const auto entry=std::find_if(status->begin(),status->end(),[](const auto& e){return e.skill==33||e.skill==3181;});
  const bool success=mode==0||mode==3;check((entry!=status->end())==success);
  if(success){check(entry->skill==(mode==3?3181u:33u)&&entry->rank==12&&entry->expires==31100);
   check(std::count(sent.begin(),sent.end(),casting::effect_add(1,33,12,entry->instance,30))==1);
   cast.update(31100,h);const auto expired=w.status_snapshot(owner,31100);check(expired&&std::none_of(expired->begin(),expired->end(),[](const auto& e){return e.skill==33||e.skill==3181;}));
  }
  if(mode==5)check(w.resources(1)->health==0);
  check(std::none_of(sent.begin(),sent.end(),[](const auto& packet){return packet==casting::generic(21,1,2077);}));
 }
 std::cout<<"PASS IW actual self cast,5Energy,1s completion,30s owned PvE/PvP enchant,cancel/interrupt/lifetime/Backfire and no fabricated body resource\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
