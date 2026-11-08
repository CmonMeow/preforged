#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Lightning Surge World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;
 world::Instance::Npc sample;
 Fixture(bool npc,bool armor=true,unsigned hp=1000,std::optional<bool> control=true,bool conditions=true){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{double(hp),1000};d.damageable=true;d.conditions_allowed=conditions;
   if(armor){d.lightning_armor=80;d.lightning_armor_evidence="test";}d.knockdown_allowed=control;d.knockdown_evidence=control.has_value()?"test":"";n.combat_definition=d;sample=n;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);check(w.lightning_armor(*w.effect_scope(2),armor?std::optional<int>{80}:std::nullopt));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
 double health(){return target.actor==100?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 bool hex(){return target.actor==100?bool(w.npc_combat_snapshot(target)->effects&0x800):!w.status_snapshot(target,1000)->empty();}
 void overcast(){check(w.activate_overcast_energy(source,219,0,1,200));}
 bool cast(unsigned rank=12,std::uint64_t at=1000){return w.lightning_surge(source,target,rank,20,at);}
};
int main()try{
 using namespace skills;using combat::conditions::Kind;
 // At the natural deadline,conversion may save a raw-lethal target and
 // permit the actual surviving target's Cracked Armor/control application.
 for(unsigned hp:{1u,1000u})for(unsigned rr:{0u,12u,20u}){
  Fixture f(false,true,hp);check(f.w.status_mutation(f.target,307,5,rr,200,8200,true));
  check(f.cast());f.w.tick(4000);const double damage=effects::lightning_surge(12)->damage;
  const double converted=std::min(damage,15.+13.*rr/3.);
  const double expected=std::max(0.,std::min(1000.,hp+converted)-(damage-converted));
  check(f.health()==std::ceil(expected));check(f.w.knocked_down(f.target,4000)==(expected>0));
  check(f.w.has_condition(f.target,Kind::cracked_armor,4000)==(expected>0));
  const auto entries=*f.w.status_snapshot(f.target,4000);
  check(std::none_of(entries.begin(),entries.end(),[](auto e){return e.skill==307;}));
 }
 for(bool npc:{false,true})for(unsigned rank:{0u,12u,20u}){
  Fixture f(npc);const auto plan=*effects::lightning_surge(rank);
  check(f.cast(rank)&&f.health()==1000&&f.hex());
  f.w.tick(3899);check(f.health()==1000&&!f.w.knocked_down(f.target,3899));
  f.w.tick(4000);const auto expected=1000-plan.damage;
  check(std::abs(f.health()-(npc?expected:std::ceil(expected)))<1e-7);
  check(f.w.knocked_down(f.target,4000)&&f.w.has_condition(f.target,Kind::cracked_armor,4000));
  check(!f.w.knocked_down(f.target,6000));
 }
 for(bool npc:{false,true}){
  Fixture f(npc);check(f.cast()&&f.cast(0,2000));
  f.w.tick(4000);check(f.health()==1000);f.w.tick(5000);check(f.health()==986);
 }
 {
  Fixture f(false);check(f.cast());check(f.w.remove_effects(f.target,2,*effects::removal(301,0,0),2000));
  f.w.tick(10000);check(f.health()==1000&&!f.w.knocked_down(f.target,10000));
 }
 for(bool npc:{false,true}){
  Fixture f(npc);check(f.cast());f.w.tick(10000);const double expected=1000-effects::lightning_surge(12)->damage;
  check(std::abs(f.health()-(npc?expected:std::ceil(expected)))<1e-7);
 }
 {
  Fixture f(true,true,70);check(f.cast()&&f.health()==70);f.w.tick(10000);check(f.health()==0);
  const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==1&&deaths[0].time==4000);
 }
 {
  Fixture f(false);check(f.w.hex_breaker(f.target,*effects::hex_breaker(12),20,500));
  check(f.cast()&&f.health()==1000);f.w.tick(10000);check(f.health()==1000);
 }
 for(bool npc:{false,true}){
  Fixture f(npc);check(f.cast());f.w.leave(1);f.w.tick(10000);check(f.health()==1000);
  Fixture bad(npc,false);check(!bad.cast()&&bad.health()==1000);
 }
 for(auto control:{std::optional<bool>{},std::optional<bool>{false}}){
  Fixture f(true,true,1000,control);check(f.cast()==control.has_value());
  f.w.tick(4000);check(!f.w.knocked_down(f.target,4000));
  check(f.health()==(control.has_value()?1000-effects::lightning_surge(12)->damage:1000));
 }
 for(bool surge_first:{false,true}){
  Fixture f(true);f.overcast();
  if(surge_first){check(f.cast(0,500));check(f.w.lightning(f.source,f.target,*effects::lightning(222,0),20,1000));}
  else{check(f.w.lightning(f.source,f.target,*effects::lightning(222,0),20,500));check(f.cast(0,1000));}
  f.w.tick(10000);const double expected=1000-14-5-5*(surge_first?std::exp2(15./40.):1);
  check(std::abs(f.health()-expected)<1e-7);
 }
 {
  Fixture f(true,true,1000,true,false);check(f.cast(0));f.w.tick(4000);
  check(f.health()==986&&f.w.knocked_down(f.target,4000)&&!f.w.has_condition(f.target,Kind::cracked_armor,4000));
 }
 for(bool npc:{false,true}){
  Fixture f(npc,true,90);check(f.w.degeneration_hex(f.source,f.target,*effects::degeneration_hex(19,12),500));
  check(f.cast());f.w.tick(10000);check(f.health()==0);
  if(npc){const auto deaths=f.w.take_npc_deaths(f.source);check(deaths.size()==1&&deaths[0].time==4000);}
 }
 for(bool npc:{false,true}){
  Fixture f(npc);check(f.cast(0));f.box->drain();f.w.tick(4000);
  const auto packets=f.box->drain();const auto control=std::find(packets.begin(),packets.end(),combat::knockdown_2022(f.target.actor,2));
  const auto damage=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162&&p[1]==0;});
  check(damage!=packets.end()&&control!=packets.end()&&damage<control);
 }
 for(bool npc:{false,true}){
  Fixture f(npc);check(!f.w.lightning_surge(f.source,f.target,12,20,UINT64_MAX-3000));
  auto stale=f.target;++stale.generation;
  check(!f.w.lightning_surge(f.source,stale,12,20,1000));
  auto forged=*effects::lightning(205,12);forged.damage=999;check(!f.w.lightning(f.source,f.target,forged,20,1000));
  check(f.health()==1000);
 }
 {
  Fixture f(false);check(f.cast());check(f.w.lightning_armor(f.target,100));
  f.w.tick(4000);check(f.health()==std::ceil(1000-effects::lightning_surge(12)->damage*std::exp2(-15./40.)));
 }
 {
  Fixture f(false);check(f.cast());const auto retired=f.target;f.w.leave(2);
  f.w.join(2,"Replacement",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(2,{1000,1000,100,100},100);
  const auto replacement=*f.w.effect_scope(2);check(replacement!=retired);
  f.box->drain();f.w.tick(4000);check(f.w.resources(2)->health==1000&&!f.w.knocked_down(replacement,4000));
 }
 {
  Fixture f(true);check(f.cast());const auto old_source=f.source,old_target=f.target;f.w.leave(1);
  f.w.configure_npcs({f.sample});f.w.join(1,"Replacement caster",0,0,f.box,0,{}, {},100,1);f.w.set_vitals(1,{1000,1000,100,100},100);
  f.source=*f.w.effect_scope(1);f.target=*f.w.effect_scope(100);check(f.source!=old_source&&f.target!=old_target);
  f.box->drain();f.w.tick(4000);check(f.health()==1000&&!f.w.knocked_down(f.target,4000));
  check(f.cast(0,5000));f.w.tick(8000);check(f.health()==986&&f.w.knocked_down(f.target,8000));
 }
 {
  Fixture f(true);check(f.w.spawn_kill_encounter(f.source,85,1,{f.sample},[]{return true;}));const auto retired=*f.w.effect_scope(60000);
  check(f.w.lightning_surge(f.source,retired,12,20,1000));check(f.w.clear_kill_objective(f.source,85,1,true));
  check(!f.w.effect_scope(retired.actor));
  check(f.w.spawn_kill_encounter(f.source,85,2,{f.sample},[]{return true;}));const auto replacement=*f.w.effect_scope(60001);
  f.box->drain();f.w.tick(4000);check(f.w.npc_combat_snapshot(replacement)->health->current==1000&&!f.w.knocked_down(replacement,4000));
  for(const auto& packet:f.box->drain())check(!(packet.size()==18&&packet[0]==162&&packet[1]==0));
 }
 {
  Fixture f(true);const combat::npcs::Lifetime life{99,1,8,146,100};
  combat::npcs::LiveActor body(*f.sample.combat_definition,life);
  const auto first=body.apply_lightning_surge_hex(life,12,9,1,1000);check(bool(first));
  check(body.hex_effect(life,1)->expires==4000&&body.actor(life)->health->current==1000);
  auto stale=life;++stale.actor_generation;
  check(!body.remove_hex_effect(stale,1,1500)&&body.hex_effect(life,1).has_value());
  check(body.remove_hex_effect(life,1,2000)&&!body.hex_effect(life,1)&&!(body.actor(life)->effects&0x800));
  check(body.apply_lightning_surge_hex(life,0,9,2,2100).has_value());
  check(body.apply_lightning_surge_hex(life,12,10,3,2200).has_value());
  check(!body.hex_effect(life,2)&&body.hex_effect(life,3)->cause==10&&body.hex_effect(life,3)->expires==5200);
  check(!body.remove_hex_effect(life,2,2300)&&body.hex_effect(life,3).has_value());
  check(body.remove_hex_effect(life,3,2400)&&!body.hex_effect(life,3));
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<7;++mode){
  Fixture f(npc,mode!=6);casting::Hooks h;unsigned commits=0,payments=0;bool stale=false;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  h.consume_energy_at=[&](unsigned cost,auto at){++payments;return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==f.target.actor)return std::optional<effects::Scope>{};return f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==f.target.actor&&range==1248;};
  if(mode!=4)h.lightning=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{*particles::targeted_body_2022(f.target.actor,1,205)}};
   std::vector<world::Packet> visuals;const bool applied=f.w.lightning(f.source,target,plan,20,at,&visuals);return {applied,std::move(visuals)};
  };
  Slots bar{};bar[0]={205,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[205]=1;attributes::Ranks ranks{};ranks[8]=12;
  const auto energy=f.w.resources(1)->energy;
  casting::Runtime cast(1);check(cast.start(1000,{205,0,f.target.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(f.w.resources(1)->energy==energy-(mode==4?0:10));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1999,h);check(!commits);cast.update(2000,h);check(commits==unsigned(mode==0||mode>=5));
  check(f.health()==1000);
  const auto packets=f.box->drain();const auto particle=*particles::targeted_body_2022(f.target.actor,1,205);
  const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  check(std::count(packets.begin(),packets.end(),particle)==unsigned(mode==0));
  if(mode==0)check(finish<std::find(packets.begin(),packets.end(),particle));
  f.w.tick(4899);check(f.health()==1000);f.w.tick(5000);
  const double expected=mode==0?1000-effects::lightning_surge(12)->damage:1000;
  check(std::abs(f.health()-(npc?expected:std::ceil(expected)))<1e-7);
  const auto expiry=f.box->drain();check(std::count(expiry.begin(),expiry.end(),particle)==0);
  check(f.w.knocked_down(f.target,5000)==(mode==0)&&payments==1);
 }
 std::cout<<"PASS Lightning Surge World no initial damage,owned natural expiry,renewal,early removal,Hex Breaker,lifetimes,control policy and shared deadline chronology\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
