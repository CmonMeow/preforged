#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Spiteful damage World line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<9;++mode){
  world::Instance w;std::vector<world::Instance::Npc> population;
  for(unsigned id:{100u,101u}){
   world::Instance::Npc n;n.id=n.definition.id=id;n.x=id==100?0.f:mode==7?166.01f:100.f;n.plane=id==101&&mode==8?1:0;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   n.definition.flags=id==101&&mode==5?0x200:id==101&&mode==6?0x210:0;
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;d.binding.definition_flags=n.definition.flags;
   d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=d.conditions_allowed=true;
   if(!(id==101&&(mode==2||mode==7||mode==8))){d.packed_allegiance=mode==3?0u:mode==4?0x6e6f6e63u:id==101&&mode==1?3u:2u;d.evidence.allegiance="explicit synthetic raw-token fixture";}
   n.combat_definition=d;population.push_back(n);
  }
  w.configure_npcs(population);w.join(1,"Hexer",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto source=*w.effect_scope(1),owner=*w.effect_scope(100),neighbor=*w.effect_scope(101);
  check(w.spiteful_spirit(source,owner,*spiteful_spirit(12),20,100));std::string why;
  if(mode==0){
   w.join(2,"Bonded",700,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);
   const auto bonded=*w.effect_scope(2);check(w.elemental_armor(bonded,60,60));
   check(w.stage_npc_bonds_of_torment(owner,bonded,*bonds_of_torment_npc(),150));
  }
  const bool blocked=mode==2||mode==4||mode==5;
  check(w.completed_skill_action(owner,1239,200,&why)==!blocked);
  check(why==(blocked?"unknown_relationship":"applied"));
  check(w.npc_combat_snapshot(owner)->health->current==(blocked?500.:471.));
  check(w.npc_combat_snapshot(neighbor)->health->current==((mode==0||mode==3||mode==6)?471.:500.));
  if(mode==0)check(w.resources(2)->health==971);
 }
 for(unsigned skill:{322u,330u,396u,395u,2144u})for(bool invalid:{false,true}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Skill release",id==1?1000.f:id==2?0.f:id==3?60.f:80.f,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{1000,1000,100,100},100);check(w.physical_armor(*w.effect_scope(id),60));}
  const auto hexer=*w.effect_scope(1),owner=*w.effect_scope(2);auto target=*w.effect_scope(3);
  const auto plan=*combat::weapon::attack_skill(skill,12);
  combat::weapon::Owned weapon{3,1,{20,20,2},12,false,false,27};
  if(skill==330)weapon.item_type=2;
  if(skill==330)target=owner;
  if(plan.required_type==5)weapon={3,1,{20,20,1},12,false,false,5};
  check(w.ordinary_weapon(owner,weapon,20));check(w.spiteful_spirit(hexer,owner,*spiteful_spirit(12),20,100));box->drain();
  if(invalid)++target.generation;
  const bool released=w.melee_attack(owner,target,weapon,plan,200);
  if(released!=!invalid)std::cerr<<"skill="<<skill<<" invalid="<<invalid<<" released="<<released<<'\n';
  check(released==!invalid);
  check(w.resources(2)->health==(invalid?1000u:971u));
  auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,2,121))==unsigned(!invalid));
  if(!invalid){check(w.completed_skill_action(owner,skill,200));check(w.resources(2)->health==971);}
  const auto immediate_outcomes=w.take_attack_outcomes(owner).size();
  w.tick(1000);check(w.resources(2)->health==(invalid?1000u:971u));
  check(immediate_outcomes+w.take_attack_outcomes(owner).size()==(invalid?0u:skill==322?1u:2u));
  packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,2,121))==0);
 }
 for(unsigned weapon_kind=0;weapon_kind<3;++weapon_kind)for(unsigned release_mode=0;release_mode<3;++release_mode){
  const bool blind=release_mode==1;
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  for(unsigned id:{1u,2u,3u}){w.join(id,"Ordinary release",id==1?1000.f:id==2?0.f:60.f,0,id==2?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==2?2:1);w.set_vitals(id,{1000,1000,100,100},100);}
  const auto hexer=*w.effect_scope(1),owner=*w.effect_scope(2),victim=*w.effect_scope(3);
  check(w.physical_armor(victim,60)&&w.combat_armor(victim,60));check(w.spiteful_spirit(hexer,owner,*spiteful_spirit(12),20,100));
  if(blind)check(w.apply_condition(hexer,owner,combat::conditions::Kind::blind,100,10000));
  combat::weapon::Owned weapon{3,1,{20,20,2},12,false,false,27};
  if(weapon_kind==1)weapon={3,1,{20,20,11},0,false,true};
  if(weapon_kind==2)weapon={3,1,{20,20,1},12,false,false,5};
  check(w.ordinary_weapon(owner,weapon,20)&&w.ordinary_attack(owner,3,100));box->drain();
  w.tick(499);check(w.resources(2)->health==1000);
  if(release_mode==2){w.cancel_ordinary_attack(owner);w.tick(600);w.tick(1200);check(w.resources(2)->health==1000);continue;}
  w.tick(600);if(w.resources(2)->health!=971)std::cerr<<"ordinary kind="<<weapon_kind<<" blind="<<blind<<" hp="<<w.resources(2)->health<<'\n';check(w.resources(2)->health==971);
  auto packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,2,121))==1);
  w.tick(600);w.tick(1200);w.cancel_ordinary_attack(owner);check(w.resources(2)->health==971);
  packets=box->drain();check(std::count(packets.begin(),packets.end(),combat::detail::integer(10,2,121))==0);
 }
 {
  world::Instance w;w.join(1,"Completion clock",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.set_vitals(1,{1000,1000,100,100},100);std::string why;
  check(w.consume_energy(1,1,*w.effect_scope(1),200));
  check(w.completed_skill_action(*w.effect_scope(1),1239,100,&why)&&why=="no_reaction");
 }
 for(unsigned mode=0;mode<9;++mode){
  world::Instance w;auto box=std::make_shared<world::Outbox>();
  if(mode==3||mode==8){world::Instance::Npc n;n.id=n.definition.id=100;n.x=100;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
   d.relation=mode==3?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{1000,1000};d.damageable=d.conditions_allowed=true;
   n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id=1;id<=4;++id){
   const float x=id==1?1000.f:id==2?0.f:id==3?(mode==1?166.01f:166.f):100.f;
   w.join(id,"Actor",x,0,box,0,{}, {},100,id==1?1:id==4?3:2);w.set_vitals(id,{mode==2&&id==2?10u:1000u,1000,100,100},100);
  }
  const auto caster=*w.effect_scope(1),owner=*w.effect_scope(2);const auto plan=*spiteful_spirit(mode==7?0:12);
  check(w.spiteful_spirit(caster,owner,plan,20,100));const auto effect=w.status_snapshot(owner,100)->front().instance;
  if(mode==5){w.leave(1);w.join(1,"Replacement",1000,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);}
  if(mode==6)check(w.status_mutation(owner,346,900,0,100,8100,true));
  if(mode==7)check(w.apply_health_regeneration(owner,2,*skills::regeneration::reviewed(299,20),100,8100,1248,true,owner));
  box->drain();std::string why;const auto now=mode==4?18100u:200u;
  SpitefulAction action{owner,SpitefulPhase::skill_completed,1239,1,now,true};
  const auto applied=mode==0?w.completed_skill_action(owner,1239,now,&why):w.spiteful_action(action,&why);
  if(applied!=(mode!=3))std::cerr<<"mode="<<mode<<" result="<<applied<<" reason="<<why<<'\n';
  check(applied==(mode!=3));
  if(mode==3){
   check(why=="unknown_relationship");
   check(w.resources(2)->health==1000&&w.resources(3)->health==1000&&w.resources(4)->health==1000);
   if(mode==3){check(w.status_mutation(owner,121,effect,12,now,std::nullopt,false));
    check(w.spiteful_action(action,&why)&&why=="no_reaction"); // Failed area preflight did not consume the event.
   }
   continue;
  }
  if(mode==4){check(why=="no_reaction"&&w.resources(2)->health==1000&&w.resources(3)->health==1000);continue;}
  check(why=="applied");
  check(w.resources(2)->health==(mode==2?0u:mode==6?942u:mode==7?1000u:971u));
  check(w.resources(3)->health==(mode==1?1000u:mode==7?995u:971u)&&w.resources(4)->health==1000);
  if(mode==8)check(w.npc_combat_snapshot(*w.effect_scope(100))->health->current==1000);
  const auto notices=box->drain();
  if(mode!=7&&mode!=5)check(std::find(notices.begin(),notices.end(),combat::detail::integer(10,2,121))!=notices.end());
  if(mode==5){
   check(std::find(notices.begin(),notices.end(),combat::detail::integer(10,2,121))==notices.end());
   check(w.resources(1)->health==1000); // Replacement receives neither damage nor caster credit.
   check(!notices.empty());
  }
  if(mode==0){
   check(!w.spiteful_action(action,&why)&&why=="event_rejected");
   check(w.spiteful_action({owner,SpitefulPhase::attack_release,322,1,201,true}));
   check(w.resources(2)->health==942&&w.resources(3)->health==942);
   check(w.spiteful_action({owner,SpitefulPhase::skill_completed,322,2,202,true}));
   check(w.spiteful_action({owner,SpitefulPhase::skill_completed,1239,3,203,false}));
   check(w.resources(2)->health==942&&w.resources(3)->health==942);
   auto stale=owner;++stale.generation;check(!w.spiteful_action({stale,SpitefulPhase::skill_completed,1239,4,204,true},&why)&&why=="stale_actor");
  }
 }
 for(bool lethal:{false,true})for(bool departed:{false,true}){
  world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=0;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
  d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{lethal?10.:500.,500};d.damageable=d.conditions_allowed=true;n.combat_definition=d;w.configure_npcs({n});
  w.join(1,"Caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
  const auto caster=*w.effect_scope(1),owner=*w.effect_scope(100);check(w.spiteful_spirit(caster,owner,*spiteful_spirit(12),20,100));
  w.join(2,"Quest witness",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.set_vitals(2,{1000,1000,100,100},100);const auto witness=*w.effect_scope(2);
  const std::array targets{owner};check(w.begin_kill_objective(witness,85,1,targets));
  if(lethal){
   w.join(3,"Bonded",500,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
   w.set_vitals(3,{1000,1000,100,100},100);
   const auto bonded=*w.effect_scope(3);check(w.elemental_armor(bonded,60,60));
   check(w.stage_npc_bonds_of_torment(owner,bonded,*bonds_of_torment_npc(),150));
  }
  if(departed)w.leave(1);
  check(w.spiteful_action({owner,SpitefulPhase::skill_completed,1239,1,200,true}));
  check(w.npc_combat_snapshot(owner)->health->current==(lethal?0.:471.));check(w.take_npc_deaths(caster).size()==unsigned(lethal&&!departed));
  if(lethal)check(w.resources(3)->health==990); // Propagate actual ten Health before NPC retirement.
  check(w.pending_quest_completions(witness).size()==unsigned(lethal));
 }
 std::cout<<"PASS Spiteful World shadow-equivalent area preflight,allied boundary/other-team exclusion,event replay/partition,unknown-fact rollback,protection/death and NPC credit;producer/Runtime wiring pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
