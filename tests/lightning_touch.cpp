#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Lightning Touch line "+std::to_string(at.line()));
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
  Fixture f;const auto plan=*effects::lightning(232,rank);std::vector<world::Packet> visuals;
  check(f.w.lightning(f.source,f.scope(npc?100:2),plan,20,1000,&visuals));
  for(unsigned id:{2u,3u,100u,101u,105u}){
   const double expected=1000-plan.damage;check(std::abs(f.health(id)-(id<100?std::ceil(expected):expected))<1e-7);
   const bool conditions=id!=105;
   check(f.w.has_condition(f.scope(id),Kind::blind,1000)==conditions&&f.w.has_condition(f.scope(id),Kind::cracked_armor,1000)==conditions);
   if(conditions){check(f.w.has_condition(f.scope(id),Kind::blind,1000+plan.blind_ms-1));check(!f.w.has_condition(f.scope(id),Kind::blind,1000+plan.blind_ms));}
  }
  for(unsigned id:{1u,4u,5u,102u,103u,104u})check(f.health(id)==1000);
  check(f.health(106)==0&&!f.w.has_condition(f.scope(106),Kind::blind,1000));
  check(visuals.size()==6);
  for(unsigned id:{2u,3u,100u,101u,105u,106u})check(std::count(visuals.begin(),visuals.end(),*particles::targeted_body_2022(id,1,232))==1);
 }
 for(unsigned unknown:{3u,101u}){
  Fixture f(unknown);check(!f.w.lightning(f.source,f.scope(100),*effects::lightning(232,12),20,1000));
  for(unsigned id:{2u,3u,100u,101u,105u})check(f.health(id)==1000&&!f.w.has_condition(f.scope(id),Kind::blind,1000));check(f.health(106)==10);
 }
 {
  Fixture f;auto target=f.scope(100);++target.generation;check(!f.w.lightning(f.source,target,*effects::lightning(232,12),20,1000));
  auto forged=*effects::lightning(232,12);++forged.radius;check(!f.w.lightning(f.source,f.scope(100),forged,20,1000));
  check(!f.w.lightning(f.source,f.scope(101),*effects::lightning(232,12),20,1000));check(f.health(100)==1000);
 }
 {
  Fixture f;const auto plan=*effects::lightning(232,12);check(f.w.lightning(f.source,f.scope(100),plan,20,1000));check(f.w.lightning(f.source,f.scope(100),plan,20,2000));
  check(std::abs(f.health(100)-(950-50*std::exp2(15./40.)))<1e-7&&f.health(105)==900);
 }
 {
  Fixture f(0,55);check(f.w.degeneration_hex(f.source,f.scope(100),*effects::degeneration_hex(19,12),500));
  check(f.w.lightning(f.source,f.scope(100),*effects::lightning(232,12),20,1000));
  check(f.health(100)==0&&!f.w.has_condition(f.scope(100),Kind::cracked_armor,1000));
  check(f.health(101)==950-2*effects::fragility(19,12)->damage); // Area Fragility also covered this neighbor;both conditions trigger it.
 }
 {
  Fixture f;f.w.set_vitals(2,{10,1000,100,100},100);f.w.set_vitals(3,{900,1000,100,100},100);
  check(f.w.degeneration_hex(f.scope(3),f.scope(2),*effects::degeneration_hex(99,12),500));
  check(f.w.lightning(f.source,f.scope(2),*effects::lightning(232,12),20,1000));
  check(f.health(2)==0&&f.health(3)==950); // Return healing must survive a later recipient's damage commit.
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  Fixture f(mode==6?101:0);const auto target=f.scope(npc?100:2);casting::Hooks h;unsigned commits=0;bool stale=false;
  h.resources=[&](unsigned actor){return f.w.resources(actor);};h.events=[&](const auto& values){f.w.skill_event(1,values);};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned actor){auto scope=f.w.effect_scope(actor);if(stale&&actor==target.actor)return std::optional<effects::Scope>{};return scope;};
  h.projectile_target_valid=[&](unsigned actor,float range){return actor==target.actor&&range==144;};
  if(mode!=4)h.lightning=[&](auto selected,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{*particles::targeted_body_2022(target.actor,1,232)}};
   std::vector<world::Packet> visuals;const bool accepted=f.w.lightning(f.source,selected,plan,20,at,&visuals);return {accepted,std::move(visuals)};
  };
  Slots bar{};bar[0]={232,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[232]=1;attributes::Ranks ranks{};ranks[8]=12;
  casting::Runtime cast(1);check(cast.start(1000,{232,0,target.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==4){check(f.w.resources(1)->energy==100);continue;}check(f.w.resources(1)->energy==95);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1749,h);check(!commits);cast.update(1750,h);check(commits==unsigned(mode==0||mode>=5));
  check(f.health(100)==(mode==0?950:1000));const auto packets=f.box->drain();
  const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  const auto particle=*particles::targeted_body_2022(target.actor,1,232);const auto effect=std::find(packets.begin(),packets.end(),particle);
  check((finish!=packets.end())==(mode==0)&&std::count(packets.begin(),packets.end(),particle)==unsigned(mode==0));
  if(mode==0)for(unsigned id:{2u,3u,100u,101u,105u,106u}){
   const auto visual=*particles::targeted_body_2022(id,1,232);
   check(std::count(packets.begin(),packets.end(),visual)==1&&finish<std::find(packets.begin(),packets.end(),visual));
  }
 }
 std::cout<<"PASS Lightning Touch mixed recipient boundaries,immunity,damage/condition ordering,atomic rejection,Fragility,return healing and real casts\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
