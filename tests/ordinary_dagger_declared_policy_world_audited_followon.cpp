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
 static Policy policy(bool per_contact=false){Policy p;p.chance={combat::weapon::OrdinaryDaggerChancePolicyFollowon::Basis::explicit_inherent_two,"local/modern mainwiki2%+rank2%; reviewed test draw"};p.timing=Policy::Timing::authored_simultaneous;p.empathy=p.vigor=per_contact?Policy::Grain::physical_contacts_declared:Policy::Grain::action_once_declared;p.evidence="AUTHORED simultaneous/grain comparison; retailtiming/grain unproven";return p;}
 Fixture(bool owned_pet=false,unsigned hp=500,bool enroll=true,bool per_contact=false,double draw=0):pet(owned_pet){
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
 void swing(){check(w.ordinary_attack(source,target.actor,1000));w.tick(1499);check(w.take_attack_outcomes(source).empty());w.tick(1500);w.cancel_ordinary_attack(source);}
 double health(){return pet?w.npc_combat_snapshot(target)->health->current:double(w.resources(target.actor)->health);}
 auto outcomes(){return w.take_attack_outcomes(source);}
};
int main()try{
 for(bool pet:{false,true})for(bool per_contact:{false,true})for(bool double_strike:{false,true}){
  Fixture f(pet,500,true,per_contact,double_strike?0:.99);const auto before=f.health();f.hex();f.vigor();f.swing();const auto out=f.outcomes();const unsigned count=double_strike?2:1;
  check(f.selections==1&&out.size()==count&&f.health()<before&&f.w.resources(1)->energy==100);
  for(const auto& event:out)check(event.target==f.target&&event.outcome==combat::weapon::AttackOutcome::hit&&event.at==1500);
  const auto reactions=per_contact?count:1;check(f.w.resources(1)->health==500-reactions*10+reactions*vigorous_spirit(12)->healing);
  const auto hp=f.health();const auto sourcehp=f.w.resources(1)->health;f.w.tick(1500);check(f.outcomes().empty()&&f.health()==hp&&f.w.resources(1)->health==sourcehp&&f.selections==1);
 }
 // Source Spiteful is one canonical action even in declared physical-reactive mode.
 {Fixture f(false,500,true,true);f.hex();f.vigor();check(f.w.spiteful_spirit(f.hexer,f.source,*spiteful_spirit(0),20,100));f.swing();
  check(f.outcomes().size()==2&&f.w.resources(1)->health==500-20+2*vigorous_spirit(12)->healing-spiteful_spirit(0)->damage);}
 // Actual first-contact Judgment changes source control while it survives.
 // Logical Spiteful must still react once after the second contact cancels.
 {Fixture f;check(f.w.player_holy_equipment(f.source,0));const auto judgment=*shield_judgment(0);
  check(f.w.complete_shield_judgment(f.target,f.target,judgment,0,100));
  check(f.w.spiteful_spirit(f.hexer,f.source,*spiteful_spirit(0),20,100));f.swing();
  const auto out=f.outcomes();check(out.size()==2&&out[0].outcome==combat::weapon::AttackOutcome::hit&&out[1].outcome==combat::weapon::AttackOutcome::cancelled);
  check(f.w.knocked_down(f.source,1500)&&f.health()<1000&&f.w.resources(1)->health==500-judgment.damage-spiteful_spirit(0)->damage);
  check(f.w.shield_judgment_failures()==0);}
 // Default remains old single ordinary contact; no injected draw/implicit enroll.
 {Fixture f(false,500,false);f.hex();f.swing();check(f.outcomes().size()==1&&f.selections==0&&f.w.resources(1)->health==490);}
 // Declared physical-grain fatal second reaction preserves only the first hit.
 {Fixture f(false,15,true,true);const auto before=f.health();f.hex();f.swing();const auto out=f.outcomes();check(out.size()==2&&out[0].outcome==combat::weapon::AttackOutcome::hit&&out[1].outcome==combat::weapon::AttackOutcome::cancelled&&f.health()<before&&f.w.resources(1)->health==0);}
 // Fatal first source reaction cannot heal through Vigor or hit any contact.
 {Fixture f(false,5);f.hex();f.vigor();f.swing();const auto out=f.outcomes();check(out.size()==2&&f.health()==1000&&f.w.resources(1)->health==0);for(const auto& event:out)check(event.outcome==combat::weapon::AttackOutcome::cancelled);}
 // First contact kills target: second reservation retires before phantom observers.
 {Fixture f(false,500,true,true);f.w.set_vitals(2,{1,1000,100,100},100);f.hex();f.vigor();f.swing();const auto out=f.outcomes();check(out.size()==2&&out[0].outcome==combat::weapon::AttackOutcome::hit&&out[1].outcome==combat::weapon::AttackOutcome::target_retired&&f.health()==0&&f.w.resources(1)->health==490+vigorous_spirit(12)->healing);}
 // Shared return-damage after first hit kills source; second contact cancels.
 {Fixture f(false,5);check(f.w.player_holy_equipment(f.source,0));check(f.w.stage_retribution(f.target,f.target,*retribution(12),100));f.swing();const auto out=f.outcomes();check(out.size()==2&&out[0].outcome==combat::weapon::AttackOutcome::hit&&out[1].outcome==combat::weapon::AttackOutcome::cancelled&&f.health()<1000&&f.w.resources(1)->health==0);}
 // Independent physical accuracy/block callbacks use existing shared consumers.
 {Fixture f;f.hex();f.vigor();check(f.w.apply_condition(f.hexer,f.source,combat::conditions::Kind::blind,100,5000));check(f.w.complete_shield_deflection(f.target,f.target,*shield_deflection(12),0,100));unsigned draws=0;f.w.configure_attack_miss_draws([&]{return draws++==0?0.:.999999;});f.w.configure_attack_block_draws([]{return 0.;});f.swing();const auto out=f.outcomes();check(out.size()==2&&out[0].outcome==combat::weapon::AttackOutcome::miss&&out[1].outcome==combat::weapon::AttackOutcome::block&&draws==2&&f.health()==1000&&f.w.resources(1)->health==490+vigorous_spirit(12)->healing);}
 // Existing ordinary cancel/control/equipment/target-lifetime state remains authority.
 for(unsigned mode=0;mode<4;++mode){Fixture f;f.hex();check(f.w.ordinary_attack(f.source,f.target.actor,1000));
  if(mode==0)f.w.cancel_ordinary_attack(f.source);
  if(mode==1)check(f.w.gale(f.hexer,f.source,12,.5,1100));
  if(mode==2){auto weapon=f.weapon;++weapon.revision;check(f.w.ordinary_weapon(f.source,weapon,20,1100,combat::weapon::CriticalProfile{1,0,20}));}
  if(mode==3){const auto old=f.target;f.w.leave(old.actor);f.w.join(old.actor,"Reused contact",80,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);f.w.set_vitals(old.actor,{1000,1000,100,100},100);check(*f.w.effect_scope(old.actor)!=old);}
  f.w.tick(1500);check(f.outcomes().empty()&&f.selections==0&&f.w.resources(1)->health==500);
 }
 std::cout<<"PASS DECLARED ordinary dagger canonical action/contacts and actual shared World Health/Empathy/Vigor/player/pet/miss/block/control/lifetimes; retail timing/grains remain explicit policy\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
