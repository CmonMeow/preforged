#include "gw_world_simulation.h"
#include "gw_cast_timing.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("weapon spell World line "+std::to_string(at.line())+context);}
struct Fixture{
 world::Instance world;Scope source,target;bool npc;std::shared_ptr<world::Outbox>box=std::make_shared<world::Outbox>();
 Fixture(bool n):npc(n){
  if(n){world::Instance::Npc actor;actor.id=actor.definition.id=100;actor.definition.level=20;actor.definition.visual_adjustment={0,0,0,100};actor.x=50;
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};d.healing_allowed=true;d.healing_evidence="fixture";d.creature_type="human";d.creature_type_evidence="fixture";actor.combat_definition=d;world.configure_npcs({actor});}
  world.join(1,"Ritualist",0,0,box,0,{}, {},100,1);world.set_vitals(1,{1000,1000,100,100},100);source=*world.effect_scope(1);check(world.energy_capacity(1,8,0,100,{80,-2}));
  if(!n){world.join(2,"Ally",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);world.set_vitals(2,{500,1000,100,100},100);target=*world.effect_scope(2);check(world.party_action(1,158,2,100)&&world.party_action(2,154,1,100));}else target=*world.effect_scope(100);
 }
 double health()const{return npc?world.npc_combat_snapshot(target)->health->current:double(world.resources(target.actor)->health);}
};
int main()try{
 const auto control=combat::weapon::attack_skill(335,0,0);check(control&&control->required_type==2&&!control->critical_interrupt&&!control->requires_critical_authority);
 for(bool npc:{false,true})for(unsigned mode=0;mode<3;++mode){context=" recipient npc="+std::to_string(npc)+" mode="+std::to_string(mode);Fixture f(npc);auto&w=f.world;std::vector<world::Packet>visuals;
  const bool applied=w.apply_weapon_spell(f.source,f.target,*weapon_spell(793,0,8,0),100,&visuals);context+=" applied="+std::to_string(applied);check(applied);
  const auto status=w.status_snapshot(f.target,100);context+=" status="+std::to_string(status?status->size():999u);check(status&&status->size()==1);
  w.tick(1100);context+=" actualHealth="+std::to_string(f.health());check(f.health()==504);
  if(mode==1)w.leave(1);
  if(mode==2)check(w.apply_weapon_spell(f.source,f.target,*weapon_spell(787,0,8,0),1100));
  w.tick(2100);check(f.health()==(mode==2?504:508));w.tick(4100);check(f.health()==(mode==2?504:512));check(w.status_snapshot(f.target,4100)->empty());
 }
 {Fixture f(false);auto&w=f.world;check(w.apply_weapon_spell(f.source,f.target,*weapon_spell(793,0,8,0),100));check(w.kill(f.target,1100));check(w.status_snapshot(f.target,1100)->empty());}
 {Fixture f(false);auto&w=f.world;check(w.apply_weapon_spell(f.source,f.target,*weapon_spell(787,0,8,0),100));
  check(w.apply_condition(f.source,f.target,combat::conditions::Kind::weakness,1100,1000));w.tick(2100);check(f.health()==502);w.tick(3100);check(f.health()==502);}
 {Fixture f(false);auto&w=f.world;check(w.consume_energy(2,1,f.target,2000));check(!w.apply_weapon_spell(f.source,f.target,*weapon_spell(793,0,8,0),1000));check(w.consume_energy(1,0,f.source,1500));}
 for(bool resilient:{false,true}){context=" lasthex Resilient="+std::to_string(resilient);Fixture f(false);auto&w=f.world;
  check(w.lightning_armor(f.target,60));
  w.join(3,"Elementalist",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);const auto foe=*w.effect_scope(3);
  if(resilient)check(w.apply_weapon_spell(f.source,f.target,*weapon_spell(787,0,8,20),100));
  check(w.lightning_surge(foe,f.target,0,20,200));w.tick(3200);
  const double amount=14*std::exp2((60.-(resilient?84.:60.)*.75)/40.);
  check(f.health()==std::ceil((resilient?506.:500.)-amount));}
 for(bool npc:{false,true})for(unsigned mode=0;mode<11;++mode){context=" Wailing npc="+std::to_string(npc)+" mode="+std::to_string(mode);world::Instance w;
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};n.x=60;combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;d.physical_armor=60;d.physical_armor_evidence="fixture";n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Ritualist",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto caster=*w.effect_scope(1);
  w.join(2,"Warrior",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{1000,1000,100,100},100);const auto attacker=*w.effect_scope(2);check(w.party_action(1,158,2,100)&&w.party_action(2,154,1,100));Scope target;
  if(npc)target=*w.effect_scope(100);else{w.join(3,"Foe",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);target=*w.effect_scope(3);check(w.physical_armor(target,60));check(w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{1,0,20}));}
  combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,2,0.};check(w.ordinary_weapon(attacker,weapon,20,100,combat::weapon::CriticalProfile{1,0,20}));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([&]{return mode==8||mode==9?.999999:0.;});w.configure_attack_block_draws([&]{return mode==3||mode==10?0.:.99;});
  check(w.apply_weapon_spell(caster,attacker,*weapon_spell(794,0,8,0),100));std::optional<combat::npcs::control::Token> token;
  if(npc){token=w.begin_npc_action(target,mode==1?combat::npcs::control::Action::spell:combat::npcs::control::Action::attack,100,3000,mode==1?std::optional(skills::Slot{281,0}):std::nullopt);check(bool(token));}
  else if(mode==1)check(w.cast_activity(target,281,100,3000));else check(w.attack_activity(target,3000,100));
  if(mode==2)check(w.apply_condition(attacker,attacker,combat::conditions::Kind::blind,100,3000));
  if(mode==3||mode==10){if(npc)check(w.stage_npc_shield_deflection(target,*shield_deflection(0),100));else check(w.apply_health_regeneration(target,target.actor,*skills::regeneration::reviewed(258,0),100,2100,1248,true,target));}
  if(mode==4||mode==5||mode==9||mode==10){if(npc)check(w.npc_interrupt_stance(target,16,12,1000,100));else check(w.status_mutation(target,16,123,12,100,100+skills::timing::stance(16,12)->duration,true));}
  if(mode==6)check(w.kill(caster,500));if(mode==7)w.leave(1);
  const auto epoch=w.control_epoch(target);check(w.melee_attack(attacker,target,weapon,*combat::weapon::attack_skill((mode==5||mode==8||mode==9||mode==10)?1402:335,0,0),1000));
  if(mode==4||mode==5||mode==9||mode==10){const auto entries=w.status_snapshot(target,1000);check(bool(entries));const bool remains=std::any_of(entries->begin(),entries->end(),[](const auto&e){return e.skill==16;});check(remains==(mode==10));}
  const bool interrupted=mode==0||mode==6||mode==7||mode==8;if(npc)check(w.complete_npc_action(*token,3000)==!interrupted);else {context+=" epochBefore="+std::to_string(epoch.value_or(0))+" epochAfter="+std::to_string(w.control_epoch(target).value_or(0))+" expectedInterrupt="+std::to_string(interrupted);check((w.control_epoch(target)!=epoch)==interrupted);}
 }
 std::cout<<"weapon spell recipient World lifecycle passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}




