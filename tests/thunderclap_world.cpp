#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Thunderclap World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source;
 Fixture(unsigned unknown=0,unsigned primary_health=1000){
  std::vector<world::Instance::Npc> population;
  for(unsigned id=100;id<=106;++id){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==101?266:id==102?267:100;n.plane=id==104?1:0;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
   d.relation=id==103?combat::npcs::Relation::protected_ally:combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{double(id==106?10:id==100?primary_health:1000),1000};d.damageable=id!=103;d.conditions_allowed=id!=103&&id!=105;
   if(id!=unknown&&id!=102){d.lightning_armor=80;d.lightning_armor_evidence="test";}n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);
  for(unsigned id=1;id<=5;++id){
   const float x=id==1?0:id==2?100:id==3?266:id==4?267:110;
   w.join(id,"Player",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:id==3?3:2);
   w.set_vitals(id,{1000,1000,100,100},100);
   check(w.lightning_armor(*w.effect_scope(id),id==unknown?std::nullopt:std::optional<int>{80}));
  }
  source=*w.effect_scope(1);box->drain();
 }
 skills::effects::Scope scope(unsigned id){return *w.effect_scope(id);}
 double health(unsigned id){return id>=100?w.npc_combat_snapshot(scope(id))->health->current:double(w.resources(id)->health);}
};
int main()try{
 using namespace skills;using combat::conditions::Kind;
 for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u}){
  Fixture f;const auto target=f.scope(npc?100:2);const auto plan=*effects::lightning(228,rank);std::vector<world::Packet> visuals;
  check(f.w.lightning(f.source,target,plan,20,1000,&visuals));
  for(unsigned id:{2u,3u,100u,101u,105u,106u}){
   const double expected=id==106?0:1000-plan.damage;
   if(std::abs(f.health(id)-(id<100?std::ceil(expected):expected))>=1e-7)std::cerr<<"recipient "<<id<<" rank "<<rank<<" actual "<<f.health(id)<<" expected "<<expected<<"\n";
   check(std::abs(f.health(id)-(id<100?std::ceil(expected):expected))<1e-7);
   const bool condition=id!=105&&id!=106;
   check(f.w.has_condition(f.scope(id),Kind::cracked_armor,1000)==condition);
   check(f.w.has_condition(f.scope(id),Kind::weakness,1000)==condition);
  }
  for(unsigned id:{1u,4u,5u,102u,103u,104u})check(f.health(id)==1000);
  check(!visuals.empty()&&visuals.front()==*particles::targeted_body_2022(target.actor,1,228));
  check(std::count(visuals.begin(),visuals.end(),visuals.front())==1);
 }
 {
  Fixture f(101);check(!f.w.lightning(f.source,f.scope(100),*effects::lightning(228,12),20,1000));
  check(f.health(100)==1000&&f.health(2)==1000);
 }
 for(bool npc:{false,true}){
  Fixture f;const auto target=f.scope(npc?100:2);const auto plan=*effects::lightning(228,12);
  const auto token=npc?f.w.begin_npc_action(target,combat::npcs::control::Action::spell,200,2000):std::optional<combat::npcs::control::Token>{};
  if(npc)check(bool(token));else check(f.w.cast_activity(target,205,200,2000));
  const auto before=f.w.control_epoch(target);
  check(f.w.lightning(f.source,target,plan,20,1000));
  if(npc)check(!f.w.complete_npc_action(*token,2000));
  else check(!f.w.casting_skill(target,1000)&&f.w.control_epoch(target)!=before);
 }
 for(unsigned id:{100u,105u})for(auto kind:{combat::npcs::control::Action::spell,combat::npcs::control::Action::attack}){
  Fixture f;const auto target=f.scope(id);const auto token=f.w.begin_npc_action(target,kind,200,2000);check(bool(token));
  check(f.w.lightning(f.source,target,*effects::lightning(228,12),20,1000));
  check(!f.w.complete_npc_action(*token,2000));
  check(f.w.has_condition(target,Kind::weakness,1000)==(id==100));
 }
 {
  Fixture f;const auto target=f.scope(2);
  check(f.w.lightning(f.source,target,*effects::lightning(228,12),20,1000));
  const auto cleanse=*effects::condition_removal(277,0,0);
  check(f.w.remove_conditions(target,2,cleanse,1100));
  check(!f.w.has_condition(target,Kind::weakness,1100)&&f.w.has_condition(target,Kind::cracked_armor,1100));
  check(f.w.remove_conditions(target,2,cleanse,1200));
  check(!f.w.has_condition(target,Kind::cracked_armor,1200));
 }
 for(unsigned hp:{50u,1000u}){
  Fixture f;const auto target=f.scope(2);f.w.set_vitals(2,{hp,1000,100,100},100);
  check(f.w.degeneration_hex(f.source,target,*effects::degeneration_hex(19,12),500));f.box->drain();
  check(f.w.lightning(f.source,target,*effects::lightning(228,12),20,1000));
  check(f.health(2)==(hp==50?0:924));
  std::vector<unsigned> gained;
  for(const auto& packet:f.box->drain())if(packet.size()==20&&packet[0]==65&&packet[1]==0&&packet[2]==2&&packet[3]==0&&packet[4]==0&&packet[5]==0)
   gained.push_back(unsigned(packet[6])|(unsigned(packet[7])<<8));
  check(gained==(hp==50?std::vector<unsigned>{2077}:std::vector<unsigned>{2077,486}));
 }
 for(unsigned stance:{16u,17u})for(bool active:{false,true}){
  Fixture f;const auto target=f.scope(2);
  check(f.w.status_mutation(target,stance,123,15,200,10000,true));
  if(active)check(f.w.cast_activity(target,205,200,2000));
  const auto before=f.w.control_epoch(target);const auto energy=f.w.resources(2)->energy;
  check(f.w.lightning(f.source,target,*effects::lightning(228,12),20,1000));
  check(f.health(2)==958&&f.w.has_condition(target,Kind::weakness,1000));
  check(f.w.control_epoch(target)==before);
  check(bool(f.w.casting_skill(target,1000))==active);
  if(!active)check(f.w.resources(2)->energy==energy&&f.w.status_snapshot(target,1000)->size()==1);
  if(active&&stance==16)check(f.w.status_snapshot(target,1000)->empty());
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  Fixture f(mode==6?101:0);const auto target=f.scope(npc?100:2);casting::Hooks h;unsigned commits=0;bool stale=false;

  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& values){f.w.skill_event(1,values);};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==target.actor)return std::optional<effects::Scope>{};return f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==target.actor&&range==1248;};
  if(mode!=4)h.lightning=[&](auto selected,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{*particles::targeted_body_2022(target.actor,1,228)}};
   std::vector<world::Packet> visuals;const bool applied=f.w.lightning(f.source,selected,plan,20,at,&visuals);return {applied,std::move(visuals)};
  };
  Slots bar{};bar[0]={228,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[228]=1;attributes::Ranks ranks{};ranks[8]=12;
  const auto before=f.w.resources(1)->energy;
  casting::Runtime cast(1);check(cast.start(1000,{228,0,target.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(f.w.resources(1)->energy==before-(mode==4?0:10));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;

  cast.update(1999,h);check(!commits);cast.update(2000,h);check(commits==unsigned(mode==0||mode>=5));
  const bool success=mode==0;check(f.health(100)==(success?958:1000));
  const auto packets=f.box->drain();const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  const auto visual=*particles::targeted_body_2022(target.actor,1,228);
  check(std::count(packets.begin(),packets.end(),visual)==unsigned(success));
  if(success){
   const auto particle=std::find(packets.begin(),packets.end(),visual);check(finish<particle);
   unsigned modifiers=0;
   for(auto it=packets.begin();it!=packets.end();++it)if(it->size()==18&&(*it)[0]==162&&(*it)[1]==0){
    check(particle<it);++modifiers;
   }
   check(modifiers>=5);
  }
 }
 std::cout<<"PASS Thunderclap mixed World recipients,active action interruption,condition immunity and actual Runtime success/failure paths\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
