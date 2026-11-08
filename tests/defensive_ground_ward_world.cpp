#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ward938 World line "+std::to_string(at.line()));
}
world::Instance::Npc ward_npc(unsigned id,bool spirit=false,bool known=true){
 world::Instance::Npc n;n.id=n.definition.id=id;n.definition.visual_adjustment={0,0,0,100};n.x=100;
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
 d.relation=id==100?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
 d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
 if(known){d.creature_type=spirit?"spirit":"human";d.creature_type_evidence="authored ward eligibility fixture";}
 n.combat_definition=d;return n;
}
int main()try{
 unsigned cases=0;
 for(unsigned mode=0;mode<9;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();
  w.join(1,"Ward owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.join(2,"Ally",mode==1?323:322,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{1000,1000,100,100},100);
  w.join(3,"Foe",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(2),foe=*w.effect_scope(3);
  const auto plan=*defensive_ground_ward_plan(938,0);std::vector<world::Packet> visuals;
  check(w.apply_defensive_ward(owner,plan,100,&visuals));check(visuals.size()==2);
  check(visuals[0]==skills::particles::ward_stability_ground_2022(0,0,0));
  check(!w.ward_knockdown_protection(foe,200));
  if(mode==2)check(w.apply_defensive_ward(owner,plan,100));
  if(mode==3)w.leave(1);
  if(mode==4)w.set_vitals(1,{0,1000,100,100},100);
  if(mode==5){w.leave(1);w.join(1,"New owner",900,0,box,0,{}, {},200,1);w.set_vitals(1,{1000,1000,100,100},100);check(!w.ward_knockdown_protection(*w.effect_scope(1),200));}
  if(mode==6){w.leave(2);w.join(2,"New ally",322,0,std::make_shared<world::Outbox>(),0,{}, {},200,1);w.set_vitals(2,{1000,1000,100,100},100);}
  const auto recipient=*w.effect_scope(2);const auto at=mode==7?10100u:mode==8?10099u:1000u;
  const bool protected_now=mode!=1&&mode!=7;
  check(w.ward_knockdown_protection(recipient,at)==protected_now);
  if(mode==6)check(!w.ward_knockdown_protection(ally,at));
  check(w.gale(foe,recipient,12,{},at));check(w.knocked_down(recipient,at)==!protected_now);
  ++cases;
 }
 for(unsigned mode=0;mode<5;++mode){world::Instance w;
  w.configure_npcs({ward_npc(100,mode==1,mode!=2),ward_npc(101)});
  w.join(1,"Owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto owner=*w.effect_scope(1),ally=*w.effect_scope(100),foe=*w.effect_scope(101);
  check(w.apply_defensive_ward(owner,*defensive_ground_ward_plan(938,0),100));
  if(mode==3){w.join(2,"Opposing team",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  if(mode==4)w.leave(1); // No live team authority for an unowned NPC.
  check(w.ward_knockdown_protection(ally,1000)==(mode==0));
  check(!w.ward_knockdown_protection(foe,1000)&&!w.ward_knockdown_protection(ally,10100));++cases;
 }
 {world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  auto owner=*w.effect_scope(1);auto plan=*defensive_ground_ward_plan(938,0);auto forged=plan;++forged.radius;
  check(!w.apply_defensive_ward(owner,forged,100));check(w.apply_defensive_ward(owner,plan,100));box->drain();
  w.leave(1);w.join(1,"Reused",0,0,box,0,{}, {},200,1);w.set_vitals(1,{1000,1000,100,100},100);box->drain();
  check(!w.apply_defensive_ward(owner,plan,200));check(w.ward_knockdown_protection(*w.effect_scope(1),200));
  w.tick(10100);const auto packets=box->drain();
  check(std::none_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==10&&p[0]==67&&p[1]==0&&p[2]==1&&p[3]==0&&p[4]==0&&p[5]==0;}));++cases;
 }
 std::cout<<"Ward938 World "<<cases<<" player/control, NPC eligibility, fixed lifetime and scope cases passed; paid Runtime absent\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
