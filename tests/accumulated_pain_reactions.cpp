#include "gw_wound_spell_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Accumulated Pain reactions line "+std::to_string(at.line())+" "+context);
}
struct Fixture {
 world::Instance w;Scope source,target;bool npc;
 Fixture(bool is_npc,unsigned health=300):npc(is_npc){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(health),480};
   d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic reaction recipient";n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u,3u}){w.join(id,"Reaction actor",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{id==1?500u:health,id==1?1000u:480u,50,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
  check(w.degeneration_hex(source,target,*degeneration_hex(129,0),100));check(w.degeneration_hex(source,target,*degeneration_hex(785,0),100));
  check(w.status_mutation(source,181,80,0,100,5100,true));
 }
 double health(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 void expected(double value){check(std::abs(health()-(npc?value:std::ceil(value)))<1e-7);}
 void cast(unsigned skill,unsigned rank=0,unsigned event=1,unsigned at=200){check(w.before_spell_effect(source,skill,event,at));
  check(w.complete_wound_spell(source,target,*wound_spell(skill,rank),20,event,at));}
};
int main()try{unsigned cases=0;
 for(unsigned skill:{1052u,3184u})for(bool npc:{false,true}){
  context="Reversal skill="+std::to_string(skill)+" npc="+std::to_string(npc);Fixture f(npc);
  if(npc)check(f.w.stage_npc_reversal(f.target,12,100));else check(f.w.status_mutation(f.target,307,81,12,100,8100,true));
  f.cast(skill,12);const auto amount=wound_spell(skill,12)->damage;f.expected(300+amount*.6-96);
  const auto entries=f.w.status_snapshot(f.target,200);check(entries&&std::none_of(entries->begin(),entries->end(),[](const auto& e){return e.skill==307;}));
  check(f.w.has_condition(f.target,combat::conditions::Kind::deep_wound,200)&&f.w.resources(1)->health==505);++cases;
 }
 for(unsigned skill:{1052u,3184u})for(bool npc:{false,true}){
  context="Fragility skill="+std::to_string(skill)+" npc="+std::to_string(npc);Fixture f(npc);
  check(f.w.degeneration_hex(f.source,f.target,*degeneration_hex(19,0),100));
  f.cast(skill);const auto amount=wound_spell(skill,0)->damage;const auto first=300-amount-96-fragility(19,0)->damage;f.expected(first);
  f.cast(skill,0,2,300);f.expected(first-amount);check(f.w.resources(1)->health==510);++cases;
 }
 for(unsigned skill:{1052u,3184u}){
  context="Healing Seed skill="+std::to_string(skill);Fixture f(false);const auto monk=*f.w.effect_scope(3);
  check(f.w.party_action(2,158,3,100)&&f.w.party_action(3,154,2,100));
  check(f.w.complete_healing_seed(monk,f.target,*healing_seed(12),0,100));f.cast(skill);
  f.expected(300-wound_spell(skill,0)->damage+healing_seed(12)->healing*.6-96);check(f.w.resources(3)->health==300+healing_seed(12)->healing);++cases;
 }

 for(unsigned skill:{1052u,3184u})for(bool npc:{false,true}){
  context="Reversal/wound debt/Fragility death skill="+std::to_string(skill)+" npc="+std::to_string(npc);Fixture f(npc,1);
  check(f.w.degeneration_hex(f.source,f.target,*degeneration_hex(19,0),100));
  if(npc)check(f.w.stage_npc_reversal(f.target,12,100));else check(f.w.status_mutation(f.target,307,81,12,100,8100,true));
  f.cast(skill);check(f.health()==0&&f.w.resources(1)->health==505);++cases;
 }
 std::cout<<"PASS "<<cases<<" player/NPC Reversal, Deep Wound Fragility gain-versus-renewal and player Healing Seed ordering cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
