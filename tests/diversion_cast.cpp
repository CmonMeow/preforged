#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Diversion cast line "+std::to_string(at.line()));}
void wire_use(world::Instance& w,skills::casting::Hooks& h,skills::effects::Scope owner){
 h.prepare_skill_use=[&,owner](auto slot,auto event,auto now){return w.prepare_diversion_skill_use(owner,slot,event,now);};
 h.finalize_skill_use=[&,owner](auto event,auto now,auto base,auto existing,bool instant){return w.finalize_diversion_skill_use(owner,event,now,base,existing,instant);};
 h.abort_skill_use=[&,owner](auto event){return w.abort_diversion_skill_use(owner,event);};
 h.skill_disabled_slot=[&,owner](auto slot,auto now){return w.diversion_recharge_until(owner,slot,now);};
}
int main()try{using namespace skills;
 for(unsigned mode=0;mode<8;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(mode==7){
   const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=world::load_npc_catalog(appearance);
   std::istringstream metadata("gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/diversion_cast.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test\n");
   world::bind_combat_catalog(population,combat::npcs::load_catalog(metadata,hash),hash);w.configure_npcs(population.for_map(146));
  }
  w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Foe",60,0,box,0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const unsigned target_id=mode==7?100:2;const auto owner=*w.effect_scope(1),target=*w.effect_scope(target_id);casting::Hooks h;unsigned commits=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy=[&](unsigned cost){check(cost==10);return w.consume_energy(1,cost,owner);};h.target_scope=[&](unsigned id){return w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==target_id&&range==1248;};h.control_epoch=[&]{return w.control_epoch(owner);};
  wire_use(w,h,owner);h.diversion=[&](auto selected,const auto& plan,auto now){++commits;return w.diversion(owner,selected,plan,now);};
  if(mode==5)h.diversion={};if(mode==6)check(w.hex_breaker(target,*effects::hex_breaker(12),20,100));
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[30]=1;Slots bar{};bar[0]={30,0};attributes::Ranks ranks{};ranks[2]=12;casting::Runtime cast(1);
  check(cast.start(100,{30,0,target_id},bar,policy,ranks,true,h)==(mode==5?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==5){check(w.resources(1)->energy==100);continue;}
  check(w.resources(1)->energy==90);cast.update(3099,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(3099,h);
  if(mode==3){w.leave(2);w.join(2,"Replacement",60,0,box,0,{}, {},3099,2);w.set_vitals(2,{500,500,100,100},100);}
  if(mode==4){w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<world::Outbox>(),0,{}, {},3099,1);w.set_vitals(1,{500,500,100,100},100);}
  box->drain();cast.update(3100,h);cast.update(3101,h);
  check(commits==(mode==0||mode==6||mode==7?1u:0u));
  if(mode==0){const auto active=*w.status_snapshot(target,3100);check(active.size()==1&&active.front().skill==30&&active.front().expires==9100);
   const auto p=box->drain();check(std::count(p.begin(),p.end(),*particles::targeted_body_2022(2,1,30))==1);check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{30,0},12))==1);}
  if(mode==6)check(w.resources(1)->health==461&&w.status_snapshot(target,3100)->empty());
  if(mode==7){check(!w.resources(100)&&(w.npc_combat_snapshot(target)->effects&0x800));const auto p=box->drain();check(std::count(p.begin(),p.end(),*particles::targeted_body_2022(100,1,30))==1);}
 }
 for(unsigned mode=0;mode<4;++mode){
  world::Instance w;w.join(1,"Victim",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Enemy",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto owner=*w.effect_scope(1),enemy=*w.effect_scope(2);check(w.diversion(enemy,owner,*effects::diversion(12),100));
  const unsigned skill=mode==3?10:67;const std::uint64_t complete=mode==3?100:1100;
  casting::Hooks h;std::vector<world::Packet> sent;h.events=[&](const auto& b){sent.insert(sent.end(),b.begin(),b.end());};h.resources=[&](unsigned id){return w.resources(id);};
  h.consume_energy=[&](unsigned cost){return w.consume_energy(1,cost,owner);};h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.target_valid=[](unsigned id,float,bool){return id==1;};
  h.control_epoch=[&]{return w.control_epoch(owner);};wire_use(w,h,owner);
  h.shatter_ally_valid=[](unsigned id,float,auto){return id==1;};h.shatter_delusions=[&](auto selected,const auto& plan,auto now){return mode==2?false:w.shatter_delusions(owner,selected,plan,20,now);};
  h.hex_breaker=[&](auto selected,const auto& plan,auto now){return w.hex_breaker(selected,plan,20,now);};
  Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[2]=12;casting::Runtime cast(1);
  check(cast.start(100,{skill,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);
  if(mode==1)cast.cancel(h);cast.update(complete,h);
  if(mode==0||mode==3){const auto base=mode==3?15000:10000;const auto until=complete+base+47000;
   check(w.diversion_recharge_until(owner,{skill,0},complete)==until);
   check(std::count(sent.begin(),sent.end(),casting::slot_event(228,1,{skill,0},mode==3?62:57))==1);
   check(cast.start(complete+1000,{skill,0,1},bar,policy,ranks,true,h)==casting::Reason::recharge);
   cast.morale_boost(h);check(cast.start(complete+1000,{skill,0,1},bar,policy,ranks,true,h)==casting::Reason::recharge);
   casting::Runtime replacement(1);check(replacement.start(complete+1000,{skill,0,1},bar,policy,ranks,true,h)==casting::Reason::recharge);
  }else{check(w.diversion_recharge_until(owner,{skill,0},complete)==0);const auto effects=*w.status_snapshot(owner,complete);check(std::any_of(effects.begin(),effects.end(),[](const auto& e){return e.skill==30;}));
   check(w.prepare_diversion_skill_use(owner,{skill,0},UINT64_C(999999999),complete));check(w.abort_diversion_skill_use(owner,UINT64_C(999999999)));
  }
 }
 std::cout<<"PASS Diversion actual3s/10Energy/12recharge cast, body52, Hex Breaker/lifetime/control rejection; used hex removal and instant stance get extra47s, cancelled/failed casts preserve hex\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
