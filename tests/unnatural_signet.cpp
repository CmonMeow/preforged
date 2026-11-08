#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Unnatural line "+std::to_string(at.line()));}
int main()try{
 for(bool reversal:{false,true}){
  world::Instance w;for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",float((id-1)*60),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2),side=*w.effect_scope(3);
  check(w.status_mutation(target,1399,80,12,100,6100,true));check(w.status_mutation(side,1399,82,12,100,6100,true));
  if(reversal)check(w.status_mutation(target,307,81,12,100,8100,true));
  const auto plan=*unnatural_signet(12);check(w.unnatural_signet(source,target,plan,20,100));
  check(w.resources(2)->health==(reversal?1000u:942u)&&w.resources(3)->health==964);
  check(w.unnatural_signet(source,target,plan,20,101));
  check(w.resources(2)->health==(reversal?947u:889u)&&w.resources(3)->health==933);
 }

 check(unnatural_signet(0)->primary==15&&unnatural_signet(12)->primary==63&&unnatural_signet(12)->adjacent==41&&unnatural_signet(20)->adjacent==65&&!unnatural_signet(21));
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id=1;id<=5;++id){float x=id==1?0:id==2?100:id==3?266:id==4?266.1f:100;
   w.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);
   w.set_vitals(id,{mode==4&&id==2?10u:1000u,1000,100,100},100);
  }
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);
  if(mode==1||mode==3||mode==4)check(w.degeneration_hex(owner,target,*degeneration_hex(129,0),100));
  if(mode==2)check(w.armor_earth(target,0,100));
  auto forged=*unnatural_signet(12);++forged.adjacent;check(!w.unnatural_signet(owner,target,forged,20,100));
  auto stale=target;++stale.generation;check(!w.unnatural_signet(owner,stale,*unnatural_signet(12),20,100));
  check(!w.unnatural_signet(owner,*w.effect_scope(5),*unnatural_signet(12),20,100));
  if(mode==5)check(w.settle_resources(*w.effect_scope(3),200)); // irrelevant future adjacent cursor must not veto a primary-only hit
  box->drain();const auto at=mode==3?20000u:100u;
  check(w.unnatural_signet(owner,target,*unnatural_signet(12),20,at));
  const bool area=mode==1||mode==2||mode==4;
  check(w.resources(2)->health==(mode==4?0u:937u)); // primary never takes the secondary hit
  check(w.resources(3)->health==(area?959u:1000u)&&w.resources(4)->health==1000&&w.resources(5)->health==1000);
  const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),*particles::targeted_body_2022(2,1,934))==1);
 }
 for(unsigned mode=0;mode<3;++mode){
  world::Instance w;std::vector<world::Instance::Npc> defs;
  for(unsigned id:{100u,101u,102u}){world::Instance::Npc n;n.id=n.definition.id=id;n.definition.visual_adjustment={0,0,0,100};
   n.x=id==100?100:id==101?266:266.1f;combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
   d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=true;d.spawn_health=combat::Health{mode==2&&id==100?10.:1000.,1000};
   d.conditions_allowed=true;n.combat_definition=d;defs.push_back(n);
  }
  w.configure_npcs(defs);w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);
  if(mode)check(w.degeneration_hex(owner,target,*degeneration_hex(129,0),100));
  check(w.unnatural_signet(owner,target,*unnatural_signet(12),20,100));
  check(w.npc_combat_snapshot(target)->health->current==(mode==2?0:937));
  check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==(mode?959:1000));
  check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==1000);
 }
 for(unsigned mode=0;mode<6;++mode){
  world::Instance w;for(unsigned id=1;id<=3;++id){w.join(id,"Actor",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto owner=*w.effect_scope(1),target=*w.effect_scope(2);check(w.degeneration_hex(owner,target,*degeneration_hex(129,0),100));
  casting::Runtime cast(1);casting::Hooks h;std::vector<Packet> wire;
  h.events=[&](const auto& batch){wire.insert(wire.end(),batch.begin(),batch.end());};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto amount,auto at){return w.consume_energy(1,amount,owner,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return w.projectile_target_valid(owner,id,range,100);};
  h.unnatural_signet=[&](auto foe,const auto& plan,auto event,auto at){return w.unnatural_signet(owner,foe,plan,20,at,event);};
  Policy p;p.primary=5;p.campaigns=255;p.learned[934]=1;p.pvp_match=mode==4;Slots bar{};bar[0]={934,0};attributes::Ranks ranks{};ranks[2]=12;
  if(mode==5)h.unnatural_signet={};
  const auto reason=cast.start(100,{934,0,2},bar,p,ranks,true,h);
  if(mode>=4){check(reason==casting::Reason::unsupported&&w.resources(1)->energy==100);continue;}
  check(reason==casting::Reason::accepted);if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(500,h);
  if(mode==3){w.leave(2);w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  cast.update(1099,h);check(w.resources(3)->health==1000);cast.update(1100,h);
  check(w.resources(2)->health==(mode?1000u:937u)&&w.resources(3)->health==(mode?1000u:959u));
  if(!mode)check(cast.start(2000,{934,0,2},bar,p,ranks,true,h)==casting::Reason::recharge);
 }
 std::cout<<"PASS Unnatural PvE primary-only/conditional other-adjacent damage,player/NPC,kill,radius,cast/lifetime/cancel;PvP missing-event hooks rejected\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
