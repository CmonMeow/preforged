#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Rust World line "+std::to_string(at.line()));}
struct Fixture{
 world::Instance w;Scope source,target,npc;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(bool unknown=false){
  world::Instance::Npc n;n.id=n.definition.id=100;n.x=266;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;if(!unknown){d.cold_armor=60;d.cold_armor_evidence="test";}n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Player",id==1?0:id==2?100:id==3?266:267,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);w.set_vitals(id,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),{},60));}
  source=*w.effect_scope(1);target=*w.effect_scope(2);npc=*w.effect_scope(100);box->drain();
 }
 void overcast(){check(w.activate_overcast_energy(source,219,0,1,200));}
 bool cast(unsigned rank=15,std::uint64_t now=1000,std::vector<world::Packet>* out=nullptr){return w.water_damage_hex(source,target,*movement_hex(204,rank),20,now,out);}
};
int main()try{
 for(bool burden:{false,true})for(bool protection:{false,true}){
  Fixture f;if(burden)f.overcast();check(f.w.cast_activity(f.target,Slot{1,7},500,2000));
  if(protection)check(f.w.status_mutation(f.target,16,123,15,500,10000,true));
  const auto epoch=f.w.control_epoch(f.target);std::vector<world::Packet> out;check(f.cast(15,1000,&out));
  check(f.w.resources(2)->health==930&&f.w.resources(3)->health==930&&f.w.resources(4)->health==1000);
  check(f.w.npc_combat_snapshot(f.npc)->health->current==930);
  check(f.w.skill_disabled_slot(f.target,{1,7},1000)==(burden?11000:0));check(f.w.skill_disabled_slot(f.target,{1,0},1000)==0);
  check((f.w.control_epoch(f.target)!=epoch)==(burden&&!protection));
  check(std::count(out.begin(),out.end(),*particles::targeted_body_2022(2,1,204))==1);
  const auto permit=*f.w.disable_reset_revision(f.target);check(f.w.reset_skill_disables(f.target,{{1,7}},permit,1100));check(f.w.skill_disabled_slot(f.target,{1,7},1100)==0);
  f.w.tick(21000);check(f.w.status_snapshot(f.target,21000)->empty());
 }
 {Fixture f(true);f.overcast();check(f.w.cast_activity(f.target,Slot{1,0},500,2000));check(!f.cast());check(f.w.resources(2)->health==1000&&f.w.skill_disabled_slot(f.target,{1,0},1000)==0);}
 {Fixture f;f.overcast();check(f.w.begin_npc_action(f.npc,combat::npcs::control::Action::spell,500,2000,Slot{1,9}).has_value());check(f.cast());check(f.w.skill_disabled_slot(f.npc,{1,9},1000)==11000);check(!f.w.begin_npc_action(f.npc,combat::npcs::control::Action::spell,1100,2000,Slot{1,9}));check(f.w.begin_npc_action(f.npc,combat::npcs::control::Action::spell,1100,2000,Slot{1,8}).has_value());}
 {Fixture f;f.overcast();check(f.w.begin_npc_action(f.npc,combat::npcs::control::Action::spell,500,2000).has_value());check(!f.cast()&&f.w.resources(2)->health==1000);}
 {Fixture f;f.overcast();check(f.w.hex_breaker(f.target,*hex_breaker(12),20,500));check(f.w.cast_activity(f.target,Slot{1,2},500,2000));check(f.cast());check(f.w.resources(2)->health==930&&f.w.status_snapshot(f.target,1000)->empty());check(f.w.skill_disabled_slot(f.target,{1,2},1000)==11000);}
 for(bool cleared:{false,true}){
  Fixture f;f.overcast();const auto burden=f.w.overcast_energy_snapshot(f.source)->curve.burden_units();
  const auto clear=200+(burden+32)/33;const auto at=clear-(cleared?0:1);
  check(f.w.cast_activity(f.target,Slot{1,4},at-100,at+100));const auto epoch=f.w.control_epoch(f.target);
  check(f.cast(15,at));check(f.w.resources(2)->health==930);
  check(f.w.skill_disabled_slot(f.target,{1,4},at)==(cleared?0:at+10000));
  check((f.w.control_epoch(f.target)!=epoch)==!cleared);
 }
 for(unsigned mode=0;mode<4;++mode){Fixture f;casting::Hooks h;unsigned calls=0;h.resources=[&](unsigned id){return f.w.resources(id);};h.events=[](const auto&){};h.consume_energy=[&](unsigned cost){return f.w.consume_energy(1,cost,f.source);};h.target_scope=[&](unsigned id){return f.w.effect_scope(id);};h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};if(mode!=3)h.movement_hex=[&](auto target,const auto& p,auto now)->casting::EffectCompletion{++calls;std::vector<world::Packet> out;return {f.w.water_damage_hex(f.source,target,p,20,now,&out),std::move(out)};};
  Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[204]=1;Slots bar{};bar[0]={204,0};attributes::Ranks ranks{};ranks[11]=15;casting::Runtime runtime(1);
  check(runtime.start(1000,{204,0,2},bar,policy,ranks,true,h)==(mode==3?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1100,h);runtime.update(3000,h);check(calls==(mode==0?1:0));check(f.w.resources(2)->health==(mode==0?930:1000));
 }
 std::cout<<"PASS Rust mixed area damage,active copy disable,protection,reset,unknown rejection,NPC identity and Runtime lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
