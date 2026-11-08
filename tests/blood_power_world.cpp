#include "gw_blood_is_power.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Blood is Power World line "+std::to_string(at.line()));
}
struct Fixture {
 world::Instance w;Scope a,b;std::shared_ptr<world::Outbox> receiver=std::make_shared<world::Outbox>();
 Fixture(bool full=false,unsigned current=480,unsigned maximum=480,unsigned target_health=1000,float x=60,unsigned team=1){
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{current,maximum,50,100},100);
  w.join(2,"Ally",x,0,receiver,0,{}, {},100,team);w.set_vitals(2,{target_health,target_health,full?100u:0u,100},100);
  a=*w.effect_scope(1);b=*w.effect_scope(2);check(w.consume_energy(2,0,b,1000));receiver->drain();
 }
 std::int64_t energy()const{const auto state=w.overcast_energy_snapshot(b);check(bool(state));return overcast::precise_current(*state);}
 unsigned effect(std::uint64_t at)const{const auto entries=w.status_snapshot(b,at);check(bool(entries));unsigned id=0;for(const auto& entry:*entries)if(entry.skill==119){check(!id);id=entry.instance;}return id;}
};
int main()try{
 // The fixture earns0.6 Energy at2pips from time100 to1000 before promotion.
 constexpr std::uint64_t initial=180000;
 const auto plan=*blood_is_power(12);std::optional<skills::sacrifice::Loss> paid;std::vector<world::Packet> visuals;
 {
  Fixture f;check(f.w.energy_enchantment(f.a,f.b,plan,1000,&paid,&visuals));
  check(paid&&paid->lost==158&&paid->remaining==322&&!paid->killed&&f.w.resources(1)->health==322);
  check(f.energy()==initial);
  check(visuals==std::vector<world::Packet>{*skills::particles::targeted_body_2022(2,1,119)}&&f.effect(1000));
  const auto packets=f.receiver->drain();check(std::find(packets.begin(),packets.end(),vitals::energy_regeneration_2022(2,7,100))!=packets.end());
  check(!f.w.status_mutation(f.b,119,123,12,1000,11000,true));
  const auto consumed=f.w.consume_energy(2,1,f.b,16000);
  if(!consumed||f.energy()!=initial+7700000||f.effect(16000))std::cerr<<"consume="<<consumed<<" Energy="<<f.energy()<<" effect="<<f.effect(16000)<<'\n';
  check(consumed);check(f.energy()==initial+7700000&&!f.effect(16000));
 }
 {
  Fixture f(true);check(f.w.energy_enchantment(f.a,f.b,plan,1000,&paid));f.receiver->drain();f.w.tick(11000);
  check(!f.effect(11000)&&f.w.resources(2)->energy==100);
  const auto packets=f.receiver->drain();check(std::find(packets.begin(),packets.end(),vitals::energy_regeneration_2022(2,2,100))!=packets.end());
 }
 {
  Fixture f;check(f.w.energy_enchantment(f.a,f.b,plan,1000,&paid));const auto id=f.effect(1000);
  check(f.w.status_mutation(f.b,119,id,12,5000,std::nullopt,false));
  check(f.w.consume_energy(2,0,f.b,16000)&&f.energy()==initial+5000000);
 }
 {
  Fixture f;check(f.w.energy_enchantment(f.a,f.b,plan,1000,&paid));const auto old=f.effect(1000);
  check(f.w.energy_enchantment(f.a,f.b,*blood_is_power(0),5000,&paid));check(f.effect(5000)!=old);
  f.w.tick(11000);check(f.effect(11000));
  f.w.tick(16000);check(f.energy()==initial+8000000&&!f.effect(16000));
 }
 for(unsigned mode=0;mode<2;++mode){
  Fixture f(false,mode==0?1:10,mode==0?1:480);
  check(f.w.energy_enchantment(f.a,f.b,plan,1000,&paid));
  check(paid&&paid->killed==(mode!=0)&&paid->lost==(mode==0?0:11));check(f.effect(1000));
 }
 {
  Fixture f;check(f.w.energy_enchantment(f.a,f.b,plan,1000,&paid));check(f.w.kill(f.b,6000));
  const auto before=f.energy();check(before==initial+3500000&&!f.effect(6000));f.w.tick(16000);check(f.energy()==before);
 }
 {
  Fixture f(false,480,480,1);check(f.w.energy_enchantment(f.a,f.b,plan,1000,&paid));
  f.w.join(3,"Enemy",400,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(3,{1000,1000,100,100},100);
  check(f.w.apply_condition(*f.w.effect_scope(3),f.b,combat::conditions::Kind::disease,1000,10000));
  f.w.tick(2000);check(f.w.resources(2)->health==0&&f.energy()==initial+87500&&!f.effect(2000));
 }
 {
  Fixture f(true);check(f.w.energy_enchantment(f.a,f.b,plan,1000,&paid));
  check(f.w.activate_overcast_energy(f.b,170,25,1,1000));f.w.tick(16000);
  check(f.energy()==28485000&&f.w.overcast_energy_snapshot(f.b)->curve.burden_units()==505000);
 }
 {
  Fixture f;check(f.w.energy_capacity(2,1,0,1000));
  check(f.w.energy_enchantment(f.a,f.b,plan,1000,&paid));check(f.energy()==-23820000);
  check(f.w.consume_energy(2,0,f.b,16000));check(f.energy()==-15820000);
 }
 for(unsigned mode=0;mode<4;++mode){
  Fixture f(false,480,480,1000,mode==1?2000.f:60.f,mode==2?2:1);auto target=mode==0?f.a:f.b;if(mode==3)++target.generation;
  check(!f.w.energy_enchantment(f.a,target,plan,1000,&paid));check(!paid&&f.w.resources(1)->health==480&&!f.w.overcast_energy_snapshot(f.b));
 }
 for(unsigned mode=0;mode<7;++mode){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=mode==2?combat::npcs::Relation::neutral:mode==3?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
  d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
  if(mode!=4)d.spawn_energy=combat::npcs::Energy{0,100};if(mode!=4&&mode!=5)d.energy_pips=2;
  n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{480,480,50,100},100);w.tick(100);
  const auto source=*w.effect_scope(1),npc=*w.effect_scope(100);auto selected=npc;if(mode==6)++selected.generation;
  const bool accepted=mode<2;check(w.energy_enchantment(source,selected,plan,1000,&paid,&visuals)==accepted);
  if(!accepted){check(!paid&&w.resources(1)->health==480);continue;}
  check(paid&&paid->lost==158&&w.resources(1)->health==322&&(*w.npc_precise_energy_snapshot(npc))==180000);
  check(visuals==std::vector<world::Packet>{*skills::particles::targeted_body_2022(100,1,119)});
  check(w.npc_combat_snapshot(npc)->effects&0x80);
  if(mode==1)check(w.energy_enchantment(source,npc,*blood_is_power(0),5000,&paid));
  w.tick(11000);check(bool(w.npc_combat_snapshot(npc)->effects&0x80)==(mode==1));
  w.tick(16000);check(w.npc_precise_energy_snapshot(npc)==8180000&&!(w.npc_combat_snapshot(npc)->effects&0x80));
 }
 {
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::allied_to_players;
  d.spawn_health=combat::Health{1,1000};d.damageable=d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{0,100};d.energy_pips=2;
  d.fleshy=true;d.fleshy_evidence="fixture";d.creature_type="human";d.creature_type_evidence="fixture";d.disease_immune=false;d.disease_immunity_evidence="fixture";
  n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id=1;id<=3;++id){w.join(id,"Player",id==2?160.f:1000.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
  w.tick(100);const auto caster=*w.effect_scope(1),infected=*w.effect_scope(2),cause=*w.effect_scope(3),npc=*w.effect_scope(100);
  check(w.energy_enchantment(caster,npc,plan,1000,&paid));check(w.apply_condition(cause,infected,combat::conditions::Kind::disease,1000,10000));
  w.tick(1100);check(w.has_condition(npc,combat::conditions::Kind::disease,1100));
  w.tick(2000);check(w.npc_combat_snapshot(npc)->health->current==0&&w.npc_precise_energy_snapshot(npc)==337500);
  const auto deaths=w.take_npc_deaths(cause);check(deaths.size()==1&&deaths[0].time==1225);
  w.tick(3000);check(w.npc_precise_energy_snapshot(npc)==337500&&w.take_npc_deaths(cause).empty());
 }
 std::cout<<"PASS Blood is Power player World owned timer/UI,precise Energy,expiry/removal/renewal,zero/lethal sacrifice,delayed death and invalid allies;reviewed NPCs and unknown-fact rejection;Runtime separately tested\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
