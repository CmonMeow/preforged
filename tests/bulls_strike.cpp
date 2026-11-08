#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Bulls Strike line "+std::to_string(at.line())+context);
}
world::Instance::Npc body(unsigned id,float x,bool hostile,std::optional<bool> susceptible=true){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
 d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::neutral;
 d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;d.physical_armor=60;
 d.knockdown_allowed=susceptible;d.knockdown_evidence=susceptible?"authored fixture":"-";n.combat_definition=d;return n;
}
int main()try{
 for(unsigned rank:{0u,12u,20u})for(unsigned mode=0;mode<7;++mode){context=" player rank="+std::to_string(rank)+" mode="+std::to_string(mode);
  warrior_test::Fixture f(332,false,mode==6?5:1000);f.weapon.item_type=27;check(f.world.ordinary_weapon(f.source,f.weapon,20));
  const bool moving=mode==1||mode==3||mode==4||mode==5||mode==6;
  if(moving||mode==2)f.world.move(2,mode==2?65:1000,0,100);
  if(mode==3)f.world.move(2,74.4f,0,150); // Accepted stop at the current point; no velocity at contact.
  if(mode==4)check(f.world.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,5000));
  if(mode==5){f.world.configure_attack_block_draws([]{return 0.;});check(f.world.status_mutation(f.target,448,123,12,100,100+*effects::escape(448,12),true));}
  const auto plan=*combat::weapon::attack_skill(332,rank,0);check(plan.bonus==unsigned(std::lround(5.+25.*rank/15.))&&plan.moving_bonus&&plan.knockdown_on_hit);
  check(f.world.melee_attack(f.source,f.target,f.weapon,plan,200));
  const bool qualifies=moving&&mode!=3&&mode!=4&&mode!=5;
  const double expected=mode==6?0:1000-(mode==4||mode==5?0:20+(qualifies?plan.bonus:0));check(f.health()==expected);
  check(f.world.knocked_down(f.target,200)==(qualifies&&mode!=6));
  if(qualifies&&mode!=6)check(f.world.knocked_down(f.target,2199)&&!f.world.knocked_down(f.target,2200));
 }
 for(unsigned skill:{332u,326u})for(unsigned mode=0;mode<6;++mode){context=" NPC skill="+std::to_string(skill)+" mode="+std::to_string(mode);world::Instance w;
  auto guard=body(100,300,false);w.configure_npcs({guard});w.join(1,"Warrior",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,30,30},100);
  const auto source=*w.effect_scope(1),guardian=*w.effect_scope(100);auto enemy=body(101,60,true,mode==3?std::optional(false):mode==4?std::nullopt:std::optional(true));
  check(w.begin_wave_objective(source,guardian,73,1,3,1)&&w.spawn_quest_wave(source,73,1,{enemy},[]{return true;}));const auto target=*w.effect_scope(60000);
  if(mode!=0)check(w.chase_quest_guardian(target,100,mode==5?1000:100));
  if(mode==2)check(w.knockdown_npc(target,2000,150)==combat::npcs::control::Knockdown::accepted);
  combat::weapon::Owned sword{12,1,{20,20,2},12,false,false,27};check(w.ordinary_weapon(source,sword,20));const auto plan=*combat::weapon::attack_skill(skill,12,0);
  const bool valid=mode!=5&&(mode!=4||skill==326);check(w.melee_target(source,target.actor,200,skill)==valid);
  check(w.melee_attack(source,target,sword,plan,200)==valid);
  const bool moving=mode==1||mode==3||mode==4;
  check(w.npc_combat_snapshot(target)->health->current==1000-(valid?20+(moving?plan.bonus:0):0));
  check(w.knocked_down(target,200)==(mode==2||(valid&&moving&&mode!=3&&skill==332)));
  if(mode==2)check(!w.knocked_down(target,2150)); // A paused target did not receive another knockdown.
 }
 for(unsigned mode=0;mode<4;++mode){context=" Runtime mode="+std::to_string(mode);warrior_test::Fixture f(332,false);f.weapon.item_type=27;check(f.world.ordinary_weapon(f.source,f.weapon,20));
  Slots bar{};bar[0]={332,0};Policy policy;policy.primary=1;policy.campaigns=255;policy.learned[332]=1;attributes::Ranks ranks{};ranks[17]=12;
  casting::Hooks h;std::vector<world::Packet> events;bool stale=false;h.resources=[&](unsigned id){return f.world.resources(id);};h.events=[&](const auto& packets){events.insert(events.end(),packets.begin(),packets.end());};
  h.target_scope=[&](unsigned id){if(stale&&id==2)return std::optional<effects::Scope>{};return f.world.effect_scope(id);};h.weapon_snapshot=[&](){return std::optional(f.weapon);};
  h.melee_target=[&](unsigned id,auto now){return f.world.melee_target(f.source,id,now,332);};h.consume_energy_at=[&](unsigned cost,auto now){check(cost==5);return f.world.consume_energy(1,cost,f.source,now);};
  h.melee_attack=[&](auto target,const auto& weapon,const auto& plan,auto now){return f.world.melee_attack(f.source,target,weapon,plan,now);};
  h.weapon_attack_timing=[&](auto now){return f.world.weapon_attack_timing(f.source,now);};h.attack_activity=[&](auto until){f.world.attack_activity(f.source,until,1000);};
  casting::Runtime runtime(1);check(runtime.start(1000,{332,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted&&f.world.resources(1)->energy==25);
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1200,h);if(mode==3)stale=true;
  f.world.move(2,1000,0,1450);runtime.update(1499,h);check(f.health()==1000);runtime.update(1500,h);
  check(f.world.knocked_down(f.target,1500)==(mode==0));check((f.health()<1000)==(mode==0));
  check(std::count(events.begin(),events.end(),casting::terminal(46,1))==unsigned(mode==0));
  const auto before=f.health();runtime.update(1500,h);check(f.health()==before);
 }
 check(!combat::weapon::attack_skill(332,21));auto forged=*combat::weapon::attack_skill(332,12);++forged.bonus;check(!combat::weapon::valid(forged));
 std::cout<<"PASS Bulls Strike contact movement,stationary/stopped/missed/blocked/lethal targets,2s KD,known routed NPCs/immunity/future clocks,Griffons Sweep NPC route bonus and actual paid/cancel/interrupt/stale Runtime casts\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
