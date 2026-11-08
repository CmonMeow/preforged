#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Glimmering Seed line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& p,unsigned at){return unsigned(p[at])|(unsigned(p[at+1])<<8)|(unsigned(p[at+2])<<16)|(unsigned(p[at+3])<<24);}
int main()try{unsigned cases=0;
 for(int allowed:{-1,0,1})for(bool fragile:{false,true}){
  if(allowed<0&&fragile)continue;
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=910;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={500,1000};d.damageable=true;
  if(allowed>=0){d.healing_allowed=allowed!=0;d.healing_evidence="synthetic mark Seed recipient";}
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u,4u}){
   w.join(id,"Seed actor",id==1?0:id==2?600:id==3?760:920,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{500,1000,100,100},100);}
  const auto source=*w.effect_scope(1),early=*w.effect_scope(2),later=*w.effect_scope(3),monk=*w.effect_scope(4),ally=*w.effect_scope(100);
  check(w.party_action(3,158,4,100)&&w.party_action(4,154,3,100));
  const auto seed=*healing_seed(12);check(w.complete_healing_seed(monk,later,seed,0,100));
  if(fragile){check(w.degeneration_hex(source,later,*degeneration_hex(2998,12),100));check(w.cast_activity(later,390,200,2000));}
  check(w.stage_glimmering_mark(source,early,*glimmering_mark(0),20,100));box->drain();w.tick(1100);
  const auto packets=box->drain();const auto bodies=std::count_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==159&&word(p,2)==20&&word(p,14)==401;});
  if(allowed<0){check(w.glimmering_refused_pulses()==1&&bodies==0);
   for(unsigned id:{2u,3u,4u})check(w.resources(id)->health==500);
   check(w.npc_combat_snapshot(ally)->health->current==500);
  }else{const auto heals=(fragile?2:1)*seed.healing;
   check(w.glimmering_refused_pulses()==0&&bodies==2);
   check(w.resources(2)->health==500-5+heals);
   check(w.resources(3)->health==500-5+heals-(fragile?fragility(2998,12)->damage:0));
   check(w.resources(4)->health==500+heals);
   check(w.npc_combat_snapshot(ally)->health->current==500+(allowed?heals:0));
   check(w.has_condition(later,combat::conditions::Kind::blind,1100)==fragile);
  }++cases;
 }
 std::cout<<cases<<" Glimmering whole-cohort Seed refusal, allowed/immune NPC healing and Blindness/Fragility/Seed chains passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
