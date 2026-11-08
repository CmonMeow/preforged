#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Glimmering end line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;Scope source,target;bool npc;
 Fixture(bool n):npc(n){
  if(n){world::Instance::Npc a;a.id=a.definition.id=100;a.x=100;a.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={480,480};d.damageable=d.conditions_allowed=true;
   a.combat_definition=d;w.configure_npcs({a});}
  for(unsigned id:{1u,2u,3u}){w.join(id,"End actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{480,480,100,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(n?100:2);
 }
 void mark(unsigned at=100){check(w.stage_glimmering_mark(source,target,*glimmering_mark(0),20,at));}
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 bool use(unsigned event,unsigned at,unsigned skill=1085){return w.complete_glimmering_targeted_use({source,target,skill,event,at});}
};
int main()try{unsigned cases=0;
 for(bool npc:{false,true}){
  {Fixture f(npc);f.mark();check(f.w.cast_activity(f.source,1085,850,1865));
   f.w.tick(1100);check(f.hp()==475&&f.w.glimmering_mark_snapshot(f.target,1100));
   check(f.use(1,1865));f.w.tick(10100);check(f.hp()==475&&!f.w.glimmering_mark_snapshot(f.target,10100));++cases;}
  {Fixture f(npc);f.mark();check(f.use(1,9600));check(f.hp()==435);
   f.w.tick(10100);check(f.hp()==435&&!f.w.glimmering_mark_snapshot(f.target,10100));++cases;}
  {Fixture f(npc);check(f.use(1,100,227));f.mark();
   check(!f.use(1,200)&&f.w.glimmering_mark_snapshot(f.target,200));
   check(!f.use(2,200,937)&&f.w.glimmering_mark_snapshot(f.target,200));
   check(f.use(2,200));f.mark(500);check(!f.use(2,600)&&f.w.glimmering_mark_snapshot(f.target,600));++cases;}
  {Fixture f(npc);f.mark();auto stale=f.source;++stale.generation;
   check(!f.w.complete_glimmering_targeted_use({stale,f.target,1085,1,500}));stale=f.target;++stale.generation;
   check(!f.w.complete_glimmering_targeted_use({f.source,stale,1085,1,500}));
   check(f.w.glimmering_mark_snapshot(f.target,500)&&f.use(1,500));++cases;}
  {Fixture f(npc);f.mark();const auto other=*f.w.effect_scope(3);
   check(f.w.complete_glimmering_targeted_use({other,f.target,1085,1,500}));
   check(f.w.glimmering_mark_snapshot(f.target,500)->caster==f.source);f.w.tick(1100);check(f.hp()==475);++cases;}
 }
 check(glimmering_direct_target_skill(227)&&glimmering_direct_target_skill(1085)&&glimmering_direct_target_skill(186));
 check(!glimmering_direct_target_skill(937)&&!glimmering_direct_target_skill(175)&&!glimmering_direct_target_skill(0));
 std::cout<<cases<<" player/NPC completed-use/late-end/replay/lifetime/non-owner cases passed; native cast observer not wired\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
