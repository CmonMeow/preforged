#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace effects=skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Condition Renewal line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope source,target;
 Fixture(bool npc,unsigned version,bool known=true){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.spawn_health={1000,1000};
   d.relation=combat::npcs::Relation::hostile_to_players;d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{100,1000,50,100},100);w.public_professions(1,4,0);
  if(!npc){w.join(2,"Enemy",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,50,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);
  if(known)gear();if(!npc)check(w.observe_condition_equipment(target,equipment::condition_equipment(inventory::baseline(),1),100));
  if(version){const auto p=*effects::ether_renewal(version,0);check(w.status_mutation(source,version,80,0,100,100+p.duration,true));}
  box->drain();
 }
 void gear(std::uint64_t at=100){check(w.observe_condition_equipment(source,equipment::condition_equipment(inventory::baseline(),1),at));}
};
int main()try{
 for(bool npc:{false,true})for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<6;++mode){
  Fixture f(npc,version,mode!=1);const auto now=mode==3&&version?100+effects::ether_renewal(version,0)->duration:200;
  f.w.tick(now);f.box->drain();check(f.w.before_spell_effect(f.source,117,1,now));
  if(mode==5)f.w.tick(now+300);
  const auto effect_time=mode==5?now+300:now;
  const auto before=*f.w.resources(1);auto scope=f.source;if(mode==4)++scope.generation;
  std::vector<world::Packet> visuals;std::function<void()> publish;
  const auto cast=[&](auto actor,auto event){return f.w.complete_condition_spell(actor,f.target,*effects::condition_spell(117,12),now,visuals,publish,event);};
  bool accepted=cast(scope,mode==2?2:1);
  if(mode==1||mode==2||mode==4){
   check(!accepted&&!publish&&visuals.empty()&&f.box->drain().empty());
   check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy&&!f.w.has_condition(f.target,combat::conditions::Kind::weakness,now));
   if(mode==1)f.gear(now);accepted=cast(f.source,1);
  }
  check(accepted&&publish&&f.w.has_condition(f.target,combat::conditions::Kind::weakness,effect_time));
  const bool reward=version&&mode!=3;
  check(f.w.resources(1)->health==before.health+(reward?5:0)&&f.w.resources(1)->energy==before.energy+(reward?1:0));
  publish();f.box->drain();publish();check(f.box->drain().empty());
  check(!cast(f.source,1)&&!publish&&visuals.empty()&&f.box->drain().empty());
 }
 for(bool npc:{false,true})for(unsigned version:{0u,181u,2860u})for(unsigned skill:{117u,118u}){
  Fixture f(npc,version);skills::casting::Runtime runtime(1);skills::casting::Hooks h;
  unsigned before_energy=0,completed=0;std::uint64_t admitted=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& packets){f.box->enqueue(packets);};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](auto id){return f.w.effect_scope(id);};h.projectile_target_valid=[&](auto id,auto range){return f.w.projectile_target_valid(f.source,id,range,100);};
  h.before_spell_effect=[&](auto id,auto event,auto at){f.w.tick(at);before_energy=f.w.resources(1)->energy;admitted=event;return f.w.before_spell_effect(f.source,id,event,at);};
  h.health_sacrifice=[&](auto id,auto at){check(f.w.resources(1)->health==100+(version?5:0));return f.w.health_sacrifice(f.source,id,at);};
  h.condition_spell_completed=[&](auto target,const auto& plan,auto event,auto at)->skills::casting::EffectCompletion{
   check(event==admitted&&event);std::vector<world::Packet> visuals;std::function<void()> publication;
   const bool ok=f.w.complete_condition_spell(f.source,target,plan,at,visuals,publication,event);check(ok);++completed;
   check(f.w.resources(1)->energy==before_energy+(version?1:0));return {ok,std::move(visuals),{},std::move(publication),true};
  };
  skills::Slots bar{};bar[0]={static_cast<std::uint16_t>(skill),0};skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;attributes::Ranks ranks{};ranks.fill(12);
  check(runtime.start(100,{static_cast<std::uint16_t>(skill),0,static_cast<std::uint16_t>(f.target.actor)},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
  f.box->drain();const auto finish=100+std::uint64_t(skills::find_skill(skill)->activation*1000);runtime.update(finish,h);check(completed==1);
  const auto loss=skills::sacrifice::resolve(skill,100+(version?5:0),1000);
  check(f.w.resources(1)->health==(loss?unsigned(loss->remaining):100+(version?5:0)));
  const auto packets=f.box->drain();const auto terminal=std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1));check(terminal!=packets.end());
  if(version){const auto healing=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162&&p[1]==0&&p[2]==55&&p[6]==1&&p[10]==1;});check(healing!=packets.end()&&healing<terminal);}
  runtime.update(finish+100,h);check(completed==1);
 }
 std::cout<<"36 condition reward admission/retry/expiry/current-clock cases and12paid player/NPC reward-before-finish/sacrifice casts pass\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
