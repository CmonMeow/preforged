// NEW unregistered diagnostic: Horns' target-allies predicate must use existing
// explicit current NPC owner teams, not globally equate all players' teams.
#include "dagger_successful_dual_fixture_draft.h"
static world::Instance::Npc remote_quest_foe(){
 world::Instance::Npc n;n.id=n.definition.id=200;n.x=1500;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding={146,200,200,0,0,0,0,20};d.relation=combat::npcs::Relation::hostile_to_players;
 d.spawn_health={1000,1000};d.damageable=true;n.combat_definition=d;return n;
}
int main()try{
 // mode0: owned by attacker team1; mode1: target team2; mode2: established
 // unowned globally-friendly NPC semantics. All bodies are adjacent to target.
 for(unsigned mode=0;mode<3;++mode){
  Fixture f(777,false,true);const auto companion=*f.world.effect_scope(101);
  if(mode!=2){const auto owner=mode==0?f.source:f.target;
   check(f.world.spawn_kill_encounter(owner,75,1,{remote_quest_foe()},[]{return true;}));
   const std::array companions{companion};check(f.world.bind_npc_opposed_allies({owner,75,1},companions));
  }
  f.arm();check(f.start()==skills::casting::Reason::accepted);f.update(f.last_permit->release_time());
  check(f.last_result.accepted&&f.last_result.successful_second);
  const auto phase=f.last_result.successful_second;f.update(phase->due());
  check(f.world.knocked_down(f.target,f.clock)==(mode==0));
  check(f.payments==1&&f.completions==1&&f.copied_observations==1);
 }
 std::cout<<"PASS actual paid Horns current owned NPC target-alliance and unowned global-friendly baseline\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
