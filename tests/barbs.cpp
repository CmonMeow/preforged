#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Barbs regression line "+std::to_string(at.line()));}
combat::weapon::Owned sword(){return {3,1,{20,20,2},12,false,false,27};}
void setup(world::Instance& w,std::shared_ptr<world::Outbox> box={}){
 for(unsigned id:{1u,2u}){w.join(id,"Actor",float((id-1)*60),0,box?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{500,500,100,100},100);}
 check(w.physical_armor(*w.effect_scope(2),60));
}
int main()try{using namespace skills::effects;
 check(!barbs(21));for(unsigned raw=0;raw<14;++raw)check(barbs_physical(raw,1)==(raw<=2));
 check(!barbs_physical({},1)&&!barbs_physical(0,0)&&!barbs_physical(1,-1)&&!barbs_physical(2,NAN));
 for(unsigned rank:{0u,12u,20u}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();setup(w,box);box->drain();auto a=*w.effect_scope(1),b=*w.effect_scope(2);
  check(w.degeneration_hex(a,b,*degeneration_hex(101,rank),100));
  auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*skills::particles::targeted_body_2022(2,1,101))==2); // Shared queue receives each peer publication.
  check(w.ordinary_weapon(a,sword(),20)&&w.ordinary_attack(a,2,100));w.tick(600);w.cancel_ordinary_attack(a);
  check(w.resources(2)->health==480-barbs(rank)->damage);w.tick(700);check(w.resources(2)->health==480-barbs(rank)->damage);
  check(w.melee_attack(a,b,sword(),*combat::weapon::attack_skill(322,0),800));
  check(w.resources(2)->health==450-2*barbs(rank)->damage);
 }
 for(unsigned mode:{0u,1u,2u,3u}){
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.degeneration_hex(a,b,*degeneration_hex(101,12),100));
  if(mode==0)w.tick(30100);
  if(mode==1){const auto entry=w.status_snapshot(b,100)->front();check(w.status_mutation(b,101,entry.instance,12,200,{},false));}
  if(mode==2){auto old=b;w.leave(2);w.join(2,"New",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);b=*w.effect_scope(2);check(old!=b&&w.physical_armor(b,60));}
  if(mode==3)check(w.physical_absorption(b,3));
  const auto now=mode==0?30200:300;
  auto gear=sword();if(mode==3)gear.damage.minimum=gear.damage.maximum=3;
  check(w.ordinary_weapon(a,gear,20)&&w.ordinary_attack(a,2,now));w.tick(now+500);w.cancel_ordinary_attack(a);
  check(w.resources(2)->health==(mode==3?500u:480u));
 }
 {
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2);check(w.degeneration_hex(a,b,*degeneration_hex(101,12),100));
  combat::weapon::Owned wand{4,1,{20,20,11},0,false,true};
  check(w.ordinary_weapon(a,wand,20)&&w.combat_armor(b,60)&&w.configure_projectiles({100,2}));
  check(w.ordinary_attack(a,2,100));w.tick(600);check(w.resources(2)->health==500);w.tick(1600);w.cancel_ordinary_attack(a);
  if(w.resources(2)->health!=480)throw std::runtime_error("elemental weapon Health="+std::to_string(w.resources(2)->health));
 }
 for(unsigned health:{500u,35u}){
  world::Instance w;const std::string hash(64,'a');
  std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto pop=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/barbs.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 "+std::to_string(health)+" 60 1 1 test test test - test 60 60 test test 60 test\n");
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(pop,catalog,hash);w.configure_npcs(pop.for_map(146));setup(w);
  auto a=*w.effect_scope(1),b=*w.effect_scope(100);check(w.degeneration_hex(a,b,*degeneration_hex(101,12),100));
  check(w.ordinary_weapon(a,sword(),20)&&w.melee_attack(a,b,sword(),*combat::weapon::attack_skill(322,0),200));
  check(w.npc_combat_snapshot(b)->health->current==(health==35?0:458));
  const auto deaths=w.take_npc_deaths(a);check(deaths.size()==(health==35?1:0));
  w.tick(300);check(w.take_npc_deaths(a).empty());
 }
 for(unsigned mode:{0u,1u,2u,3u}){
  using namespace skills::casting;world::Instance w;setup(w);auto a=*w.effect_scope(1);Hooks h;unsigned commits=0;std::vector<world::Packet> sent;
  h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};h.consume_energy=[&](unsigned cost){return w.consume_energy(1,cost,a);};
  h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};h.target_scope=[&](unsigned id){return w.effect_scope(id);};
  h.degeneration_hex=[&](auto target,const auto& plan,auto now){++commits;return w.degeneration_hex(a,target,plan,now);};
  skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[101]=1;skills::Slots slots{};slots[0]={101,0};attributes::Ranks ranks{};ranks[7]=12;Runtime cast(1);
  check(cast.start(100,{101,0,2},slots,policy,ranks,true,h)==Reason::accepted);
  check(w.resources(1)->energy==90);
  cast.update(2099,h);check(!commits);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(500,h);
  if(mode==3){w.leave(2);w.join(2,"New",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{500,500,100,100},100);}
  cast.update(2100,h);cast.update(2100,h);check(commits==(mode?0u:1u));
  if(!mode)check(std::count(sent.begin(),sent.end(),slot_event(228,1,{101,0},5))==1);
 }
 for(unsigned lifecycle:{0u,1u,2u}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();setup(w,box);
  w.join(3,"Physical ally",0,0,box,0,{}, {},100,1);w.set_vitals(3,{500,500,100,100},100);
  const auto caster=*w.effect_scope(1),attacker=*w.effect_scope(3),target=*w.effect_scope(2);
  check(w.degeneration_hex(caster,target,*degeneration_hex(101,12),100));
  if(lifecycle==1)check(w.kill(caster,200));
  if(lifecycle==2){w.leave(1);w.join(1,"New caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,500,100,100},100);}
  box->drain();check(w.ordinary_weapon(attacker,sword(),20)&&w.ordinary_attack(attacker,2,300));w.tick(800);w.cancel_ordinary_attack(attacker);
  check(w.resources(2)->health==(lifecycle==2?480u:468u));
  const auto packets=box->drain();const auto attributed=combat::detail::integer(10,1,101);
  check((std::find(packets.begin(),packets.end(),attributed)!=packets.end())==(lifecycle!=2));
 }
 {
  world::Instance w;setup(w);auto a=*w.effect_scope(1),b=*w.effect_scope(2);
  check(w.degeneration_hex(a,b,*degeneration_hex(101,12),100)&&w.degeneration_hex(a,b,*degeneration_hex(101,0),1100));
  check(w.ordinary_weapon(a,sword(),20)&&w.ordinary_attack(a,2,1200));w.tick(1700);w.cancel_ordinary_attack(a);check(w.resources(2)->health==479);
 }
 std::cout<<"PASS Barbs physical-only separate hit,rank,expiry,removal,stale scope,NPC and actual cast\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
