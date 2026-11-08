#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Delayed damage Reversal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;skills::effects::Scope source,target;bool npc;
 Fixture(bool n,unsigned hp,float x,bool crowd=false):npc(n){
  std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){if(!crowd&&(!npc||id!=100))continue;world::Instance::Npc body;body.id=body.definition.id=id;body.x=id==100?x:x+166;body.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health={double(id==100?hp:1000u),1000};d.damageable=d.conditions_allowed=true;d.holy_traits.light_sensitive=false;d.light_sensitive_evidence="synthetic";
   d.lightning_armor=60;d.healing_allowed=true;d.healing_evidence="synthetic";body.combat_definition=d;population.push_back(body);
  }if(!population.empty())w.configure_npcs(population);
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc||crowd){w.join(2,"Foe",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);check(w.player_holy_equipment(*w.effect_scope(2),0)&&w.lightning_armor(*w.effect_scope(2),60));}
  if(crowd)for(unsigned id:{3u,4u}){w.join(id,"Side foe",x+(id==3?166:167),0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(id,{1000,1000,100,100},100);check(w.lightning_armor(*w.effect_scope(id),60));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);check(w.configure_projectiles({1800,32}));
 }
 double hp(unsigned id){return id<100?w.resources(id)->health:w.npc_combat_snapshot(*w.effect_scope(id))->health->current;}
 double hp(){return hp(target.actor);}
 void reversal(unsigned rank,unsigned at=1200,unsigned id=0){if(!id)id=target.actor;const auto scope=*w.effect_scope(id);if(id<100)check(w.status_mutation(scope,307,id+10,rank,at,at+8000,true));else check(w.stage_npc_reversal(scope,rank,at));}
 void launch(bool spear){std::vector<world::Packet> visuals;std::function<void()> publish;
  check(spear?w.spear_light(source,target,*skills::effects::spear_light(12),20,1000,&visuals,&publish):w.spirit_rift(source,target,*skills::effects::spirit_rift(12),20,1000,&visuals,&publish));
  w.skill_event(1,visuals,1000);publish();
 }
};
int main()try{unsigned spear_cases=0,rift_cases=0;
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(bool attacking:{false,true}){
  ++spear_cases;Fixture f(npc,hp,900);f.launch(true);
  if(attacking){if(npc)check(f.w.begin_npc_action(f.target,combat::npcs::control::Action::attack,1100,2500).has_value());else check(f.w.attack_activity(f.target,2000,1100));}
  f.reversal(rr);f.w.tick(1400);check(f.hp()==hp);f.w.tick(1500);
  const auto p=*skills::effects::spear_light(12);const double converted=std::min(double(p.damage),double(skills::effects::reversal_fortune(rr)->maximum_healing));
  double expected=std::max(0.,std::min(1000.,hp+converted)-(p.damage-converted));if(attacking&&expected>0)expected=std::max(0.,expected-p.attacking_bonus);
  check(std::abs(f.hp()-expected)<1.1);f.w.tick(1600);check(std::abs(f.hp()-expected)<1.1);
  if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(expected==0));
 }
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u}){
  ++rift_cases;Fixture f(npc,hp,600);f.launch(false);f.reversal(rr,2000);f.w.tick(3900);check(f.hp()==hp);f.w.tick(4000);
  const auto p=*skills::effects::spirit_rift(12);const double converted=std::min(p.damage,double(skills::effects::reversal_fortune(rr)->maximum_healing));const double expected=std::max(0.,std::min(1000.,hp+converted)-(p.damage-converted));
  check(std::abs(f.hp()-expected)<1.1&&f.w.has_condition(f.target,combat::conditions::Kind::cracked_armor,4000)==(expected>0));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==unsigned(expected==0));
 }
 for(bool npc:{false,true}){Fixture f(npc,10,600,true);f.launch(false);
  for(unsigned id:{2u,3u,100u,101u})f.reversal(id==f.target.actor?0:12,2000,id);f.w.tick(4000);
  for(unsigned id:{2u,3u,100u,101u}){const double before=id==2||id==100?10:1000,converted=id==f.target.actor?15:67;const double expected=std::max(0.,std::min(1000.,before+converted)-(105-converted));
   check(std::abs(f.hp(id)-expected)<1.1&&f.w.has_condition(*f.w.effect_scope(id),combat::conditions::Kind::cracked_armor,4000)==(expected>0));
  }check(f.hp(4)==1000);
 }
 for(bool spear:{false,true})for(bool npc:{false,true}){Fixture f(npc,10,spear?900.f:600.f);f.launch(spear);
  if(npc){check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,1100,10000));check(f.w.condition_transfer(f.source,f.target,*skills::effects::condition_transfer(132,12),1100));}
  else check(f.w.apply_condition(f.source,f.target,combat::conditions::Kind::deep_wound,1100,10000));
  f.reversal(12);f.w.tick(spear?1500:4000);check(f.hp()==0&&!f.w.has_condition(f.target,combat::conditions::Kind::cracked_armor,spear?1500:4000));
  if(npc)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 check(spear_cases==36&&rift_cases==18);std::cout<<"PASS36 Spear/18 Rift conversions,2 mixed fields,4 healing deaths;normal307 stays unbound\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
