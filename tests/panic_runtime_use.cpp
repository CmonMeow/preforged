#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Panic Runtime use line "+std::to_string(at.line()));}
int main()try{using namespace skills;
 for(unsigned mode=0;mode<3;++mode){world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float(id*30),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{500,500,100,100},100);}const auto caster=*w.effect_scope(1),user=*w.effect_scope(2),victim=*w.effect_scope(3);check(w.apply_panic(caster,user,12,100));
  casting::Hooks hu,hv;std::vector<effects::PanicUse> uses;std::vector<world::Packet> interrupted;
  auto wire=[&](casting::Hooks& h,effects::Scope owner){h.events=[](const auto&){};h.resources=[&](unsigned id){return w.resources(id);};h.consume_energy=[&,owner](unsigned cost){return w.consume_energy(owner.actor,cost,owner);};h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[owner](unsigned id,float,bool self){return id==owner.actor&&self;};h.projectile_target_valid=[](unsigned id,float){return id==1;};h.before_spell_effect=[&,owner](unsigned skill,auto event,auto now){return w.before_spell_effect(owner,skill,event,now);};h.cast_activity=[&,owner](unsigned skill,auto begin,auto end){check(w.cast_activity(owner,skill,begin,end));};h.control_epoch=[&,owner]{return w.control_epoch(owner);};h.control_event_time=[&,owner](auto epoch){return w.control_event_time(owner,epoch);};h.mantra_recall=[&](auto selected,unsigned rank,auto now){return w.mantra_recall(selected,rank,now);};h.hex_breaker=[&](auto selected,const auto& plan,auto now){return w.hex_breaker(selected,plan,20,now);};h.panic=[&,owner](auto selected,unsigned rank,auto now){return w.apply_panic(owner,selected,rank,now);};h.panic_skill_used=[&](const auto& use){uses.push_back(use);return w.panic_skill_used(use);};};
  wire(hu,user);wire(hv,victim);hv.events=[&](const auto& batch){interrupted.insert(interrupted.end(),batch.begin(),batch.end());};
  const unsigned used_skill=mode==2?10:82;Policy pu;pu.primary=5;pu.campaigns=255;pu.learned[used_skill]=1;Policy pv=pu;pv.learned.clear();pv.learned[52]=1;Slots bu{},bv{};bu[0]={used_skill,0};bv[0]={52,0};attributes::Ranks ranks{};ranks[2]=12;ranks[3]=12;casting::Runtime ru(2),rv(3);
  check(rv.start(mode==2?100:500,{52,0,1},bv,pv,ranks,true,hv)==casting::Reason::accepted);
  check(ru.start(100,{used_skill,0,2},bu,pu,ranks,true,hu)==casting::Reason::accepted);
  if(mode==1)ru.cancel(hu);if(mode!=2)ru.update(1100,hu);
  rv.update(mode==2?100:1100,hv);
  if(mode==1){check(uses.empty()&&rv.active());rv.cancel(hv);continue;}
  check(uses.size()==1&&uses[0].owner==user&&uses[0].skill==used_skill&&uses[0].began==100&&uses[0].used==(mode==2?100u:1100u));
  check(!rv.active()&&!ru.active()&&w.control_epoch(user)==0&&w.control_epoch(victim)==1);
  check(std::count(interrupted.begin(),interrupted.end(),casting::slot_event(228,3,{52,0},15))==1);
  check(w.status_snapshot(caster,1100)->empty());rv.update(1600,hv);check(uses.size()==1&&w.resources(2)->health==500&&w.resources(3)->health==500);
 }
 std::cout<<"PASS actual spell/instant successful uses trigger Panic once and interrupt another Runtime with full recharge; cancelled use does not trigger\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
