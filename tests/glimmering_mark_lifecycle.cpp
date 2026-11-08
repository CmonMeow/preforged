#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Glimmering lifecycle line "+std::to_string(at.line())+" "+context);
}
struct Fixture{
 world::Instance w;Scope caster,target;bool npc;
 Fixture(bool n):npc(n){
  if(n){world::Instance::Npc a;a.id=a.definition.id=100;a.x=100;a.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={480,480};d.damageable=d.conditions_allowed=true;
   a.combat_definition=d;w.configure_npcs({a});}
  join(1,1,0,100);join(2,2,100,100);caster=*w.effect_scope(1);target=*w.effect_scope(n?100:2);
  check(w.stage_glimmering_mark(caster,target,*glimmering_mark(0),20,100));
 }
 void join(unsigned id,unsigned team,float x,unsigned at){w.join(id,"Lifetime",x,0,std::make_shared<world::Outbox>(),0,{}, {},at,team);w.set_vitals(id,{480,480,100,100},100);}
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
};
int main()try{unsigned cases=0;
 for(bool npc:{false,true})for(bool dense:{false,true})for(unsigned mode=0;mode<3;++mode){
  context="npc="+std::to_string(npc)+" dense="+std::to_string(dense)+" mode="+std::to_string(mode);
  Fixture f(npc);f.w.tick(1100);check(f.hp()==475);
  if(mode==0)check(f.w.kill(f.caster,1200));
  else f.w.leave(1);
  if(mode==2){f.join(1,2,1000,1200);const auto fresh=*f.w.effect_scope(1);check(fresh!=f.caster);
   check(f.w.complete_glimmering_targeted_use({fresh,f.target,1085,1,1300}));
   check(f.w.glimmering_mark_snapshot(f.target,1300)->caster==f.caster);
   check(!f.w.complete_glimmering_targeted_use({f.caster,f.target,1085,2,1300}));}
  if(dense)for(unsigned at=1300;at<10100;at+=100)f.w.tick(at);
  f.w.tick(10100);check(f.hp()==430&&f.w.glimmering_refused_pulses()==0);
  check(!f.w.glimmering_mark_snapshot(f.target,10100));
  if(mode==2)check(f.w.resources(1)->health==480);++cases;
 }
 for(bool dense:{false,true}){
  context="target rejoin";Fixture f(false);f.w.tick(1100);check(f.hp()==475);const auto old=f.target;
  f.w.leave(2);f.join(2,2,100,1200);const auto fresh=*f.w.effect_scope(2);check(fresh!=old);
  if(dense)for(unsigned at=1300;at<10100;at+=100)f.w.tick(at);
  f.w.tick(10100);check(f.w.resources(2)->health==480&&!f.w.glimmering_mark_snapshot(fresh,10100));
  check(!f.w.complete_glimmering_targeted_use({f.caster,old,1085,1,10200}));
  check(f.w.stage_glimmering_mark(f.caster,fresh,*glimmering_mark(0),20,10200));f.w.tick(11200);
  check(f.w.resources(2)->health==475);++cases;
 }
 for(unsigned at:{500u,9600u}){
  context="NPC early manual end at="+std::to_string(at);Fixture f(true);
  check(f.w.complete_glimmering_targeted_use({f.caster,f.target,1085,1,at}));
  const auto hp=f.hp();f.w.tick(10100);check(f.hp()==hp&&!f.w.glimmering_mark_snapshot(f.target,10100));++cases;
 }
 for(bool npc:{false,true}){
  context="unrelated completed use behind resource frontier";Fixture f(npc);
  check(f.w.complete_glimmering_targeted_use({f.caster,f.target,1085,1,200}));f.w.tick(600);
  check(f.w.complete_glimmering_targeted_use({f.caster,f.target,1085,2,500}));
  check(f.w.stage_glimmering_mark(f.caster,f.target,*glimmering_mark(0),20,600));
  check(!f.w.complete_glimmering_targeted_use({f.caster,f.target,1085,2,700}));
  check(!f.w.complete_glimmering_targeted_use({f.caster,f.target,1085,3,500}));
  check(f.w.glimmering_mark_snapshot(f.target,700).has_value());
  check(f.w.complete_glimmering_targeted_use({f.caster,f.target,1085,3,700}));++cases;
 }
 std::cout<<cases<<" dense/sparse source death/departure/ID-reuse, target rejoin, early NPC end and unrelated-use frontier cases passed; source continuity is explicit private policy pending retail verification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
