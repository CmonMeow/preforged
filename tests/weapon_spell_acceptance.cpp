// Detached acceptance matrix: normal Runtime casts onto actual allied NPC recipients.
#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include "gw_cast_timing.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace effects;
std::string context;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("weapon spell NPC cast line "+std::to_string(at.line())+context);}
int main()try{
 for(auto [id,pvp]:{std::pair{787u,false},std::pair{793u,false},std::pair{793u,true},std::pair{794u,false}})for(unsigned mode:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,12u,13u,14u,15u}){
  context=" NPC id="+std::to_string(id)+" pvp="+std::to_string(pvp)+" mode="+std::to_string(mode);world::Instance w;
  world::Instance::Npc actor;actor.id=actor.definition.id=100;actor.definition.level=20;actor.definition.visual_adjustment={0,0,0,100};actor.x=50;
  combat::npcs::Definition definition;definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;definition.binding.level=20;
  definition.relation=combat::npcs::Relation::allied_to_players;definition.spawn_health=combat::Health{500,1000};definition.healing_allowed=true;
  definition.healing_evidence="weapon spell NPC Runtime fixture";definition.creature_type="human";definition.creature_type_evidence="fixture";actor.combat_definition=definition;w.configure_npcs({actor});
  w.join(1,"Ritualist",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto source=*w.effect_scope(1);check(w.energy_capacity(1,8,0,100,{80,-2}));
  // Actual non-spirit allied NPC authority, not a simulated player resource callback.
  // Configure before joining players, as the public NPC population API requires.

  const auto target=*w.effect_scope(100);
  casting::Runtime runtime(1);casting::Hooks h;std::vector<Packet>wire;unsigned commits=0;std::optional<WeaponSpellPlan>completed;
  h.events=[&](const auto&p){wire.insert(wire.end(),p.begin(),p.end());};h.resources=[&](auto actor){return w.resources(actor);};h.target_scope=[&](auto actor){return w.effect_scope(actor);};
  h.target_valid=[&](auto actor,float,bool){return actor==100&&w.effect_scope(actor)==target;};
  h.consume_energy_at=[&](auto amount,auto at){check(amount==(id==794?5u:10u));return w.consume_energy(1,amount,source,at);};
  h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
  h.weapon_spell_target_valid=[&](Scope recipient,const WeaponSpellPlan&plan,auto at){return mode!=14&&w.valid_weapon_spell_target(source,recipient,plan,at);};
  h.weapon_spell=[&](Scope recipient,const WeaponSpellPlan&plan,auto at){++commits;completed=plan;std::vector<Packet>visuals;std::function<void()>publication;
   const bool accepted=w.apply_weapon_spell(source,recipient,plan,at,&visuals,&publication);return casting::EffectCompletion{accepted,std::move(visuals),{},std::move(publication),true};};
  Policy policy;policy.primary=8;policy.campaigns=255;policy.learned[id]=1;policy.pvp_match=pvp;Slots bar{};bar[0]={static_cast<std::uint16_t>(id),0};attributes::Ranks ranks{};ranks[33]=ranks[34]=ranks[36]=12;
  if(mode==3)h.weapon_spell={};if(mode==4)h.weapon_spell_target_valid={};if(mode==5)h.target_scope={};if(mode==6)h.before_spell_effect={};if(mode==15){policy.primary=2;policy.secondary=8;}
  const bool missing=mode>=3&&mode<=6||mode==15;const auto reason=runtime.start(1000,{id,0,100},bar,policy,ranks,true,h);
  check(reason==(missing?casting::Reason::unsupported:mode==14?casting::Reason::target:casting::Reason::accepted));
  if(missing||mode==14){check(w.resources(1)->energy==100&&commits==0);continue;}
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1200,h);if(mode==7)h.weapon_spell={};if(mode==8)h.weapon_spell_target_valid={};if(mode==9)h.target_scope={};if(mode==10)h.before_spell_effect={};

  if(mode==13){ranks[33]=ranks[34]=ranks[36]=0;policy.primary=2;policy.secondary=8;check(w.energy_capacity(1,2,0,1500,{80,-2}));}
  runtime.update(1999,h);check(commits==0);runtime.update(2000,h);if(mode==12){runtime.update(2000,h);runtime.update(2100,h);}
  const bool success=mode==0||mode==12||mode==13;check(commits==unsigned(success));check(w.resources(1)->energy==(id==794?95u:90u));
  const auto current=*w.effect_scope(100);const auto state=w.status_snapshot(current,2000);check(bool(state)&&state->size()==unsigned(success));
  const unsigned effect=pvp&&id==793?2893:id;const auto*row=find_skill(effect);
  if(success){check(completed&&completed->skill==effect&&completed->rank==12&&completed->primary==8&&completed->spawning==12);
   const unsigned duration=id==787?15000:id==793?10000:12000;check(state->front().skill==effect&&state->front().expires==2000+duration);
   check(std::count(wire.begin(),wire.end(),casting::generic(21,100,row->target_body_resource))==1);
   check(std::count(wire.begin(),wire.end(),casting::generic(21,100,1155))==1);
   check(std::count(wire.begin(),wire.end(),casting::generic(21,1,1282))==unsigned(id==787));
   check(std::count(wire.begin(),wire.end(),casting::terminal(58,1))==1);
  }else check(std::count(wire.begin(),wire.end(),casting::terminal(59,1))==1);
  check(std::count(wire.begin(),wire.end(),casting::generic(21,1,2077))==0&&std::count(wire.begin(),wire.end(),casting::generic(21,100,2077))==0);
  if(mode==0||mode==12){check(runtime.start(2749,{id,0,100},bar,policy,ranks,true,h)==casting::Reason::busy);check(runtime.start(2750,{id,0,100},bar,policy,ranks,true,h)==casting::Reason::recharge);const auto deadline=2000+row->recharge*1000;check(runtime.start(deadline-1,{id,0,100},bar,policy,ranks,true,h)==casting::Reason::recharge);check(runtime.start(deadline,{id,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted);runtime.cancel(h);}
 }
 for(bool npc:{false,true})for(bool protected_action:{false,true}){
  context=" multi-arrow Wailing npc="+std::to_string(npc)+" protected="+std::to_string(protected_action);world::Instance w;
  if(npc){world::Instance::Npc actor;actor.id=actor.definition.id=100;actor.definition.level=20;actor.definition.visual_adjustment={0,0,0,100};actor.x=100;
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;d.relation=combat::npcs::Relation::hostile_to_players;
   d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.conditions_allowed=true;d.physical_armor=60;d.physical_armor_evidence="fixture";actor.combat_definition=d;w.configure_npcs({actor});}
  w.join(1,"Ritualist",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto caster=*w.effect_scope(1);
  w.join(2,"Ranger",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{1000,1000,100,100},100);const auto source=*w.effect_scope(2);
  Scope target;if(npc)target=*w.effect_scope(100);else{w.join(3,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);target=*w.effect_scope(3);check(w.physical_armor(target,60));check(w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{1,0,20}));}
  combat::weapon::Owned bow{12,1,{20,20,2},12,false,false,5,0.};check(w.ordinary_weapon(source,bow,20,100,combat::weapon::CriticalProfile{2,0,20}));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([]{return .99;});w.configure_attack_critical_draws([]{return .99;});
  check(w.apply_weapon_spell(caster,source,*weapon_spell(794,0,8,0),100));std::optional<combat::npcs::control::Token>token;
  if(npc){token=w.begin_npc_action(target,combat::npcs::control::Action::attack,100,3000);check(bool(token));}
  else check(w.attack_activity(target,3000,100));
  if(protected_action){if(npc)check(w.npc_interrupt_stance(target,16,12,1000,100));else check(w.status_mutation(target,16,123,12,100,100+skills::timing::stance(16,12)->duration,true));}
  const auto before=w.control_epoch(target);const auto plan=combat::weapon::attack_skill(396,0,0);check(plan&&plan->arrows==2&&!plan->critical_interrupt);check(w.melee_attack(source,target,bow,*plan,1000));w.tick(1100);
  const auto outcomes=w.take_attack_outcomes(source);check(outcomes.size()==2&&outcomes[0].event!=outcomes[1].event);for(const auto&outcome:outcomes)check(outcome.outcome==combat::weapon::AttackOutcome::hit);
  if(npc)check(!w.complete_npc_action(*token,3000));else check(before&&w.control_epoch(target)==std::optional<std::uint64_t>{*before+1});
  if(protected_action){const auto status=w.status_snapshot(target,1100);check(status&&std::none_of(status->begin(),status->end(),[](const auto&e){return e.skill==16;}));}
  w.tick(1200);check(w.take_attack_outcomes(source).empty());
 }
 for(bool pvp:{false,true})for(unsigned mode=0;mode<3;++mode){
  context=" Warding release pvp="+std::to_string(pvp)+" mode="+std::to_string(mode);world::Instance w;
  w.join(1,"Ritualist",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto source=*w.effect_scope(1);
  w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);const auto target=*w.effect_scope(2);check(w.physical_armor(target,60));check(w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{1,0,20}));
  combat::weapon::Owned bow{12,1,{20,20,2},12,false,false,5,0.};check(w.ordinary_weapon(source,bow,20,100,combat::weapon::CriticalProfile{8,0,20}));
  check(w.apply_weapon_spell(source,source,*weapon_spell(pvp?2893:793,0,8,0),100));const auto plan=*combat::weapon::attack_skill(397,0,0);
  auto proposed=target;if(mode==0)++proposed.generation;check(w.melee_attack(source,proposed,bow,plan,1000)==(mode!=0));
  auto status=w.status_snapshot(source,1000);check(status&&status->size()==unsigned(!pvp||mode==0));
  if(mode==2)check(w.kill(target,1001));w.tick(1100);status=w.status_snapshot(source,1100);check(status&&status->size()==unsigned(!pvp||mode==0));
 }
 std::cout<<"weapon spell NPC typed Runtime lifecycle passed\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
