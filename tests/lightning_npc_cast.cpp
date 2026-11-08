#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("lightning NPC line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;skills::effects::Scope source,target;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool conditions=true,bool armor=true,unsigned health=1000,bool hostile=true,float x=200){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=x;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::protected_ally;
  d.spawn_health=combat::Health{double(health),1000};d.damageable=hostile;d.conditions_allowed=hostile&&conditions;
  if(armor){d.lightning_armor=80;d.lightning_armor_evidence="test";}
  n.combat_definition=d;w.configure_npcs({n});w.join(1,"Caster",0,0,box,0,{}, {},100,1);
  w.set_vitals(1,{1000,1000,100,100},100);source=*w.effect_scope(1);target=*w.effect_scope(100);
 }
 double health(){return target.actor==100?w.npc_combat_snapshot(target)->health->current:double(w.resources(target.actor)->health);}
};
int main()try{
 using namespace skills;using combat::conditions::Kind;
 for(unsigned id:{224u,865u,3396u})for(unsigned rank:{0u,12u,20u})for(bool conditions:{false,true}){
  Fixture f(conditions);const auto plan=*effects::lightning(id,rank);
  check(f.w.lightning(f.source,f.target,plan,20,1000));check(std::abs(f.health()-(1000-plan.damage))<1e-8);
  const auto kind=id==224?Kind::weakness:Kind::cracked_armor;const auto duration=id==224?plan.weakness_ms:plan.cracked_ms;
  check(f.w.has_condition(f.target,kind,1000)==(conditions&&duration));
  if(conditions&&duration){check(f.w.has_condition(f.target,kind,1000+duration-1));check(!f.w.has_condition(f.target,kind,1000+duration));}
 }
 {
  Fixture f;const auto plan=*effects::lightning(865,12);
  check(f.w.lightning(f.source,f.target,plan,20,1000));check(f.w.lightning(f.source,f.target,plan,20,2000));
  check(std::abs(f.health()-(1000-82-82*std::exp2(15./40.)))<1e-8); // Existing Cracked Armor affects the next hit,not the first.
 }
 {
  Fixture f(true,true,50);
  check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),500));
  check(f.w.lightning(f.source,f.target,*effects::lightning(224,12),20,1000));
  check(f.health()==0); // Weakness gained triggers Fragility after the nonlethal lightning hit.
 }
 for(unsigned mode=0;mode<7;++mode){
  Fixture f(true,mode!=0,1000,mode!=1,mode==2?1300.f:200.f);auto source=f.source,target=f.target;auto plan=*effects::lightning(865,12);
  if(mode==3)++source.generation;if(mode==4)++target.generation;if(mode==5)++plan.damage;
  check(!f.w.lightning(source,target,plan,mode==6?0:20,1000));check(f.health()==1000&&!f.w.has_condition(f.target,Kind::cracked_armor,1000));
 }
 for(unsigned id:{224u,865u,3396u})for(bool npc:{false,true})for(unsigned mode=0;mode<8;++mode){
  Fixture f(mode!=5,true,mode==6?10:1000);const auto* row=find_skill(id);
  if(!npc){f.w.join(2,"Player foe",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{mode==6?10u:1000u,1000,100,100},100);f.target=*f.w.effect_scope(2);check(f.w.lightning_armor(f.target,80));}
  Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[id]=1;
  attributes::Ranks ranks{};ranks[8]=12;casting::Hooks h;unsigned commits=0;bool stale=false;std::vector<world::Packet> packets;
  h.resources=[&](unsigned actor){return f.w.resources(actor);};h.events=[&](const auto& values){packets.insert(packets.end(),values.begin(),values.end());};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned actor){auto scope=f.w.effect_scope(actor);if(stale&&actor==f.target.actor)return std::optional<effects::Scope>{};return scope;};
  h.projectile_target_valid=[&](unsigned actor,float range){return actor==f.target.actor&&range==1248;};
  if(mode!=4)h.lightning=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;const auto particle=*particles::targeted_body_2022(target.actor,1,id);
   if(mode==7)return {false,{particle}};
   f.box->drain();std::vector<world::Packet> visuals;const bool accepted=f.w.lightning(f.source,target,plan,20,at,&visuals);
   const auto early=f.box->drain();check(std::find(early.begin(),early.end(),particle)==early.end());
   return {accepted,std::move(visuals)};
  };
  casting::Runtime cast(1);check(cast.start(1000,{id,0,f.target.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==4){check(f.w.resources(1)->energy==100);continue;}
  check(f.w.resources(1)->energy==(id==224?90u:75u));
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  const auto end=1000+std::uint64_t(std::llround(row->activation*1000));cast.update(end-1,h);check(!commits);cast.update(end,h);
  check(commits==unsigned(mode==0||mode>=5));
  check(f.health()==(mode==6?0:mode==0||mode==5?1000-effects::lightning(id,12)->damage:1000));
  check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==unsigned(mode==0||mode==5||mode==6));
  const auto particle=*particles::targeted_body_2022(f.target.actor,1,id);
  check(std::count(packets.begin(),packets.end(),particle)==unsigned(mode==0||mode==5||mode==6));
  if(mode==0||mode==5||mode==6)check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))<std::find(packets.begin(),packets.end(),particle));
  if(mode==6)check(!f.w.has_condition(f.target,Kind::cracked_armor,end)&&!f.w.has_condition(f.target,Kind::weakness,end));
 }
 std::cout<<"PASS lightning single-NPC damage/penetration,condition immunity/expiry,Cracked Armor ordering and real cast lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
