#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Rift World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank)for(unsigned mode=0;mode<5;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=650;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;d.lightning_armor=60;d.lightning_armor_evidence="test";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u,4u}){
   w.join(id,"Actor",id==1?0:id==2?600:id==3?766:767,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
   w.set_vitals(id,{1000,1000,100,100},100);check(w.lightning_armor(*w.effect_scope(id),60));
  }
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);std::vector<world::Packet> visuals;std::function<void()> publish;
  auto selected=target;if(mode==4)++selected.generation;
  check(w.spirit_rift(source,selected,*effects::spirit_rift(rank),20,1000,&visuals,&publish)==(mode!=4));
  if(mode==4){check(visuals.empty()&&!publish);continue;}
  check(visuals.size()==1&&publish);
  if(mode==1){publish();w.tick(5000);check(w.resources(2)->health==1000);continue;}
  w.skill_event(1,visuals,mode==2?1200:1000);publish();
  if(mode==3){w.leave(2);w.join(2,"Reused elsewhere",1000,0,std::make_shared<world::Outbox>(),0,{}, {},1500,2);w.set_vitals(2,{1000,1000,100,100},100);check(w.lightning_armor(*w.effect_scope(2),60));}
  const unsigned due=mode==2?4200:4000;w.tick(due-100);check(w.resources(3)->health==1000);w.tick(due);
  const double damage=25.+100.*rank/15.;
  check(std::abs(double(w.resources(3)->health)-(1000-damage))<1.1);check(w.resources(4)->health==1000&&w.resources(1)->health==1000);
  check(std::abs(w.npc_combat_snapshot(*w.effect_scope(100))->health->current-(1000-damage))<1e-6);
  if(mode==3)check(w.resources(2)->health==1000); // Original target does not drag or erase the ground field.
  const auto hp=w.resources(3)->health;w.tick(due+100);check(w.resources(3)->health==hp);
 }
 {
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Armored foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.lightning_armor(target,100));
  check(w.apply_condition(source,target,combat::conditions::Kind::cracked_armor,500,10000));
  std::vector<world::Packet> visuals;std::function<void()> publish;
  check(w.spirit_rift(source,target,*effects::spirit_rift(15),20,1000,&visuals,&publish));
  w.skill_event(1,visuals,1000);publish();box->drain();w.tick(4000);
  check(std::abs(double(w.resources(2)->health)-(1000-125/std::sqrt(2.)))<1.1); // Exactly one20armor reduction.
  const auto packets=box->drain();auto damage=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>1&&p[0]==162;});
  auto condition=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=20&&p[0]==65&&p[6]==29&&p[7]==8;});
  check(damage!=packets.end()&&condition!=packets.end()&&damage<condition);
 }
 std::cout<<"PASS Rift World105rank/mode cases,fixed center,adjacent boundary,delayed publication,NPC/player damage,retired target and one-shot delivery\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
