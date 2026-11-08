#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("critical World line "+std::to_string(at.line())+context);}
struct Fixture{
 world::Instance world;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();Scope source,target;combat::weapon::Owned weapon;
 Fixture(bool npc,bool bow){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;d.physical_armor=60;d.physical_armor_evidence="test";n.combat_definition=d;world.configure_npcs({n});}
  world.join(1,"Assassin",0,0,box,0,{}, {},100,1);world.set_vitals(1,{1000,1000,30,30},100);source=*world.effect_scope(1);check(world.energy_capacity(1,7,0,100,{10,-2}));world.configure_attack_miss_draws([]{return .5;});
  if(!npc){world.join(2,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);world.set_vitals(2,{1000,1000,30,30},100);target=*world.effect_scope(2);check(world.physical_armor(target,60));check(world.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{1,0,20}));}else target=*world.effect_scope(100);
  weapon={12,1,{20,20,bow?1u:2u},12,false,false,bow?5u:2u,0.};check(world.ordinary_weapon(source,weapon,20,100,combat::weapon::CriticalProfile{7,13,20}));
  check(world.consume_energy(1,10,source,100));check(world.resources(1)->energy==20);box->drain();
 }
 double health()const{return target.actor==100?world.npc_combat_snapshot(target)->health->current:double(world.resources(target.actor)->health);}
};
bool critical_notice(const std::vector<Packet>& packets){return std::any_of(packets.begin(),packets.end(),[](const auto& p){return p.size()==18&&p[0]==162&&p[1]==0&&p[2]==17&&p[3]==0&&p[4]==0&&p[5]==0;});}
int main()try{
 for(bool npc:{false,true})for(bool bow:{false,true})for(unsigned mode=0;mode<5;++mode){context=" npc="+std::to_string(npc)+" bow="+std::to_string(bow)+" mode="+std::to_string(mode);Fixture f(npc,bow);auto& w=f.world;unsigned draws=0;w.configure_attack_critical_draws([&]{++draws;return mode==1?.999999:0.;});
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,3000));
  if(mode==3){if(npc)check(w.stage_npc_shield_deflection(f.target,*shield_deflection(12),100));else check(w.apply_health_regeneration(f.target,2,*regeneration::reviewed(258,12),100,6100,1248,true,f.target));w.configure_attack_block_draws([]{return 0.;});}
  if(mode==4){auto unknown=f.weapon;unknown.critical_penalty.reset();check(w.ordinary_weapon(f.source,unknown,20,100,combat::weapon::CriticalProfile{7,13,20}));f.weapon=unknown;}
  const auto id=bow?1467u:340u;check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(id,12,0),1000));if(bow)w.tick(1100);
  const bool landed=mode!=2&&(mode!=3||bow);const bool critical=landed&&mode!=1&&mode!=4;
  check(draws==unsigned(landed&&mode!=4));check(critical_notice(f.box->drain())==critical);check((f.health()<1000)==landed);check(w.resources(1)->energy==(critical?23u:20u));
 }
 // Balanced Stance denies extra damage while retaining the critical event and
 // attribute reward. This differs from Stoneflesh's explicit Energy denial.
 {Fixture f(false,false);auto& w=f.world;w.configure_attack_critical_draws([]{return 0.;});check(w.status_mutation(f.target,371,123,0,100,8100,true));check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12,0),1000));check(critical_notice(f.box->drain()));check(w.resources(1)->energy==23);check(f.health()==980);}
 for(bool npc:{false,true}){
  context=" midflightlevelup npc="+std::to_string(npc);Fixture f(npc,true);auto& w=f.world;unsigned draws=0;w.configure_attack_critical_draws([&]{++draws;return 0.;});
  const auto old=w.critical_profile_snapshot(f.source,100);check(old&&old->revision==1);check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(1467,12,0),1000));
  // Same equipped weapon; only authoritative level/attribute profile changes
  // before the48ms arrow arrives. Damage keeps the releasedlevel20 snapshot.
  check(w.ordinary_weapon(f.source,f.weapon,21,1020,combat::weapon::CriticalProfile{7,18,21}));check(!w.critical_profile_snapshot(f.source,1019));check(w.critical_profile_snapshot(f.source,1020)->revision==2);check(f.health()==1000&&draws==0);
  w.tick(1100);check(f.health()<1000&&draws==1&&w.resources(1)->energy==24);const auto events=w.take_attack_outcomes(f.source);check(events.size()==1&&events[0].at==1048);w.tick(1200);check(draws==1&&w.resources(1)->energy==24);
  const auto previous=f.source;w.leave(1);check(!w.critical_profile_snapshot(previous,1200));
 }
 for(bool npc:{false,true}){
  context=" preciseEnergy npc="+std::to_string(npc);Fixture f(npc,false);auto& w=f.world;w.configure_attack_critical_draws([]{return 0.;});check(w.consume_energy(1,18,f.source,100));check(w.activate_overcast_energy(f.source,231,0,1,100));check(w.energy_capacity(1,7,0,100,{0,-2}));
  const auto before=w.overcast_energy_snapshot(f.source);check(before&&overcast::precise_current(*before)==-8*std::int64_t(overcast::energy_unit));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12,0),1000));const auto after=w.overcast_energy_snapshot(f.source);check(after&&overcast::precise_current(*after)-overcast::precise_current(*before)==3*std::int64_t(overcast::energy_unit));
 }
 std::cout<<"weapon critical World passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
