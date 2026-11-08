#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("World timed order line "+std::to_string(at.line()));
}
int main()try{
 for(bool npc:{false,true})for(bool resource_event:{false,true}){
  unsigned reference=0;
  for(bool sparse:{false,true}){
   world::Instance w;
   if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;
    n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
    d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
    d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;
    d.lightning_armor=80;d.fire_armor=60;d.lightning_armor_evidence=d.fire_armor_evidence="test";
    n.combat_definition=d;w.configure_npcs({n});}
   for(unsigned id:{1u,3u}){
    w.join(id,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
    w.set_vitals(id,{1000,1000,100,100},100);
   }
   if(!npc){w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    w.set_vitals(2,{1000,1000,100,100},100);
    check(w.lightning_armor(*w.effect_scope(2),80));check(w.elemental_armor(*w.effect_scope(2),60,{}));}
   const auto source=*w.effect_scope(1),field_source=*w.effect_scope(3),target=*w.effect_scope(npc?100:2);
   check(w.activate_overcast_energy(source,219,0,1,200));
   check(w.lightning(source,target,*skills::effects::lightning(222,12),20,1000));
   const auto plan=*skills::effects::fire_storm(0,197);
   check(w.fire_storm(field_source,target,plan,20,1000));
   if(!sparse)for(unsigned at=2000;at<11000;at+=100)w.tick(at);
   // Exercise both tick-driven and external resource-driven settlement.
   if(resource_event)check(w.restore_energy(1,0,source,11000));else w.tick(11000);
   const auto hp=npc?unsigned(w.npc_combat_snapshot(target)->health->current):w.resources(2)->health;
   check(hp==1000-82-10*plan.damage);
   if(!sparse)reference=hp;else check(hp==reference);
  }
 }
 for(bool reverse:{false,true})for(bool external:{false,true}){
  std::vector<unsigned> reference;
  for(bool sparse:{false,true}){
   world::Instance w;const unsigned root=reverse?3:2,neighbor=reverse?2:3;
   for(unsigned id:{1u,2u,3u}){
    w.join(id,"Actor",id==1?0:id==root?60:120,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
    w.set_vitals(id,{500,500,100,100},100);
    check(w.elemental_armor(*w.effect_scope(id),60,{}));
   }
   const auto c=*w.effect_scope(1),a=*w.effect_scope(root),b=*w.effect_scope(neighbor);
   check(w.lightning_armor(a,80)&&w.activate_overcast_energy(c,219,0,1,200));
   check(w.apply_fevered_dreams(c,a,12,false,200,skills::effects::FeveredDazedSpan::attempted_duration));
   check(w.degeneration_hex(c,b,*skills::effects::degeneration_hex(2998,12),200));
   check(w.status_mutation(a,37,100,0,200,5200,true));
   check(w.lightning(c,a,*skills::effects::lightning(222,12),20,1000));
   check(w.fire_storm(c,a,*skills::effects::fire_storm(0,197),20,1000));
   if(!sparse)for(unsigned at=1100;at<9100;at+=100)w.tick(at);
   if(external)check(w.settle_resources(c,9100));else w.tick(9100);
   check(w.fevered_boundary_failures()==0);
   check(!w.has_condition(a,combat::conditions::Kind::crippled,9100)&&
         !w.has_condition(b,combat::conditions::Kind::crippled,9100));
   const std::vector<unsigned> result{w.resources(root)->health,w.resources(neighbor)->health};
   check(result[0]==378&&result[1]==426); //82 Lightning,40 field,34 Fragility on neighbor.
   if(!sparse)reference=result;else check(result==reference);
  }
 }
 for(bool move:{false,true})for(bool sparse:{false,true}){
  world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
  check(w.configure_projectiles({100,32}));const auto c=*w.effect_scope(1),a=*w.effect_scope(2);
  const auto flare=*skills::effects::elemental(194,12);
  check(w.elemental(c,a,flare,20,1000)&&w.fire_storm(c,a,*skills::effects::fire_storm(0,197),20,1000));
  if(!sparse)for(unsigned at=1100;at<3000;at+=100)w.tick(at);
  if(move){w.move(2,1000,0,3000);check(w.resources(2)->health==1000-flare.damage-10);}
  if(!sparse)for(unsigned at=3000;at<5000;at+=100)w.tick(at);
  w.tick(5000);check(w.resources(2)->health==1000-flare.damage-(move?10:20));
 }
 for(bool npc:{false,true}){
  world::Instance w;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};
   d.damageable=true;d.conditions_allowed=true;d.cold_armor=60;d.cold_armor_evidence="test";n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:600,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{1000,1000,100,100},100);check(w.elemental_armor(*w.effect_scope(id),{},60));}
  const auto c=*w.effect_scope(1),a=*w.effect_scope(npc?100:2);
  check(w.water_projectile(c,a,*skills::effects::water_projectile(213,15),20,1000));
  w.tick(10000);
  check(npc?w.npc_combat_snapshot(a)->health->current==915:w.resources(2)->health==915);
  check(npc?!(w.npc_combat_snapshot(a)->effects&0x800):w.status_snapshot(a,10000)->empty());
 }
 {
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=200;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{10,1000};
  d.damageable=true;d.conditions_allowed=true;d.fire_armor=60;d.fire_armor_evidence="test";n.combat_definition=d;w.configure_npcs({n});
  for(unsigned id:{1u,3u}){w.join(id,"Caster",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{1000,1000,100,100},100);}
  check(w.configure_projectiles({100,32}));const auto a=*w.effect_scope(1),b=*w.effect_scope(3),target=*w.effect_scope(100);
  const auto flare=*skills::effects::elemental(194,12);
  check(w.elemental(a,target,flare,20,1000)&&w.elemental(b,target,flare,20,1000));
  w.tick(10000);check(w.take_npc_deaths(a).empty());const auto deaths=w.take_npc_deaths(b);
  check(deaths.size()==1&&deaths[0].time==2000);
 }
 for(bool sparse:{false,true}){
  world::Instance w;std::vector<world::Instance::Npc> population;
  for(unsigned id=100;id<103;++id){world::Instance::Npc n;n.id=n.definition.id=id;n.x=100+200.f*(id-100);n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{30,30};
   d.damageable=true;d.conditions_allowed=true;d.cold_armor=60;d.cold_armor_evidence="test";n.combat_definition=d;population.push_back(n);}
  w.configure_npcs(population);w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1);check(w.deathly_swarm(source,*w.effect_scope(100),*skills::effects::deathly_swarm(0),20,100));
  if(!sparse)for(unsigned at=1100;at<3100;at+=100)w.tick(at);
  w.tick(3100);const auto deaths=w.take_npc_deaths(source);check(deaths.size()==3);
  for(unsigned n=0;n<3;++n)check(deaths[n].time==1100+1000*n&&deaths[n].target.actor==100+n);
 }
 {
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1,100};
  d.damageable=true;d.conditions_allowed=true;d.lightning_armor=60;d.lightning_armor_evidence="test";n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1);check(w.configure_projectiles({100,32}));
  check(w.lightning_javelin(source,*w.effect_scope(100),*skills::effects::lightning_javelin(0),20,166,1000));
  w.tick(10000);const auto deaths=w.take_npc_deaths(source);check(deaths.size()==1&&deaths[0].time==2000);
 }
 for(bool npc:{false,true})for(bool blind_after:{false,true}){
  world::Instance w;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};
   d.damageable=true;d.conditions_allowed=true;d.earth_armor=60;d.evidence.armor="test";n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:100,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{500,500,100,100},100);check(w.combat_armor(*w.effect_scope(id),60));}
  const auto c=*w.effect_scope(1),a=*w.effect_scope(npc?100:2),enemy=*w.effect_scope(2);
  combat::weapon::Owned wand{1,1,{20,20,11},0,false,true};
  check(w.configure_projectiles({100,32})&&w.ordinary_weapon(c,wand,20));w.configure_attack_miss_draws([]{return .5;});
  check(w.ordinary_attack(c,a.actor,100));w.tick(600);w.cancel_ordinary_attack(c);
  check(w.apply_condition(enemy,c,combat::conditions::Kind::blind,blind_after?1800:800,5000));
  w.tick(5000);const auto events=w.take_attack_outcomes(c);
  check(events.size()==1&&events[0].at==1600&&events[0].released==600);
  check(events[0].outcome==(blind_after?combat::weapon::AttackOutcome::hit:combat::weapon::AttackOutcome::miss));
  check(npc?w.npc_combat_snapshot(a)->health->current==(blind_after?480:500):w.resources(2)->health==(blind_after?480:500));
 }
 {
  // Same-position foes retain the policy's minimum1ms flight and the separate
  // second release. A sparse update must finish both without replay.
  world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{1000,1000,100,100},100);check(w.combat_armor(*w.effect_scope(id),60));}
  check(w.stone_daggers(*w.effect_scope(1),*w.effect_scope(2),*skills::effects::stone_daggers(15),20,1000));
  w.tick(1501);check(w.resources(2)->health==934);w.tick(1601);check(w.resources(2)->health==934);
 }
 std::cout<<"PASS World timed effects,spell and released ordinary deliveries: fine/sparse,external movement,hex expiry,exact death credit,Blind-at-impact and minimum-flight progress;NPC deliveries remain pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
