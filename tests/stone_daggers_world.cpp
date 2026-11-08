#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <string>
#include <fstream>
#include <filesystem>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Stone Daggers World line "+std::to_string(at.line()));}
std::shared_ptr<const navigation::Mesh> corner_mesh(){
 const auto path=std::filesystem::temp_directory_path()/"stone-daggers-predicted-wall.fwnav";
 std::ofstream out(path,std::ios::binary|std::ios::trunc);
 auto word=[&](std::uint32_t n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
 auto real=[&](float n){word(std::bit_cast<std::uint32_t>(n));};
 out.write("FWNAV001",8);for(auto n:{1u,7u,0xEEFE704cu,12u,1u,1u})word(n);
 for(float n:{0.f,0.f,1000.f,1000.f})real(n);word(1);word(3);
 for(unsigned i=0;i<3;++i){word(i?i-1:0xffffffff);word(i<2?i+1:0xffffffff);for(unsigned j=0;j<3;++j)word(0xffffffff);
  const float left=i==0?0.f:i==1?100.f:200.f,right=i==0?100.f:i==1?200.f:1000.f;
  for(float n:{0.f,i==0?10.f:1000.f,left,right,left,right})real(n);
 }
 out.close();return std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,target;bool npc;
 Fixture(bool n,bool armor=true,bool conditions=true):npc(n){
  if(npc){world::Instance::Npc actor;actor.id=actor.definition.id=100;actor.x=600;actor.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=conditions;
   d.fleshy=true;d.fleshy_evidence="synthetic living target";
   if(armor){d.earth_armor=60;d.evidence.armor="test";}actor.combat_definition=d;w.configure_npcs({actor});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);source=*w.effect_scope(1);
  if(!npc){w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);check(w.combat_armor(*w.effect_scope(2),armor?std::optional<int>{60}:std::nullopt));}
  target=*w.effect_scope(npc?100:2);box->drain();
 }
 void overcast(){check(w.activate_overcast_energy(source,219,0,1,200));}
 double health(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 bool cast(std::uint64_t at=1000){return w.stone_daggers(source,target,*effects::stone_daggers(15),20,at);}
 unsigned launches(){const auto p=box->drain();return unsigned(std::count_if(p.begin(),p.end(),[](const auto& b){return b.size()>1&&b[0]==163;}));}
};
int main()try{
 for(bool npc:{false,true})for(bool overcast:{false,true}){
  Fixture f(npc);if(overcast)f.overcast();check(f.cast());check(f.launches()==1&&f.health()==1000);
  f.w.tick(1234);check(f.health()==1000);f.w.tick(1334);check(f.health()==967);
  check(f.w.has_condition(f.target,combat::conditions::Kind::bleeding,1334)==overcast);
  f.w.tick(1434);check(f.launches()==0);f.w.tick(1534);check(f.launches()==1);f.w.tick(1634);check(f.launches()==0);
  const auto before=f.health();f.w.tick(1868);check(f.health()<=before-33&&f.health()>=930);
  check(f.w.has_condition(f.target,combat::conditions::Kind::bleeding,1868)==overcast);
  f.w.tick(1968);check(f.launches()==0);
 }
 for(bool npc:{false,true}){
  Fixture missing(npc,false);check(!missing.cast()&&missing.launches()==0);
  Fixture f(npc);check(f.cast());f.w.leave(1);f.w.tick(4000);check(f.health()==1000);
  Fixture dead(npc);check(dead.cast()&&dead.w.kill(dead.source,1100));dead.w.tick(1334);check(dead.health()==967);dead.w.tick(1500);dead.w.tick(1834);check(dead.health()==934);
  Fixture late(npc);check(late.cast());late.w.tick(2500);check(late.health()==934);late.w.tick(2501);check(late.health()==934);late.w.tick(2834);check(late.health()==934);
 }
 {Fixture f(true,true,false);f.overcast();check(f.cast());f.w.tick(1334);check(f.health()==967&&!f.w.has_condition(f.target,combat::conditions::Kind::bleeding,1334));}
 {Fixture f(false);check(f.cast());f.w.move(2,600,500,1050);f.w.tick(1334);check(f.health()==1000);f.w.tick(1500);f.w.tick(1834);check(f.health()==1000);}
 {Fixture f(false);check(f.cast());f.w.leave(2);f.w.join(2,"Replacement",600,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);f.w.set_vitals(2,{1000,1000,100,100},100);f.w.tick(3000);check(f.w.resources(2)->health==1000);}
 // A1500 input settles the1334 impact before replacing movement/resource
 // history. An older sampled tick cannot postpone or replay that delivery.
 {Fixture f(false);check(f.cast());f.w.move(2,600,0,1500);check(f.health()==967);f.w.tick(1400);check(f.health()==967);f.w.tick(1500);check(f.health()==967);}
 {Fixture f(false);check(f.cast());check(f.w.settle_resources(f.target,1500));check(f.health()==967);f.w.tick(1400);check(f.health()==967);f.w.tick(1500);check(f.health()==967);}
 for(bool npc:{false,true}){Fixture f(npc);std::vector<world::Packet> visuals;
  check(f.w.stone_daggers(f.source,f.target,*effects::stone_daggers(15),20,1000,&visuals)&&visuals.size()==1);
  f.w.tick(2000);check(f.health()==1000&&f.launches()==0);
  f.w.skill_event(1,visuals);check(f.launches()==0); // No trusted publication time: cannot open a flight.
  auto forged=visuals;forged[0][6]^=1;f.w.skill_event(1,forged,2000);check(f.launches()==0);
  f.w.skill_event(1,visuals,2000);check(f.launches()==1);f.w.skill_event(1,visuals,2001);check(f.launches()==0);
  f.w.tick(2234);check(f.health()==1000);f.w.tick(2334);check(f.health()==967);f.w.tick(2500);f.w.tick(2834);check(f.health()==934);
 }
 {world::Instance w;auto box=std::make_shared<world::Outbox>();w.configure_navigation(corner_mesh());
  w.join(1,"Caster",5,9,box,0,{}, {},100,1);w.join(2,"Foe",105,9,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  for(unsigned id:{1u,2u})w.set_vitals(id,{1000,1000,100,100},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.combat_armor(target,60));
  w.steer(2,0,1,1000);box->drain();check(w.projectile_target_valid(source,2,1248,1000));
  check(!w.stone_daggers(source,target,*effects::stone_daggers(15),20,1000));
  w.tick(3000);check(w.resources(2)->health==1000);
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  Fixture f(npc);casting::Hooks h;unsigned commits=0,payments=0;bool stale=false;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){f.w.skill_event(1,p,2000);};
  h.consume_energy_at=[&](auto cost,auto at){++payments;check(cost==5);return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](auto id){return stale&&id==f.target.actor?std::optional<effects::Scope>{}:f.w.effect_scope(id);};
  h.projectile_target_valid=[&](auto id,float range){return id==f.target.actor&&range==1248;};
  if(mode!=4)h.stone_daggers=[&](auto target,const auto& p,auto at)->casting::EffectCompletion{++commits;if(mode==5)return {false,{{163,0}}};std::vector<world::Packet> out;const bool ok=f.w.stone_daggers(f.source,target,p,20,at,&out);return {ok,std::move(out)};};
  Slots bar{};bar[0]={172,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[172]=1;attributes::Ranks ranks{};ranks[9]=12;casting::Runtime cast(1);
  check(cast.start(1000,{172,0,f.target.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(payments==unsigned(mode!=4));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(2000,h);check(commits==unsigned(mode==0||mode==5));const auto packets=f.box->drain();
  const auto launch=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>1&&p[0]==163;});check((launch!=packets.end())==(mode==0));
  if(mode==0){const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));const auto body=std::find(packets.begin(),packets.end(),casting::generic(21,1,307));check(finish<body&&body<launch);}
  f.w.tick(2334);check(f.health()==(mode==0?972:1000));f.w.tick(2500);f.w.tick(2834);check(f.health()==(mode==0?944:1000));
 }
 std::cout<<"PASS Stone Daggers player/NPC twin releases,damage,Bleeding,scope/movement/death/late publication and Runtime payment/cancel/interrupt/native ordering\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
