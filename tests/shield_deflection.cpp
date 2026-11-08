#include "gw_world_simulation.h"
#include "gw_block_attribution.h"
#include "warrior_condition_fixture.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Shield Deflection line "+std::to_string(at.line()));}
int main()try{
 for(unsigned rank=0;rank<=20;++rank){const auto plan=*shield_deflection(rank);check(plan.block_percent==75&&plan.armor==15+rank);
  const Scope caster{1,1,1},target{1,100,1};Registry registry(target);check(registry.apply(target,{1,1,259,rank,Category::enchantment,1000,1000+plan.duration},Replace::same_skill).accepted);
  auto entries=registry.active(target,1000);check(reviewed_block_probability(entries,false,1000)==.75&&reviewed_block_probability(entries,true,1000)==0.);
  auto sources=reviewed_block_sources(entries,1000);check(sources&&sources->size()==1&&sources->front().skill==259&&sources->front().probability==.75);
  for(double draw:{0.,.749,.75,.999}){auto result=resolve_attributed_block_draw(*sources,draw);check(result&&result->outcome==(draw<.75?combat::weapon::AttackOutcome::block:combat::weapon::AttackOutcome::hit));}
  entries.push_back(entries[0]);check(reviewed_block_probability(entries,false,1000)==.75&&reviewed_block_sources(entries,1000)->size()==1);
  entries.push_back({2,1,258,12,Category::enchantment,1000,20000});check(reviewed_block_probability(entries,false,1000)==.875&&reviewed_block_sources(entries,1000)->size()==2);
  entries.push_back({3,1,137,12,Category::hex,1000,20000});check(reviewed_block_probability(entries,false,1000)==0.);
  check(shield_deflection_armor(registry,target,999)==0&&shield_deflection_armor(registry,target,1000)==plan.armor&&shield_deflection_armor(registry,target,1000+plan.duration)==0);
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=d.physical_armor=d.general_armor=60;
  combat::npcs::LiveActor n(d,{1,1,1,146,100});const auto life=n.lifetime();auto forged=plan;++forged.armor;check(!n.apply_shield_deflection(life,caster,forged,1,1000));
  check(n.apply_shield_deflection(life,caster,plan,1,1000));check(!n.apply_shield_deflection(life,caster,plan,1,1000));
  const auto expected=60+int(plan.armor);check(n.final_earth_armor(life,1000)==expected&&n.final_physical_armor(life,1000)==expected&&n.final_caster_armor(life,3,1000)==expected);
  const auto hit=n.apply_damage(life,{combat::Kind::cold,100,20,144},1000);check(hit.reason==combat::Reason::accepted&&std::abs(hit.damage-100*std::exp2(-double(plan.armor)/40.))<1e-9);
  const auto unprotected=n.apply_damage(life,{combat::Kind::cold,100,20,144},1000+plan.duration);check(unprotected.reason==combat::Reason::accepted&&unprotected.damage==100);
  check(n.final_physical_armor(life,1000+plan.duration)==60);
 }
 // Real World attack admission must use attribution as well as the aggregate probability.
 for(unsigned mode=0;mode<7;++mode){world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Attacker",0,0,box,0,{}, {},100,1);w.join(2,"Defender",mode==3?900.f:60.f,0,box,0,{}, {},100,2);for(unsigned id:{1u,2u})w.set_vitals(id,{500,500,100,100},100);
  const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.physical_armor(target,60));check(w.status_mutation(target,259,50,12,100,9100,true));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return mode==5?1.:mode==6?.9:0.;});combat::weapon::Owned weapon{1,1,{20,20,2},12,false,false,mode==2?2u:mode==3?5u:27u};check(w.ordinary_weapon(source,weapon,20));const unsigned at=mode==4?9200:200;
  if(mode==0||mode==4||mode==6){check(w.ordinary_attack(source,2,at));w.tick(at+500);w.cancel_ordinary_attack(source);}
  else if(mode==3){check(w.melee_attack(source,target,weapon,*combat::weapon::attack_skill(394,12),at));w.tick(at+360);}
  else if(mode==2)check(w.melee_attack(source,source,weapon,*combat::weapon::attack_skill(330,12),at));
  else check(w.melee_attack(source,target,weapon,*combat::weapon::attack_skill(322,0),at)==(mode!=5));
  check(w.resources(2)->health==(mode==4?480u:mode==6?488u:500u));check(draws==(mode==4?0u:1u));
  auto outcomes=w.take_attack_outcomes(source);check(outcomes.size()==1);check(outcomes[0].outcome==(mode==4||mode==6?combat::weapon::AttackOutcome::hit:mode==5?combat::weapon::AttackOutcome::cancelled:combat::weapon::AttackOutcome::block));
 }
 {warrior_test::Fixture f(400,false);unsigned draws=0;check(f.world.status_mutation(f.target,259,99,12,100,9100,true));f.world.configure_attack_block_draws([&]{++draws;return 0.;});
  check(f.world.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(400,12),1000));f.world.tick(1100);check(draws==0&&f.health()==979);
 }
 {const std::array<Entry,1> bad{{{1,1,259,21,Category::enchantment,100,1000}}};check(!reviewed_block_probability(bad,true,200)&&!reviewed_block_sources(bad,200));}
 check(!shield_deflection(21));std::cout<<"PASS21ranks Deflection block/attribution/armor/NPC primitives and8actual World attack cases;normal casting/NPC block contact pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
