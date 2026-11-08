#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Shell Shock line "+std::to_string(at.line()));
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
 for(unsigned rank=0;rank<=20;++rank){const auto plan=*effects::lightning(2059,rank);
  check(plan.damage==effects::damage_scale(10,30,rank)&&plan.penetration==25&&plan.range==1248&&plan.radius==0&&plan.blind_ms==0&&plan.cracked_ms==(5+rank)*1000);}
 check(!effects::lightning(2059,21));
 for(bool npc:{false,true})for(bool expanded:{false,true})for(unsigned rank:{0u,12u,20u}){
  Fixture f;const auto target=f.scope(npc?100:2);const auto plan=*effects::lightning(2059,rank);std::vector<world::Packet> visuals;
  if(expanded)check(f.w.activate_overcast_energy(f.source,219,0,1,200));
  check(f.w.lightning(f.source,target,plan,20,1000,&visuals));
  for(unsigned id:{2u,3u,100u,101u,105u,106u}){
   const bool hit=id==target.actor||expanded;const bool condition=hit&&id!=105&&id!=106;
   const double expected=id==106?(hit?0:10):1000-(hit?plan.damage:0);
   check(std::abs(f.health(id)-(id<100?std::ceil(expected):expected))<1e-7);
   check(f.w.has_condition(f.scope(id),Kind::cracked_armor,1000)==condition);
   check(!f.w.has_condition(f.scope(id),Kind::blind,1000));
   const auto particle=*particles::targeted_body_2022(id,1,2059);
   check(std::count(visuals.begin(),visuals.end(),particle)==unsigned(hit));
  }
  for(unsigned id:{1u,4u,5u,102u,103u,104u})check(f.health(id)==1000);
  check(visuals.size()==(expanded?6:1));
 }
 for(bool expanded:{false,true}){
  Fixture f(101);if(expanded)check(f.w.activate_overcast_energy(f.source,219,0,1,200));
  check(f.w.lightning(f.source,f.scope(100),*effects::lightning(2059,12),20,1000)==!expanded);
  check(f.health(100)==(expanded?1000:974)); // Unknown secondary armor matters only when actually in the recipient set.
 }
 {
  Fixture f;const auto plan=*effects::lightning(2059,12);
  check(f.w.lightning(f.source,f.scope(100),plan,20,1000));
  check(f.w.lightning(f.source,f.scope(100),plan,20,2000));
  check(std::abs(f.health(100)-(974-26*std::exp2(15./40.)))<1e-7); // Cracked Armor affects the next hit,not the first.
  check(f.w.has_condition(f.scope(100),Kind::cracked_armor,18999)&&!f.w.has_condition(f.scope(100),Kind::cracked_armor,19000));
 }
 {
  Fixture f;check(f.w.activate_overcast_energy(f.source,219,0,1,200));
  check(f.w.lightning(f.source,f.scope(100),*effects::lightning(2059,12),20,100000));
  check(f.health(100)==974&&f.health(2)==1000&&f.health(101)==1000);
 }
 {
  Fixture f;auto forged=*effects::lightning(2059,12);forged.radius=166;
  check(!f.w.lightning(f.source,f.scope(100),forged,20,1000));
  auto stale=f.scope(100);++stale.generation;
  check(!f.w.lightning(f.source,stale,*effects::lightning(2059,12),20,1000));check(f.health(100)==1000);
 }
 {
  Fixture f(0,40);check(f.w.degeneration_hex(f.source,f.scope(100),*effects::degeneration_hex(19,12),500));
  check(f.w.lightning(f.source,f.scope(100),*effects::lightning(2059,12),20,1000));
  check(f.health(100)==0&&f.w.take_npc_deaths(f.source).size()==1&&!f.w.has_condition(f.scope(100),Kind::blind,1000));
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<8;++mode){
  Fixture f(mode==6?101:0);const auto target=f.scope(npc?100:2);casting::Hooks h;unsigned commits=0;bool stale=false;
  if(mode!=7)check(f.w.activate_overcast_energy(f.source,219,0,1,200));
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& values){f.w.skill_event(1,values);};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==target.actor)return std::optional<effects::Scope>{};return f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==target.actor&&range==1248;};
  if(mode!=4)h.lightning=[&](auto selected,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{*particles::targeted_body_2022(target.actor,1,2059)}};
   std::vector<world::Packet> visuals;const bool applied=f.w.lightning(f.source,selected,plan,20,at,&visuals);return {applied,std::move(visuals)};
  };
  Slots bar{};bar[0]={2059,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[2059]=1;attributes::Ranks ranks{};ranks[8]=12;
  const auto before=f.w.resources(1)->energy;
  casting::Runtime cast(1);check(cast.start(1000,{2059,0,target.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(f.w.resources(1)->energy==before-(mode==4?0:5));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  if(mode==7)check(f.w.activate_overcast_energy(f.source,219,0,1,1500)); // Expansion is selected at completion.
  cast.update(1999,h);check(!commits);cast.update(2000,h);check(commits==unsigned(mode==0||mode>=5));
  const bool success=mode==0||mode==7;check(f.health(100)==(success?974:1000));
  const auto packets=f.box->drain();const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  const auto visual=*particles::targeted_body_2022(target.actor,1,2059);
  check(std::count(packets.begin(),packets.end(),visual)==unsigned(success));
  if(success)for(unsigned id:{2u,3u,100u,101u,105u,106u}){
   const auto body=*particles::targeted_body_2022(id,1,2059);
   check(std::count(packets.begin(),packets.end(),body)==1&&finish<std::find(packets.begin(),packets.end(),body));
  }
 }
 std::cout<<"PASS Shell Shock mixed recipients,owned Overcast selection,post-damage Cracked Armor,immunity and real casts\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
