#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Glimmering cast end line "+std::to_string(at.line())+" "+context);
}
int main()try{unsigned cases=0;
 for(bool npc:{false,true})for(unsigned mode=0;mode<9;++mode){
  context="npc="+std::to_string(npc)+" mode="+std::to_string(mode);
  world::Instance w;
  if(npc){world::Instance::Npc a;a.id=a.definition.id=100;a.x=100;a.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={500,500};d.earth_armor=60;
   d.damageable=d.conditions_allowed=true;a.combat_definition=d;w.configure_npcs({a});}
  for(unsigned id:{1u,2u}){w.join(id,"Cast end",float((id-1)*100),0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{500,500,100,100},100);}
  const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
  if(!npc)check(w.combat_armor(target,60));
  check(w.stage_glimmering_mark(source,target,*glimmering_mark(0),20,100));
  casting::Runtime runtime(1);casting::Hooks h;unsigned observations=0,applications=0;std::uint64_t observed=0;
  h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};
  h.consume_energy_at=[&](auto amount,auto at){return w.consume_energy(1,amount,source,at);};
  h.target_scope=[&](auto id){return w.effect_scope(id);};
  h.projectile_target_valid=[](auto,float){return true;};
  const auto observe=[&](const GlimmeringTargetedUse& use){++observations;check(use.caster==source&&use.target==target&&use.skill==1085&&use.at==1850&&use.event);observed=use.event;
   return mode==8?false:w.complete_glimmering_targeted_use(use);};
  if(mode!=7)h.glimmering_targeted_use=observe;
  h.ash_blast=[&](auto foe,const auto& plan,auto at)->casting::EffectCompletion{
   ++applications;if(mode==0)check(!w.glimmering_mark_snapshot(target,at));
   std::function<void()> publication;const bool ok=w.stage_ash_blast(source,foe,plan,20,at,&publication);
   return {ok,{}, {},std::move(publication),true};};
  Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[1085]=1;
  Slots bar{};bar[0]={1085,0};attributes::Ranks ranks{};
  check(runtime.start(850,{1085,0,target.actor},bar,policy,ranks,true,h)==casting::Reason::accepted);
  check(w.resources(1)->energy==95);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(900,h);
  if(mode==3)h.glimmering_targeted_use={};if(mode==4)h.target_scope={};
  if(mode==5)h.target_scope=[&](auto id){auto s=w.effect_scope(id);if(s&&id==target.actor)++s->generation;return s;};
  if(mode==6)h.projectile_target_valid=[](auto,float){return false;};
  if(mode==7)h.glimmering_targeted_use=observe;
  w.tick(1100);check(w.glimmering_mark_snapshot(target,1100).has_value());
  runtime.update(1849,h);check(!observations&&!applications);
  runtime.update(1850,h);runtime.update(1850,h);
  check(observations==unsigned(mode==0||mode==8));check(applications==unsigned(mode==0||mode==7));
  check(w.glimmering_mark_snapshot(target,1850).has_value()==(mode!=0));
  const auto hp=[&]{return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);};
  const auto before=hp();w.tick(2100);check(hp()==before-(mode==0?0:5));
  if(mode==0)check(!w.complete_glimmering_targeted_use({source,target,1085,observed,2100}));
  ++cases;
 }
 std::cout<<cases<<" paid Ash Blast player/NPC Mark-end completion, cancellation, interrupt, lost-hook/scope, stale-target, range, late-hook and refusal cases passed; staged observer only\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
