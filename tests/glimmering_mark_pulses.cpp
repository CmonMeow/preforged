#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Glimmering pulse line "+std::to_string(at.line())+" "+context);
}
world::Instance::Npc body(unsigned id,float x,double hp=480,bool conditions=true){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
 d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{hp,480};
 d.damageable=true;d.conditions_allowed=conditions;d.healing_allowed=true;d.healing_evidence="synthetic mark protection recipient";
 n.combat_definition=d;return n;
}
struct Fixture {
 world::Instance w;Scope caster,target;bool npc;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool n,double hp=480):npc(n){
  auto other_plane=body(104,100);other_plane.plane=1;
  w.configure_npcs({body(100,100,n?hp:480),body(101,266),body(102,267),body(103,400),other_plane});
  for(unsigned id=1;id<=5;++id){const auto x=id==1?0:id==3?266:id==4?267:100;
   w.join(id,"Mark actor",float(x),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);
   w.set_vitals(id,{id==2&&!n?unsigned(hp):480u,480,100,100},100);}
  caster=*w.effect_scope(1);target=*w.effect_scope(n?100:2);box->drain();
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 void mark(unsigned rank=0){check(w.stage_glimmering_mark(caster,target,*glimmering_mark(rank),20,100));box->drain();}
};
int main()try{unsigned ranks=0;
 for(bool npc:{false,true})for(unsigned rank=0;rank<=20;++rank)for(bool dense:{false,true}){
  context="rank="+std::to_string(rank)+" npc="+std::to_string(npc)+" dense="+std::to_string(dense);
  Fixture f(npc);f.mark(rank);const auto damage=unsigned(std::lround(5.+20.*rank/15.));
  if(dense)for(unsigned at=200;at<10100;at+=100)f.w.tick(at);
  f.w.tick(10100);check(f.hp()==480-10*damage);check(f.w.resources(3)->health==480-10*damage);
  check(f.w.resources(4)->health==480&&f.w.resources(5)->health==480);
  check(f.w.npc_combat_snapshot(*f.w.effect_scope(101))->health->current==480-10*damage);
  for(unsigned id:{102u,103u,104u})check(f.w.npc_combat_snapshot(*f.w.effect_scope(id))->health->current==480);
  check(!f.w.glimmering_mark_snapshot(f.target,10100)&&f.w.glimmering_refused_pulses()==0);
  const auto packets=f.box->drain();world::Packet expected{159,0};for(unsigned v:{20u,f.target.actor,1u,401u})combat::detail::put(expected,v);
  check(std::count(packets.begin(),packets.end(),expected)==10);
  f.w.tick(10200);check(f.hp()==480-10*damage);check(f.w.stage_glimmering_mark(f.caster,f.target,*glimmering_mark(rank),20,10200));++ranks;
 }
 {context="moving centre";Fixture f(false);f.mark();f.w.move(2,500,0,200);f.w.tick(1100);
  check(f.w.npc_combat_snapshot(*f.w.effect_scope(100))->health->current==480);
  check(f.w.npc_combat_snapshot(*f.w.effect_scope(103))->health->current==475);}
 for(bool npc:{false,true}){context="conditional Blindness";Fixture f(npc);f.mark();
  check(f.w.cast_activity(*f.w.effect_scope(2),390,200,2000));
  check(f.w.cast_activity(*f.w.effect_scope(3),186,200,2000));
  check(f.w.begin_npc_action(*f.w.effect_scope(100),combat::npcs::control::Action::attack,200,2000,skills::Slot{390,0}).has_value());
  check(f.w.begin_npc_action(*f.w.effect_scope(101),combat::npcs::control::Action::attack,200,2000).has_value());
  f.w.tick(1100);check(f.w.has_condition(*f.w.effect_scope(2),combat::conditions::Kind::blind,1100));
  check(f.w.has_condition(*f.w.effect_scope(100),combat::conditions::Kind::blind,1100));
  check(!f.w.has_condition(*f.w.effect_scope(3),combat::conditions::Kind::blind,1100));
  check(!f.w.has_condition(*f.w.effect_scope(101),combat::conditions::Kind::blind,1100));
  f.w.tick(4100);check(!f.w.has_condition(*f.w.effect_scope(2),combat::conditions::Kind::blind,4100));
 }
 for(bool npc:{false,true}){context="Reversal";Fixture f(npc,300);f.mark();
  if(npc)check(f.w.stage_npc_reversal(f.target,12,200));else check(f.w.status_mutation(f.target,307,80,12,200,8200,true));
  f.w.tick(1100);check(f.hp()==305);f.w.tick(2100);check(f.hp()==300);}
 for(bool npc:{false,true}){context="Fragility";Fixture f(npc);f.mark();
  check(f.w.degeneration_hex(f.caster,f.target,*degeneration_hex(2998,12),200));
  if(npc)check(f.w.begin_npc_action(f.target,combat::npcs::control::Action::attack,200,2000,skills::Slot{390,0}).has_value());
  else check(f.w.cast_activity(f.target,390,200,2000));
  f.w.tick(1100);check(f.hp()==480-5-fragility(2998,12)->damage);
  f.w.tick(2100);check(f.hp()==480-10-fragility(2998,12)->damage);}
 {context="early player dispel";Fixture f(false);f.mark();f.w.tick(1100);
  check(f.w.remove_effects(f.target,f.target.actor,*removal(301,0,0),1600));
  f.w.tick(10100);check(f.hp()==475&&!f.w.glimmering_mark_snapshot(f.target,10100));}
 {context="lethal primary";Fixture f(true,1);f.mark();f.w.tick(10100);check(f.hp()==0);
  check(f.w.resources(3)->health==475&&!f.w.has_condition(f.target,combat::conditions::Kind::blind,10100));}
 {context="received versus cast ownership";Fixture f(false);f.mark();
  f.w.join(6,"Third team",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,3);f.w.set_vitals(6,{480,480,100,100},100);
  const auto third=*f.w.effect_scope(6);check(f.w.stage_glimmering_mark(f.target,third,*glimmering_mark(0),20,100));
  f.w.tick(1100);check(f.w.resources(6)->health==475);
  check(f.w.remove_effects(f.target,f.target.actor,*removal(301,0,0),1600));f.w.tick(2100);
  check(f.hp()==475&&f.w.resources(6)->health==470&&f.w.glimmering_mark_snapshot(third,2100)->caster==f.target);
 }
 {context="unknown attack identity refuses whole pulse";Fixture f(false);f.mark();
  check(f.w.begin_npc_action(*f.w.effect_scope(101),combat::npcs::control::Action::spell,200,2000).has_value());
  f.w.tick(1100);check(f.w.glimmering_refused_pulses()==1&&f.hp()==480&&f.w.resources(3)->health==480);
 }
 std::cout<<ranks<<" all-rank player/NPC dense/sparse pulse cases; moving cohort, bounds/layers, final pulse, body401, attack-skill Blindness, Reversal, Fragility, dispel and death passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
