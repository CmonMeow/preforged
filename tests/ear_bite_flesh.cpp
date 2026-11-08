#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ear Bite flesh line "+std::to_string(at.line()));
}
int main()try{
 using combat::conditions::Kind;using combat::conditions::flesh_eligibility;
 for(auto kind:{Kind::bleeding,Kind::poison,Kind::disease}){
  check(!flesh_eligibility(kind,std::nullopt));check(flesh_eligibility(kind,false)==false);check(flesh_eligibility(kind,true)==true);
 }
 for(auto kind:{Kind::blind,Kind::burning,Kind::crippled,Kind::deep_wound,Kind::dazed,Kind::weakness,Kind::cracked_armor})
  check(flesh_eligibility(kind,std::nullopt)==true&&flesh_eligibility(kind,false)==true);
 check(!flesh_eligibility(Kind::count,true));

 for(unsigned mode=0;mode<6;++mode){
  std::optional<bool> flesh=mode==0?std::optional(true):mode==1||mode==5?std::optional(false):std::nullopt;
  const bool conditions=mode!=3,lethal=mode==4||mode==5;const unsigned health=lethal?40:1000;
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.spawn_health={double(health),1000};d.damageable=true;d.conditions_allowed=conditions;d.relation=combat::npcs::Relation::hostile_to_players;
  d.physical_armor=60;d.physical_armor_evidence="synthetic physical armor";
  d.fleshy=flesh;if(flesh)d.fleshy_evidence="synthetic body trait";
  n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  check(w.degeneration_hex(source,target,*effects::degeneration_hex(2998,12),100,20));box->drain();
  std::function<void()> publication;const bool accepted=w.ear_bite(source,target,*effects::ear_bite(10),20,200,&publication);
  if(mode==2){check(!accepted&&!publication&&box->drain().empty()&&w.npc_combat_snapshot(target)->health->current==1000);continue;}
  check(accepted&&publication&&box->drain().empty());
  const auto loss=effects::fragility(2998,12)->damage;const bool bleed=mode==0;
  check(w.npc_combat_snapshot(target)->health->current==(lethal?0:930-(bleed?loss:0)));
  check(w.has_condition(target,combat::conditions::Kind::bleeding,200)==bleed);
  publication();check(!box->drain().empty());publication();check(box->drain().empty());
 }
 std::cout<<"6Ear Bite damage/flesh/unknown/lethal cases and deferred publication pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
