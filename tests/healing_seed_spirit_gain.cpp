// Shared common consumer regression: Healing Seed explicitly grants Health
// to spirits (local wiki Healing_Seed Notes), independently of healing immunity.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Healing Seed spirit gain line "+std::to_string(at.line()));
}
static world::Instance::Npc body(unsigned id,bool hostile,float x,bool spirit=false){
 world::Instance::Npc n;n.id=n.definition.id=id;n.x=x;n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
 d.relation=hostile?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
 d.spawn_health={500,1000};d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=0;
 d.damageable=true;d.physical_armor=60;d.conditions_allowed=!spirit;
 d.creature_type=spirit?"spirit":"human";d.creature_type_evidence="authored Seed spirit gain fixture";
 d.fleshy=!spirit;d.fleshy_evidence="authored explicit body class";
 d.healing_allowed=!spirit;d.healing_evidence="spirit cannot receive healing; Seed grants Health";
 n.combat_definition=d;return n;
}
int main()try{
 // The ordinary healing authority stays denied before and after real gain.
 const auto spirit_definition=*body(102,false,120,true).combat_definition;
 combat::npcs::LiveActor primitive(spirit_definition,{1,1,1,146,102});const auto life=primitive.lifetime();
 check(!primitive.heal_ally(life,25,100));check(primitive.actor(life)->health->current==500);
 check(primitive.gain_ally_health(life,25,100));check(primitive.actor(life)->health->current==525);
 check(!primitive.heal_ally(life,25,100));check(primitive.actor(life)->health->current==525);
 // Explicit spirit grants require the existing reviewed body/immunity facts.
 for(unsigned missing:{0u,1u}){
  auto incomplete=spirit_definition;if(!missing)incomplete.fleshy.reset();else incomplete.healing_allowed.reset();
  combat::npcs::LiveActor unknown(incomplete,{1,1,1,146,102});
  check(!unknown.gain_ally_health(unknown.lifetime(),25,100));
 }
 for(unsigned initial:{10u,500u}){
  auto d=*body(110,false,100).combat_definition;d.spawn_health={double(initial),1000};
  combat::npcs::LiveActor gained(d,{1,1,1,146,110});const auto own=gained.lifetime();
  check(gained.apply_condition(own,combat::conditions::Kind::deep_wound,2,100,10000));
  check(gained.gain_ally_health(own,25,100));
  check(gained.actor(own)->health->current==(initial==10?0.:425.));
  if(initial==10)check(gained.actor(own)->effects&0x10);
 }
 for(bool wounded_caster:{false,true}){
 auto w=std::make_unique<world::Instance>();auto box=std::make_shared<world::Outbox>();
 auto wounded_npc=body(104,false,130);wounded_npc.combat_definition->spawn_health={10,1000};
 w->configure_npcs({body(100,false,100),body(101,true,110),body(102,false,120,true),body(103,false,500,true),wounded_npc});
 w->join(1,"Seed caster",100,0,box,0,{}, {},100,1);w->set_vitals(1,{500,1000,100,100},100);
 w->join(2,"Defile caster",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w->set_vitals(2,{500,1000,100,100},100);
 const auto hexer=*w->effect_scope(2);check(w->degeneration_hex(hexer,*w->effect_scope(1),*degeneration_hex(129,12),100));
 if(wounded_caster)check(w->apply_condition(hexer,*w->effect_scope(1),combat::conditions::Kind::deep_wound,100,10000));
 w->join(3,"Wounded ally",130,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w->set_vitals(3,{10,1000,100,100},100);
 const auto wounded_player=*w->effect_scope(3),wounded_scope=*w->effect_scope(104);
 check(w->apply_condition(hexer,wounded_player,combat::conditions::Kind::deep_wound,100,10000));
 check(w->apply_condition(hexer,wounded_scope,combat::conditions::Kind::deep_wound,100,10000));
 const auto before_epoch=*w->control_epoch(wounded_player);
 check(w->stage_retribution(wounded_player,wounded_player,*retribution(0),100));
 check(w->maintained_enchantment_count(wounded_player,100)==1);
 const auto pending=w->begin_npc_action(wounded_scope,combat::npcs::control::Action::spell,100,1750);check(bool(pending));
 const auto before_caster=w->resources(1)->health;
 const auto target=*w->effect_scope(100),foe=*w->effect_scope(101),spirit=*w->effect_scope(102),far=*w->effect_scope(103),caster=*w->effect_scope(1);
 const auto seed=*healing_seed(12);check(w->complete_healing_seed(caster,target,seed,0,100));
 w->join(4,"Seed gain ally",120,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w->set_vitals(4,{500,1000,100,100},100);
 check(w->complete_healing_seed(caster,*w->effect_scope(4),seed,0,100));
 const combat::ai::ordinary::Reviewed weapon{20,20,12,0,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1200,32},"authored20damage Seed gain"};
 check(w->configure_npc_opposed_ordinary(foe,weapon,1000));const auto token=w->begin_npc_opposed_ordinary(foe,target,1000);check(bool(token));box->drain();w->tick(token->completes);
 check(w->npc_combat_snapshot(target)->health->current==480+seed.healing);
 // Defile Flesh reduces healing, but this gain retains its full25Health.
 check(w->resources(1)->health==before_caster+seed.healing);
 check(w->resources(2)->health==500);
 // Receiving a gain cannot re-trigger this second Seed as damage or healing.
 check(w->resources(4)->health==500+seed.healing);
 check(w->npc_combat_snapshot(spirit)->health->current==500+seed.healing);
 check(w->npc_combat_snapshot(far)->health->current==500);
 check(w->resources(3)->health==0&&*w->control_epoch(wounded_player)==before_epoch+1);
 check(w->player_death_times({3,1})[0]==token->completes);
 check(w->maintained_enchantment_count(wounded_player,token->completes)==0);
 const auto retired=w->status_snapshot(wounded_player,token->completes);
 check(retired&&std::none_of(retired->begin(),retired->end(),[](const auto& e){return e.skill==248;}));
 check(w->npc_combat_snapshot(wounded_scope)->health->current==0);
 check(w->npc_combat_snapshot(wounded_scope)->effects&0x10);
 check(!w->complete_npc_action(*pending,token->completes));
 const auto packets=box->drain();check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(102,1,seed.healing,1000))==1);
 w->tick(token->completes);check(w->resources(3)->health==0&&*w->control_epoch(wounded_player)==before_epoch+1);
 check(w->npc_combat_snapshot(wounded_scope)->health->current==0);
 }
 std::cout<<"PASS Healing Seed real-hit nearby spirit Health gain, healing immunity and range\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
