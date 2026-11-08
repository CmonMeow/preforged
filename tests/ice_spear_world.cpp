#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Ice Spear World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;
 Fixture(bool npc,bool armor=true){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
   if(armor){d.cold_armor=60;d.cold_armor_evidence="test";}n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{500,1000,100,100},100);
  if(!npc){w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(2),{},armor?std::optional<int>{60}:std::nullopt));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
 void overcast(){check(w.activate_overcast_energy(source,219,0,1,200));}
 bool cast(unsigned rank=15,std::uint64_t now=1000){return w.water_projectile(source,target,*skills::effects::water_projectile(214,rank),20,now);}
 double health(){return target.actor==100?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
};
int main()try{
 for(bool npc:{false,true})for(bool overcast:{false,true})for(unsigned rank:{0u,15u,20u}){
  Fixture f(npc);if(overcast)f.overcast();check(f.cast(rank));check(f.health()==1000);
  f.w.tick(1400);check(f.health()==1000);f.w.tick(1500);
  check(std::abs(f.health()-(1000-skills::effects::damage_scale(10,60,rank)))<(npc?1e-7:1));
  f.w.tick(6000);const unsigned expected=500+(overcast?10*skills::effects::interpolate(1,4,rank):0);
  check(f.w.resources(1)->health==expected);
  f.w.tick(7000);check(f.w.resources(1)->health==expected);
 }
 {
  Fixture f(false);f.overcast();check(f.cast());f.w.leave(2);f.w.tick(6000);
  check(f.w.resources(1)->health==540); // Miss/departure cannot cancel completion regeneration.
 }
 {
  Fixture f(false);f.overcast();check(f.cast());check(f.cast(15,3000));f.w.tick(8000);
  check(f.w.resources(1)->health==556); // Renewal replaces the source instead of stacking.
 }
 for(bool npc:{false,true}){
  Fixture f(npc,false);f.overcast();check(!f.cast());f.w.tick(6000);check(f.w.resources(1)->health==500);
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<6;++mode){
  Fixture f(npc);f.overcast();skills::casting::Hooks h;unsigned payments=0,commits=0;bool stale=false;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  h.consume_energy_at=[&](unsigned cost,auto at){check(cost==5);++payments;return f.w.consume_energy(1,cost,f.source,at);};
  h.target_scope=[&](unsigned id){return stale&&id==f.target.actor?std::optional<skills::effects::Scope>{}:f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==f.target.actor&&range==1248;};
  if(mode!=4)h.water_projectile=[&](auto target,const auto& plan,auto at)->skills::casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{{163,0}}};std::vector<world::Packet> out;
   const bool ok=f.w.water_projectile(f.source,target,plan,20,at,&out);return {ok,std::move(out)};
  };
  skills::Slots bar{};bar[0]={214,0};skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[214]=1;
  attributes::Ranks ranks{};ranks[11]=12;skills::casting::Runtime cast(1);
  check(cast.start(1000,{214,0,f.target.actor},bar,policy,ranks,true,h)==(mode==4?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
  check(payments==unsigned(mode!=4));if(mode==4)continue;
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1999,h);check(!commits);cast.update(2000,h);check(commits==unsigned(mode==0||mode==5)&&payments==1);
  const auto packets=f.box->drain();auto launch=std::find_if(packets.begin(),packets.end(),[](const auto& p){return !p.empty()&&p[0]==163;});
  check((launch!=packets.end())==(mode==0));
  const auto body=*skills::casting::success_body_effect(1,214);
  check(std::count(packets.begin(),packets.end(),body)==unsigned(mode==0));
  if(mode==0)check(std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1))<std::find(packets.begin(),packets.end(),body)&&std::find(packets.begin(),packets.end(),body)<launch);
  check(f.health()==1000);f.w.tick(7000);check(f.health()==(mode==0?950:1000));check(f.w.resources(1)->health==(mode==0?530:500));
 }
 std::cout<<"PASS Ice Spear World delayed cold damage,Overcast completion regeneration,miss independence,renewal and rejection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
