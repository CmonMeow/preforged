#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Patient Spirit regression line "+std::to_string(at.line()));}
void setup(world::Instance& w,std::shared_ptr<world::Outbox> ally={}){
 w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{world::Packet{165,0,1,0,0,0,3,0}}, {},100,1);
 w.join(2,"Ally",100,0,ally?ally:std::make_shared<world::Outbox>(),0,{}, {},100,1);
 w.join(3,"Foe",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
 check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));
 for(unsigned id:{1u,2u,3u})w.set_vitals(id,{100,1000,40,100},100);
}
int main()try{using namespace skills;
 check(!regeneration::reviewed(2061,21));
 for(unsigned rank:{0u,12u,20u}){
  auto p=*regeneration::reviewed(2061,rank);check(p.pips==0&&p.duration==2000&&!p.self_only);
  check(regeneration::end_healing(2061,rank)==30+6*rank);
  world::Instance w;auto box=std::make_shared<world::Outbox>();setup(w,box);box->drain();auto a=*w.effect_scope(1),b=*w.effect_scope(2);
  check(w.protective_enchantment(a,b,p,12,100)&&w.resources(2)->health==138);
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,2061))==1);
  w.tick(2000);check(w.resources(2)->health==138);w.tick(2100);check(w.resources(2)->health==168+6*rank);
  w.tick(5100);check(w.resources(2)->health==168+6*rank);
 }
 for(bool self:{false,true})for(unsigned action:{0u,1u,2u,3u}){
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(self?1:2);auto p=*regeneration::reviewed(2061,12);
  check(w.protective_enchantment(a,b,p,0,100));const auto entry=w.status_snapshot(b,100)->front();
  if(action==0){check(w.status_mutation(b,2061,entry.instance,12,1100,{},false));w.tick(5100);check(w.resources(b.actor)->health==100);}
  if(action==1){check(w.protective_enchantment(a,b,p,0,1100));w.tick(2100);check(w.resources(b.actor)->health==100);w.tick(3100);check(w.resources(b.actor)->health==202);}
  if(action==2){check(w.kill(b,1100));w.tick(5100);check(w.resources(b.actor)->health==0);}
  if(action==3){w.tick(5100);check(w.resources(b.actor)->health==202);}
 }
 {
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2),c=*w.effect_scope(3);auto p=*regeneration::reviewed(2061,12);
  check(!w.protective_enchantment(a,c,p,0,100));auto stale=b;++stale.generation;check(!w.protective_enchantment(a,stale,p,0,100));
  auto forged=p;forged.duration=1;check(!w.protective_enchantment(a,b,forged,0,100));
  check(w.protective_enchantment(a,b,p,0,100)&&w.enchantment_removal(c,b,*effects::enchantment_removal(68,0),20,1100));
  w.tick(3100);check(w.resources(2)->health==100);
 }
 {
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2),c=*w.effect_scope(3);
  check(w.degeneration_hex(c,b,*effects::degeneration_hex(129,12),100));
  check(w.protective_enchantment(a,b,*regeneration::reviewed(2061,12),0,100));w.tick(2100);
  check(w.resources(2)->health==168); //Defile receives66%:102*.66, visible Health rounds upward.
 }
 for(unsigned mode:{0u,1u,2u,3u}){
  using namespace casting;world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2);Hooks h;unsigned commits=0;std::vector<Packet> sent;
  h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy=[&](unsigned cost){return w.consume_energy(1,cost,a);};h.target_valid=[](unsigned id,float range,bool self){return id==2&&range==1248&&self;};
  h.target_scope=[&](unsigned id){return w.effect_scope(id);};h.control_epoch=[&]{return w.control_epoch(a);};
  h.protective_enchantment=[&](auto target,const auto& p,unsigned divine,auto now){++commits;return w.protective_enchantment(a,target,p,divine,now);};
  Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[2061]=1;Slots bar{};bar[0]={2061,0};attributes::Ranks ranks{};ranks[13]=12;ranks[16]=12;Runtime cast(1);
  check(cast.start(100,{2061,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted&&w.resources(1)->energy==35);
  cast.update(349,h);check(!commits);
  if(mode==1)cast.cancel(h);
  if(mode==2)cast.interrupt(200,h);
  if(mode==3){w.leave(2);w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{100,1000,40,100},100);}
  cast.update(350,h);check(w.resources(2)->health==(mode?100u:138u));
  w.tick(2350);check(w.resources(2)->health==(mode?100u:240u));
  if(!mode)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{2061,0},4))==1);
 }
 std::cout<<"PASS Patient Spirit expiry-only heal,Divine Favor,removal,renewal,death,modifiers,cast and lifetime\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

