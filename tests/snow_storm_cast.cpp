#include "gw_snow_storm_completion.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Snow cast line "+std::to_string(at.line())+" "+context);
}
struct Fixture{
 world::Instance w;Scope source,target;bool npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 casting::Hooks h;unsigned applications=0;unsigned rank;
 Fixture(bool n,unsigned renewal,unsigned title):npc(n),rank(title){
  if(n){world::Instance::Npc b;b.id=b.definition.id=100;b.x=100;b.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={1000,1000};d.cold_armor=60;
   d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic paid cold fixture";b.combat_definition=d;w.configure_npcs({b});}
  for(unsigned id:{1u,2u}){w.join(id,"Paid Snow",float((id-1)*100),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{id==1?500u:1000u,1000,50,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  source=*w.effect_scope(1);target=*w.effect_scope(n?100:2);
  if(renewal){const auto p=*ether_renewal(renewal,0);check(w.status_mutation(source,renewal,80,0,100,100+p.duration,true));}
  h.events=[this](const auto& ps){box->enqueue(ps);};h.resources=[this](auto id){return w.resources(id);};
  h.consume_energy_at=[this](auto amount,auto at){return w.consume_energy(1,amount,source,at);};
  h.target_scope=[this](auto id){return w.effect_scope(id);};h.projectile_target_valid=[](auto,float){return true;};
  h.deldrimor_rank=[title]{return std::optional<unsigned>(title);};
  h.glimmering_targeted_use=[this](const auto& use){return w.complete_glimmering_targeted_use(use);};
  h.before_spell_effect=[this](auto skill,auto event,auto at){w.tick(at);return w.before_spell_effect(source,skill,event,at);};
  auto native=casting::make_snow_storm_completion(w,[this]{return w.effect_scope(1);},20);
  h.snow_storm_completed=[this,native](auto scope,const auto& plan,auto event,auto at)mutable{
   ++applications;check(plan.deldrimor_rank==rank);return native(scope,plan,event,at);};box->drain();
 }
 double hp(){return npc?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
};
int main()try{unsigned cases=0;
 for(bool npc:{false,true})for(unsigned rank=0;rank<=10;++rank)for(unsigned renewal:{0u,181u,2860u})for(unsigned mode=0;mode<16;++mode){
  context="npc="+std::to_string(npc)+" rank="+std::to_string(rank)+" renewal="+std::to_string(renewal)+" mode="+std::to_string(mode);
  Fixture f(npc,renewal,rank);casting::Runtime runtime(1);Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[2222]=1;policy.pvp_match=mode==12;
  Slots bar{};bar[0]={2222,0};attributes::Ranks ranks{};
  if(mode==8)f.h.snow_storm_completed={};if(mode==9)f.h.before_spell_effect={};if(mode==10)f.h.deldrimor_rank={};
  if(mode==11)f.h.deldrimor_rank=[]{return std::optional<unsigned>(11);};
  if(mode==14)f.h.target_scope=[&](auto id){return id==1?std::optional<Scope>{}:f.w.effect_scope(id);};
  const bool admitted=mode<8||mode==13||mode==15;
  const auto reason=runtime.start(100,{2222,0,f.target.actor},bar,policy,ranks,true,f.h);
  check((reason==casting::Reason::accepted)==admitted);check(f.w.resources(1)->energy==(admitted?40:50));
  if(mode==1)runtime.cancel(f.h);if(mode==2)runtime.interrupt(200,f.h);
  if(mode==3)f.h.snow_storm_completed={};if(mode==4)f.h.before_spell_effect={};if(mode==5)f.h.target_scope={};
  if(mode==6)f.h.target_scope=[&](auto id){auto s=f.w.effect_scope(id);if(s&&id==f.target.actor)++s->generation;return s;};
  if(mode==7)f.h.projectile_target_valid=[](auto,float){return false;};if(mode==13)f.h.glimmering_targeted_use={};
  if(mode==15)f.h.deldrimor_rank=[]{return std::optional<unsigned>(11);};
  runtime.update(1099,f.h);check(!f.applications&&f.hp()==1000);f.box->drain();runtime.update(1100,f.h);runtime.update(1100,f.h);
  const bool success=mode==0||mode==15;check(f.applications==unsigned(success));
  check(f.w.resources(1)->health==500+(success&&renewal?5:0));
  const auto packets=f.box->drain();
  if(success){check(f.w.resources(1)->energy==40+(renewal?1:0));
   const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
   const auto bodies=particles::snow_storm_ground_2022(1,100,0,0);
   const auto first=std::find(packets.begin(),packets.end(),bodies[0]),second=std::find(packets.begin(),packets.end(),bodies[1]);
   check(finish!=packets.end()&&first!=packets.end()&&second!=packets.end()&&finish<first&&first<second);
   const auto recharge=casting::slot_event(228,1,{2222,0},10);
   check(std::find(packets.begin(),packets.end(),recharge)<finish&&std::count(packets.begin(),packets.end(),recharge)==1);
   if(mode==15)f.h.deldrimor_rank=[rank]{return std::optional<unsigned>(rank);}; // Isolate aftercast check with a valid new request.
   check(runtime.start(1101,{2222,0,f.target.actor},bar,policy,ranks,true,f.h)==casting::Reason::busy);
  }
  f.w.tick(2100);check(f.hp()==1000-(success?snow_storm(rank)->damage:0));++cases;
 }
 for(bool npc:{false,true})for(unsigned renewal:{181u,2860u}){
  context="reward death";Fixture f(npc,renewal,0);f.w.set_vitals(1,{50,1000,50,100},100);
  check(f.w.apply_condition(f.source,f.source,combat::conditions::Kind::deep_wound,100,60000));
  check(f.w.before_spell_effect(f.source,2222,1,200));auto native=casting::make_snow_storm_completion(f.w,[&]{return f.w.effect_scope(1);},20);
  const auto result=native(f.target,*snow_storm(0),1,200);check(result.accepted&&result.visuals.empty()&&f.w.resources(1)->health==0);
  f.w.tick(5200);check(f.hp()==1000&&!native(f.target,*snow_storm(0),1,5200).accepted);++cases;
 }
 std::cout<<cases<<" paid Snow player/NPC all-title cost/recharge/packet order, PvE-only, retained rank/hooks, cancel/interrupt, lifetime/range and reward-death cases passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
