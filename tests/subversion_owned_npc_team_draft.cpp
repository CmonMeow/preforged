#include "gw_world_simulation.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills::effects;
static void check(bool value,std::source_location at=std::source_location::current()){
 if(!value)throw std::runtime_error("shared NPC/player alliance line "+std::to_string(at.line()));
}
static world::Instance::Npc body(unsigned id,combat::npcs::Relation relation,unsigned map=146,float x=90){
 world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=20;n.x=x;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding={map,id,id,0,0,0,0,20};d.relation=relation;d.spawn_health={500,1000};
 d.damageable=true;d.physical_armor=d.general_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=0;
 d.healing_allowed=true;d.healing_evidence="authored owner-domain probe";n.combat_definition=d;return n;
}
static void relation(world::Instance& w,Scope player,Scope npc,SpellTargetRelation expected){
 const auto a=w.subversion_spell_target(player,npc),b=w.subversion_spell_target(npc,player);
 check(a&&b&&a->relation==expected&&b->relation==expected);
}
static void quest_domains(){
 world::Instance w;w.configure_npcs({body(101,combat::npcs::Relation::allied_to_players),body(102,combat::npcs::Relation::allied_to_players),
  body(103,combat::npcs::Relation::neutral),body(104,combat::npcs::Relation::allied_to_players)});
 for(unsigned id:{1u,2u,3u}){w.join(id,"Team actor",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
 const auto owner=*w.effect_scope(1),enemy=*w.effect_scope(2),ally=*w.effect_scope(3),a=*w.effect_scope(101),b=*w.effect_scope(102),global=*w.effect_scope(104);
 for(const auto& [caster,npc]:{std::pair{owner,a},std::pair{enemy,b}}){
  check(w.spawn_kill_encounter(caster,75,1,{body(200,combat::npcs::Relation::hostile_to_players,146,100)},[]{return true;}));
  const std::array recipients{npc};check(w.bind_npc_opposed_allies({caster,75,1},recipients));
 }
 relation(w,owner,a,SpellTargetRelation::ally);relation(w,enemy,a,SpellTargetRelation::foe);
 relation(w,enemy,b,SpellTargetRelation::ally);relation(w,owner,b,SpellTargetRelation::foe);
 relation(w,owner,global,SpellTargetRelation::ally);relation(w,enemy,global,SpellTargetRelation::ally);
 relation(w,owner,*w.effect_scope(103),SpellTargetRelation::unknown);
 const auto quest_foe=*w.effect_scope(60000);relation(w,owner,quest_foe,SpellTargetRelation::foe);relation(w,enemy,quest_foe,SpellTargetRelation::foe);
 auto stale=a;++stale.generation;check(!w.subversion_spell_target(owner,stale)&&!w.subversion_spell_target(stale,owner));
 check(w.kill(owner,1100));check(!w.subversion_spell_target(owner,a));
 // A dead owner's current Scope still proves its retained companions' team.
 relation(w,ally,a,SpellTargetRelation::ally);relation(w,enemy,a,SpellTargetRelation::foe);
 w.leave(1);check(!w.subversion_spell_target(owner,a)&&!w.subversion_spell_target(enemy,quest_foe));
 // Removing the old domain restores the established global-friendly baseline.
 relation(w,enemy,a,SpellTargetRelation::ally);
 w.join(1,"Reused owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},1200,2);w.set_vitals(1,{1000,1000,100,100},100);const auto next=*w.effect_scope(1);check(next!=owner);
 check(w.spawn_kill_encounter(next,75,1,{body(201,combat::npcs::Relation::hostile_to_players,146,1500)},[]{return true;}));
 const std::array rebound{a};check(w.bind_npc_opposed_allies({next,75,1},rebound));
 relation(w,enemy,a,SpellTargetRelation::ally);relation(w,ally,a,SpellTargetRelation::foe);
 // A real same-domain NPC attack kills b; a corpse never becomes a global ally.
 const auto killer=*w.effect_scope(60001);
 const combat::ai::ordinary::Reviewed weapon{1000,1000,0,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"authored lethal NPC contact"};
 check(w.configure_npc_opposed_ordinary(killer,weapon,1300,world::Instance::FriendlyOrdinaryQuest{enemy,75,1}));
 check(bool(w.begin_npc_opposed_ordinary(killer,b,1300)));w.tick(2050);
 check(w.npc_combat_snapshot(b)->health->current==0&&!w.subversion_spell_target(enemy,b)&&!w.subversion_spell_target(b,enemy));
}
static void pet_domains(){
 world::Instance w;w.configure_npcs({body(100,combat::npcs::Relation::hostile_to_players,162,1500)});
 for(unsigned id:{1u,2u,3u}){w.join(id,"Pet team",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
 const auto owner=*w.effect_scope(1),enemy=*w.effect_scope(2),ally=*w.effect_scope(3);
 const auto create=[&](Scope source,std::uint64_t at){check(w.restore_pet_profile(source,1,pets::Profile{pets::Pet{}},1));check(w.pet_owner_combat(source,1,0,at));check(w.reconcile_pet_bar_mode(source,162,true,false,std::array{436u},at));return *w.owned_pet_scope(source);};
 const auto pet=create(owner,1000);relation(w,owner,pet,SpellTargetRelation::ally);relation(w,enemy,pet,SpellTargetRelation::foe);
 check(w.kill(owner,1100));check(w.owned_pet_scope(owner)==pet);relation(w,ally,pet,SpellTargetRelation::ally);relation(w,enemy,pet,SpellTargetRelation::foe);
 w.leave(1);check(!w.subversion_spell_target(enemy,pet));
 w.join(1,"Reused pet owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},1300,2);w.set_vitals(1,{1000,1000,100,100},100);const auto next=*w.effect_scope(1);check(next!=owner);
 const auto fresh=create(next,1300);check(fresh!=pet);relation(w,enemy,fresh,SpellTargetRelation::ally);relation(w,ally,fresh,SpellTargetRelation::foe);
 check(!w.subversion_spell_target(enemy,pet));
}
int main()try{quest_domains();pet_domains();std::cout<<"PASS shared Subversion mixed NPC/player current owner teams, dead/reused quest/pet scopes, global/neutral baseline\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
