#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Recall cast line "+std::to_string(at.line()));}
int main()try{using namespace skills;
 for(unsigned mode=0;mode<6;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Mesmer",0,0,box,0,{}, {},100,1);w.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(1,{mode==5?1u:500u,500,50,100},100);w.set_vitals(2,{500,500,100,100},100);const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);check(w.energy_capacity(1,5,0,100,{80,-2}));
  if(mode==5)check(w.backfire(foe,owner,*effects::backfire(28,12),20,100));
  casting::Hooks h;unsigned commits=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy=[&](unsigned cost){check(cost==10);return w.consume_energy(1,cost,owner);};h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[](unsigned id,float range,bool self){return id==1&&range==1248&&self;};
  h.before_spell_effect=[&](unsigned skill,auto event,auto now){return w.before_spell_effect(owner,skill,event,now);};
  h.mantra_recall=[&](auto selected,unsigned rank,auto now){++commits;return w.mantra_recall(selected,rank,now);};
  if(mode==4)h.mantra_recall={};Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[82]=1;Slots bar{};bar[0]={82,0};attributes::Ranks ranks{};ranks[3]=12;casting::Runtime cast(1);
  check(cast.start(100,{82,0,2},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==4){check(w.resources(1)->energy==50);continue;}check(w.resources(1)->energy==40);cast.update(1099,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1099,h);
  if(mode==3){w.leave(1);w.join(1,"Replacement",0,0,box,0,{}, {},1099,1);w.set_vitals(1,{500,500,40,100},100);}
  box->drain();cast.update(1100,h);check(!cast.active()&&commits==(mode==0?1u:0u));
  if(mode==0){const auto status=*w.status_snapshot(owner,1100);check(status.size()==1&&status.front().skill==82&&status.front().expires==21100);
   const auto events=box->drain();check(std::count(events.begin(),events.end(),casting::generic(21,1,140))==1);
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{82,0},30))==1);
   check(w.settle_resources(owner,21099)&&w.resources(1)->energy==40);check(w.settle_resources(owner,21100)&&w.resources(1)->energy==62);
   const auto ended=box->drain();check(std::count(ended.begin(),ended.end(),casting::generic(54,1,22))==1);
  }
  if(mode==5)check(w.resources(1)->health==0);
 }
 std::cout<<"PASS Recall actual1second/10Energy/30recharge self cast, captured body140 and Energy cue, ordered expiry, cancellation/interruption/lifetime/Backfire rejection\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
