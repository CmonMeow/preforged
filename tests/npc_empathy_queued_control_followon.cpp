// NEW unregistered actual paid NPC activation -> player attack reactions.
#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("NPCcaster player Empathy World line "+std::to_string(at.line()));}
world::Instance::Npc caster_template(unsigned caster_hp=1000){
  world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=7;n.definition.primary=5;n.definition.flags=4;n.definition.visual_adjustment={0,0,0,100};
  combat::npcs::Definition d;d.binding={145,100,100,0,0,0,4,7};d.relation=combat::npcs::Relation::hostile_to_players;
  d.fire_armor=60;d.fire_armor_evidence="authored Meteor regression armor";d.knockdown_allowed=true;d.knockdown_evidence="authored NPC control authority";
  d.spawn_health={double(caster_hp),1000};d.spawn_energy=combat::npcs::Energy{100,100};d.energy_pips=0;d.energy_evidence=d.energy_pips_evidence="authored NPC caster resources";
  d.damageable=d.conditions_allowed=true;d.physical_armor=60;d.physical_armor_evidence="authored NPC armor";d.healing_allowed=d.fleshy=true;d.healing_evidence=d.fleshy_evidence="authored living Mesmer";
  n.combat_definition=d;return n;
}
struct Fixture{
 world::Instance w;Scope caster,owner,target;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();unsigned skill,rank;
 combat::weapon::Owned sword{12,1,{20,20,2},12,false,false,27,0.};
 Fixture(unsigned id=26,bool ai=true,unsigned caster_hp=1000,unsigned r=0,unsigned owner_hp=1000,unsigned owner_max=1000,bool owned=false):skill(id),rank(r){
  if(!owned)w.configure_npcs({caster_template(caster_hp)});
  w.join(1,"Hex recipient",100,0,box,0,{}, {},100,1);w.set_vitals(1,{owner_hp,owner_max,100,100},100);owner=*w.effect_scope(1);
  w.join(2,"Weapon target",160,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{1000,1000,100,100},100);target=*w.effect_scope(2);
  if(owned){check(w.spawn_kill_encounter(owner,75,1,{caster_template(caster_hp)},[]{return true;}));caster=*w.effect_scope(60000);}else caster=*w.effect_scope(100);
  check(w.physical_armor(owner,60)&&w.physical_armor(target,60));check(w.ordinary_weapon(owner,sword,20,100,combat::weapon::CriticalProfile{1,0,20}));check(w.ordinary_weapon(target,std::nullopt,20,100,combat::weapon::CriticalProfile{3,0,20}));
  w.configure_attack_miss_draws([]{return .5;});w.configure_attack_critical_draws([]{return .999999;});
  if(ai){combat::ai::FlarePolicy p;p.source=caster;p.target=owner;p.level=7;p.speed=50;p.acquire=200;p.follow=1500;p.leash=1000;p.flight={1000,32};p.evidence="authored paid NPC Empathy";
   const auto reviewed=combat::ai::activated::Reviewed::empathy_spell({static_cast<std::uint16_t>(id),0},r,p.evidence);check(bool(reviewed));if(owned){check(w.configure_npc_owned_immediate(owner,75,1,p,{*reviewed},{{{0,0}},200,4000,p.evidence},1000));
    check(w.damage_npc(owner,caster,{combat::Kind::armor_ignoring,1,20,194},1248,1000).reason==combat::Reason::accepted);
   }else check(w.configure_npc_immediate_ai(p,{*reviewed},1000));}
  box->drain();
 }
 void cast(){w.tick(1000);const auto controller=w.npc_flare_ai_snapshot(caster);check(controller&&controller->cast&&controller->cast->skill&&controller->cast->skill->skill==skill);const auto due=controller->cast->completes;check(w.npc_energy_snapshot(caster)->current==90);w.tick(due-1);check(!has_hex(due-1));w.tick(due);check(w.npc_energy_snapshot(caster)->current==90);}
 bool has_hex(std::uint64_t at){const auto entries=w.status_snapshot(owner,at);return entries&&std::any_of(entries->begin(),entries->end(),[&](const auto&e){return e.skill==skill&&e.cause==caster.actor;});}
 void attack(bool ordinary,std::uint64_t at=3300){if(ordinary){check(w.ordinary_attack(owner,target.actor,at));w.tick(at+600);w.cancel_ordinary_attack(owner);}else{const auto plan=combat::weapon::attack_skill(320,12);check(bool(plan));check(w.melee_attack(owner,target,sword,*plan,at));}}
};
int main()try{
 for(bool dense:{false,true}){
  Fixture f;check(f.w.elemental_armor(f.owner,60,60)&&f.w.elemental_armor(f.target,60,60));
  f.w.tick(1000);const auto started=f.w.npc_flare_ai_snapshot(f.caster);check(started&&started->cast);const auto old=*started->cast;
  check(old.completes==3000);check(f.w.fire_storm(f.owner,f.caster,*fire_storm(0,187),20,1000));
  if(dense)for(std::uint64_t at=1100;at<4000;at+=100)f.w.tick(at);
  f.w.tick(4000);check(!f.has_hex(4000));const auto after=f.w.npc_flare_ai_snapshot(f.caster);check(after&&!after->cast);
  // Meteor hit1500/control through3500 must invalidate old cast even though
  // the coarse observation4000 sees the NPC standing again.
  check(f.w.npc_combat_snapshot(f.caster)->health->current<1000);check(f.w.npc_energy_snapshot(f.caster)->current==90);
  f.w.tick(4000);f.w.tick(5000);check(!f.has_hex(5000));const auto final=f.w.npc_flare_ai_snapshot(f.caster);check(final&&!final->cast);
 }
 std::cout<<"PASS queued Meteor invalidates paid NPC Empathy token across coarse expired control interval\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
