#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Barbed Signet target effect line "+std::to_string(at.line()));
}
int main()try{
 check(skills::find_skill(131)->target_body_resource==241);
 for(bool npc_target:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(npc_target){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.spawn_health={1000,1000};
   d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=d.conditions_allowed=true;d.fleshy=true;d.fleshy_evidence="synthetic target";
   n.combat_definition=d;w.configure_npcs({n});
  }
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc_target){w.join(2,"Victim",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);}
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(npc_target?100:2);box->drain();
  check(w.condition_spell(caster,target,*skills::effects::condition_spell(131,12),200));
  check(w.has_condition(target,combat::conditions::Kind::bleeding,200));const auto packets=box->drain();
  const auto expected=*skills::particles::targeted_body_2022(target.actor,caster.actor,131);
  check(std::count(packets.begin(),packets.end(),expected)==1);
  const auto legacy=*skills::particles::target_body_2022(target.actor,131);
  check(std::find(packets.begin(),packets.end(),legacy)==packets.end());
  w.tick(200);const auto next=box->drain();check(std::find(next.begin(),next.end(),expected)==next.end());
  skills::casting::Runtime runtime(1);skills::casting::Hooks h;std::function<void()> duplicate;
  h.events=[&](const auto& packets){box->enqueue(packets);};h.resources=[&](auto id){return w.resources(id);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.consume_energy_at=[&](unsigned cost,auto now){return w.consume_energy(1,cost,caster,now);};
  h.projectile_target_valid=[&](auto id,auto range){return w.projectile_target_valid(caster,id,range,300);};
  h.health_sacrifice=[&](auto skill,auto now){return w.health_sacrifice(caster,skill,now);};
  h.condition_spell_completed=[&](auto victim,const auto& plan,auto event,auto now)->skills::casting::EffectCompletion{
   check(event==0); // A signet is not an admitted spell event.
   std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool accepted=w.complete_condition_spell(caster,victim,plan,now,visuals,publication);
   check(accepted&&box->drain().empty()&&visuals.size()==1&&publication);duplicate=publication;
   return {accepted,std::move(visuals),{},std::move(publication),true};
  };
  skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[131]=1;
  skills::Slots bar{};bar[0]={131,0};attributes::Ranks ranks{};ranks[4]=12;
  check(runtime.start(300,{131,0,static_cast<std::uint16_t>(target.actor)},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
  box->drain();runtime.update(1299,h);check(box->drain().empty());runtime.update(1300,h);
  const auto completed=box->drain();const auto finish=skills::casting::terminal(58,1);
  const auto end=std::find(completed.begin(),completed.end(),finish),body=std::find(completed.begin(),completed.end(),expected);
  check(end!=completed.end()&&body!=completed.end()&&end<body&&std::count(completed.begin(),completed.end(),expected)==1);
  duplicate();runtime.update(1300,h);check(box->drain().empty());

 }
 std::cout<<"Barbed Signet targeted241caster/victim particle,player/NPC and no replay pass\n";
 return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
