#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Energy hex World line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;skills::effects::Scope a,b;std::shared_ptr<world::Outbox> target_box=std::make_shared<world::Outbox>();
 Fixture(){for(unsigned id:{1u,2u}){
  w.join(id,"Actor",float(id*100),0,id==2?target_box:std::make_shared<world::Outbox>(),0,{}, {},100,id);
  w.set_vitals(id,{1000,1000,id==1?100u:0u,100},100);
 }
 a=*w.effect_scope(1);b=*w.effect_scope(2);check(w.settle_resources(a,1000)&&w.settle_resources(b,1000));}
 unsigned effect(unsigned skill,unsigned at){const auto entries=w.status_snapshot(b,at);check(bool(entries));
  unsigned found=0;for(const auto& e:*entries)if(e.skill==skill){check(!found);found=e.instance;}return found;}
};
int main()try{
 for(unsigned skill:{125u,140u}){
  Fixture f;const auto plan=*skills::effects::energy_degeneration_hex(skill,12);
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,plan,20,1000));const auto id=f.effect(skill,1000);check(id);
  check(f.w.settle_resources(f.b,4000)&&f.w.settle_resources(f.a,4000));
  check(f.w.resources(2)->health==(skill==125?976:1000));
  check(f.w.resources(1)->health==(skill==140?994:1000));
  const auto state=f.w.overcast_energy_snapshot(f.b);check(state&&overcast::precise_current(*state)==480000);
  check(f.w.status_mutation(f.b,skill,id,12,4000,std::nullopt,false));
  check(f.w.settle_resources(f.b,7000)&&f.w.settle_resources(f.a,7000));
  check(!f.effect(skill,7000)&&f.w.resources(2)->health==(skill==125?976:1000));
  check(f.w.resources(1)->health==(skill==140?994:1000));
  check(overcast::precise_current(*f.w.overcast_energy_snapshot(f.b))==1080000);
 }
 for(unsigned skill:{125u,140u}){
  Fixture f;auto stale=f.a;++stale.generation;const auto plan=*skills::effects::energy_degeneration_hex(skill,12);
  check(!f.w.stage_energy_degeneration_hex(stale,f.b,plan,20,1000));
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,plan,20,1000));const auto old=f.effect(skill,1000);
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,plan,20,2000)&&f.effect(skill,2000)!=old);
  check(f.w.settle_resources(f.b,4000)&&f.w.settle_resources(f.a,4000));
  check(f.w.resources(2)->health==(skill==125?976:1000)&&f.w.resources(1)->health==(skill==140?994:1000));
 }
 for(unsigned skill:{125u,140u}){
  Fixture f;const auto plan=*skills::effects::energy_degeneration_hex(skill,12);
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,plan,20,1000));
  f.w.tick(40000);check(!f.effect(skill,40000));
  check(f.w.resources(2)->health==(skill==125?768:1000)&&f.w.resources(1)->health==(skill==140?942:1000));
  check(overcast::precise_current(*f.w.overcast_energy_snapshot(f.b))==5080000);
 }
 {
  Fixture f;const auto plan=*skills::effects::energy_degeneration_hex(140,12);
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,plan,20,1000));
  f.w.join(3,"Replacement caster",100,0,std::make_shared<world::Outbox>(),0,{}, {},1000,3);
  f.w.set_vitals(3,{1000,1000,100,100},100);const auto replacement=*f.w.effect_scope(3);
  check(f.w.settle_resources(replacement,2000));
  check(f.w.stage_energy_degeneration_hex(replacement,f.b,plan,20,2000));
  check(f.w.settle_resources(f.a,4000)&&f.w.settle_resources(replacement,4000));
  check(f.w.resources(1)->health==998&&f.w.resources(3)->health==996);
 }
 for(unsigned skill:{125u,140u})for(bool sparse:{false,true}){
  Fixture f;check(f.w.energy_capacity(2,1,0,1000,{0,-2}));f.w.set_vitals(2,{1000,1000,1,20},100);
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  if(!sparse)for(unsigned at=1100;at<10000;at+=100)f.w.tick(at);
  check(f.w.settle_resources(f.b,10000));
  if(f.effect(skill,10000)||f.w.resources(2)->health!=(skill==125?913:959))
   std::cerr<<"single skill="<<skill<<" sparse="<<sparse<<" hp="<<f.w.resources(2)->health<<" effect="<<f.effect(skill,10000)<<" energy="<<overcast::precise_current(*f.w.overcast_energy_snapshot(f.b))<<'\n';
  check(!f.effect(skill,10000)&&f.w.resources(2)->health==(skill==125?913:959));
  check(f.w.resources(1)->health==(skill==140?994:1000));
  check(overcast::precise_current(*f.w.overcast_energy_snapshot(f.b))==0);
  f.w.tick(10000);check(f.w.resources(2)->health==(skill==125?913:959));
 }
 for(bool mind:{false,true})for(bool sparse:{false,true}){
  Fixture f;check(f.w.energy_capacity(2,1,0,1000,{0,-2}));f.w.set_vitals(2,{1000,1000,1,20},100);
  for(unsigned skill:{125u,140u})check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  if(mind)check(f.w.apply_mind_wrack(f.a,f.b,12,false,1000));
  if(!sparse)for(unsigned at=1100;at<10000;at+=100)f.w.tick(at);
  check(f.w.settle_resources(f.b,10000));
  check(f.w.resources(2)->health==(mind?801:884)&&f.w.resources(1)->health==997);
  check(!f.effect(125,10000)&&!f.effect(140,10000)&&!f.effect(49,10000));
  f.w.tick(10000);check(f.w.resources(2)->health==(mind?801:884));
 }
 for(bool sparse:{false,true}){
  Fixture f;check(f.w.energy_capacity(2,1,0,1000,{0,-1}));f.w.set_vitals(2,{1000,1000,1,20},100);
  for(unsigned skill:{125u,140u})check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  if(!sparse)for(unsigned at=1100;at<10000;at+=100)f.w.tick(at);
  check(f.w.settle_resources(f.b,10000));
  check(f.w.resources(2)->health==872&&overcast::precise_current(*f.w.overcast_energy_snapshot(f.b))==600000);
  check(f.w.consume_energy(2,1,f.b,10000));
  check(overcast::precise_current(*f.w.overcast_energy_snapshot(f.b))==300000);
 }
 {
  Fixture f;check(f.w.energy_capacity(2,1,0,1000,{0,-2}));f.w.set_vitals(2,{1000,1000,1,20},100);
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  f.w.leave(1); // Exact departure retirement now permits the zero transaction.
  for(unsigned attempt=0;attempt<2;++attempt){
   f.w.move(2,250,0,10000);check(f.w.settle_resources(f.b,10000));
   const auto state=f.w.overcast_energy_snapshot(f.b);
   check(state&&state->curve.cursor()==10000&&overcast::precise_current(*state)==0);
   check(f.w.resources(2)->health==1000&&!f.effect(140,10000)&&!f.w.pending_energy_zero_snapshot(f.b));
  }
  const auto retired=f.b;f.w.leave(2);
  f.w.join(2,"Replacement target",200,0,std::make_shared<world::Outbox>(),0,{}, {},10000,2);
  f.w.set_vitals(2,{1000,1000,10,20},100);const auto replacement=*f.w.effect_scope(2);check(replacement!=retired);
  check(f.w.settle_resources(replacement,11000)&&f.w.resources(2)->health==1000);
 }
 for(bool drain:{false,true})for(unsigned mode=0;mode<4;++mode){
  Fixture f;const bool both=mode>=2;const unsigned single=mode==1?140:125;
  check(f.w.energy_capacity(2,1,0,1000,{0,both?0:-1}));f.w.set_vitals(2,{1000,1000,1,20},100);
  if(both){for(unsigned skill:{125u,140u})check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000));}
  else check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(single,12),20,1000));
  if(mode==3)check(f.w.apply_mind_wrack(f.a,f.b,12,false,1000));
  if(drain)check(f.w.energy_drain(f.a,f.b,*skills::effects::energy_drain(79,0),20,2000));
  else check(f.w.consume_energy(2,1,f.b,2000));
  const unsigned expected=mode==0?929:mode==1?959:mode==2?888:805;
  check(f.w.resources(2)->health==expected&&f.w.resources(1)->health==(mode?998:1000));
  check(!f.effect(125,2000)&&!f.effect(140,2000)&&!f.effect(49,2000));
  f.w.tick(2000);check(f.w.resources(2)->health==expected);
 }
 {
  Fixture f;f.w.set_vitals(2,{1000,1000,1,100},100);
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(125,12),20,1000));
  check(f.w.consume_energy(2,1,f.b,1001));
  check(f.w.resources(2)->health==1000&&f.effect(125,1001));
  check(overcast::precise_current(*f.w.overcast_energy_snapshot(f.b))==100); // UI zero is still positive Energy.
 }
 {
  Fixture f;check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  check(f.w.settle_resources(f.b,2000));f.w.leave(2);
  check(f.w.settle_resources(f.a,10000)&&f.w.resources(1)->health==998);
 }
 for(bool regeneration:{false,true}){
  Fixture f;check(f.w.energy_capacity(2,1,0,1000,{0,regeneration?-2:0}));f.w.set_vitals(2,{50,50,1,20},100);
  for(unsigned skill:{125u,140u})check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  check(f.w.apply_mind_wrack(f.a,f.b,12,false,1000));
  if(regeneration)f.w.tick(10000);else check(f.w.consume_energy(2,1,f.b,2000));
  check(f.w.resources(2)->health==0&&f.w.status_snapshot(f.b,10000)->empty());
  check(f.w.resources(1)->health==(regeneration?997:998));
  f.w.tick(11000);check(f.w.resources(2)->health==0&&f.w.resources(1)->health==(regeneration?997:998));
 }
 for(bool debt:{false,true}){
  Fixture f;check(f.w.energy_capacity(2,1,0,1000,{0,-1}));f.w.set_vitals(2,{1000,1000,1,debt?100u:21u},100);
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  check(f.w.energy_capacity(2,1,0,2000,{0,-1}));
  check(f.w.resources(2)->health==(debt?1000:959)&&bool(f.effect(140,2000))==debt);
  check(overcast::precise_current(*f.w.overcast_energy_snapshot(f.b))==(debt?-79*overcast::energy_unit:0));
 }
 for(unsigned skill:{125u,140u})for(bool deferred:{false,true}){
  Fixture f;f.target_box->drain();std::vector<world::Packet> visuals;
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000,deferred?&visuals:nullptr));
  const world::Packet expected{159,0,20,0,0,0,2,0,0,0,1,0,0,0,static_cast<unsigned char>(skill==125?233:252),0,0,0};
  const auto packets=f.target_box->drain();
  check(std::count(packets.begin(),packets.end(),expected)==(deferred?0:1));
  check(visuals==(deferred?std::vector<world::Packet>{expected}:std::vector<world::Packet>{}));
 }
 for(unsigned skill:{125u,140u}){
  Fixture f;check(f.w.hex_breaker(f.b,*skills::effects::hex_breaker(12),20,1000));std::vector<world::Packet> visuals;
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000,&visuals));
  check(visuals.empty()&&!f.effect(skill,1000)&&f.w.status_snapshot(f.b,1000)->empty());
  check(f.w.resources(1)->health==961&&!f.w.overcast_energy_snapshot(f.b));
  f.w.tick(2000);check(f.w.resources(1)->health==961&&f.w.resources(2)->health==1000);
 }
 for(unsigned skill:{125u,140u}){
  Fixture f;check(f.w.soul_barbs(f.a,f.b,*skills::effects::soul_barbs(12),20,1000));
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  check(f.w.resources(2)->health==973&&f.effect(skill,1000));
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  check(f.w.resources(2)->health==946);f.w.tick(1000);check(f.w.resources(2)->health==946);
 }
 for(bool blocked:{false,true}){
  Fixture f;if(!blocked)f.w.set_vitals(2,{10,10,20,100},100);
  check(f.w.soul_barbs(f.a,f.b,*skills::effects::soul_barbs(12),20,1000));
  if(blocked)check(f.w.hex_breaker(f.b,*skills::effects::hex_breaker(12),20,1000));
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  check(f.w.resources(2)->health==(blocked?1000:0)&&!f.effect(140,1000));
  f.w.tick(2000);check(f.w.resources(1)->health==(blocked?961:1000));
  if(blocked)check(f.w.resources(2)->health==1000); // Prevention is not an application event.
 }
 for(unsigned skill:{125u,140u}){
  Fixture f;check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  check(f.w.settle_resources(f.b,2000));f.target_box->drain();f.w.set_vitals(2,{700,700,0,30},100);
  const auto reset_packets=f.target_box->drain();
  check(std::find(reset_packets.begin(),reset_packets.end(),emotes::effects_2022(2,0))!=reset_packets.end());
  check(!f.effect(skill,2000)&&!f.w.overcast_energy_snapshot(f.b));
  check(f.w.settle_resources(f.a,7000)&&f.w.settle_resources(f.b,7000));
  check(f.w.resources(2)->health==700&&f.w.resources(1)->health==(skill==140?998:1000));
  // New ownership on the same lifetime must not find the old broken curve.
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,7000));
 }
 {
  Fixture f;check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  check(f.w.settle_resources(f.a,3000)); // Preserve the longer already-settled caster history.
  f.w.set_vitals(2,{700,700,10,30},100);
  check(f.w.settle_resources(f.a,7000)&&f.w.resources(1)->health==996);
 }
 {
  Fixture f;check(f.w.energy_capacity(2,1,0,1000,{0,-2}));f.w.set_vitals(2,{1000,1000,1,20},100);
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  f.w.leave(1);f.w.move(2,250,0,10000);
  check(f.w.overcast_energy_snapshot(f.b)->curve.cursor()==4000);
  f.w.set_vitals(2,{700,700,5,30},100); // Explicit replacement remains valid after a completed zero event.
  check(f.w.settle_resources(f.b,11000)&&f.w.resources(2)->health==700&&!f.effect(140,11000));
 }
 for(unsigned skill:{125u,140u})for(bool known:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  if(known){d.spawn_energy=combat::npcs::Energy{0,100};d.energy_pips=2;}n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);w.tick(100);
  const auto a=*w.effect_scope(1),target=*w.effect_scope(100);check(w.settle_resources(a,1000));
  std::vector<world::Packet> visuals;
  check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(skill,12),20,1000,&visuals)==known);
  if(!known){check(visuals.empty());w.tick(4000);check(w.resources(1)->health==1000);continue;}
  const world::Packet expected{159,0,20,0,0,0,100,0,0,0,1,0,0,0,static_cast<unsigned char>(skill==125?233:252),0,0,0};
  check(visuals==std::vector<world::Packet>{expected});
  check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(skill,12),20,2000));
  w.tick(4000);const auto snapshot=w.npc_combat_snapshot(target);check(snapshot&&snapshot->health);
  check(snapshot->health->current==(skill==125?976:1000)&&w.resources(1)->health==(skill==140?994:1000));
  check(w.npc_precise_energy_snapshot(target)==480000);
 }
 for(unsigned mode=0;mode<4;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{10,10};d.damageable=d.conditions_allowed=true;
  d.spawn_energy=combat::npcs::Energy{20,100};d.energy_pips=2;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);w.tick(100);
  const auto a=*w.effect_scope(1),target=*w.effect_scope(100);check(w.settle_resources(a,1000));
  if(mode==0)check(w.soul_barbs(a,target,*skills::effects::soul_barbs(12),20,1000));
  check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(mode>=2?125:140,12),20,1000));
  if(mode==3)check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  if(mode==1)check(w.damage_npc(a,target,{combat::Kind::armor_ignoring,2000,20,194},1248,2000).killed);
  w.tick(4000);check(w.npc_combat_snapshot(target)->health->current==0&&w.resources(1)->health==((mode==1||mode==3)?998:1000));
  const auto deaths=w.take_npc_deaths(a);check(deaths.size()==1&&deaths[0].time==(mode==0?1000:mode==1?2000:2250));
 }
 {
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  d.spawn_energy=combat::npcs::Energy{1,20};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);w.tick(100);
  const auto a=*w.effect_scope(1),target=*w.effect_scope(100);check(w.settle_resources(a,1000));
  check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  for(unsigned attempt=0;attempt<2;++attempt){w.move(1,10,0,10000);
   check(w.npc_precise_energy_snapshot(target)==0&&w.npc_combat_snapshot(target)->health->current==959&&w.resources(1)->health==994);
  }
 }
 for(unsigned mode=0;mode<4;++mode)for(bool sparse:{false,true})for(bool lethal:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  const unsigned hp=lethal?20:1000;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(hp),hp};
  d.damageable=d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{1,20};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);w.tick(100);
  const auto a=*w.effect_scope(1),target=*w.effect_scope(100);check(w.settle_resources(a,1000));
  check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(mode==1?140:125,12),20,1000));
  if(mode>=2)check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  if(mode==3)check(w.apply_mind_wrack(a,target,12,false,1000));
  if(!sparse)for(unsigned at=1100;at<10000;at+=100)w.tick(at);
  check(w.settle_resources(a,10000));
  const auto snapshot=w.npc_combat_snapshot(target);const unsigned expected=mode==0?913:mode==1?959:mode==2?884:801;
  check(snapshot&&snapshot->health&&std::abs(snapshot->health->current-(lethal?0:expected))<1e-7&&!(snapshot->effects&0x800));
  //20HP Wither alone dies from degeneration at3500,before its4000 zero.
  const unsigned caster_hp=mode==0?1000:mode==1?994:997;
  check(w.resources(1)->health==caster_hp);
  const auto deaths=w.take_npc_deaths(a);check(deaths.size()==unsigned(lethal));
  if(lethal)check(deaths[0].time==(mode==0?3500:mode==1?4000:2500));
  w.tick(11000);check(w.take_npc_deaths(a).empty()&&w.resources(1)->health==caster_hp);
 }
 for(bool sparse:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  d.spawn_energy=combat::npcs::Energy{1,20};d.energy_pips=1;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);w.tick(100);
  const auto a=*w.effect_scope(1),target=*w.effect_scope(100);check(w.settle_resources(a,1000));
  for(unsigned skill:{125u,140u})check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  if(!sparse)for(unsigned at=1100;at<10000;at+=100)w.tick(at);
  w.tick(10000);
  check(std::abs(w.npc_combat_snapshot(target)->health->current-864.8)<1e-7&&w.resources(1)->health==993);
  check(w.npc_precise_energy_snapshot(target)==510000); // Restored base pip from4900 through10000.
 }
 for(unsigned mode=0;mode<4;++mode)for(bool lethal:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  const unsigned hp=lethal?20:1000;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(hp),hp};d.damageable=d.conditions_allowed=true;
  d.spawn_energy=combat::npcs::Energy{1,20};d.energy_pips=mode>=2?2:1;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},1000,id);w.set_vitals(id,{1000,1000,100,100},100);}
  w.tick(1000);const auto a=*w.effect_scope(1),drainer=*w.effect_scope(2),target=*w.effect_scope(100);
  check(w.settle_resources(a,1000));
  check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(mode==1?140:125,12),20,1000));
  if(mode>=2)check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(140,12),20,1000));
  if(mode==3)check(w.apply_mind_wrack(a,target,12,false,1000));
  check(w.energy_drain(drainer,target,*skills::effects::energy_drain(40,0),20,2000));
  const double expected=lethal?0:mode==0?929:mode==1?959:mode==2?888:805;
  check(std::abs(w.npc_combat_snapshot(target)->health->current-expected)<1e-7&&!(w.npc_combat_snapshot(target)->effects&0x800));
  check(w.resources(1)->health==(mode?998:1000));
  const auto deaths=w.take_npc_deaths(a);check(deaths.size()==unsigned(lethal)&&w.take_npc_deaths(drainer).empty());
  if(lethal)check(deaths[0].time==2000);
  w.tick(3000);check(std::abs(w.npc_combat_snapshot(target)->health->current-expected)<1e-7&&w.resources(1)->health==(mode?998:1000));
  check(w.take_npc_deaths(a).empty());
 }
 for(bool hexes:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  d.spawn_energy=combat::npcs::Energy{1,20};d.energy_pips=2;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);w.set_vitals(id,{1000,1000,100,100},100);}
  w.tick(100);const auto a=*w.effect_scope(1),drainer=*w.effect_scope(2),target=*w.effect_scope(100);
  check(w.settle_resources(a,1000));
  if(hexes)for(unsigned skill:{125u,140u})check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  check(w.apply_mind_wrack(a,target,12,false,1000));
  check(w.energy_drain(drainer,target,*skills::effects::energy_drain(40,0),20,2000));
  check(std::abs(w.npc_combat_snapshot(target)->health->current-(hexes?992:1000))<1e-7);
  check(w.npc_precise_energy_snapshot(target)==(hexes?180000:80000));
  check(w.npc_combat_snapshot(target)->effects&0x800);
 }
 for(unsigned caster_health:{2u,3u,4u})for(bool sparse:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  d.spawn_energy=combat::npcs::Energy{1,20};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},1000,id);w.set_vitals(id,{id==1?caster_health:1000u,1000,100,100},100);}
  w.tick(1000);const auto a=*w.effect_scope(1),observer=*w.effect_scope(2),target=*w.effect_scope(100);
  check(w.settle_resources(a,1000));
  for(unsigned skill:{125u,140u})check(w.stage_energy_degeneration_hex(a,target,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  check(w.apply_mind_wrack(a,target,12,false,1000));
  if(!sparse)for(unsigned at=1100;at<10000;at+=100)w.tick(at);
  const unsigned expected_caster=caster_health>3?1:0;
  w.tick(10000);if(w.resources(1)->health!=expected_caster||w.npc_combat_snapshot(target)->effects&0x800)
   std::cerr<<"coincident caster="<<w.resources(1)->health<<" npc="<<w.npc_combat_snapshot(target)->health->current<<" effects="<<w.npc_combat_snapshot(target)->effects<<" energy="<<*w.npc_precise_energy_snapshot(target)<<" deferred="<<w.energy_hex_zero_diagnostic()<<'\n';
  check(w.resources(1)->health==expected_caster&&!(w.npc_combat_snapshot(target)->effects&0x800));
  check(std::abs(w.npc_combat_snapshot(target)->health->current-(caster_health>3?801:988))<1e-7);
  const auto deaths=w.player_death_times({1,2});
  check(deaths[0]==(caster_health>3?std::optional<std::uint64_t>{}:std::optional<std::uint64_t>{1000+caster_health*500}));
  check(w.settle_resources(observer,11000));
 }
 for(unsigned skill:{125u,140u}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},1000,1);w.set_vitals(1,{1000,1000,100,100},100);
  w.tick(1000);const auto a=*w.effect_scope(1),target=*w.effect_scope(100);
  const auto plan=*skills::effects::energy_degeneration_hex(skill,12);
  check(w.stage_energy_degeneration_hex(a,target,plan,20,1000));
  w.tick(1000+plan.duration+100);
  check(!(w.npc_combat_snapshot(target)->effects&0x800));
  check(w.stage_energy_degeneration_hex(a,target,plan,20,1000+plan.duration+200));
 }
 for(unsigned initial_energy:{2u,100u}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  d.spawn_energy=combat::npcs::Energy{initial_energy,100};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},1000,id);w.set_vitals(id,{id==1?1u:1000u,1000,100,100},100);}
  w.tick(1000);const auto first=*w.effect_scope(1),second=*w.effect_scope(2),target=*w.effect_scope(100);
  const auto plan=*skills::effects::energy_degeneration_hex(140,12);
  check(w.stage_energy_degeneration_hex(first,target,plan,20,1000));
  w.tick(2000);check(w.resources(1)->health==0);
  check(w.stage_energy_degeneration_hex(second,target,plan,20,2000));
  w.tick(4000);check(w.resources(1)->health==0&&w.resources(2)->health==996);
  check(w.npc_combat_snapshot(target)->effects&0x800);
  if(initial_energy==2){
   w.tick(10000);check(w.resources(1)->health==0&&w.resources(2)->health==990);
   check(w.npc_combat_snapshot(target)->health->current==959&&!(w.npc_combat_snapshot(target)->effects&0x800));
   w.tick(11000);check(w.npc_combat_snapshot(target)->health->current==959&&w.resources(2)->health==990);
  }
 }
 {
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  d.spawn_energy=combat::npcs::Energy{1,20};d.energy_pips=0;n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,2u}){w.join(id,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},1000,id);w.set_vitals(id,{1000,1000,100,100},100);}
  w.tick(1000);const auto caster=*w.effect_scope(1),target=*w.effect_scope(100);
  for(unsigned skill:{125u,140u})check(w.stage_energy_degeneration_hex(caster,target,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  w.leave(1);w.tick(10000);
  check(!w.pending_energy_zero_snapshot(target));
  const auto hp=w.npc_combat_snapshot(target)->health->current;check(hp==988);
  for(unsigned at:{11000u,12000u}){
   w.tick(at);check(!w.pending_energy_zero_snapshot(target));
   check(w.npc_combat_snapshot(target)->health->current==hp);
  }
  w.leave(2);w.configure_npcs({n});
  check(*w.effect_scope(100)!=target);w.tick(13000);
  check(!w.pending_energy_zero_snapshot(target));
 }
 for(unsigned caster_health:{2u,3u,4u})for(bool sparse:{false,true}){
  Fixture f;f.w.set_vitals(1,{caster_health,1000,100,100},100);
  check(f.w.energy_capacity(2,1,0,1000,{0,-2}));f.w.set_vitals(2,{1000,1000,1,20},100);
  for(unsigned skill:{125u,140u})check(f.w.stage_energy_degeneration_hex(f.a,f.b,*skills::effects::energy_degeneration_hex(skill,12),20,1000));
  if(!sparse)for(unsigned at=1100;at<10000;at+=100)f.w.tick(at);
  f.w.tick(10000);
  check(f.w.resources(1)->health==(caster_health>3?1u:0u));
  check(f.w.resources(2)->health==(caster_health>3?884u:988u));
  check(!f.effect(125,10000)&&!f.effect(140,10000)&&!f.w.pending_energy_zero_snapshot(f.b));
  check(f.w.settle_resources(f.b,11000));
 }
 {
  Fixture f;f.w.set_vitals(1,{1,1000,100,100},100);
  check(f.w.energy_capacity(2,1,0,1000,{0,-2}));f.w.set_vitals(2,{1000,1000,2,20},100);
  f.w.join(3,"New caster",100,0,std::make_shared<world::Outbox>(),0,{}, {},1000,1);f.w.set_vitals(3,{1000,1000,100,100},100);
  const auto next=*f.w.effect_scope(3);const auto plan=*skills::effects::energy_degeneration_hex(140,12);
  check(f.w.stage_energy_degeneration_hex(f.a,f.b,plan,20,1000));f.w.tick(2000);
  check(f.w.resources(1)->health==0&&f.w.stage_energy_degeneration_hex(next,f.b,plan,20,2000));
  f.w.tick(10000);
  if(f.w.resources(1)->health!=0||f.w.resources(3)->health!=990)
   std::cerr<<"player handoff old="<<f.w.resources(1)->health<<" new="<<f.w.resources(3)->health<<" target="<<f.w.resources(2)->health<<" energy="<<overcast::precise_current(*f.w.overcast_energy_snapshot(f.b))<<" pending="<<bool(f.w.pending_energy_zero_snapshot(f.b))<<'\n';
  check(f.w.resources(1)->health==0&&f.w.resources(3)->health==990);
  check(f.w.resources(2)->health==959&&!f.effect(140,10000));
  check(f.w.settle_resources(f.b,11000)&&f.w.resources(2)->health==959);
 }
 for(unsigned skill:{125u,140u})for(unsigned cleanse:{67u,302u})for(unsigned remove_at:{2000u,4000u}){
  world::Instance w;
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?1000:0,0,std::make_shared<world::Outbox>(),0,{}, {},1000,id==1?1:2);w.set_vitals(id,{1000,1000,id==2?1u:100u,id==2?20u:100u},100);}
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(2),ally=*w.effect_scope(3);
  check(w.energy_capacity(2,1,0,1000,{0,-2}));w.set_vitals(2,{1000,1000,1,20},100);
  const auto plan=*skills::effects::energy_degeneration_hex(skill,12);
  check(w.stage_energy_degeneration_hex(caster,target,plan,20,1000));
  const auto old=w.status_snapshot(target,1000)->front().instance;
  const auto removal=cleanse==67?*skills::effects::shatter_hex(0):*skills::effects::smite_hex(0);
  check(w.shatter_delusions(ally,target,removal,20,remove_at));
  w.tick(10000);
  check(w.status_snapshot(target,10000)->empty()&&!w.pending_energy_zero_snapshot(target));
  check(w.resources(2)->health==(remove_at==4000?(skill==125?913u:959u):(skill==125?992u:1000u)));
  check(w.resources(1)->health==(skill==125?1000u:remove_at==4000?994u:998u));
  w.set_vitals(2,{1000,1000,2,20},100);
  check(w.stage_energy_degeneration_hex(caster,target,plan,20,11000));
  check(w.status_snapshot(target,11000)->front().instance!=old);
 }
 for(unsigned skill:{125u,140u})for(unsigned cleanse:{22u,1049u})for(unsigned remove_at:{2000u,4000u}){
  world::Instance w;
  for(unsigned id:{1u,2u,3u}){w.join(id,"Actor",id==1?1000:0,0,std::make_shared<world::Outbox>(),0,{}, {},1000,id==1?1:2);w.set_vitals(id,{1000,1000,id==2?1u:100u,id==2?20u:100u},100);}
  const auto caster=*w.effect_scope(1),target=*w.effect_scope(2),ally=*w.effect_scope(3);
  check(w.energy_capacity(2,1,0,1000,{0,-2}));w.set_vitals(2,{1000,1000,1,20},100);
  const auto plan=*skills::effects::energy_degeneration_hex(skill,12);
  check(w.stage_energy_degeneration_hex(caster,target,plan,20,1000));
  const auto offer=w.prepare_inspired_hex(ally,target,*skills::effects::inspired_hex(cleanse,12),remove_at);check(bool(offer));
  const auto result=w.commit_inspired_hex(*offer);check(bool(result));
  check(bool(result->removed)==(remove_at==2000));
  if(result->removed){check(result->removed->skill==skill&&result->copy.kind==skills::effects::HexCopyKind::player);check(!w.commit_inspired_hex(*offer));}
  w.tick(10000);
  check(w.status_snapshot(target,10000)->empty()&&!w.pending_energy_zero_snapshot(target));
  check(w.resources(2)->health==(remove_at==4000?(skill==125?913u:959u):(skill==125?992u:1000u)));
  check(w.resources(1)->health==(skill==125?1000u:remove_at==4000?994u:998u));
  w.set_vitals(2,{1000,1000,2,20},100);check(w.stage_energy_degeneration_hex(caster,target,plan,20,11000));
 }
 std::cout<<"PASS trusted World player/NPC Energy hex application,regeneration/discrete zero commits,credit and native packets;native casting pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
