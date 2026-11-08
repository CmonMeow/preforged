#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Snow Storm World line "+std::to_string(at.line())+" "+context);
}
world::Instance::Npc body(unsigned id,float x,unsigned plane=0){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.plane=plane;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
 d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={1000,1000};d.cold_armor=60;d.damageable=d.conditions_allowed=true;
 d.healing_allowed=true;d.healing_evidence="synthetic Snow Storm protection fixture";
 n.combat_definition=d;return n;
}
struct Fixture{
 world::Instance w;Scope source,target;bool npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool n,bool unknown=false):npc(n){
  w.configure_npcs({body(100,100),body(101,266),body(102,267),body(103,100,1)});
  for(unsigned id=1;id<=5;++id){w.join(id,"Cold field",id==1?0:id==3?266:id==4?267:100,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);
   w.set_vitals(id,{1000,1000,50,100},100);if(!(unknown&&id==3))check(w.elemental_armor(*w.effect_scope(id),60,60));}
  source=*w.effect_scope(1);target=*w.effect_scope(n?100:2);box->drain();
 }
 bool cast(unsigned rank=0,unsigned at=100){return w.stage_snow_storm(source,target,*snow_storm(rank),20,at);}
 double hp(unsigned id){return id<100?double(w.resources(id)->health):w.npc_combat_snapshot(*w.effect_scope(id))->health->current;}
};
int main()try{unsigned cases=0;
 for(bool npc:{false,true})for(unsigned rank=0;rank<=10;++rank)for(bool dense:{false,true}){
  context="npc="+std::to_string(npc)+" rank="+std::to_string(rank)+" dense="+std::to_string(dense);
  Fixture f(npc);check(f.cast(rank));const auto damage=snow_storm(rank)->damage;
  if(dense)for(unsigned at=200;at<5100;at+=100)f.w.tick(at);f.w.tick(5100);
  for(unsigned id:{2u,3u,100u,101u})check(f.hp(id)==1000-5*damage);
  for(unsigned id:{1u,4u,5u,102u,103u})check(f.hp(id)==1000);
  check(!f.w.snow_storm_refused_pulses());const auto before=f.hp(2);f.w.tick(6100);check(f.hp(2)==before);++cases;
 }
 {context="fixed centre after target moves";Fixture f(false);check(f.cast());f.w.move(2,500,0,200);f.w.tick(5100);
  check(f.hp(2)==1000&&f.hp(100)==850&&f.hp(3)==850);++cases;}
 {context="whole pulse refusal for unknown armor";Fixture f(false,true);check(f.cast());f.w.tick(5100);
  for(unsigned id:{2u,3u,100u,101u})check(f.hp(id)==1000);check(f.w.snow_storm_refused_pulses()==5);++cases;}
 {context="typed armor";Fixture f(false);check(f.w.elemental_armor(f.target,60,100));check(f.cast());f.w.tick(5100);
  check(f.hp(2)==925&&f.hp(100)==850);++cases;}
 {context="shared Reversal player and NPC";Fixture f(false);check(f.w.status_mutation(f.target,307,80,12,100,8100,true));
  check(f.w.stage_npc_reversal(*f.w.effect_scope(100),12,100));
  check(f.cast());f.w.tick(5100);
  check(f.hp(2)==880&&f.hp(100)==880&&f.hp(3)==850);++cases;}
 {context="target death leaves fixed field";Fixture f(false);check(f.cast());check(f.w.kill(f.target,200));f.w.tick(5100);
  check(f.hp(2)==0&&f.hp(100)==850);++cases;}
 {context="source death retains field";Fixture f(false);check(f.cast());check(f.w.kill(f.source,200));f.w.tick(5100);
  check(f.hp(2)==850&&f.hp(100)==850);++cases;}
 {context="source ID reuse cannot inherit field";Fixture f(false);check(f.cast());f.w.leave(1);
  f.w.join(1,"New source",1000,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);f.w.set_vitals(1,{1000,1000,50,100},100);
  check(f.w.effect_scope(1)!=f.source);f.w.tick(5100);check(f.hp(2)==1000&&f.hp(100)==1000);++cases;}
 for(bool npc:{false,true})for(unsigned renewal:{0u,181u,2860u}){
  context="admitted npc="+std::to_string(npc)+" renewal="+std::to_string(renewal);Fixture f(npc);f.w.set_vitals(1,{500,1000,50,100},100);
  if(renewal){const auto r=*ether_renewal(renewal,0);check(f.w.status_mutation(f.source,renewal,80,0,100,100+r.duration,true));}
  check(f.w.before_spell_effect(f.source,2222,1,200));
  std::vector<world::Packet> visuals;check(!f.w.complete_snow_storm(f.source,f.target,*snow_storm(0),20,2,200,&visuals));
  auto stale=f.target;++stale.generation;check(!f.w.complete_snow_storm(f.source,stale,*snow_storm(0),20,1,200,&visuals));
  check(f.w.complete_snow_storm(f.source,f.target,*snow_storm(0),20,1,200,&visuals));
  check(visuals==skills::particles::snow_storm_ground_2022(1,100,0,0));
  check(f.w.resources(1)->health==500+(renewal?5:0)&&f.w.resources(1)->energy==50+(renewal?1:0));
  check(!f.w.complete_snow_storm(f.source,f.target,*snow_storm(0),20,1,200,&visuals)&&visuals.empty());++cases;
 }
 std::cout<<cases<<" all-title player/NPC dense/sparse cold fields, fixed centre, cohorts, armor/Reversal, lifetimes, admitted Renewal and exact visual forms passed; Runtime remains unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
