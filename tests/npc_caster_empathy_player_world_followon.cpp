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
 // Existing Flare uses the SAME world timer bypass, with no Empathy installed.
 // Visiting one millisecond early must not suppress its paid release deadline.
 {Fixture f(26,false);check(f.w.elemental_armor(f.owner,60,60));
  combat::ai::FlarePolicy p;p.source=f.caster;p.target=f.owner;p.level=7;p.fire_magic=0;p.speed=50;p.acquire=200;p.follow=1500;p.leash=1000;p.flight={1000,32};p.evidence="authored legacy Flare deadline";
  check(f.w.configure_npc_flare_ai(p,1000));f.w.tick(1000);const auto started=f.w.npc_flare_ai_snapshot(f.caster);check(started&&started->cast);
  const auto token=*started->cast;const auto paid=100-skills::find_skill(194)->energy_cost();check(f.w.npc_energy_snapshot(f.caster)->current==paid);
  f.w.tick(token.completes-1);const auto early=f.w.npc_flare_ai_snapshot(f.caster);check(early&&early->cast&&early->shots.empty());check(!f.has_hex(token.completes-1)&&f.w.resources(1)->health==1000);
  f.w.tick(token.completes);const auto released=f.w.npc_flare_ai_snapshot(f.caster);check(released&&!released->cast&&released->shots.size()==1&&released->shots.front().released==token.completes);
  check(f.w.npc_energy_snapshot(f.caster)->current==paid);f.w.tick(token.completes);check(f.w.npc_flare_ai_snapshot(f.caster)->shots.size()==1&&f.w.npc_energy_snapshot(f.caster)->current==paid);
 }
 for(unsigned id:{26u,3151u})for(bool ordinary:{false,true})for(bool dead:{false,true}){
  Fixture baseline(id,false),f(id);f.cast();
  const bool applied=f.has_hex(3000);const auto owner_health=f.w.resources(1)->health;
  if(!applied||owner_health!=1000){
   const auto status=f.w.status_snapshot(f.owner,3000);const auto motion=f.w.npc_flare_ai_snapshot(f.caster);
   std::cerr<<"NPC apply id="<<id<<" ordinary="<<ordinary<<" dead="<<dead<<" applied="<<applied<<" ownerHP="<<owner_health<<" targetHP="<<f.w.resources(2)->health<<" energy="<<f.w.npc_energy_snapshot(f.caster)->current<<" pending="<<bool(motion&&motion->cast)<<" selected="<<(motion?motion->policy.target.actor:0)<<'\n';
   if(motion&&motion->cast)std::cerr<<" token begins="<<motion->cast->begins<<" due="<<motion->cast->completes<<" epoch="<<motion->cast->epoch<<" observedAt=3000"<<'\n';
   const auto body=f.w.npc_combat_snapshot(f.caster);
   if(body)std::cerr<<" casterActor="<<body->id<<" generation="<<body->generation<<" expectedGeneration="<<f.caster.generation<<" casterHP="<<(body->health?body->health->current:-1)<<'\n';
   if(status)for(const auto& e:*status)std::cerr<<" status skill="<<e.skill<<" cause="<<e.cause<<" begins="<<e.applied<<'\n';
  }
  check(applied);check(owner_health==1000);
  if(dead)check(f.w.damage_npc(f.owner,f.caster,{combat::Kind::armor_ignoring,5000,20,194},1248,3100).killed);
  check(f.has_hex(3200));baseline.attack(ordinary);f.attack(ordinary);check(f.w.resources(1)->health==1000-(id==26?10u:15u));
  check(f.w.resources(2)->health==baseline.w.resources(2)->health+(id==26?1u:0u));f.w.tick(4000);check(f.w.resources(1)->health==1000-(id==26?10u:15u));
 }
 // Shared selected HexBreaker consumes its actual stance and retaliates against
 // the real NPC through existing protection/Reversal credit, no Player proxy.
 for(bool reversal:{false,true}){Fixture f;check(f.w.hex_breaker(f.owner,*hex_breaker(12),20,1000));if(reversal)check(f.w.stage_npc_reversal(f.caster,12,1000));f.cast();
  check(!f.has_hex(3000));const auto status=f.w.status_snapshot(f.owner,3000);check(status&&std::none_of(status->begin(),status->end(),[](const auto&e){return e.skill==10;}));
  check(f.w.npc_combat_snapshot(f.caster)->health->current==(reversal?1000:1000-hex_breaker(12)->damage));check(f.w.resources(1)->health==1000);
 }
 // Prevented completion can kill the actual NPC caster without installing hex.
 {Fixture f(26,true,20);check(f.w.hex_breaker(f.owner,*hex_breaker(12),20,1000));f.cast();check(!f.has_hex(3000)&&f.w.npc_combat_snapshot(f.caster)->health->current==0);check(f.w.take_npc_deaths(f.owner).size()==1);f.attack(false);check(f.w.resources(1)->health==1000);}
 // Dying during activation is different from an already-applied persistent hex.
 {Fixture f;f.w.tick(1000);check(f.w.npc_flare_ai_snapshot(f.caster)->cast.has_value());check(f.w.damage_npc(f.owner,f.caster,{combat::Kind::armor_ignoring,5000,20,194},1248,1500).killed);f.w.tick(3000);check(!f.has_hex(3000)&&f.w.resources(1)->health==1000);}
 // The player recipient's own Protection remains shared for NPC credited damage.
 {Fixture f(3151,true,1000,0,100,100);check(f.w.complete_protective_spirit(f.owner,f.owner,*protective_spirit(12),0,1000));f.cast();f.attack(false);check(f.w.resources(1)->health==90);}
 // Actual NPC authority rejects stale Scope and incorrect caller level hints.
 {Fixture f(26,false);auto stale=f.caster;++stale.generation;check(!f.w.empathy(stale,f.owner,26,0,7,1000));check(!f.w.empathy(f.caster,f.owner,26,0,20,1000));check(!f.has_hex(1000));}
 // Actual quest retirement leaves the recipient and non-maintained hex
 // alive. A fresh quest body receives a distinct Scope and no old damage credit.
 {Fixture f(26,true,1000,0,1000,1000,true);f.cast();const auto old=f.caster;check(f.w.clear_kill_objective(f.owner,75,1,true));check(!f.w.effect_scope(old.actor));check(f.has_hex(3100));
  check(f.w.spawn_kill_encounter(f.owner,75,2,{caster_template()},[]{return true;}));const auto replacement=*f.w.effect_scope(60001);check(replacement!=old);f.box->drain();f.attack(false);
  check(f.w.resources(1)->health==990);for(const auto& packet:f.box->drain())if(packet.size()>=18&&packet[0]==162&&packet[1]==0){const auto word=[&](unsigned off){return unsigned(packet[off])|(unsigned(packet[off+1])<<8)|(unsigned(packet[off+2])<<16)|(unsigned(packet[off+3])<<24);};check(!(word(2)==55&&word(6)==f.owner.actor&&word(10)==replacement.actor));}}
 // Real player numeric-ID reuse exercises retained generic caster Scope credit:
 // health loss persists, but the new generation must not receive attribution.
 {Fixture f(26,false);const auto old=f.target;check(f.w.empathy(old,f.owner,26,0,20,1000));f.w.leave(old.actor);
  f.w.join(old.actor,"Reused caster",160,0,std::make_shared<world::Outbox>(),0,{}, {},1100,2);f.w.set_vitals(old.actor,{1000,1000,100,100},100);f.target=*f.w.effect_scope(old.actor);check(f.target!=old);
  check(f.w.physical_armor(f.target,60));check(f.w.ordinary_weapon(f.target,std::nullopt,20,1100,combat::weapon::CriticalProfile{3,0,20}));f.box->drain();f.attack(false);check(f.w.resources(1)->health==990);
  for(const auto& packet:f.box->drain())if(packet.size()>=18&&packet[0]==162&&packet[1]==0){const auto word=[&](unsigned off){return unsigned(packet[off])|(unsigned(packet[off+1])<<8)|(unsigned(packet[off+2])<<16)|(unsigned(packet[off+3])<<24);};check(!(word(2)==55&&word(6)==f.owner.actor&&word(10)==old.actor));}}
 std::cout<<"PASS real paid NPC Empathy cast, player ordinary/skills, caster death, HexBreaker/Reversal and Protection\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
