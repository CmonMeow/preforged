#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Spear World line "+std::to_string(at.line()));}
int main()try{
 unsigned cases=0;
 for(bool npc:{false,true})for(unsigned rank=0;rank<=20;++rank)for(unsigned mode=0;mode<5;++mode){
  world::Instance w;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=900;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;d.holy_traits.light_sensitive=false;d.light_sensitive_evidence="test";
   n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Foe",900,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);check(w.player_holy_equipment(*w.effect_scope(2),0));
  check(w.configure_projectiles({1800,32}));std::vector<world::Packet> visuals;std::function<void()> publish;
  auto selected=target;if(mode==4)++selected.generation;
  check(w.spear_light(owner,selected,*skills::effects::spear_light(rank),20,1000,&visuals,&publish)==(mode!=4));
  const auto hp=[&](){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);};
  if(mode==4){check(visuals.empty()&&!publish&&hp()==1000);continue;}
  check(visuals.size()==1&&publish&&hp()==1000);
  if(mode==1){publish();w.tick(1500);check(hp()==1000);w.leave(1);check(w.configure_projectiles({1800,32}));continue;} // Unpublished visual cannot resolve damage.
  w.skill_event(1,visuals,1000);publish();
  if(mode==2&&!npc)check(w.attack_activity(target,2000,1100));
  if(mode==3){w.leave(1);w.tick(1500);check(hp()==1000);continue;}
  w.tick(1400);check(hp()==1000);w.tick(1500);
  const auto expected=1000-(26+2*rank)-(mode==2&&!npc?15+3*rank:0);
  check(hp()==expected);w.tick(1600);check(hp()==expected);++cases;
 }
 {
  world::Instance w;w.join(1,"Monk",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Foe",900,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.player_holy_equipment(target,0)&&w.configure_projectiles({1800,32}));
  std::vector<world::Packet> visuals;std::function<void()> publish;
  check(w.spear_light(source,target,*skills::effects::spear_light(0),20,1000,&visuals,&publish));
  w.skill_event(1,visuals,1200);publish();w.tick(1600);check(w.resources(2)->health==1000);
  w.tick(1700);check(w.resources(2)->health==974);
 }
 std::cout<<"PASS Spear World "<<cases<<" delayed-impact contexts plus publication/lifetime/rejection cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

