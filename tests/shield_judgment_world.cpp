#include "warrior_condition_fixture.h"
#include "gw_shield_judgment.h"
#include <iostream>
#include <source_location>
#include <thread>
#include <barrier>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Judgment World line "+std::to_string(at.line()));}
int main()try{unsigned cases=0;
 // Each holy retaliation owns one cumulative reduction step, including a
 // converted first hit. It is not a weapon contact on the protected attacker.
 for(unsigned mode:{0u,1u,2u}){
  const bool reversal=mode!=0;
  warrior_test::Fixture f(320,false);auto& w=f.world;
  check(w.player_holy_equipment(f.source,0));
  w.join(3,"Caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
  w.set_vitals(3,{1000,1000,100,100},100);const auto caster=*w.effect_scope(3);
  check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));
  check(w.ordinary_weapon(caster,combat::weapon::Owned{3,1,{20,20,2},12,false,false,27},20));
  check(w.complete_shield_judgment(caster,f.target,*skills::effects::shield_judgment(12),0,100));
  check(w.status_mutation(f.source,1399,80,12,100,6100,true));
  if(mode==2)w.set_vitals(1,{10,1000,100,100},100);
  if(reversal)check(w.status_mutation(f.source,307,81,mode==2?0:12,100,8100,true));
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(320,0),500));
  const unsigned first=mode==2?4:reversal?1000:964;
  check(w.resources(1)->health==first);
  if(mode==2){check(w.shield_judgment_failures()==0);continue;}
  check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(320,0),3000));
  check(w.resources(1)->health==first-31);
  const auto before=w.resources(1)->health;const auto followup=*skills::effects::wound_spell(1052,12);
  check(w.wound_spell(f.target,f.source,followup,20,3001));
  check(w.resources(1)->health==before-followup.damage+15);
  check(w.shield_judgment_failures()==0);
 }
 for(unsigned skill:{320u,394u,330u,400u})for(unsigned mode=0;mode<8;++mode){++cases;warrior_test::Fixture f(skill,false);auto& w=f.world;
  w.join(3,"Caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);const auto caster=*w.effect_scope(3);
  check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));
 check(w.ordinary_weapon(caster,combat::weapon::Owned{3,1,{20,20,2},12,false,false,27},20));
 check(w.player_holy_equipment(f.source,mode==4?std::optional<unsigned>{}:std::optional<unsigned>{0}));
  if(mode==5)w.set_vitals(2,{10,1000,100,100},100);if(mode==6)w.set_vitals(1,{10,1000,100,100},100);
  check(w.ordinary_weapon(caster,combat::weapon::Owned{3,1,{20,20,2},12,false,false,27},20));
 check(w.complete_shield_judgment(caster,f.target,*skills::effects::shield_judgment(12),0,100));
  if(mode==1)check(w.status_mutation(f.target,259,99,12,100,9100,true));
  if(mode==2)check(w.apply_condition(f.source,f.source,combat::conditions::Kind::blind,100,20000));
  if(mode==7)check(w.status_mutation(f.source,361,77,0,100,8100,true));
  w.configure_attack_block_draws([]{return 0.;});const unsigned at=mode==3?18200:500;
  const auto before_control=w.control_epoch(f.source);
  const auto accepted=w.melee_attack(f.source,skill==330?f.source:f.target,f.weapon,*combat::weapon::attack_skill(skill,12),at);
  const bool blocked=mode==1&&skill!=400;const bool reacts=mode!=2&&mode!=3&&mode!=4&&!blocked;
  if(accepted!=(mode!=4||skill==394||skill==400))std::cerr<<"skill="<<skill<<" mode="<<mode<<" accepted="<<accepted<<"\n";
 check(accepted==(mode!=4||skill==394||skill==400));
 w.tick(at+400);const auto outcomes=w.take_attack_outcomes(f.source);
 check(outcomes.size()==1);
 if(skill==330&&mode==4){
  // Atomic sharedhit-contract preflight rejects UNKNOWN sourceHoly facts
  // beforemake_action; launchedBow394/400 intentionallyremain distinct.
  check(!accepted&&outcomes[0].outcome==combat::weapon::AttackOutcome::cancelled&&!outcomes[0].before_damage);
  check(outcomes[0].attacker==f.source&&outcomes[0].target==f.target);
  check(w.resources(1)->health==1000&&w.resources(2)->health==1000&&!w.knocked_down(f.source,at+400));
  check(before_control&&w.control_epoch(f.source)==before_control);
 }
  check(w.resources(1)->health==(mode==6?0u:reacts?959u:1000u));
 check(w.shield_judgment_failures()==0&&w.shield_judgment_pending_contacts()==0);
  if(reacts&&mode!=6)check(w.knocked_down(f.source,at+400)==(mode!=7));
  if(mode==0){w.tick(3000);
 check(w.melee_attack(f.source,skill==330?f.source:f.target,f.weapon,*combat::weapon::attack_skill(skill,12),3000));
 w.tick(3400);
 check(w.resources(1)->health==918&&w.shield_judgment_failures()==0);}
 }
 for(unsigned type:{27u,5u}){warrior_test::Fixture f(type==5?394:320,false);auto& w=f.world;f.weapon.item_type=type;
 check(w.ordinary_weapon(f.source,f.weapon,20));
 check(w.player_holy_equipment(f.source,0));
  w.join(3,"Caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);const auto caster=*w.effect_scope(3);
 check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));
  check(w.ordinary_weapon(caster,combat::weapon::Owned{3,1,{20,20,2},12,false,false,27},20));
 check(w.complete_shield_judgment(caster,f.target,*skills::effects::shield_judgment(12),0,100));
 check(w.ordinary_attack(f.source,2,500));
 w.tick(1000);w.cancel_ordinary_attack(f.source);
 w.tick(1200);
 if(w.resources(1)->health!=959)std::cerr<<"ordinary type="<<type<<" hp="<<w.resources(1)->health<<" failures="<<w.shield_judgment_failures()<<"\n";check(w.resources(1)->health==959&&w.knocked_down(f.source,1200)&&w.shield_judgment_failures()==0);
 }
 // Retaliation is holy skill damage,not a synthetic weapon attack. Putting
 // Judgment on both combatants must not recurse or create extra hit events.
 {warrior_test::Fixture f(320,false);auto& w=f.world;check(w.player_holy_equipment(f.source,0));
  w.join(3,"Caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);const auto caster=*w.effect_scope(3);
  check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));check(w.ordinary_weapon(caster,combat::weapon::Owned{3,1,{20,20,2},12,false,false,27},20));
  check(w.complete_shield_judgment(caster,f.target,*skills::effects::shield_judgment(12),0,100));check(w.complete_shield_judgment(f.source,f.source,*skills::effects::shield_judgment(12),0,100));
  check(w.ordinary_attack(f.source,2,500));w.tick(1000);w.cancel_ordinary_attack(f.source);w.tick(1200);
  check(w.resources(1)->health==959&&w.resources(2)->health==980);check(w.take_attack_outcomes(f.source).size()==1&&w.take_attack_outcomes(f.target).empty()&&w.take_attack_outcomes(caster).empty());
  check(w.shield_judgment_failures()==0&&w.shield_judgment_pending_contacts()==0);
 }
 // An unrelated producer cannot split retaliation control and attributed
 // damage publication. Exercise the actual World contact,not a mocked queue.
 for(unsigned run=0;run<16;++run){warrior_test::Fixture f(320,false);auto& w=f.world;f.weapon.item_type=27;check(w.ordinary_weapon(f.source,f.weapon,20));check(w.player_holy_equipment(f.source,0));
  w.join(3,"Caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(3,{1000,1000,100,100},100);const auto caster=*w.effect_scope(3);
  check(w.party_action(3,158,2,100)&&w.party_action(2,154,3,100));check(w.ordinary_weapon(caster,combat::weapon::Owned{3,1,{20,20,2},12,false,false,27},20));
  check(w.complete_shield_judgment(caster,f.target,*skills::effects::shield_judgment(12),0,100));check(w.ordinary_attack(f.source,2,500));f.box->drain();
  const world::Packet marker{250,0,27,0};std::barrier start(2);std::thread producer([&]{start.arrive_and_wait();for(unsigned i=0;i<100;++i){f.box->enqueue({marker});std::this_thread::yield();}});
  start.arrive_and_wait();w.tick(1000);producer.join();const auto packets=f.box->drain();
  const auto knock=std::find(packets.begin(),packets.end(),combat::knockdown_2022(1,2.f));const auto skill=std::find(packets.begin(),packets.end(),combat::detail::integer(10,3,262));const auto health=std::find(packets.begin(),packets.end(),combat::detail::fraction(1,{959,1000}));
  check(knock!=packets.end()&&skill!=packets.end()&&health!=packets.end()&&knock<skill&&skill<health);check(std::find(knock,health,marker)==health&&std::count(packets.begin(),packets.end(),marker)==100);
 }
 std::cout<<"PASS32 Judgment World attack-skill cases plus2 ordinary/projectile contacts and reciprocal-reaction isolation,16 concurrent publication cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

