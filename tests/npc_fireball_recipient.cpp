#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("NPC Fireball recipient line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<4;++mode){world::Instance w;std::vector<world::Instance::Npc> bodies;
  for(unsigned id=100;id<103;++id){world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==102?500.f:float(100+(id-100)*40);n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
   if(mode!=1||id!=101)d.fire_armor=60;n.combat_definition=d;bodies.push_back(n);
  }
  w.configure_npcs(bodies);auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  check(w.energy_capacity(1,1,0,100,{80,-2}));w.join(2,"Enemy",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
  check(w.elemental_armor(*w.effect_scope(2),60,60));const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);
  Slots bar{};bar[0]={186,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[186]=1;attributes::Ranks ranks{};
  casting::Hooks h;std::vector<world::Packet> packets;h.resources=[&](unsigned id){return w.resources(id);};h.target_scope=[&](unsigned id){return w.effect_scope(id);};
  h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};h.consume_energy_at=[&](unsigned cost,auto at){return w.consume_energy(1,cost,owner,at);};
  h.projectile_target_valid=[](unsigned id,float range){return id==100&&range==1248;};
  h.elemental=[&](auto selected,const auto& plan,auto at)->casting::EffectCompletion{std::vector<world::Packet> visuals;const bool ok=w.elemental(owner,selected,plan,20,at,&visuals);return {ok,std::move(visuals)};};
  casting::Runtime runtime(1);check(runtime.start(1000,{186,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted);
  check(w.resources(1)->energy==90);if(mode==2)runtime.cancel(h);if(mode==3)runtime.interrupt(1100,h);
  runtime.update(3000,h);check(w.npc_combat_snapshot(target)->health->current==1000);
  check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==unsigned(mode<2));
  w.tick(4000);const bool hit=mode==0;
  check(w.npc_combat_snapshot(target)->health->current==(hit?993:1000));
  check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==(hit?993:1000));
  check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==1000);
  check(w.resources(2)->health==(hit?993u:1000u));w.tick(4000);
  check(w.npc_combat_snapshot(target)->health->current==(hit?993:1000));
 }
 std::cout<<"PASS paid Fireball NPC launch,delayed mixed-area damage,range exclusion,unknown-neighbor atomic admission,cancel/interrupt and no replay\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
