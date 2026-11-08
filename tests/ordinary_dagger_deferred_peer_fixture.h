#pragma once
// Exact current delayed fixture extraction.
// NEW unregistered. All chance/timing/reaction grains are explicitly AUTHORED,
// never claims of full ordinary dagger retail timing or random enable.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("declared ordinary dagger World line "+std::to_string(at.line()));}
struct Fixture {
 world::Instance w;Scope source,target,hexer,pet_owner;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,32,0.};unsigned selections=0;bool pet=false;
 using Policy=world::Instance::OrdinaryDaggerPolicyFollowon;
 static Policy policy(bool per_contact=false){Policy p;p.chance={combat::weapon::OrdinaryDaggerChancePolicyFollowon::Basis::explicit_inherent_two,"local/modern mainwiki2%+rank2%; reviewed test draw"};p.second_contact_base_ms=500;p.timing=Policy::Timing::empirical_delayed_declared;p.empathy=p.vigor=per_contact?Policy::Grain::physical_contacts_declared:Policy::Grain::action_once_declared;p.evidence="DECLARED empirical500ms*factor/grains; exactretail timing unproven";return p;}
 Fixture(bool owned_pet=false,unsigned hp=500,bool enroll=true,bool per_contact=false,double draw=0,bool unknown_ally=false):pet(owned_pet){
  if(unknown_ally){world::Instance::Npc n;n.id=n.definition.id=100;n.x=30;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
   d.binding={146,100,100,0,0,0,0,20};d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};d.damageable=true;
   d.creature_type="spirit";d.creature_type_evidence="authored unsupported Seed gain recipient";d.fleshy=false;d.fleshy_evidence="authored non-fleshy spirit";n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Dagger owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{hp,1000,100,100},100);source=*w.effect_scope(1);
  w.join(2,"Contact owner",80,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);target=pet_owner=*w.effect_scope(2);
  w.join(3,"Hex caster",-1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);hexer=*w.effect_scope(3);
  check(w.physical_armor(target,60)&&w.elemental_armor(target,60,60));check(w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));
  if(pet){check(w.restore_pet_profile(pet_owner,1,pets::Profile{pets::Pet{}},1)&&w.pet_owner_combat(pet_owner,1,0,100));check(w.reconcile_pet_bar_mode(pet_owner,162,true,false,std::array{436u},100));target=*w.owned_pet_scope(pet_owner);check(w.command_pet(pet_owner,{pets::wire::Command::Kind::behavior,target.actor,2},100).has_value());}
  check(w.ordinary_weapon(source,weapon,20,100,combat::weapon::CriticalProfile{1,0,20}));w.configure_attack_critical_draws([]{return .999999;});w.configure_attack_miss_draws([]{return .5;});
  if(enroll)check(w.configure_ordinary_dagger_policy_followon(source,policy(per_contact),[this,draw]{++selections;return draw;},100));box->drain();
 }
 void hex(){check(w.empathy(hexer,source,26,0,20,100));}
 void vigor(){check(w.complete_vigorous_spirit(source,source,*vigorous_spirit(12),0,100));}
 void start(){check(w.ordinary_attack(source,target.actor,1000));}
 auto first(std::uint64_t at=1500){w.tick(at-1);check(outcomes().empty());w.tick(at);return outcomes();}
 auto second(std::uint64_t at=2000){w.tick(at-1);check(outcomes().empty());w.tick(at);return outcomes();}

 double health(){return pet?w.npc_combat_snapshot(target)->health->current:double(w.resources(target.actor)->health);}
 std::vector<combat::weapon::ResolvedAttack> outcomes(){return w.take_attack_outcomes(source);}
};
