#include "gw_npc_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Judgment NPC line "+std::to_string(at.line()));}
int main()try{for(bool npc_target:{false,true})for(unsigned mode=0;mode<10;++mode){
 const std::string hash(64,'a');std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=world::load_npc_catalog(appearance);
 std::istringstream metadata("gw-npc-combat 3 "+hash+"\nsource test authored "+hash+" tests/shield_judgment_npc.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test 60 60 test test 0 test\n");const auto catalog=combat::npcs::load_catalog(metadata,hash);world::bind_combat_catalog(population,catalog,hash);
 auto enemies=population.for_map(146);auto& d=*enemies.front().combat_definition;d.holy_traits.light_sensitive=mode==6?std::optional<bool>{}:std::optional(mode==4);d.light_sensitive_evidence="authored";d.knockdown_allowed=mode==5?std::optional<bool>{}:std::optional(mode!=3);d.knockdown_evidence="authored";
 if(mode==9)d.spawn_health=combat::Health{10,500};
 if(mode>=8){d.healing_allowed=true;d.healing_evidence="authored Judgment conversion fixture";}
 auto guardian=enemies.front();guardian.combat_definition->relation=combat::npcs::Relation::neutral;
 world::Instance w;w.configure_npcs({guardian});w.join(2,"Owner",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(2,{500,500,100,100},100);const auto owner=*w.effect_scope(2),guard=*w.effect_scope(100),target=npc_target?guard:owner;
 check(w.physical_armor(owner,60));
 check(w.accept_quest_waves(owner,guard,73,1,{enemies},[]{return true;}));const auto attacker=*w.effect_scope(60000);
 check(w.ordinary_weapon(owner,combat::weapon::Owned{3,1,{20,20,2},12,false,false,27},20));
 if(npc_target&&mode==0){auto stale=owner;++stale.generation;check(!w.stage_guardian_shield_judgment(stale,target,*skills::effects::shield_judgment(12),100));check(!w.stage_guardian_shield_judgment(owner,attacker,*skills::effects::shield_judgment(12),100));auto forged=*skills::effects::shield_judgment(12);++forged.damage;check(!w.stage_guardian_shield_judgment(owner,target,forged,100));}
 check(npc_target?w.stage_guardian_shield_judgment(owner,target,*skills::effects::shield_judgment(12),100):w.complete_shield_judgment(owner,target,*skills::effects::shield_judgment(12),0,100));
 if(mode>=7)check(w.stage_npc_shield_absorption(attacker,12,100));
 if(mode>=8)check(w.stage_npc_reversal(attacker,mode==9?0:12,100));
 if(mode==1){if(npc_target)check(w.stage_npc_shield_deflection(target,*skills::effects::shield_deflection(12),100));else check(w.status_mutation(target,259,77,12,100,9100,true));}
 if(mode==2)check(w.blindness(owner,attacker,*skills::effects::blindness(220,0),100));w.configure_attack_block_draws([]{return 0.;});w.configure_attack_miss_draws([]{return .5;});
 combat::weapon::Strike strike;strike.level=enemies.front().definition.level;strike.minimum=strike.maximum=10;strike.location_armor=60;strike.base_roll=strike.miss_roll=.5;
 if(npc_target)w.damage_quest_guardian(attacker,target,{},128,1000,strike,1000);else w.damage_quest_player(attacker,target,128,1000,strike,1000);
 const auto events=w.take_attack_outcomes(attacker);
 check(events.size()==1);const bool reacts=mode==0||mode==3||mode==4||mode>=7;
 check(events[0].outcome==(mode==1?combat::weapon::AttackOutcome::block:mode==2?combat::weapon::AttackOutcome::miss:(mode==5||mode==6)?combat::weapon::AttackOutcome::cancelled:combat::weapon::AttackOutcome::hit));
 check(w.npc_combat_snapshot(attacker)->health->current==(reacts?(mode==9?4:mode==8?500:mode==7?464:mode==4?418:459):500));
 if(mode>=7&&mode<9){const double first=mode==8?500:464;
  if(npc_target)w.damage_quest_guardian(attacker,target,{},128,3500,strike,1000);else w.damage_quest_player(attacker,target,128,3500,strike,1000);
  if(w.npc_combat_snapshot(attacker)->health->current!=first-31)std::cerr<<"npc_target="<<npc_target<<" mode="<<mode<<" second_health="<<w.npc_combat_snapshot(attacker)->health->current<<" failures="<<w.shield_judgment_failures()<<" outcomes="<<w.take_attack_outcomes(attacker).size()<<"\n";
  check(w.npc_combat_snapshot(attacker)->health->current==first-31);
  check(w.damage_npc(owner,attacker,{combat::Kind::armor_ignoring,50,20,42},1248,3501).reason==combat::Reason::accepted);
  check(w.npc_combat_snapshot(attacker)->health->current==first-66);
 }
 if(mode<7)check(w.knocked_down(attacker,1000)==(reacts&&mode!=3));
 check(w.shield_judgment_failures()==0&&w.shield_judgment_pending_contacts()==0);
 }
 std::cout<<"PASS20 NPC-to-player/NPC Judgment hit,block,miss,immunity,holy sensitivity and unknown-evidence cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}



