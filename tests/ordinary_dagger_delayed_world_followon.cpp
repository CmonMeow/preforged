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
int main()try{
 for(bool pet:{false,true})for(bool per_contact:{false,true})for(bool double_strike:{false,true}){
  Fixture f(pet,500,true,per_contact,double_strike?0:.99);f.hex();f.vigor();const auto before=f.health();f.start();
  const auto first=f.first();check(first.size()==1&&first[0].at==1500&&first[0].outcome==combat::weapon::AttackOutcome::hit&&f.selections==1&&f.health()<before);
  const auto after_first=f.health();check(f.w.resources(1)->health==490+vigorous_spirit(12)->healing&&f.w.resources(1)->energy==100);
  const auto second=f.second();check(second.size()==(double_strike?1u:0u));
  if(double_strike){check(second[0].event>first[0].event&&second[0].at==2000&&second[0].outcome==combat::weapon::AttackOutcome::hit&&f.health()<after_first);}
  else check(f.health()==after_first);
  const auto reactions=double_strike&&per_contact?2u:1u;check(f.w.resources(1)->health==500-reactions*10+reactions*vigorous_spirit(12)->healing);
  f.w.cancel_ordinary_attack(f.source);const auto hp=f.health();f.w.tick(2000);check(f.outcomes().empty()&&f.health()==hp&&f.selections==1);
 }
 // Shared speed policy captures first-contact factor ONCE; existing windup and
 // declared second gap are distinct parameters even when both base500 here.
 for(unsigned mask=0;mask<4;++mask){Fixture f;
  if(mask&1)check(f.w.status_mutation(f.source,346,123,0,100,8100,true));
  if(mask&2)check(f.w.faintheartedness(f.hexer,f.source,*faintheartedness(12),100));
  const auto timing=f.w.weapon_attack_timing(f.source,1000);check(bool(timing));const auto first_at=1000+timing->windup_ms;
  // Independent typed scheduling oracle, not a second World motion clock.
  const unsigned multiplier=(mask&1?67u:100u)*(mask&2?150u:100u);const auto gap=(500*multiplier+5000)/10000;
  f.start();const auto first=f.first(first_at);check(first.size()==1&&first[0].at==first_at);const auto hp=f.health();
  const auto second=f.second(first_at+gap);check(second.size()==1&&second[0].at==first_at+gap&&f.health()<hp);f.w.cancel_ordinary_attack(f.source);
 }
 // First contact survives an ordinary cancel/control/equipment change; there is
 // NO fabricated second physical event before its release boundary.
 for(unsigned mode=0;mode<6;++mode){Fixture f;f.hex();f.start();const auto first=f.first();check(first.size()==1);const auto hp=f.health();
  if(mode==0)f.w.cancel_ordinary_attack(f.source);
  if(mode==1)check(f.w.gale(f.hexer,f.source,12,.5,1600));
  if(mode==2){auto changed=f.weapon;++changed.revision;check(f.w.ordinary_weapon(f.source,changed,20,1600,combat::weapon::CriticalProfile{1,0,20}));}
  if(mode==3){const auto old=f.target;f.w.leave(old.actor);f.w.join(old.actor,"Reused contact",80,0,std::make_shared<world::Outbox>(),0,{}, {},1600,2);f.w.set_vitals(old.actor,{1000,1000,100,100},100);check(*f.w.effect_scope(old.actor)!=old);}
  if(mode==4){const auto old=f.source;f.w.leave(old.actor);f.w.join(old.actor,"Reused owner",0,0,f.box,0,{}, {},1600,1);f.w.set_vitals(old.actor,{500,1000,100,100},100);check(*f.w.effect_scope(old.actor)!=old);}
  if(mode==5)check(f.w.ordinary_attack(f.source,f.target.actor,1600)); // SAME target/new Intent epoch
  f.w.tick(2000);check(f.outcomes().empty());
  if(mode!=3)check(f.health()==hp);else check(f.w.resources(2)->health==1000);
 }
 // Cancel/restart with newly faster speed may release a NEW action before
 // the old phase's2000 deadline. Old held parent must not block the new action.
 {Fixture f;f.start();check(f.first().size()==1);check(f.w.status_mutation(f.source,346,123,0,1600,9600,true));
  check(f.w.ordinary_attack(f.source,f.target.actor,1600));f.w.tick(1934);check(f.outcomes().empty());f.w.tick(1935);
  const auto out=f.outcomes();check(out.size()==1&&out[0].at==1935&&f.selections==2);
  const auto hp=f.health();f.w.tick(2000);check(f.outcomes().empty()&&f.health()==hp);f.w.cancel_ordinary_attack(f.source);}
 // Range is sampled at the real delayed contact using shared player motion.
 {Fixture f;f.start();check(f.first().size()==1);const auto hp=f.health();f.w.move(f.target.actor,500,0,1600);check(f.second().empty()&&f.health()==hp);}
 // A retired owned pet cannot borrow its owner's newer actor or generation.
 {Fixture f(true);f.start();check(f.first().size()==1);const auto pet=f.target;f.w.leave(f.pet_owner.actor);f.w.tick(2000);check(!f.w.effect_scope(pet.actor)&&f.outcomes().empty());}
 // Actual second Empathy reaction can kill before second damage. Source first
 // hit stays committed, Vigor cannot revive a corpse, no second timing reuse.
 {Fixture f(false,15,true,true);f.hex();f.start();const auto first=f.first();check(first.size()==1&&f.w.resources(1)->health==5);const auto hp=f.health();
  const auto second=f.second();check(second.size()==1&&second[0].outcome==combat::weapon::AttackOutcome::cancelled&&f.w.resources(1)->health==0&&f.health()==hp);}
 {Fixture f(false,5);f.hex();f.vigor();f.start();const auto first=f.first();check(first.size()==1&&first[0].outcome==combat::weapon::AttackOutcome::cancelled&&f.health()==1000&&f.w.resources(1)->health==0);check(f.second().empty());}
 // Target death/return damage/control after the first physical hit prevent any
 // future second contact allocation, without undoing the first actual hit.
 {Fixture f;f.w.set_vitals(2,{1,1000,100,100},100);f.start();check(f.first().size()==1&&f.health()==0);check(f.second().empty());}
 {Fixture f(false,5);check(f.w.player_holy_equipment(f.source,0));check(f.w.stage_retribution(f.target,f.target,*retribution(12),100));f.start();const auto first=f.first();check(first.size()==1&&first[0].outcome==combat::weapon::AttackOutcome::hit&&f.w.resources(1)->health==0);check(f.second().empty());}
 {Fixture f;check(f.w.player_holy_equipment(f.source,0));const auto judgment=*shield_judgment(0);check(f.w.complete_shield_judgment(f.target,f.target,judgment,0,100));check(f.w.spiteful_spirit(f.hexer,f.source,*spiteful_spirit(0),20,100));f.start();const auto first=f.first();check(first.size()==1&&first[0].outcome==combat::weapon::AttackOutcome::hit&&f.w.knocked_down(f.source,1500)&&f.w.resources(1)->health==500-judgment.damage-spiteful_spirit(0)->damage);check(f.second().empty());}
 // Two actual contact windows resolve accuracy/block independently.
 {Fixture f;f.hex();check(f.w.apply_condition(f.hexer,f.source,combat::conditions::Kind::blind,100,5000));check(f.w.complete_shield_deflection(f.target,f.target,*shield_deflection(12),0,100));unsigned draws=0;f.w.configure_attack_miss_draws([&]{return draws++==0?0.:.999999;});f.w.configure_attack_block_draws([]{return 0.;});f.start();const auto first=f.first();check(first.size()==1&&first[0].outcome==combat::weapon::AttackOutcome::miss);const auto second=f.second();check(second.size()==1&&second[0].outcome==combat::weapon::AttackOutcome::block&&draws==2&&f.health()==1000&&f.w.resources(1)->health==490);}
 // An action already observed without Empathy cannot retroactively preflight
 // a newly installed hex/unsupported Seed cohort in action-once declaration.
 // Physical-contact declaration WOULD react, so its unready future contact
 // refuses without a fake release/outcome or retrospective first-hit rollback.
 for(bool per_contact:{false,true}){Fixture f(false,500,true,per_contact,0,true);f.start();check(f.first().size()==1);const auto hp=f.health();
  f.w.join(4,"Seed monk",-60,0,std::make_shared<world::Outbox>(),0,{}, {},1600,1);f.w.set_vitals(4,{500,1000,100,100},100);
  check(f.w.party_action(1,158,4,1600)&&f.w.party_action(4,154,1,1600));check(f.w.complete_healing_seed(*f.w.effect_scope(4),f.source,*healing_seed(12),0,1600));
  check(f.w.empathy(f.hexer,f.source,26,0,20,1650));const auto second=f.second();check(second.size()==(per_contact?0u:1u)&&f.w.resources(1)->health==500);
  check(per_contact?f.health()==hp:f.health()<hp);check(f.w.npc_combat_snapshot(*f.w.effect_scope(100))->health->current==500);}
 // The World mincut, not only outer tick cadence, owns the delayed release.
 // Sparse observation2300 must drain the actual contact at2000 before actors
 // are integrated beyond it. Compare real Health against independent denseWorld.
 {Fixture dense,sparse;for(auto* f:{&dense,&sparse}){check(f->w.apply_condition(f->hexer,f->source,combat::conditions::Kind::poison,100,5000));f->start();check(f->first().size()==1);}
  dense.w.tick(2000);const auto d=dense.outcomes();check(d.size()==1&&d[0].at==2000);dense.w.tick(2300);
  sparse.w.tick(2300);const auto s=sparse.outcomes();check(s.size()==1&&s[0].at==2000&&dense.health()==sparse.health()&&dense.w.resources(1)->health==sparse.w.resources(1)->health);}
 // No enrollment means no chance draw or implicit second-contact support.
 {Fixture f(false,500,false);f.start();check(f.first().size()==1);check(f.second().empty()&&f.selections==0);}
 std::cout<<"PASS declared empirical delayed ordinary dagger retained canonical phase and exact mincut/player/pet/Health/control/miss/block/grains; no exactretail timing claim\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
