// Unregistered actual-World acceptance for staged common player Empathy candidate.
#include "gw_world_simulation.h"
#include "dagger_retained_owner_fixture_draft.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Player Empathy World line "+std::to_string(at.line()));}
struct Fixture {
 world::Instance w;std::shared_ptr<world::Outbox> owner_box=std::make_shared<world::Outbox>();std::unique_ptr<test::RetainedDaggerOwnerDraft> paid_owner;Scope actor,hexer,target;combat::weapon::Owned weapon{12,1,{20,20,2},12,false,false,27,0.};bool npc_target;
 Fixture(bool npc=false,float distance=60,bool known_armor=true,bool unknown_ally=false):npc_target(npc){
  if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=distance;n.definition.level=20;n.definition.primary=3;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding={146,100,100,0,0,0,0,20};d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=4;d.damageable=d.conditions_allowed=true;if(known_armor)d.physical_armor=60;d.knockdown_allowed=true;d.knockdown_evidence="synthetic";d.healing_allowed=true;d.healing_evidence="synthetic";n.combat_definition=d;w.configure_npcs({n});}
  if(unknown_ally){world::Instance::Npc n;n.id=n.definition.id=100;n.x=30;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
   d.binding={146,100,100,0,0,0,0,20};d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{500,1000};d.damageable=true;
   d.creature_type="spirit";d.creature_type_evidence="authored unsupported Seed gain recipient";d.fleshy=false;d.fleshy_evidence="authored non-fleshy spirit";
   combat::npcs::LiveActor authority(d,{1,1,1,146,100});const auto life=authority.lifetime();check(authority.healable_ally(life)&&!authority.healing_allowed(life).has_value()&&!authority.gain_ally_health(life,25,100));
   n.combat_definition=d;w.configure_npcs({n});}
  w.join(1,"Attacker",0,0,owner_box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);actor=*w.effect_scope(1);
  w.join(3,"Hexer",-1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);hexer=*w.effect_scope(3);
  if(!npc){w.join(2,"Defender",distance,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);target=*w.effect_scope(2);check(w.physical_armor(target,60));check(w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));}
  else target=*w.effect_scope(100);
  equip(27);w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .999999;});
 }
 bool retained_dual(const combat::weapon::AttackPlan& plan,std::uint64_t at){
  if(!paid_owner)paid_owner=std::make_unique<test::RetainedDaggerOwnerDraft>(w,actor,owner_box);
  return paid_owner->execute(target,plan.skill,plan.rank,at);
 }
 void equip(unsigned type){weapon.item_type=type;weapon.damage.raw_damage_type=type==5?1:2;check(w.ordinary_weapon(actor,weapon,20,100,combat::weapon::CriticalProfile{type==32?7u:1u,0,20}));}
 void hex(unsigned id=26,std::uint64_t at=100){check(w.empathy(hexer,actor,id,0,20,at));}
 double health()const{return npc_target?w.npc_combat_snapshot(target)->health->current:double(w.resources(target.actor)->health);}
 void block(){if(npc_target)check(w.stage_npc_shield_deflection(target,*shield_deflection(12),100));else check(w.complete_shield_deflection(target,target,*shield_deflection(12),0,100));w.configure_attack_block_draws([]{return 0.;});}
 double ordinary(unsigned mode,bool apply,unsigned hexskill=26){
  if(apply)hex(hexskill);if(mode==1){check(w.apply_condition(hexer,actor,combat::conditions::Kind::blind,100,10000));w.configure_attack_miss_draws([]{return 0.;});}if(mode==2)block();
  check(w.ordinary_attack(actor,target.actor,1000));w.tick(1800);w.cancel_ordinary_attack(actor);const auto after=w.resources(1)->health;w.tick(1801);check(w.resources(1)->health==after);return 1000-health();
 }
};
int main()try{
 for(bool npc:{false,true})for(unsigned mode=0;mode<3;++mode)for(unsigned skill:{26u,3151u}){
  Fixture baseline(npc),active(npc);const auto raw=baseline.ordinary(mode,false),actual=active.ordinary(mode,true,skill);
  check(active.w.resources(1)->health==1000-(skill==26?10:15));
  check(actual==(mode?0.:std::max(0.,raw-(skill==26?1.:0.))));
 }
 // Cancel before a real release: no Empathy event is fabricated from input.
 {Fixture f;f.hex();check(f.w.ordinary_attack(f.actor,f.target.actor,1000));f.w.cancel_ordinary_attack(f.actor);f.w.tick(1800);check(f.w.resources(1)->health==1000&&f.health()==1000);}
 // Actual failed Earth Shaker counts; a spell/non-attack never calls consumer.
 for(bool npc:{false,true}){Fixture f(npc);f.equip(15);f.hex();f.w.configure_earth_shaker_failure_draws([]{return 0.;});check(f.w.melee_attack(f.actor,f.target,f.weapon,*combat::weapon::earth_shaker_attack_plan(4),1000));check(f.w.resources(1)->health==990&&f.health()==1000);const auto failed=f.w.take_attack_outcomes(f.actor);check(failed.size()==1&&failed.front().skill_failed&&failed.front().outcome==combat::weapon::AttackOutcome::cancelled);}
 // Real arrow launch and later impact have independent hex snapshots.
 for(bool before_launch:{false,true}){Fixture baseline(false,600),f(false,600);baseline.equip(5);f.equip(5);
  if(before_launch)f.hex();
  check(baseline.w.ordinary_attack(baseline.actor,baseline.target.actor,4400));check(f.w.ordinary_attack(f.actor,f.target.actor,4400));baseline.w.tick(5000);f.w.tick(5000);baseline.w.cancel_ordinary_attack(baseline.actor);f.w.cancel_ordinary_attack(f.actor);
  check(f.health()==1000&&f.w.resources(1)->health==(before_launch?990u:1000u));
  if(!before_launch)f.hex(26,5100);
  baseline.w.tick(5500);f.w.tick(5500);const auto raw=1000-baseline.health(),actual=1000-f.health();
  check(actual==std::max(0.,raw-(before_launch?0.:1.)));check(f.w.resources(1)->health==(before_launch?990u:1000u));
 }
 // Existing real dual-contact owner: one Empathy reaction for EACH strike,
 // separate from once-per-logical-attack Spiteful bookkeeping; Vigorous is per actual attack.
 {Fixture f;f.equip(32);check(f.w.gale(f.actor,f.target,12,.5,1000));
  const auto off=combat::weapon::attack_skill(778,0,0,0,true),dual=combat::weapon::attack_skill(775,0,0,0,true);check(off&&dual);
  check(f.w.melee_attack(f.actor,f.target,f.weapon,*off,1000,{},true));f.w.take_attack_outcomes(f.actor);f.hex(26,1000);
  check(f.retained_dual(*dual,1000));check(f.w.take_attack_outcomes(f.actor).size()==2);check(f.w.resources(1)->health==980);
 }
 // Cyclone is one attack with multiple hits; Empathy reacts only once.
 {Fixture f;f.equip(2);f.w.join(4,"Area defender",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(4,{1000,1000,100,100},100);const auto extra=*f.w.effect_scope(4);
  check(f.w.physical_armor(extra,60));check(f.w.ordinary_weapon(extra,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));f.hex();const auto plan=combat::weapon::attack_skill(330,0);check(bool(plan));
  check(f.w.melee_attack(f.actor,f.actor,f.weapon,*plan,1000));check(f.w.take_attack_outcomes(f.actor).size()==2);check(f.w.resources(1)->health==990);
 }
 // As with the existing NPC consumer, a lethal release reaction cancels the
 // current melee contact and leaves no orphan pending outcome reservation.
 {Fixture f;f.w.set_vitals(1,{5,1000,100,100},100);f.hex();const auto plan=combat::weapon::attack_skill(320,12);check(bool(plan));
  check(!f.w.melee_attack(f.actor,f.target,f.weapon,*plan,1000));check(f.w.resources(1)->health==0&&f.health()==1000);
 }
 // Shared selected-hex prevention consumes Hex Breaker, never installs Empathy.
 {Fixture f;check(f.w.hex_breaker(f.actor,*hex_breaker(12),20,100));f.hex();
  const auto status=f.w.status_snapshot(f.actor,100);check(status&&std::none_of(status->begin(),status->end(),[](const auto&e){return e.skill==26||e.skill==10;}));
  check(f.w.resources(3)->health==961);f.ordinary(0,false);check(f.w.resources(1)->health==1000);}
 // IW replacement bypasses hit/miss/block but each actual release still reacts.
 for(bool npc:{false,true}){Fixture f(npc);skills::Slots bar{};bar[0]={33,0};check(f.w.configure_adrenaline(f.actor,bar,100));
  check(f.w.status_mutation(f.actor,33,123,12,100,30100,true));f.hex();
  check(f.w.apply_condition(f.hexer,f.actor,combat::conditions::Kind::blind,100,10000));f.block();
  check(f.w.ordinary_attack(f.actor,f.target.actor,1000));f.w.tick(1800);f.w.cancel_ordinary_attack(f.actor);
  const auto events=f.w.take_attack_outcomes(f.actor);check(events.size()==1&&events.front().outcome==combat::weapon::AttackOutcome::replaced);
  check(f.w.resources(1)->health==990&&f.health()==966);}
 // Empathy is shared armor-ignoring spell damage: Protective Spirit caps it.
 {Fixture f;f.w.set_vitals(1,{100,100,100,100},100);check(f.w.complete_protective_spirit(f.actor,f.actor,*protective_spirit(12),0,100));
  f.hex(3151);f.ordinary(0,false);check(f.w.resources(1)->health==90);}
 // Healing Seed observes the same committed reactive spell hit and heals once.
 {Fixture f;f.w.join(4,"Seed monk",-60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);f.w.set_vitals(4,{500,1000,100,100},100);f.w.set_vitals(1,{500,1000,100,100},100);
  check(f.w.party_action(1,158,4,100));check(f.w.party_action(4,154,1,100));const auto monk=*f.w.effect_scope(4);
  check(f.w.complete_healing_seed(monk,f.actor,*healing_seed(12),0,100));f.hex();f.ordinary(0,false);
  check(f.w.resources(1)->health==515&&f.w.resources(4)->health==525);f.w.tick(1802);check(f.w.resources(4)->health==525);}
 // A real Barrage/Volley emits one accepted release per physical arrow.
 for(unsigned skill:{395u,2144u}){Fixture f(false,600);f.equip(5);f.w.join(4,"Bow extra",630,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(4,{1000,1000,100,100},100);
  const auto extra=*f.w.effect_scope(4);check(f.w.physical_armor(extra,60));check(f.w.ordinary_weapon(extra,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));f.hex();
  const auto plan=combat::weapon::attack_skill(skill,12);check(bool(plan));check(f.w.melee_attack(f.actor,f.target,f.weapon,*plan,1000));check(f.w.resources(1)->health==980);
  f.w.cancel_ordinary_attack(f.actor);f.w.tick(2000);check(f.w.take_attack_outcomes(f.actor).size()==2&&f.w.resources(1)->health==980);}
 // Caster reaction precedes Vigor healing: a lethal Empathy release cannot
 // gain enough healing to survive and then contact the selected defender.
 {Fixture f;f.w.set_vitals(1,{10,1000,100,100},100);check(f.w.complete_vigorous_spirit(f.actor,f.actor,*vigorous_spirit(12),0,100));f.hex();
  const auto plan=combat::weapon::attack_skill(320,12);check(bool(plan));check(!f.w.melee_attack(f.actor,f.target,f.weapon,*plan,1000));
  check(f.w.resources(1)->health==0&&f.health()==1000);}
 // Missing authoritative target facts refuse before an attack is made.
 {Fixture f(true,60,false);f.hex();const auto plan=combat::weapon::attack_skill(320,12);check(bool(plan));
  check(!f.w.melee_attack(f.actor,f.target,f.weapon,*plan,1000));check(f.w.resources(1)->health==1000&&f.health()==1000&&f.w.take_attack_outcomes(f.actor).empty());}
 {Fixture f(true);f.hex();const auto plan=combat::weapon::attack_skill(382,12);check(bool(plan));
  check(!f.w.melee_attack(f.actor,f.target,f.weapon,*plan,1000));check(f.w.resources(1)->health==1000&&f.health()==1000&&f.w.take_attack_outcomes(f.actor).empty());}
 // Unknown nearby NPC healing/gain authority makes Seed reaction unready;
 // neither Empathy nor the target weapon contact may commit from that refusal.
 {Fixture f(false,60,true,true);f.w.join(4,"Seed monk",-60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);f.w.set_vitals(4,{500,1000,100,100},100);
  check(f.w.party_action(1,158,4,100));check(f.w.party_action(4,154,1,100));check(f.w.complete_healing_seed(*f.w.effect_scope(4),f.actor,*healing_seed(12),0,100));
f.hex();
  const auto plan=combat::weapon::attack_skill(320,12);check(bool(plan));const bool accepted=f.w.melee_attack(f.actor,f.target,f.weapon,*plan,1000);
  const auto outcomes=f.w.take_attack_outcomes(f.actor);std::cout<<"sourceSeedGate spirit_unknown_gain accepted="<<accepted<<" sourceHP="<<f.w.resources(1)->health<<" targetHP="<<f.health()<<" outcomes="<<outcomes.size()<<" empathyLoss="<<1000-f.w.resources(1)->health<<'\n';
  check(!accepted);check(f.w.resources(1)->health==1000&&f.health()==1000);}
 // A later legitimate source death cancels impact, not the already-made
 // launch attack. Its Empathy reaction remains committed exactly once.
 {Fixture f(false,600);f.equip(5);f.hex();check(f.w.ordinary_attack(f.actor,f.target.actor,1000));f.w.tick(1600);f.w.cancel_ordinary_attack(f.actor);
  check(f.w.resources(1)->health==990&&f.health()==1000);check(f.w.settle_resources(f.actor,1650));f.w.set_vitals(1,{0,1000,100,100},100);f.w.tick(2000);
  const auto outcomes=f.w.take_attack_outcomes(f.actor);check(outcomes.size()==1&&outcomes.front().outcome==combat::weapon::AttackOutcome::cancelled);
  check(f.w.resources(1)->health==0&&f.health()==1000);}
 // Nonattack spell completion has no physical attack release to consume.
 {Fixture f;f.hex();check(f.w.gale(f.actor,f.target,12,.5,1000));check(f.w.resources(1)->health==1000&&f.w.take_attack_outcomes(f.actor).empty());}
 // Adopted paid failure producer is separate from resolve_attack_skill:
 // real prerequisite failure still triggers each physical Empathy release.
 for(unsigned skill:{780u,3251u,775u}){Fixture f;f.equip(32);f.hex();const auto plan=combat::weapon::attack_skill(skill,0,0,0,true);check(bool(plan));
  const auto release=1000+plan->windup_ms;const auto permit=f.w.prepare_dagger_cast_draft(f.actor,f.target,f.weapon,*plan,1000,release);check(bool(permit));
  auto result=f.w.complete_dagger_cast_draft(f.actor,f.target,f.weapon,*plan,permit,release);check(result.accepted&&result.prerequisite_failed&&result.outcomes.size()==1&&result.outcomes.front().skill_failed);
  check(f.w.resources(1)->health==990&&f.health()==1000);
  if(result.dual_failure_feedback){const auto phase=result.dual_failure_feedback;check(bool(phase->consume(0,f.actor,f.target,skill,0,permit,release)));
   auto second=f.w.complete_dagger_failed_second_draft(phase,phase->second_due());check(second.accepted&&second.prerequisite_failed&&second.outcomes.size()==1&&second.outcomes.front().skill_failed);check(f.w.resources(1)->health==980);}
 }
 // Interleave caster reactions per actual arrow: Vigor from the first made
 // attack can keep the source alive for the second. No up-front damage batch.
 for(bool vigor:{false,true}){Fixture f(false,600);f.equip(5);f.w.set_vitals(1,{15,1000,100,100},100);f.w.join(4,"Bow extra",630,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(4,{1000,1000,100,100},100);
  const auto extra=*f.w.effect_scope(4);check(f.w.physical_armor(extra,60));check(f.w.ordinary_weapon(extra,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));
  if(vigor)check(f.w.complete_vigorous_spirit(f.actor,f.actor,*vigorous_spirit(12),0,100));f.hex();const auto plan=combat::weapon::attack_skill(395,12);check(bool(plan));
  check(f.w.melee_attack(f.actor,f.target,f.weapon,*plan,1000)==vigor);check(f.w.resources(1)->health==(vigor?15-20+2*vigorous_spirit(12)->healing:0));
  f.w.tick(2000);const auto outcomes=f.w.take_attack_outcomes(f.actor);check(outcomes.size()==2);
  if(!vigor){check(f.health()==1000&&f.w.resources(4)->health==1000);check(std::all_of(outcomes.begin(),outcomes.end(),[](const auto&e){return e.outcome==combat::weapon::AttackOutcome::cancelled;}));}}
 // A dual's first physical contact is committed before a fatal second release.
 {Fixture f;f.equip(32);check(f.w.gale(f.actor,f.target,12,.5,1000));const auto off=combat::weapon::attack_skill(778,0,0,0,true),dual=combat::weapon::attack_skill(775,0,0,0,true);check(off&&dual);
  check(f.w.melee_attack(f.actor,f.target,f.weapon,*off,1000,{},true));f.w.take_attack_outcomes(f.actor);f.w.set_vitals(1,{15,1000,100,100},100);f.hex(26,1000);
  check(f.retained_dual(*dual,1000));const auto outcomes=f.w.take_attack_outcomes(f.actor);check(outcomes.size()==2&&outcomes[0].outcome==combat::weapon::AttackOutcome::hit&&outcomes[1].outcome==combat::weapon::AttackOutcome::cancelled);check(f.w.resources(1)->health==0);}
 // Launch already observed without Empathy. A late hex plus an unready new
 // Seed cohort cannot retroactively run readiness or cancel this actual arrow.
 {Fixture f(false,600,true,true);f.equip(5);check(f.w.ordinary_attack(f.actor,f.target.actor,1000));f.w.tick(1600);f.w.cancel_ordinary_attack(f.actor);
  f.w.join(4,"Seed monk",-60,0,std::make_shared<world::Outbox>(),0,{}, {},1600,1);f.w.set_vitals(4,{500,1000,100,100},100);
  check(f.w.party_action(1,158,4,1600));check(f.w.party_action(4,154,1,1600));check(f.w.complete_healing_seed(*f.w.effect_scope(4),f.actor,*healing_seed(12),0,1600));
f.hex(26,1650);
  f.w.tick(2000);const auto outcomes=f.w.take_attack_outcomes(f.actor);check(outcomes.size()==1&&outcomes.front().outcome==combat::weapon::AttackOutcome::hit&&f.health()<1000&&f.w.resources(1)->health==1000);}
 // Wailing and independent critical interruption share readiness but retain
 // the established one critical draw after accuracy/block, before contact.
 for(bool critical:{false,true}){Fixture f;f.equip(2);
  check(f.w.physical_armor(f.actor,60)); // The defender now really attacks this actor.
  check(f.w.ordinary_weapon(f.target,f.weapon,20,100,combat::weapon::CriticalProfile{1,0,20}));
  const auto wailing=weapon_spell(794,12,8,0);check(bool(wailing));
  const bool applied=f.w.apply_weapon_spell(f.actor,f.actor,*wailing,100);
  std::cout<<"wailingGate critical="<<critical<<" applied="<<applied<<'\n';check(applied);
  f.hex();const bool attacking=f.w.ordinary_attack(f.target,f.actor.actor,1000);
  std::cout<<"wailingGate attacking="<<attacking<<" sourceHP="<<f.w.resources(1)->health<<'\n';check(attacking);
  unsigned critical_draws=0;f.w.configure_attack_critical_draws([&]{++critical_draws;return critical?0.:.999999;});
  const auto plan=combat::weapon::attack_skill(1402,12);check(bool(plan));
  const bool accepted=f.w.melee_attack(f.actor,f.target,f.weapon,*plan,1000);
  std::cout<<"wailingGate admitted="<<accepted<<" criticalDraws="<<critical_draws<<" sourceHP="<<f.w.resources(1)->health<<" targetHP="<<f.health()<<'\n';check(accepted);
  check(critical_draws==1&&f.w.resources(1)->health==990);
  f.w.tick(1800);std::cout<<"wailingGate postTickHP="<<f.w.resources(1)->health<<'\n';check(f.w.resources(1)->health==990);}
 // Ordinary melee uses the same pre-Vigor caster phase as skill attacks.
 for(bool npc:{false,true}){Fixture f(npc);f.w.set_vitals(1,{10,1000,100,100},100);check(f.w.complete_vigorous_spirit(f.actor,f.actor,*vigorous_spirit(12),0,100));f.hex();
  check(f.w.ordinary_attack(f.actor,f.target.actor,1000));f.w.tick(1800);check(f.w.resources(1)->health==0&&f.health()==1000);}
 // A known miss is still an attack; hypothetical target Seed gain readiness
 // is irrelevant. Ordinary accuracy stays in its existing draw phase too.
 for(bool ordinary:{false,true}){Fixture f(false,60,true,true);f.w.set_vitals(1,{500,1000,100,100},100);check(f.w.complete_vigorous_spirit(f.actor,f.actor,*vigorous_spirit(12),0,100));
  check(f.w.party_action(3,158,2,100));check(f.w.party_action(2,154,3,100));check(f.w.complete_healing_seed(f.hexer,f.target,*healing_seed(12),0,100));

  f.hex();check(f.w.apply_condition(f.hexer,f.actor,combat::conditions::Kind::blind,100,10000));f.w.configure_attack_miss_draws([]{return 0.;});
  if(ordinary){check(f.w.ordinary_attack(f.actor,f.target.actor,1000));f.w.tick(1800);f.w.cancel_ordinary_attack(f.actor);}
  else{const auto plan=combat::weapon::attack_skill(320,12);check(bool(plan));check(f.w.melee_attack(f.actor,f.target,f.weapon,*plan,1000));}
  const auto outcomes=f.w.take_attack_outcomes(f.actor);check(outcomes.size()==1&&outcomes.front().outcome==combat::weapon::AttackOutcome::miss);check(f.w.resources(1)->health==500-10+vigorous_spirit(12)->healing&&f.health()==1000);}
 // Replacing PvE/PvP aliases never stacks reactions. Stale scopes reject.
 {Fixture f;auto stale=f.actor;++stale.generation;check(!f.w.empathy(f.hexer,stale,26,0,20,100));f.hex();f.hex(3151,100);
  const auto entries=f.w.status_snapshot(f.actor,100);check(entries&&entries->size()==1&&entries->front().skill==3151);f.ordinary(0,false);check(f.w.resources(1)->health==985);}
 std::cout<<"PASS staged player Empathy actual hit/miss/block/failed/cancel/dual and launch-vs-impact lifetimes\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
