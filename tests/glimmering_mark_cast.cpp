#include "gw_glimmering_mark_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Glimmering cast line "+std::to_string(at.line())+" "+context);
}
struct Fixture{
 world::Instance w;Scope source,target;bool npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 casting::Hooks h;unsigned applications=0,observations=0;
 Fixture(bool n,unsigned renewal):npc(n){
  if(n){world::Instance::Npc a;a.id=a.definition.id=100;a.x=100;a.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.earth_armor=60;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={480,480};d.damageable=d.conditions_allowed=true;
   a.combat_definition=d;w.configure_npcs({a});}
  for(unsigned id:{1u,2u}){w.join(id,"Paid Mark",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{id==1?500u:480u,id==1?1000u:480u,50,100},100);}
  source=*w.effect_scope(1);target=*w.effect_scope(n?100:2);
  // Player2 is also an incidental hostile recipient when the NPC is selected.
  // Ash Blast must admit the entire cohort, so both recipients need armor.
  check(w.combat_armor(*w.effect_scope(2),60));
  if(renewal){const auto plan=*ether_renewal(renewal,0);check(w.status_mutation(source,renewal,80,0,100,100+plan.duration,true));}
  h.events=[this](const auto& packets){box->enqueue(packets);};h.resources=[this](auto id){return w.resources(id);};
  h.consume_energy_at=[this](auto amount,auto at){return w.consume_energy(1,amount,source,at);};
  h.target_scope=[this](auto id){return w.effect_scope(id);};h.projectile_target_valid=[](auto,float){return true;};
  h.before_spell_effect=[this](auto skill,auto event,auto at){w.tick(at);return w.before_spell_effect(source,skill,event,at);};
  h.glimmering_targeted_use=[this](const auto& use){++observations;return w.complete_glimmering_targeted_use(use);};
  auto complete=casting::make_glimmering_mark_completion(w,[this]{return w.effect_scope(1);},20);
  h.glimmering_mark_completed=[this,complete](auto target,const auto& plan,auto event,auto at)mutable{
   ++applications;check(plan.rank==12);return complete(target,plan,event,at);};box->drain();
 }
 bool mark(unsigned at){return w.glimmering_mark_snapshot(target,at).has_value();}
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
};
int main()try{unsigned cases=0;
 for(bool npc:{false,true})for(bool pvp:{false,true})for(unsigned renewal:{0u,181u,2860u})for(unsigned mode=0;mode<11;++mode){
  context="npc="+std::to_string(npc)+" pvp="+std::to_string(pvp)+" renewal="+std::to_string(renewal)+" mode="+std::to_string(mode);
  Fixture f(npc,renewal);casting::Runtime runtime(1);Policy policy;policy.primary=6;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[227]=1;
  Slots bar{};bar[0]={227,0};attributes::Ranks ranks{};ranks[8]=12;
  if(mode==8)f.h.glimmering_mark_completed={};if(mode==9)f.h.glimmering_targeted_use={};if(mode==10)f.h.before_spell_effect={};
  const auto admission=runtime.start(100,{227,0,f.target.actor},bar,policy,ranks,true,f.h);
  check(admission==(mode>=8?casting::Reason::unsupported:casting::Reason::accepted));
  check(f.w.resources(1)->energy==(mode>=8?50:40));
  if(mode==1)runtime.cancel(f.h);if(mode==2)runtime.interrupt(200,f.h);
  if(mode==3)f.h.glimmering_mark_completed={};if(mode==4)f.h.glimmering_targeted_use={};if(mode==5)f.h.before_spell_effect={};
  if(mode==6)f.h.target_scope=[&](auto id){auto scope=f.w.effect_scope(id);if(scope&&id==f.target.actor)++scope->generation;return scope;};
  if(mode==7)f.h.projectile_target_valid=[](auto,float){return false;};
  f.box->drain();runtime.update(1099,f.h);check(!f.applications&&!f.observations&&!f.mark(1099));
  runtime.update(1100,f.h);runtime.update(1100,f.h);
  check(f.applications==unsigned(mode==0)&&f.observations==unsigned(mode==0));check(f.mark(1100)==(mode==0));
  check(f.w.resources(1)->health==500+(mode==0&&renewal?5:0));
  if(mode==0){
   check(f.w.resources(1)->energy==40+(renewal?1:0));const auto packets=f.box->drain();
   const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
   const auto body=std::find(packets.begin(),packets.end(),*particles::targeted_body_2022(f.target.actor,1,227));
   check(finish!=packets.end()&&body!=packets.end()&&finish<body);
   check(std::find(packets.begin(),packets.end(),casting::slot_event(228,1,{227,0},15))!=packets.end());
   check(runtime.start(1101,{227,0,f.target.actor},bar,policy,ranks,true,f.h)==casting::Reason::busy);
   f.w.tick(2100);check(f.hp()==480-glimmering_mark(12)->damage);
  }++cases;
 }
 for(bool npc:{false,true}){
  context="followup npc="+std::to_string(npc);
  Fixture f(npc,0);casting::Runtime runtime(1);Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[227]=policy.learned[1085]=1;
  Slots bar{};bar[0]={227,0};bar[1]={1085,0};attributes::Ranks ranks{};ranks[8]=12;
  f.h.ash_blast=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   check(!f.mark(unsigned(at)));std::function<void()> publication;const bool ok=f.w.stage_ash_blast(f.source,target,plan,20,at,&publication);
   return {ok,{}, {},std::move(publication),true};};
  check(runtime.start(100,{227,0,f.target.actor},bar,policy,ranks,true,f.h)==casting::Reason::accepted);runtime.update(1100,f.h);
  check(f.mark(1100));check(runtime.start(1850,{1085,0,f.target.actor},bar,policy,ranks,true,f.h)==casting::Reason::accepted);
  f.w.tick(2100);const auto before=f.hp();runtime.update(2850,f.h);
  if(f.mark(2850)||f.hp()!=before-35)std::cerr<<context<<" mark="<<f.mark(2850)<<" before="<<before<<" after="<<f.hp()<<" observations="<<f.observations<<"\n";
  check(!f.mark(2850)&&f.hp()==before-35);
  const auto ended=f.hp();f.w.tick(11100);check(f.hp()==ended&&f.observations==2);++cases;
 }
 std::cout<<cases<<" staged paid player/NPC PvE/PvP Mark casts: cost, completion/body order, pulse, cancel/interrupt/lost hooks, target/range and next-targeted-use end passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
