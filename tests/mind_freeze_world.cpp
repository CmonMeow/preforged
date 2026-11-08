#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Mind Freeze World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope source,target;
 Fixture(bool npc,unsigned own=40,unsigned other=20,bool known=true,unsigned hp=1000,bool armor=true,std::optional<unsigned> pips=0){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{double(hp),1000};d.damageable=true;d.conditions_allowed=false;
   if(armor){d.cold_armor=60;d.cold_armor_evidence="test";}
   if(known){d.spawn_energy=combat::npcs::Energy{other,100};d.energy_evidence="test";d.energy_pips=pips;d.energy_pips_evidence="test";}
   n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,own,100},100);check(w.energy_capacity(1,6,0,100,{80,-2}));
  if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,other,100},100);check(w.energy_capacity(2,6,0,100,{80,-2}));check(w.elemental_armor(*w.effect_scope(2),{},armor?std::optional<int>{60}:std::nullopt));}
  source=*w.effect_scope(1);target=*w.effect_scope(npc?100:2);box->drain();
 }
 double hp(){return target.actor==100?w.npc_combat_snapshot(target)->health->current:double(w.resources(2)->health);}
 bool hex(std::uint64_t at){return target.actor==100?bool(w.npc_combat_snapshot(target)->effects&0x800):!w.status_snapshot(target,at)->empty();}
 bool cast(unsigned skill=209,unsigned rank=12,std::uint64_t now=1000,std::vector<world::Packet>* out=nullptr){return w.mind_freeze(source,target,*skills::effects::mind_freeze(skill,rank),20,now,out);}
};
int main()try{
 using namespace skills::effects;
 for(unsigned skill:{209u,2803u})for(bool npc:{false,true})for(unsigned other:{39u,40u,41u})for(unsigned rank:{0u,12u,20u}){
  Fixture f(npc,40,other);std::vector<world::Packet> out;check(f.cast(skill,rank,1000,&out));
  const auto p=*mind_freeze(skill,rank);const bool bonus=other<40;
  check(std::abs(f.hp()-(1000-p.damage*(bonus?2:1)))<(npc?1e-7:1));check(f.hex(1000)==bonus);
  check(!out.empty()&&out.front()==*skills::particles::targeted_body_2022(f.target.actor,1,skill));
  f.w.tick(1000+p.duration);check(!f.hex(1000+p.duration));
 }
 for(bool npc:{false,true})for(unsigned hp:{10u,50u,51u,100u,101u}){
  Fixture f(npc,40,20,true,hp);check(f.cast());check(f.hp()==(hp<=100?0:1));check(f.hex(1000)==(hp>100));
 }
 for(bool npc:{false,true}){
  Fixture f(npc,40,20,true,1000,false);check(!f.cast()&&f.hp()==1000&&!f.hex(1000));
 }
 for(bool known:{false,true}){
  Fixture f(true,40,20,known,1000,true,std::nullopt);check(!f.cast()&&f.hp()==1000&&!f.hex(1000));
 }
 {
  Fixture f(false);check(f.w.hex_breaker(f.target,*hex_breaker(12),20,500));check(f.cast());
  check(f.hp()==900&&!f.hex(1000));
 }
 {
  Fixture f(false);check(f.cast());check(f.w.remove_effects(f.target,2,*removal(301,0,0),1500));check(!f.hex(1500));
 }
 {
  Fixture f(false);check(f.cast());check(f.cast(209,0,2000));f.w.tick(3000);check(!f.hex(3000));
 }
 {
  Fixture f(false);f.w.leave(1);check(!f.cast());
 }
 for(unsigned skill:{209u,2803u})for(bool npc:{false,true})for(unsigned mode=0;mode<8;++mode){
  Fixture f(npc,80,mode==6?77:20);skills::casting::Hooks h;bool stale=false;unsigned paid=0,commits=0;
  h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[&](const auto& packets){f.w.skill_event(1,packets);};
  const unsigned cost=skill==209?5:10;
  h.consume_overcast_energy=[&](unsigned id,unsigned amount,auto event,auto at){++paid;check(id==skill&&amount==cost);return f.w.activate_overcast_energy(f.source,id,amount,event,at);};
  h.target_scope=[&](unsigned id){return stale&&id==f.target.actor?std::optional<Scope>{}:f.w.effect_scope(id);};
  h.projectile_target_valid=[&](unsigned id,float range){return id==f.target.actor&&range==1248;};
  if(mode!=4)h.movement_hex=[&](auto target,const auto& plan,auto at)->skills::casting::EffectCompletion{
   ++commits;if(mode==5)return {false,{*skills::particles::targeted_body_2022(f.target.actor,1,skill)}};
   std::vector<world::Packet> out;const auto freeze=mind_freeze(plan.skill,plan.rank);check(bool(freeze));
   const bool ok=f.w.mind_freeze(f.source,target,*freeze,20,at,&out);return {ok,std::move(out)};
  };
  if(mode==7)h.consume_overcast_energy={};
  skills::Slots bar{};bar[0]={skill,0};skills::Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[skill]=1;
  attributes::Ranks ranks{};ranks[11]=12;skills::casting::Runtime cast(1);const bool missing=mode==4||mode==7;
  check(cast.start(1000,{skill,0,f.target.actor},bar,policy,ranks,true,h)==(missing?skills::casting::Reason::unsupported:skills::casting::Reason::accepted));
  check(paid==unsigned(!missing)&&f.w.resources(1)->energy==(missing?80:80-cost));if(missing)continue;
  check(f.w.overcast_energy_snapshot(f.source)->curve.burden_units()==5u*overcast::CeilingCurve::units_per_energy);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(1100,h);if(mode==3)stale=true;
  cast.update(1999,h);check(!commits);cast.update(2000,h);check(paid==1&&commits==unsigned(mode==0||mode==5||mode==6));
  const bool success=mode==0||mode==6,bonus=mode==0;const double damage=skill==209?50:34;
  check(f.hp()==(success?1000-damage*(bonus?2:1):1000));check(f.hex(2000)==bonus);
  const auto packets=f.box->drain();const auto visual=*skills::particles::targeted_body_2022(f.target.actor,1,skill);
  check(std::count(packets.begin(),packets.end(),visual)==unsigned(success));
  if(success)check(std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1))<std::find(packets.begin(),packets.end(),visual));
 }
 std::cout<<"PASS Mind Freeze World PvE/PvP energy gating,cold hits,owned hex,Hex Breaker,unknown NPC profiles,renewal/removal/expiry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
