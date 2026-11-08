#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Gaze Seed line "+std::to_string(at.line()));
}
int main()try{
 unsigned cases=0;
 for(int allowed:{-1,0,1})for(bool absorption:{false,true})for(int reversal:{-1,0,20})
 for(unsigned initial:{60u,100-effects::healing_seed(12)->healing+1}){
  ++cases;world::Instance w;std::vector<world::Instance::Npc> npcs;
  for(unsigned id:{100u,101u}){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?60:375;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=474;d.binding.actor=d.binding.definition=id;d.binding.level=20;
   d.relation=id==100?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
   d.spawn_health={id==100?1000.:500.,1000};d.damageable=true;d.general_armor=60;d.general_armor_evidence="synthetic Gaze Seed";
   if(id==101){if(allowed>=0)d.healing_allowed=bool(allowed);if(allowed>=0)d.healing_evidence="synthetic Gaze Seed";}
   n.combat_definition=d;npcs.push_back(n);
  }
  w.configure_npcs(npcs);auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u,4u}){
   w.join(id,"Actor",id==1?0:id==2?220:id==3?230:380,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==3?initial:500u,1000,100,100},100);
  }
  const auto source=*w.effect_scope(1),primary=*w.effect_scope(100),ally=*w.effect_scope(101),a=*w.effect_scope(2),b=*w.effect_scope(3),monk=*w.effect_scope(4);
  for(auto victim:{a,b}){
   check(w.holy_armor_locations(victim,std::array<int,5>{60,60,60,60,60}));check(w.complete_healing_seed(monk,victim,*effects::healing_seed(12),0,100));
   if(absorption)check(w.status_mutation(victim,1399,800+victim.actor,12,100,6100,true));
  }
  if(reversal>=0)check(w.status_mutation(b,307,900,reversal,100,8100,true));
  const auto action=w.begin_npc_action(primary,combat::npcs::control::Action::spell,100,3000,skills::Slot{194,0});check(action.has_value());
  box->drain();std::function<void()> publication;
  const auto result=w.stage_lightbringers_gaze_world(source,primary,*effects::lightbringers_gaze(2),20,1000,&publication);
  if(allowed<0){
   check(!result&&!publication&&box->drain().empty());check(w.resources(2)->health==500&&w.resources(3)->health==initial);
   check(w.npc_combat_snapshot(primary)->health->current==1000&&w.npc_combat_snapshot(ally)->health->current==500);
   check(w.complete_npc_action(*action,3100));continue;
  }
  check(result&&result->size()==3&&publication&&box->drain().empty());
  const unsigned raw=100-(absorption?5:0),converted=reversal<0?0:std::min(raw,effects::reversal_fortune(reversal)->maximum_healing);
  const bool second=raw>converted;const unsigned heal=effects::healing_seed(12)->healing;
  check(w.resources(2)->health==500-raw+heal+(second?heal:0));
  // Seed triggers from lethal damage too, but never resurrects its recipient.
  const int remaining=int(initial+heal+converted)-int(raw-converted);
  const unsigned expected=remaining>0?unsigned(remaining)+(second?heal:0):0;
  if(w.resources(3)->health!=expected)
   throw std::runtime_error("Gaze Seed later Health allowed="+std::to_string(allowed)+" shield="+std::to_string(absorption)+
    " reversal="+std::to_string(reversal)+" initial="+std::to_string(initial)+" actual="+std::to_string(w.resources(3)->health)+
    " expected="+std::to_string(expected));
  check(w.resources(4)->health==500+heal+(second?heal:0));
  check(w.npc_combat_snapshot(primary)->health->current==900);
  check(w.npc_combat_snapshot(ally)->health->current==500+(allowed?heal+(second?heal:0):0));
  check(!w.complete_npc_action(*action,3100));publication();const auto packets=box->drain();check(!packets.empty());publication();check(box->drain().empty());
 }
 check(cases==36);std::cout<<cases<<" Gaze Seed cohort cases: refusal before damage/control, sequential survival, protection and one-shot publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
