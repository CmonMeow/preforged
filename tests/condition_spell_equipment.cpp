#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Equipped condition spell line "+std::to_string(at.line()));
}
inventory::Profile gear(bool disease,bool prefix,bool rune,bool inscription){
 auto p=inventory::baseline();const auto add=[&](unsigned id,unsigned type,unsigned slot,unsigned word){
  inventory::Item item;item.definition.id=id;item.definition.type=type;item.definition.quantity=1;
  item.bag=2;item.slot=slot;item.definition.modifiers={word};p.items.emplace(id,item);
 };
 if(prefix){add(10,26,0,0x246801e6);p.weapons.sets[0].weapon=10;}
 if(rune)add(11,7,2,disease?0x27780605:0x27780801);
 if(inscription){add(12,24,1,disease?0x247801e3:0x247801e6);p.weapons.sets[0].offhand=12;}
 return p;
}
void player(world::Instance& w,unsigned id,float x,unsigned team){
 w.join(id,"Actor",x,0,std::make_shared<world::Outbox>(),0,{}, {},100,team);w.set_vitals(id,{500,1000,100,100},100);
}
int main()try{
 unsigned cases=0;
 for(unsigned skill:{117u,2859u,118u,2885u,2229u})for(unsigned mode=0;mode<6;++mode){
  ++cases;world::Instance w;player(w,1,0,1);player(w,2,100,2);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);const bool disease=skill==2229;
  const auto plan=disease?*effects::signet_infection_condition(7):*effects::condition_spell(skill,12);
  if(disease)check(w.apply_condition(source,target,combat::conditions::Kind::bleeding,150,10000));
  check(w.observe_condition_equipment(source,equipment::condition_equipment(gear(false,!disease&&(mode==0||mode==5),false,false),1),200));
  check(w.observe_condition_equipment(target,mode==4?std::nullopt:equipment::condition_equipment(gear(disease,false,mode==1||mode==2||mode==5,mode==2||mode==3),1),200));
  const bool result=w.condition_spell_equipped(source,target,plan,200);
  if(mode==4||(mode==5&&!disease)){check(!result&&!w.has_condition(target,plan.kind,200));continue;}
  unsigned duration=0;
  if(disease)duration=mode==0?20000:mode==2?12000:16000;
  else if(skill==117||skill==2859)duration=mode==0?35000:mode==2?16000:21000;
  else if(skill==118)duration=mode==0?23000:mode==2?11000:14000;
  else duration=mode==0?17000:mode==2?7000:10000;
  check(result&&w.has_condition(target,plan.kind,200+duration-1)&&!w.has_condition(target,plan.kind,200+duration));
 }
 for(bool known:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={500,1000};d.damageable=d.conditions_allowed=true;
  n.combat_definition=d;w.configure_npcs({n});player(w,1,0,1);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  check(w.observe_condition_equipment(source,known?equipment::condition_equipment(gear(false,true,false,false),1):std::nullopt,100));
  check(w.condition_spell_equipped(source,target,*effects::condition_spell(117,12),200)==known);
  if(known)check(w.has_condition(target,combat::conditions::Kind::weakness,35199)&&!w.has_condition(target,combat::conditions::Kind::weakness,35200));
  else check(!w.has_condition(target,combat::conditions::Kind::weakness,200));
 }
 // A later unknown recipient cannot leave an earlier area condition applied.
 {world::Instance w;player(w,1,0,1);player(w,2,100,2);player(w,3,150,2);
  const auto source=*w.effect_scope(1),a=*w.effect_scope(2),b=*w.effect_scope(3);
  for(auto scope:{source,a})check(w.observe_condition_equipment(scope,equipment::condition_equipment(inventory::baseline(),0),100));
  check(!w.condition_spell_equipped(source,a,*effects::condition_spell(118,12),200));
  check(!w.has_condition(a,combat::conditions::Kind::weakness,200)&&!w.has_condition(b,combat::conditions::Kind::weakness,200));
 }
 check(cases==30);std::cout<<"30equipped Weakness/Disease timers,2NPC source-equipment cases and area rollback pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
