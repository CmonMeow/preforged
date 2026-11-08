#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Mind Shock World line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;
 Fixture(bool npc,unsigned own=80,unsigned other=20,bool known=true,std::optional<bool> control=true,unsigned hp=1000,bool armor=true,std::optional<unsigned> npc_pips=0){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{double(hp),1000};d.damageable=true;d.conditions_allowed=true;
   if(armor){d.lightning_armor=80;d.lightning_armor_evidence="test";}
   if(known){d.spawn_energy=combat::npcs::Energy{other,100};d.energy_evidence="test";d.energy_pips=npc_pips;d.energy_pips_evidence="test";}
   d.knockdown_allowed=control;d.knockdown_evidence=control.has_value()?"test":"";n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,own,100},100);check(w.energy_capacity(1,6,0,100,{80,-2}));
  if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,other,100},100);check(w.energy_capacity(2,6,0,100,{80,-2}));check(w.lightning_armor(*w.effect_scope(2),armor?std::optional<int>{80}:std::nullopt));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
 double health(){return target.actor==100?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 bool cast(unsigned skill=226,unsigned rank=12,std::uint64_t now=1000,std::vector<world::Packet>* out=nullptr){return w.lightning(source,target,*skills::effects::lightning(skill,rank),20,now,out);}
};
int main()try{
 using namespace skills;
 for(unsigned skill:{226u,2804u})for(bool npc:{false,true})for(unsigned other:{19u,20u,21u})for(unsigned rank:{0u,12u,20u}){
  Fixture f(npc,20,other);const bool bonus=other<20;const double damage=effects::damage_scale(10,50,rank);
  check(f.cast(skill,rank));const double expected=1000-damage*(bonus?2:1);
  check(std::abs(f.health()-(npc?expected:std::ceil(expected)))<1e-7);check(f.w.knocked_down(f.target,1000)==bonus);
 }
 // Reversal may save a raw-lethal first hit. Resolve the second hit and
 // knockdown against post-healing authority,without consuming it twice.
 unsigned reversal_cases=0;
 for(unsigned skill:{226u,2804u})for(unsigned rank:{0u,12u,20u})
 for(unsigned hp:{1u,42u,1000u})for(bool bonus:{false,true}){
  ++reversal_cases;Fixture f(false,80,bonus?20:80,true,true,hp);
  check(f.w.status_mutation(f.target,307,5,rank,100,8100,true));
  check(f.cast(skill));const double first=42,converted=std::min(first,15.+13.*rank/3.);
  double expected=std::max(0.,std::min(1000.,hp+converted)-(first-converted));
  if(bonus&&expected>0)expected=std::max(0.,expected-first);
  check(f.health()==std::ceil(expected));check(f.w.knocked_down(f.target,1000)==(bonus&&expected>0));
  const auto entries=*f.w.status_snapshot(f.target,1000);
  check(std::none_of(entries.begin(),entries.end(),[](auto e){return e.skill==307;}));
 }
 check(reversal_cases==36);
 for(unsigned hp:{1u,42u,43u,84u,85u})for(bool npc:{false,true}){
  Fixture f(npc,80,20,true,true,hp);check(f.cast());check(f.health()==std::max(0.,double(hp)-84));
  check(f.w.knocked_down(f.target,1000)==(hp>84));if(npc&&hp<=84)check(f.w.take_npc_deaths(f.source).size()==1);
 }
 for(unsigned mode=0;mode<3;++mode){
  Fixture f(true,80,20,true,mode==0?std::optional<bool>{false}:mode==1?std::optional<bool>{}:std::optional<bool>{true});
  if(mode==2)check(f.w.gale(f.source,f.target,12,{},500));
  check(f.cast()==(mode!=1));check(f.health()==(mode==1?1000:916));check(f.w.knocked_down(f.target,1000)==(mode==2));
 }
 {
  Fixture f(true,80,20,false);std::vector<world::Packet> packets{{1,2}};
  check(!f.cast(226,12,1000,&packets)&&packets.empty()&&f.health()==1000);
 }
 {
  Fixture f(true,80,20,true,true,1000,true,std::nullopt);check(!f.cast()&&f.health()==1000);
 }
 for(bool frenzy:{false,true}){
  Fixture f(false);check(f.w.protective_enchantment(f.target,f.target,*regeneration::reviewed(299,12),0,200));
  if(frenzy)check(f.w.status_mutation(f.target,346,10,0,200,3000,true));
  check(f.cast()&&f.health()==(frenzy?862:946)); // Reduction applies independently after each hit's multiplier.
 }
 for(bool npc:{false,true}){
  Fixture f(npc,80,20,true,true,1000,false);check(!f.cast()&&f.health()==1000);
  Fixture stale(npc);auto target=stale.target;++target.generation;check(!stale.w.mind_shock(stale.source,target,*effects::mind_shock(226,12),20,1000));
 }
 // A converted first hit must not spend the last PvP Watch Yourself
 // charge in the copied armor sequence before the second hit is resolved.
 for(unsigned skill:{226u,2804u})for(unsigned rr:{12u,20u}){Fixture f(false,80,20,true,true,10);
  check(f.w.stage_watch_yourself_recipient(f.target,f.target,*effects::watch_yourself(0,true),100));
  check(f.w.status_mutation(f.target,307,5,rr,100,8100,true));check(f.cast(skill));
  check(std::abs(f.health()-10)<=1&&f.w.knocked_down(f.target,1000));
  check(f.w.watch_yourself_armor(f.target,1000)==0);
 }
 // Same UI integer Energy can hide different fractional current Energy.
 for(unsigned source_pips:{1u,3u}){
  Fixture f(true,20,20,true,true,1000,true,2);check(f.w.energy_capacity(1,6,0,100,{80,int(source_pips)-2}));
  f.w.tick(100);check(f.cast(226,12,400));check(f.health()==(source_pips==1?958:916));
 }
 for(unsigned skill:{226u,2804u})for(bool npc:{false,true})for(unsigned mode=0;mode<8;++mode){
  Fixture f(npc,80,mode==6?77:20);casting::Hooks h;bool stale=false;unsigned paid=0,commits=0;std::function<void()> replay;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  h.consume_overcast_energy=[&](unsigned id,unsigned cost,auto event,auto at){++paid;check(id==skill&&cost==5);return f.w.activate_overcast_energy(f.source,id,cost,event,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==f.target.actor)return std::optional<effects::Scope>{};return f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==f.target.actor&&range==1248;};
  if(mode!=4)h.lightning=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{*particles::targeted_body_2022(f.target.actor,1,skill)}};
   std::vector<world::Packet> packets;std::function<void()> publish;const bool applied=f.w.lightning(f.source,target,plan,20,at,&packets,&publish);replay=publish;return {applied,std::move(packets),{},std::move(publish),true};
  };
  if(mode==7)h.consume_overcast_energy={};
  Slots bar{};bar[0]={skill,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[skill]=1;attributes::Ranks ranks{};ranks[8]=12;
  casting::Runtime cast(1);const bool missing=mode==4||mode==7;
  check(cast.start(1000,{skill,0,f.target.actor},bar,policy,ranks,true,h)==(missing?casting::Reason::unsupported:casting::Reason::accepted));
  check(paid==unsigned(!missing)&&f.w.resources(1)->energy==(missing?80:75));if(missing)continue;
  check(f.w.overcast_energy_snapshot(f.source)->curve.burden_units()==(skill==226?5u:10u)*overcast::CeilingCurve::units_per_energy);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1999,h);check(!commits);cast.update(2000,h);check(paid==1&&commits==unsigned(mode==0||mode==5||mode==6));
  const bool success=mode==0||mode==6;const bool bonus=mode==0;
  check(f.health()==(success?(bonus?916:958):1000));check(f.w.knocked_down(f.target,2000)==bonus);
  const auto packets=f.box->drain();if(replay){replay();check(f.box->drain().empty());}const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  const auto visual=*particles::targeted_body_2022(f.target.actor,1,skill);
  check(std::count(packets.begin(),packets.end(),visual)==unsigned(success));
  if(success){
   const auto particle=std::find(packets.begin(),packets.end(),visual);check(finish<particle);
   world::Packet modifier{162,0};combat::detail::put(modifier,16);combat::detail::put(modifier,f.target.actor);combat::detail::put(modifier,1);combat::detail::put(modifier,std::bit_cast<std::uint32_t>(float(-42./1000.)));
   const auto first=std::find(packets.begin(),packets.end(),modifier);check(particle<first&&std::count(packets.begin(),packets.end(),modifier)==(bonus?2:1));
   const auto down=std::find(packets.begin(),packets.end(),combat::knockdown_2022(f.target.actor,2.f));
   if(bonus){const auto second=std::find(first+1,packets.end(),modifier);check(first<second&&second<down);}else check(down==packets.end());
  }
 }
 std::cout<<"PASS Mind Shock player/NPC precise Energy,two-hit control,atomic Overcast,immunity and ordered real casts\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
