// Actual finite-charge World contact and lifecycle acceptance.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Splinter World line "+std::to_string(at.line())+context);
}
struct Fixture{
 world::Instance w;Scope caster,attacker,primary;bool npcs;
 std::shared_ptr<world::Outbox>box=std::make_shared<world::Outbox>();
 combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,2,0.};
 explicit Fixture(bool npc,std::function<std::uint64_t()>clock={},bool healing_known=true):npcs(npc){
  if(clock)w.configure_movement_clock(std::move(clock));
  if(npc){std::vector<world::Instance::Npc>actors;
   for(unsigned id=100;id<=106;++id){world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};n.x=id==106?227.f:60.f;n.y=0;
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;if(healing_known){d.healing_allowed=true;d.healing_evidence="authored Splinter conversion fixture";}d.physical_armor=400;d.physical_armor_evidence="fixture";n.combat_definition=d;actors.push_back(n);}
   w.configure_npcs(actors);}
  w.join(1,"Ritualist",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);caster=*w.effect_scope(1);
  w.join(2,"Attacker",50,0,box,0,{}, {},100,1);w.set_vitals(2,{1000,1000,100,100},100);attacker=*w.effect_scope(2);
  check(w.ordinary_weapon(attacker,weapon,20,100,combat::weapon::CriticalProfile{1,0,20}));
  if(npc)primary=*w.effect_scope(100);
  else for(unsigned id=3;id<=9;++id){w.join(id,"Foe",id==9?227.f:60.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(id,{1000,1000,100,100},100);const auto scope=*w.effect_scope(id);check(w.physical_armor(scope,400));if(id==3)primary=scope;}
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .999999;});w.configure_attack_block_draws([]{return .99;});
 }
 double health(unsigned id)const{if(npcs)return w.npc_combat_snapshot(*w.effect_scope(id))->health->current;return w.resources(id)->health;}
};
int main()try{
 const auto control=combat::weapon::attack_skill(335,0,0);check(control&&!control->critical_interrupt);
 {context=" Seed settlement preserves later recipient healing";Fixture f(false);auto& w=f.w;
  for(unsigned id=4;id<=7;++id)w.set_vitals(id,{500,1000,100,100},100);
  for(unsigned id:{4u,6u,7u})check(w.party_action(5,158,id,100)&&w.party_action(id,154,5,100));
  const auto seed=*healing_seed(12);
  check(w.complete_healing_seed(*w.effect_scope(5),*w.effect_scope(4),seed,0,100));
  for(unsigned id=4;id<=7;++id)check(w.status_mutation(*w.effect_scope(id),1399,80,12,100,6100,true));
  const auto plan=*weapon_spell(792,12,8,0);
  check(w.apply_weapon_spell(f.caster,f.attacker,plan,100));
  check(w.melee_attack(f.attacker,f.primary,f.weapon,*control,1000));
  for(unsigned id=4;id<=7;++id)check(f.health(id)==500-plan.amount+5+seed.healing);
  check(w.splinter_late_refusals()==0);
 }
 for(unsigned skill:{792u,2868u})for(bool npc:{false,true}){
  context=" zero contact consumes exactly one charge skill="+std::to_string(skill)+" npc="+std::to_string(npc);
  Fixture f(npc);auto& w=f.w;
  for(unsigned i=0;i<4;++i){const auto recipient=*w.effect_scope((npc?101:4)+i);
   if(npc)check(w.stage_npc_shield_absorption(recipient,12,100));else check(w.status_mutation(recipient,1399,80,12,100,6100,true));
  }
  const auto recipient=*w.effect_scope(npc?101:4);
  check(w.degeneration_hex(f.caster,recipient,*degeneration_hex(101,12),100));
  check(w.degeneration_hex(f.caster,recipient,*degeneration_hex(150,12),100));
  check(w.apply_weapon_spell(f.caster,f.attacker,*weapon_spell(skill,0,8,0),100));
  check(w.melee_attack(f.attacker,f.primary,f.weapon,*control,1000));
  for(unsigned i=0;i<4;++i)check(f.health((npc?101:4)+i)==1000);
  const auto status=w.status_snapshot(f.attacker,1000);
  check(status&&std::none_of(status->begin(),status->end(),[&](const auto& e){return e.skill==skill;}));
  if(npc){check(w.damage_npc(f.attacker,recipient,{combat::Kind::armor_ignoring,50,20,42},1248,1100).reason==combat::Reason::accepted);check(f.health(101)==960);}
  else{check(w.energy_drain(f.attacker,recipient,*energy_drain(42,12),20,1100));check(f.health(4)==938);}
  check(w.splinter_late_refusals()==0&&w.take_attack_outcomes(f.attacker).size()==1);
 }
 for(unsigned skill:{792u,2868u})for(bool npc:{false,true})for(bool reversal:{false,true})for(unsigned equipment:{0u,3u}){
  if(npc&&equipment)continue;
  context=" protection skill="+std::to_string(skill)+" npc="+std::to_string(npc)+" reversal="+std::to_string(reversal)+" equipment="+std::to_string(equipment);
  Fixture f(npc);auto& w=f.w;const auto plan=*weapon_spell(skill,12,8,0);
  check(w.apply_weapon_spell(f.caster,f.attacker,plan,100));
  for(unsigned i=0;i<4;++i){const auto recipient=*w.effect_scope((npc?101:4)+i);
   if(npc)check(w.stage_npc_shield_absorption(recipient,12,100));else check(w.status_mutation(recipient,1399,80,12,100,6100,true));
   if(i==0){
    if(!npc)check(w.physical_absorption(recipient,equipment));
    if(reversal){if(npc)check(w.stage_npc_reversal(recipient,12,100));else check(w.status_mutation(recipient,307,81,12,100,8100,true));}
   }
  }
  double expected[4]{1000,1000,1000,1000};
  for(unsigned shot=0;shot<plan.charges+1;++shot){
   check(w.melee_attack(f.attacker,f.primary,f.weapon,*control,1000+100*shot));
   for(unsigned i=0;i<4;++i){
    if(shot<plan.charges&&!(reversal&&i==0&&shot==0))
     expected[i]-=std::max(0.,double(plan.amount)-(i==0?equipment:0)-5.*(shot+1));
    check(f.health((npc?101:4)+i)==expected[i]);
   }
  }
  const auto recipient=*w.effect_scope(npc?101:4);
  if(npc){check(w.damage_npc(f.attacker,recipient,{combat::Kind::armor_ignoring,50,20,42},1248,1600).reason==combat::Reason::accepted);
   check(f.health(101)==expected[0]-25);}
  else{check(w.energy_drain(f.attacker,recipient,*energy_drain(42,12),20,1600));check(f.health(4)==expected[0]-47);}
  const auto status=w.status_snapshot(f.attacker,1600);
  check(status&&std::none_of(status->begin(),status->end(),[&](const auto& e){return e.skill==skill;}));
  check(w.take_attack_outcomes(f.attacker).size()==plan.charges+1&&w.splinter_late_refusals()==0);
 }
 for(bool npc:{false,true})for(unsigned mode=0;mode<5;++mode){context=" npc="+std::to_string(npc)+" mode="+std::to_string(mode);Fixture f(npc);auto&w=f.w;
  const auto plan=weapon_spell(792,0,8,0);check(bool(plan));check(w.apply_weapon_spell(f.caster,f.attacker,*plan,100));
  if(mode==1)check(w.apply_condition(f.attacker,f.attacker,combat::conditions::Kind::blind,100,3000));
  if(mode==2){w.configure_attack_block_draws([]{return 0.;});if(npc)check(w.stage_npc_shield_deflection(f.primary,*shield_deflection(0),100));else check(w.apply_health_regeneration(f.primary,f.primary.actor,*skills::regeneration::reviewed(258,0),100,2100,1248,true,f.primary));}
  if(mode==3)w.leave(1);if(mode==4)check(w.kill(f.caster,500));
  f.box->drain();check(w.melee_attack(f.attacker,f.primary,f.weapon,*control,1000));
  const bool hit=mode!=1&&mode!=2;
  for(unsigned index=0;index<5;++index){const unsigned id=(npc?101:4)+index;const double expected=hit&&index<4?995.:1000.;context+=" secondary="+std::to_string(id)+" actual="+std::to_string(f.health(id));check(f.health(id)==expected);}
  check(f.health(npc?106:9)==1000);const auto status=w.status_snapshot(f.attacker,1000);check(bool(status));
  check(std::any_of(status->begin(),status->end(),[](const auto&e){return e.skill==792;})==!hit);
  const auto packets=f.box->drain();const auto fx=[&](unsigned target){world::Packet packet{159,0};combat::detail::put(packet,20);combat::detail::put(packet,target);combat::detail::put(packet,f.attacker.actor);combat::detail::put(packet,1152);return packet;};
  check(std::count(packets.begin(),packets.end(),fx(f.primary.actor))==unsigned(hit));
  for(unsigned index=0;index<5;++index)check(std::count(packets.begin(),packets.end(),fx((npc?101:4)+index))==unsigned(hit&&index<4));
  check(std::count(packets.begin(),packets.end(),combat::detail::integer(21,f.attacker.actor,2077))==0);
  const auto events=w.take_attack_outcomes(f.attacker);check(events.size()==1&&events.front().outcome==(mode==1?combat::weapon::AttackOutcome::miss:mode==2?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit));
 }
 {context=" primary MoP kills candidate after Splinter qualification";Fixture f(false);auto&w=f.w;
  for(unsigned id=4;id<=8;++id)w.set_vitals(id,{1,1000,100,100},100);
  check(w.apply_weapon_spell(f.caster,f.attacker,*weapon_spell(792,0,8,0),100));
  check(w.degeneration_hex(f.caster,f.primary,*degeneration_hex(150,12),100));
  check(w.melee_attack(f.attacker,f.primary,f.weapon,*control,1000));
  for(unsigned id=4;id<=8;++id)check(w.resources(id)->health==0);
  const auto status=w.status_snapshot(f.attacker,1000);check(status&&std::none_of(status->begin(),status->end(),[](const auto&e){return e.skill==792;}));
  check(w.take_attack_outcomes(f.attacker).size()==1);
 }
 for(bool npc:{false,true}){context=" rank12 charge exhaustion npc="+std::to_string(npc);Fixture f(npc);auto&w=f.w;
  check(w.apply_weapon_spell(f.caster,f.attacker,*weapon_spell(792,12,8,0),100));
  for(unsigned shot=0;shot<5;++shot){check(w.melee_attack(f.attacker,f.primary,f.weapon,*control,1000+100*shot));
   for(unsigned index=0;index<4;++index)check(f.health((npc?101:4)+index)==1000-41*std::min(shot+1,4u));
   const auto status=w.status_snapshot(f.attacker,1000+100*shot);check(status&&std::any_of(status->begin(),status->end(),[](const auto&e){return e.skill==792;})==(shot<3));}
  check(w.take_attack_outcomes(f.attacker).size()==5);
 }
 for(bool npc:{false,true})for(bool expires:{false,true}){context=" exact expiry npc="+std::to_string(npc)+" expired="+std::to_string(expires);Fixture f(npc);auto&w=f.w;
  check(w.apply_weapon_spell(f.caster,f.attacker,*weapon_spell(792,0,8,0),100));
  const auto at=expires?20100u:20099u;check(w.melee_attack(f.attacker,f.primary,f.weapon,*control,at));
  check(f.health(npc?101:4)==(expires?1000.:995.));check(w.status_snapshot(f.attacker,at)->empty());
 }
 for(bool npc:{false,true})for(unsigned future=0;future<2;++future){context=" secondary observer npc="+std::to_string(npc)+" far="+std::to_string(future);Fixture f(npc);auto&w=f.w;
  w.leave(2);w.join(2,"Archer",-190,0,f.box,0,{}, {},100,1);w.set_vitals(2,{1000,1000,100,100},100);f.attacker=*w.effect_scope(2);
  combat::weapon::Owned bow{12,1,{20,20,2},12,false,false,5};check(w.ordinary_weapon(f.attacker,bow,20,100,combat::weapon::CriticalProfile{1,0,20}));
  check(w.apply_weapon_spell(f.caster,f.attacker,*weapon_spell(792,0,8,0),100));
  const auto observed=*w.effect_scope(npc?(future?106:101):(future?9:4));
  // Publish the future observation before the earlier-sampled release. Applying
  // a condition after release would legitimately drain its flight first.
  if(npc)check(w.degeneration_hex(f.caster,observed,*degeneration_hex(103,0),2000));
  else check(w.apply_condition(f.caster,observed,combat::conditions::Kind::weakness,2000,1000));
  check(w.melee_attack(f.attacker,f.primary,bow,*combat::weapon::attack_skill(394,0,0),1000));
  w.tick(1100);if(!future){const auto held=w.status_snapshot(f.attacker,1100);check(held&&std::any_of(held->begin(),held->end(),[](const auto& e){return e.skill==792;}));check(w.splinter_late_refusals()==0);const auto early=w.take_attack_outcomes(f.attacker);context+=" earlyHealth="+std::to_string(f.health(npc?101:4))+" earlyOutcomes="+std::to_string(early.size());check(f.health(npc?101:4)==1000&&early.empty());w.tick(2000);}
  check(f.health(npc?101:4)==995);const auto events=w.take_attack_outcomes(f.attacker);check(events.size()==1&&events.front().at==(future?1100u:2000u));
  w.tick(2100);check(w.take_attack_outcomes(f.attacker).empty());
 }
 for(bool future:{false,true}){context=" actual NPC attacker future secondary="+std::to_string(future);world::Instance w;std::vector<world::Instance::Npc>population;
  for(unsigned id=100;id<=106;++id){world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};n.x=id==100?0:100;
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;d.relation=id==100?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="fixture";d.creature_type="human";d.creature_type_evidence="fixture";d.physical_armor=400;d.physical_armor_evidence="fixture";n.combat_definition=d;population.push_back(n);}
  w.configure_npcs(population);w.join(1,"Ritualist",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto caster=*w.effect_scope(1),attacker=*w.effect_scope(100),primary=*w.effect_scope(101);
  check(w.apply_weapon_spell(caster,attacker,*weapon_spell(792,0,8,0),100));
  const combat::ai::ordinary::Reviewed profile{20,20,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"synthetic known NPC weapon, no retail tuning claim"};
  if(future)check(w.degeneration_hex(caster,*w.effect_scope(102),*degeneration_hex(103,0),2000));
  check(w.configure_npc_opposed_ordinary(attacker,profile,1000));check(w.begin_npc_opposed_ordinary(attacker,primary,1000).has_value());
  w.tick(1750);if(future){check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==1000&&w.take_attack_outcomes(attacker).empty());w.tick(2000);}
  for(unsigned id=102;id<=105;++id)check(w.npc_combat_snapshot(*w.effect_scope(id))->health->current==995);
  check(w.npc_combat_snapshot(*w.effect_scope(106))->health->current==1000);
  const auto events=w.take_attack_outcomes(attacker);check(events.size()==1&&events.front().at==(future?2000u:1750u));check(w.status_snapshot(attacker,future?2000:1750)->empty());
 }
 {context=" secondary future motion after release";std::uint64_t movement_now=100;Fixture f(false,[&]{return movement_now;});auto&w=f.w;
  w.leave(2);w.join(2,"Archer",-190,0,f.box,0,{}, {},100,1);w.set_vitals(2,{1000,1000,100,100},100);f.attacker=*w.effect_scope(2);
  combat::weapon::Owned bow{12,1,{20,20,2},12,false,false,5};check(w.ordinary_weapon(f.attacker,bow,20,100,combat::weapon::CriticalProfile{1,0,20}));
  check(w.apply_weapon_spell(f.caster,f.attacker,*weapon_spell(792,0,8,0),100));check(w.melee_attack(f.attacker,f.primary,bow,*combat::weapon::attack_skill(394,0,0),1000));
  movement_now=2000;w.move(4,61,0,1000);w.tick(1100);check(f.health(4)==1000&&w.take_attack_outcomes(f.attacker).empty());
  w.tick(2000);check(f.health(4)==995);const auto events=w.take_attack_outcomes(f.attacker);check(events.size()==1&&events.front().at==2000);
  w.tick(2100);check(w.take_attack_outcomes(f.attacker).empty()&&f.health(4)==995);
 }
 {context=" recycled attacker cannot consume new generation charge";Fixture f(false);auto&w=f.w;
  w.leave(2);w.join(2,"Old archer",-190,0,f.box,0,{}, {},100,1);w.set_vitals(2,{1000,1000,100,100},100);const auto old=*w.effect_scope(2);
  combat::weapon::Owned bow{12,1,{20,20,2},12,false,false,5};check(w.ordinary_weapon(old,bow,20,100,combat::weapon::CriticalProfile{1,0,20}));
  check(w.apply_weapon_spell(f.caster,old,*weapon_spell(792,0,8,0),100));check(w.melee_attack(old,f.primary,bow,*combat::weapon::attack_skill(394,0,0),1000));
  w.leave(2);w.join(2,"New archer",-190,0,f.box,0,{}, {},1050,1);w.set_vitals(2,{1000,1000,100,100},100);const auto replacement=*w.effect_scope(2);check(replacement!=old);
  check(w.ordinary_weapon(replacement,bow,20,1050,combat::weapon::CriticalProfile{1,0,20}));check(w.apply_weapon_spell(f.caster,replacement,*weapon_spell(792,0,8,0),1050));
  w.tick(1100);for(unsigned id=4;id<=8;++id)check(f.health(id)==1000);
  const auto status=w.status_snapshot(replacement,1100);check(status&&status->size()==1&&status->front().skill==792);check(w.take_attack_outcomes(replacement).empty());
  check(w.melee_attack(replacement,f.primary,bow,*combat::weapon::attack_skill(394,0,0),1200));w.tick(1300);check(f.health(4)==995&&w.status_snapshot(replacement,1300)->empty());
 }
 {context=" fully absorbed secondary still consumes landed charge";Fixture f(false);auto&w=f.w;const auto shielded=*w.effect_scope(4);
  check(w.protective_enchantment(shielded,shielded,*skills::regeneration::reviewed(299,12),0,100));
  check(w.apply_weapon_spell(f.caster,f.attacker,*weapon_spell(792,0,8,0),100));check(w.melee_attack(f.attacker,f.primary,f.weapon,*control,1000));
  check(f.health(4)==1000);for(unsigned id=5;id<=7;++id)check(f.health(id)==995);check(f.health(8)==1000);
  check(w.status_snapshot(f.attacker,1000)->empty());const auto events=w.take_attack_outcomes(f.attacker);check(events.size()==1&&events.front().outcome==combat::weapon::AttackOutcome::hit);
 }
 std::cout<<"Splinter World hit-only charge, four secondary foes and source provenance passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}


