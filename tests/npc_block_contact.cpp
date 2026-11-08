#include "warrior_condition_fixture.h"
#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("NPC block contact line "+std::to_string(at.line()));}
int main()try{using O=combat::weapon::AttackOutcome;unsigned cases=0;
 for(unsigned skill:{320u,394u,330u,408u,400u,321u,328u,327u,341u,356u,386u,1134u})for(unsigned mode=0;mode<5;++mode){++cases;
  warrior_test::Fixture f(skill,true);auto& w=f.world;if(skill==327||skill==1134||skill==321){f.weapon.item_type=27;check(w.ordinary_weapon(f.source,f.weapon,20));}
  check(w.stage_npc_shield_deflection(f.target,*skills::effects::shield_deflection(12),100));unsigned draws=0;
  w.configure_attack_block_draws([&]{++draws;return mode==1?.99:mode==4?1.:0.;});
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,20000));
  const auto token=w.begin_npc_action(f.target,combat::npcs::control::Action::spell,100,20000,skills::Slot{180,0});check(bool(token));
  const unsigned at=mode==3?9200:500;const bool accepted=w.melee_attack(f.source,skill==330?f.source:f.target,f.weapon,*combat::weapon::attack_skill(skill,12,0),at);
  const bool unblockable=skill==400||skill==321||skill==328;
  if(!accepted){if(mode!=4||unblockable)std::cerr<<"rejected skill="<<skill<<" mode="<<mode<<" draws="<<draws<<"\n";check(mode==4&&!unblockable);continue;}
  w.tick(at+300);const auto events=w.take_attack_outcomes(f.source);check(events.size()==1);
  const auto expected=mode==2?O::miss:mode==3||mode==1||unblockable?O::hit:mode==4?O::cancelled:O::block;
  check(events[0].outcome==expected);check(draws==(mode==2||mode==3||unblockable?0u:1u));check(bool(events[0].before_damage)==(expected==O::hit));
  const auto reaction=combat::weapon::blocked_attack_effect(*combat::weapon::attack_skill(skill,12,0),expected);
  if(expected==O::block){check(reaction?f.health()<1000:f.health()==1000);if(!reaction)check(w.complete_npc_action(*token,20000));}
  if(expected==O::miss)check(f.health()==1000);
  if(expected==O::block&&reaction){check(w.knocked_down(f.target,at)==bool(reaction->knockdown_ms));if(reaction->condition)check(w.has_condition(f.target,*reaction->condition,at+300));}
 }
 // Ordinary swings and actual projectile contacts share the same NPC decision.
 for(unsigned type:{27u,5u})for(bool blocked:{false,true}){warrior_test::Fixture f(type==5?394:320,true);auto& w=f.world;f.weapon.item_type=type;check(w.ordinary_weapon(f.source,f.weapon,20));check(w.stage_npc_shield_deflection(f.target,*skills::effects::shield_deflection(12),100));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return blocked?0.:.99;});check(w.ordinary_attack(f.source,f.target.actor,500));w.tick(1000);w.cancel_ordinary_attack(f.source);w.tick(1200);
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes[0].outcome==(blocked?O::block:O::hit)&&draws==1);check(bool(outcomes[0].before_damage)==!blocked);
 }
 // A blocked attack never qualifies Insidious Parasite's pre-hit transfer.
 for(bool blocked:{false,true}){warrior_test::Fixture f(320,true);auto& w=f.world;w.join(3,"Hex caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(1,{500,1000,30,30},100);w.set_vitals(3,{100,1000,30,30},100);
  check(w.apply_insidious_parasite(*w.effect_scope(3),f.source,12,100));check(w.stage_npc_shield_deflection(f.target,*skills::effects::shield_deflection(12),100));w.configure_attack_block_draws([&]{return blocked?0.:.99;});
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(320,0),500));check(w.resources(1)->health==(blocked?500u:461u)&&w.resources(3)->health==(blocked?100u:139u));
 }
 for(bool rigor:{false,true})for(unsigned skill:{320u,330u}){warrior_test::Fixture f(skill,true);auto& w=f.world;check(w.stage_npc_shield_deflection(f.target,*skills::effects::shield_deflection(12),100));
  if(rigor)check(w.degeneration_hex(f.source,f.target,*skills::effects::degeneration_hex(137,0),100));
  else check(w.status_mutation(f.source,362,123,0,100,5100,true));
  unsigned draws=0;w.configure_attack_block_draws([&]{++draws;return 0.;});check(w.melee_attack(f.source,skill==330?f.source:f.target,f.weapon,*combat::weapon::attack_skill(skill,0),500));
  const auto outcomes=w.take_attack_outcomes(f.source);check(outcomes.size()==1&&outcomes[0].outcome==O::hit&&draws==0&&f.health()<1000);
 }
 for(unsigned mode=0;mode<4;++mode){const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto pop=world::load_npc_catalog(appearance);
  std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/npc_block_contact.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");
  const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(pop,catalog,hash);auto enemies=pop.for_map(146);auto guardian=enemies.front();guardian.combat_definition->relation=combat::npcs::Relation::neutral;
  world::Instance w;w.configure_npcs({guardian});w.join(2,"Owner",100,0,std::make_shared<world::Outbox>());w.set_vitals(2,{500,500,100,100},100);const auto owner=*w.effect_scope(2),guard=*w.effect_scope(100);
  check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);check(w.stage_npc_shield_deflection(guard,*skills::effects::shield_deflection(12),100));
  if(mode==2)check(w.blindness(owner,attacker,*skills::effects::blindness(220,0),100));unsigned draws=0;w.configure_attack_miss_draws([]{return .5;});w.configure_attack_block_draws([&]{++draws;return mode==3?1.:mode==1?.99:0.;});
  combat::weapon::Strike strike;strike.level=enemies.front().definition.level;strike.minimum=strike.maximum=10;strike.base_roll=strike.miss_roll=.5;
  w.damage_quest_guardian(attacker,guard,{},128,1000,strike,1000);const auto events=w.take_attack_outcomes(attacker);check(events.size()==1&&events[0].outcome==(mode==0?O::block:mode==1?O::hit:mode==2?O::miss:O::cancelled));
  check(bool(events[0].before_damage)==(mode==1)&&draws==(mode==2?0u:1u));check((w.npc_combat_snapshot(guard)->health->current<500)==(mode==1));
 }
 // Renewal retires the old wire effect exactly once; its old deadline must
 // not remove the replacement. Rejected scopes/plans emit no status packets.
 {warrior_test::Fixture f(320,true);auto& w=f.world;const auto plan=*skills::effects::shield_deflection(12);f.box->drain();
  auto stale=f.target;++stale.generation;check(!w.stage_npc_shield_deflection(stale,plan,100));auto forged=plan;++forged.armor;check(!w.stage_npc_shield_deflection(f.target,forged,100));check(f.box->drain().empty());
  auto added=[&](const auto& packets){const auto found=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()==20&&p[0]==65&&p[6]==3&&p[7]==1;});check(found!=packets.end());return unsigned((*found)[12])|(unsigned((*found)[13])<<8)|(unsigned((*found)[14])<<16)|(unsigned((*found)[15])<<24);};
  check(w.stage_npc_shield_deflection(f.target,plan,100));const auto first=added(f.box->drain());
  check(w.stage_npc_shield_deflection(f.target,plan,200));const auto renewed=f.box->drain();const auto second=added(renewed);check(first!=second);
  const auto old_end=skills::casting::effect_remove(f.target.actor,first),new_end=skills::casting::effect_remove(f.target.actor,second);check(std::count(renewed.begin(),renewed.end(),old_end)==1);
  w.tick(9100);const auto old_deadline=f.box->drain();check(std::count(old_deadline.begin(),old_deadline.end(),old_end)==0&&std::count(old_deadline.begin(),old_deadline.end(),new_end)==0);
  w.tick(9200);const auto expired=f.box->drain();check(std::count(expired.begin(),expired.end(),new_end)==1);w.tick(9300);const auto later=f.box->drain();check(std::count(later.begin(),later.end(),new_end)==0);
 }
 std::cout<<"PASS "<<cases<<" NPC attack-skill block cases plus ordinary/projectile contacts,pre-hit isolation,bypass,NPC quest-guardian and owned renewal/expiry checks\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
