#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("Lightning Strike World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;
 Fixture(bool npc,bool armor=true,unsigned hp=1000){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{double(hp),1000};d.damageable=true;d.conditions_allowed=true;
   if(armor){d.lightning_armor=80;d.lightning_armor_evidence="test";}n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);check(w.lightning_armor(*w.effect_scope(2),armor?std::optional<int>{80}:std::nullopt));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
 double health(){return target.actor==100?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 bool hex(){return target.actor==100?bool(w.npc_combat_snapshot(target)->effects&0x800):!w.status_snapshot(target,1000)->empty();}
 void overcast(){check(w.activate_overcast_energy(source,219,0,1,200));}
 bool cast(unsigned rank=12,std::uint64_t at=1000){return w.lightning(source,target,*skills::effects::lightning(222,rank),20,at);}
};
int main()try{
 using namespace skills;
 // Damage conversion must not be resurrected by the Overcast hex snapshot.
 for(bool expanded:{false,true})for(unsigned hp:{1u,1000u})for(unsigned rr:{0u,12u}){
  Fixture f(false,true,hp);if(expanded)f.overcast();
  check(f.w.status_mutation(f.target,307,5,rr,200,8200,true));check(f.cast());
  const double converted=std::min(41.,rr?67.:15.);
  const double expected=std::max(0.,std::min(1000.,hp+converted)-(41-converted));
  check(f.health()==expected);
  const auto entries=*f.w.status_snapshot(f.target,1000);
  check(std::none_of(entries.begin(),entries.end(),[](auto e){return e.skill==307;}));
  check(std::any_of(entries.begin(),entries.end(),[](auto e){return e.skill==222;})==(expanded&&expected>0));
 }
 // Reversal applied after the first strike must convert the owned delayed hit.
 for(unsigned rr:{0u,12u}){
  Fixture f(false);f.overcast();check(f.cast());check(f.health()==959);
  check(f.w.status_mutation(f.target,307,5,rr,2000,10000,true));f.w.tick(4000);
  check(f.health()==(rr?1000:948));const auto entries=*f.w.status_snapshot(f.target,4000);
  check(std::none_of(entries.begin(),entries.end(),[](auto e){return e.skill==307;}));
 }
 for(bool npc:{false,true})for(bool expanded:{false,true})for(unsigned rank:{0u,12u,20u}){
  Fixture f(npc);if(expanded)f.overcast();const double damage=5+3*rank;
  const bool accepted=f.cast(rank);const bool hex=f.hex();
  if(!accepted||f.health()!=1000-damage||hex!=expanded)std::cerr<<"npc "<<npc<<" Overcast "<<expanded<<" rank "<<rank<<" accepted "<<accepted<<" HP "<<f.health()<<" hex "<<hex<<'\n';
  check(accepted&&f.health()==1000-damage&&hex==expanded);
  f.w.tick(3899);check(f.health()==1000-damage); // Leave a full100ms World tick before the exact deadline.
  f.w.tick(4000);if(f.health()!=1000-damage*(expanded?2:1))std::cerr<<"expiry npc "<<npc<<" Overcast "<<expanded<<" rank "<<rank<<" HP "<<f.health()<<'\n';check(f.health()==1000-damage*(expanded?2:1));
  f.w.tick(10000);check(f.health()==1000-damage*(expanded?2:1));
 }
 for(bool npc:{false,true}){
  Fixture f(npc);f.overcast();check(f.cast());check(f.cast(0,2000));
  check(f.health()==954);f.w.tick(4000);check(f.health()==954);f.w.tick(5000);check(f.health()==949); // Renewal cancels the old41-point deadline.
 }
 {
  Fixture f(false);f.overcast();check(f.cast());
  check(f.w.remove_effects(f.target,2,*effects::removal(301,0,0),2000));
  f.w.tick(5000);check(f.health()==959);
 }
 for(bool npc:{false,true}){
  Fixture f(npc);f.overcast();check(f.cast());f.w.tick(10000);check(f.health()==918); // Sparse expiry is not delayed to10000.
 }
 {
  Fixture f(true,true,70);f.overcast();check(f.cast()&&f.health()==29);
  f.w.tick(10000);check(f.health()==0);const auto deaths=f.w.take_npc_deaths(f.source);
  check(deaths.size()==1&&deaths[0].time==4000); // Credit the original deadline,not the observing tick.
 }
 {
  Fixture f(false);f.overcast();check(f.w.hex_breaker(f.target,*effects::hex_breaker(12),20,500));
  check(f.cast()&&f.health()==959); // Initial damage precedes prevention of the optional hex.
  f.w.tick(10000);check(f.health()==959);
 }
 for(bool npc:{false,true}){
  Fixture f(npc);f.overcast();check(f.cast());f.w.leave(1);
  f.w.tick(10000);check(f.health()==959); // No stale caster generation or departed-owner dispatch.
 }
 for(bool npc:{false,true}){
  Fixture f(npc,false);f.overcast();check(!f.cast()&&f.health()==1000);
  Fixture killed(npc,true,10);killed.overcast();check(killed.cast()&&killed.health()==0);killed.w.tick(10000);check(killed.health()==0);
 }
 for(bool npc:{false,true}){
  Fixture f(npc);auto stale=f.target;++stale.generation;
  check(!f.w.lightning_strike(f.source,stale,12,20,1000));
  auto forged=*effects::lightning(222,12);forged.damage=999;check(!f.w.lightning(f.source,f.target,forged,20,1000));
  check(f.health()==1000);
 }
 // Real casting dispatcher owns ordinary cost,deadline,cancellation and success particles.
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  Fixture f(npc);f.overcast();casting::Hooks h;unsigned commits=0;bool stale=false;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  h.consume_energy_at=[&](unsigned cost,auto at){return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){if(stale&&id==f.target.actor)return std::optional<effects::Scope>{};return f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==f.target.actor&&range==1248;};
  if(mode!=4)h.lightning=[&](auto target,const auto& plan,auto at)->casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{*particles::targeted_body_2022(f.target.actor,1,222)}};
   std::vector<world::Packet> visuals;const bool applied=f.w.lightning(f.source,target,plan,20,at,&visuals);return {applied,std::move(visuals)};
  };
  Slots bar{};bar[0]={222,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[222]=1;attributes::Ranks ranks{};ranks[8]=12;
  const auto energy=f.w.resources(1)->energy;
  casting::Runtime cast(1);check(cast.start(1000,{222,0,f.target.actor},bar,policy,ranks,true,h)==(mode==4?casting::Reason::unsupported:casting::Reason::accepted));
  check(f.w.resources(1)->energy==energy-(mode==4?0:5));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1999,h);check(!commits);cast.update(2000,h);check(commits==unsigned(mode==0||mode==5));
  check(f.health()==(mode==0?959:1000));
  const auto packets=f.box->drain();const auto particle=*particles::targeted_body_2022(f.target.actor,1,222);
  const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  check(std::count(packets.begin(),packets.end(),particle)==unsigned(mode==0));
  if(mode==0)check(finish<std::find(packets.begin(),packets.end(),*casting::success_body_effect(1,222))&&finish<std::find(packets.begin(),packets.end(),particle));
  f.w.tick(5000);check(f.health()==(mode==0?918:1000));
  const auto expiry=f.box->drain();check(std::count(expiry.begin(),expiry.end(),particle)==unsigned(mode==0));
  if(mode==0){
   world::Packet damage{162,0};combat::detail::put(damage,16);combat::detail::put(damage,f.target.actor);combat::detail::put(damage,1);
   combat::detail::put(damage,std::bit_cast<std::uint32_t>(float(-41./1000.)));
   check(std::find(expiry.begin(),expiry.end(),damage)<std::find(expiry.begin(),expiry.end(),particle));
  }
 }
 std::cout<<"PASS Lightning Strike player/NPC initial hit,Overcast-only expiry,renewal,early removal,sparse ticks and real casts\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
