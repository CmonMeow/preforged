// Actual Seed gain must respect explicit current NPC owner domains,
// while retaining the established unowned globally-friendly NPC behavior.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <tuple>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Seed owned team line "+std::to_string(at.line()));
}
static world::Instance::Npc body(unsigned id,bool hostile=false,float x=100){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;
 d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
 d.spawn_health={500,1000};d.damageable=true;d.healing_allowed=true;d.healing_evidence="authored explicit owner fixture";
 n.combat_definition=d;return n;
}
int main()try{
 auto w=std::make_unique<world::Instance>();auto box=std::make_shared<world::Outbox>();
 w->configure_npcs({body(102),body(103),body(104),body(105)});
 for(unsigned id:{1u,2u,3u,4u}){w->join(id,"Actor",id==1?0.f:100.f,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1?2:1);w->set_vitals(id,{500,1000,100,100},100);}
 const auto enemy=*w->effect_scope(1),target=*w->effect_scope(2),caster=*w->effect_scope(3),retired_owner=*w->effect_scope(4);
 for(const auto [owner,npc,foe]:{std::tuple{caster,102u,200u},std::tuple{enemy,103u,201u},std::tuple{retired_owner,105u,202u}}){
  check(w->spawn_kill_encounter(owner,75,1,{body(foe,true,1500)},[]{return true;}));
  const std::array allies{*w->effect_scope(npc)};check(w->bind_npc_opposed_allies({owner,75,1},allies));
 }
 // Leaving correctly retires the entire old opposition domain. Rebind the
 // reused actor on its new opposing team; do not pretend the now-unowned
 // globally-friendly NPC still has the deleted old domain.
 w->leave(4);w->join(4,"Reused owner",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w->set_vitals(4,{500,1000,100,100},100);
 const auto next_owner=*w->effect_scope(4);check(next_owner!=retired_owner);
 check(w->spawn_kill_encounter(next_owner,75,1,{body(203,true,1500)},[]{return true;}));
 const std::array rebound{*w->effect_scope(105)};check(w->bind_npc_opposed_allies({next_owner,75,1},rebound));
 const auto seed=*healing_seed(12);check(w->complete_healing_seed(caster,target,seed,0,100));check(w->player_holy_equipment(target,0));
 const auto plan=*holy_damage(252,0);box->drain();check(w->holy_damage(enemy,target,plan,20,200));
 check(w->resources(2)->health==500-plan.damage+seed.healing);
 check(w->npc_combat_snapshot(*w->effect_scope(102))->health->current==500+seed.healing);
 check(w->npc_combat_snapshot(*w->effect_scope(103))->health->current==500);
 check(w->npc_combat_snapshot(*w->effect_scope(104))->health->current==500+seed.healing);
 check(w->npc_combat_snapshot(*w->effect_scope(105))->health->current==500);
 std::cout<<"PASS Seed explicit ally/enemy/reused owner domains and unowned global friendly gain\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
