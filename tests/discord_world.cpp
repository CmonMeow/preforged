#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Discord World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank)for(unsigned bits=0;bits<8;++bits){
  world::Instance w;w.join(1,"Necro",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  if(bits&1)check(w.apply_condition(source,target,combat::conditions::Kind::blind,1000,10000));
  if(bits&2)check(w.status_mutation(target,99,800,0,1000,11000,true));
  if(bits&4)check(w.status_mutation(target,38,801,0,1000,11000,true));
  std::vector<world::Packet> visuals;check(w.discord(source,target,*effects::discord(rank),20,1000,&visuals)&&visuals.size()==1);
  const double damage=((bits&1)&&(bits&6))?effects::damage_scale(30,110,rank):0;
  check(std::abs(double(w.resources(2)->health)-(1000-damage))<1.1);
 }
 for(unsigned bits=0;bits<4;++bits)for(unsigned rank:{0u,12u,20u}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Necro",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
  if(bits&1)check(w.blindness(source,target,*effects::blindness(220,0),1000));
  if(bits&2)check(w.degeneration_hex(source,target,*effects::degeneration_hex(31,1),1000));
  const auto before=w.npc_combat_snapshot(target)->health->current;std::vector<world::Packet> visuals;
  check(w.discord(source,target,*effects::discord(rank),20,1000,&visuals)&&visuals.size()==1);
  check(std::abs(w.npc_combat_snapshot(target)->health->current-(before-(bits==3?effects::damage_scale(30,110,rank):0)))<1e-6);
 }
 {
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Necro",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
  check(w.apply_condition(source,target,combat::conditions::Kind::blind,1000,10000)&&w.status_mutation(target,99,800,0,1000,11000,true));box->drain();
  std::vector<world::Packet> visuals;std::function<void()> publication;
  check(w.discord(source,target,*effects::discord(0),20,1000,&visuals,&publication)&&publication&&w.resources(2)->health==970);
  const auto damage=[](const auto& p){return p.size()>=18&&p[0]==162&&p[2]==55;};
  const auto before=box->drain();check(std::none_of(before.begin(),before.end(),damage));
  publication();const auto after=box->drain();check(std::count_if(after.begin(),after.end(),damage)==1);
  publication();check(box->drain().empty());
 }
 // Reversal is itself an enchantment, so blinded targets meet Discord's
 // predicate even without a hex. Missing conditions must remain a no-effect cast.
 for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(bool blind:{false,true}){
  world::Instance w;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health={double(hp),1000};d.damageable=true;d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});
  }else{w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);}
  w.join(1,"Necro",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
  if(blind){if(npc)check(w.blindness(source,target,*effects::blindness(220,0),100));else check(w.apply_condition(source,target,combat::conditions::Kind::blind,100,10000));}
  if(npc)check(w.stage_npc_reversal(target,rr,100));else check(w.status_mutation(target,307,5,rr,100,8100,true));
  std::vector<world::Packet> visuals;check(w.discord(source,target,*effects::discord(12),20,1000,&visuals)&&visuals.size()==1);
  const double raw=effects::damage_scale(30,110,12),converted=std::min(raw,double(effects::reversal_fortune(rr)->maximum_healing));
  const double expected=blind?std::max(0.,std::min(1000.,hp+converted)-(raw-converted)):hp;
  const double actual=npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;check(std::abs(actual-expected)<1.1);
  // A no-effect cast must retain Reversal for the next eligible Discord.
  if(!blind){
   if(npc)check(w.blindness(source,target,*effects::blindness(220,0),1000));else check(w.apply_condition(source,target,combat::conditions::Kind::blind,1000,10000));
   check(w.discord(source,target,*effects::discord(12),20,1000,&visuals));
   const double after=npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;
   check(std::abs(after-std::max(0.,std::min(1000.,hp+converted)-(raw-converted)))<1.1);
  }
 }
 std::cout<<"PASS Discord World168rank/predicate contexts,conditional shared damage and no-effect visuals\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
