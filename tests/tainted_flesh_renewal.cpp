#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
std::string context;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Tainted Flesh Renewal line "+std::to_string(at.line())+" "+context);
}
struct Fixture {
 world::Instance w;Scope caster,target;inventory::Profile gear;
 std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
 Fixture(unsigned kind,unsigned version){
  if(kind==2){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.visual_adjustment={0,0,0,100};
   combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
   d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health=combat::Health{100,1000};
   d.damageable=d.conditions_allowed=true;d.fleshy=true;d.fleshy_evidence="authored fixture";
   d.disease_immune=false;d.disease_immunity_evidence="authored fixture";n.combat_definition=d;w.configure_npcs({n});}
  for(unsigned id:{1u,2u}){w.join(id,"Actor",float(id*10),0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{100,1000,50,100},100);}
  caster=*w.effect_scope(1);target=*w.effect_scope(kind==0?1:kind==1?2:100);
  if(version){const auto plan=*ether_renewal(version,0);check(w.status_mutation(caster,version,80,0,100,100+plan.duration,true));}
  gear.bags.emplace(1,inventory::Bag{1,1,0,20});inventory::Item item;item.definition.id=50;item.definition.type=26;
  item.definition.quantity=1;item.definition.modifiers={0x22b80014};item.bag=1;gear.items.emplace(50,item);gear.weapons.sets[0].weapon=50;box->drain();
 }
 std::optional<EnchantmentDuration> quote(unsigned rank,std::uint64_t at,bool wrong=false){
  auto scope=caster;if(wrong)++scope.generation;
  const auto equipment=items::weapon_passives::equipped_enchanting(gear,1);const auto active=w.status_snapshot(caster,at);const auto plan=tainted_flesh(rank);
  return equipment&&active&&plan?enchanting_duration(113,plan->duration,scope,*equipment,*active,at):std::nullopt;
 }
 bool complete(Scope source,const TaintedFleshPlan& plan,std::uint64_t event,std::uint64_t at,const EnchantmentDuration* duration){
  return w.complete_tainted_flesh(source,target,plan,at,duration,event);
 }
};
int main()try{unsigned manual=0,paid=0;
 for(unsigned kind=0;kind<3;++kind)for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<7;++mode){
  context="manual kind="+std::to_string(kind)+" version="+std::to_string(version)+" mode="+std::to_string(mode);
  Fixture f(kind,version);const auto at=mode==4&&version?100+ether_renewal(version,0)->duration:200;
  f.w.tick(at);check(f.w.before_spell_effect(f.caster,113,1,at));f.box->drain();const auto before=*f.w.resources(1);
  auto source=f.caster;if(mode==2)++source.generation;auto plan=*tainted_flesh(0);if(mode==6)++plan.rank;
  auto quote=f.quote(0,at,mode==3);check(bool(quote));
  bool ok=f.complete(source,plan,mode==1?2:1,at,&*quote);
  if(mode==1||mode==2||mode==3||mode==6){check(!ok&&f.box->drain().empty());check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);
   quote=f.quote(0,at);ok=f.complete(f.caster,*tainted_flesh(0),1,at,&*quote);}
  check(ok);const bool reward=version&&mode!=4;
  check(f.w.resources(1)->health==before.health+(reward?5:0)&&f.w.resources(1)->energy==before.energy+(reward?1:0));
  const auto entries=*f.w.status_snapshot(f.target,at);check(std::count_if(entries.begin(),entries.end(),[&](const auto& e){return e.skill==113&&e.expires==at+24000;})==1);
  const auto packets=f.box->drain();const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
  const auto visual=f.target==f.caster?combat::detail::integer(21,1,214):*particles::targeted_body_2022(f.target.actor,1,113);
  const auto body=std::find(packets.begin(),packets.end(),visual);const auto status=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>1&&p[0]==65&&p[1]==0;});
  check(finish<status&&status<body&&body!=packets.end());check(std::count(packets.begin(),packets.end(),visual)==1);
  if(reward){const auto heal=std::find_if(packets.begin(),packets.end(),[](const auto& p){return p.size()>=6&&p[0]==162&&p[2]==55;});check(heal<finish);}
  check(!f.complete(f.caster,*tainted_flesh(0),1,at,&*quote)&&f.box->drain().empty());
  if(mode==5){const auto next=at+100;f.w.tick(next);check(f.w.before_spell_effect(f.caster,113,2,next));quote=f.quote(0,next);const auto prior=*f.w.resources(1);
   check(f.complete(f.caster,*tainted_flesh(0),2,next,&*quote));const unsigned n=version?(kind==0?2:1):0;
   check(f.w.resources(1)->health==prior.health+5*n&&f.w.resources(1)->energy==prior.energy+n);
   const auto active=*f.w.status_snapshot(f.target,next);check(std::count_if(active.begin(),active.end(),[](const auto& e){return e.skill==113;})==1);}
  ++manual;
 }
 for(unsigned kind=0;kind<3;++kind)for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<9;++mode){
  context="paid kind="+std::to_string(kind)+" version="+std::to_string(version)+" mode="+std::to_string(mode);
  Fixture f(kind,version);casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0,before_health=0,before_energy=0;std::uint64_t admitted=0;
  h.resources=[&](auto id){return f.w.resources(id);};h.events=[&](const auto& p){f.box->enqueue(p);};h.target_scope=[&](auto id){return f.w.effect_scope(id);};
  h.consume_energy_at=[&](auto n,auto at){return f.w.consume_energy(1,n,f.caster,at);};
  h.tainted_flesh_target_valid=[&](auto id,const auto& plan,auto at){return f.w.valid_tainted_flesh_target(f.caster,id,plan,at);};
  h.tainted_flesh=[](auto,const auto&,auto)->casting::EffectCompletion{throw std::runtime_error("typed Tainted Flesh downgraded");};
  h.enchantment_duration_ready=[&](auto used,auto base,auto at){check(used==113&&base==20000);return bool(f.quote(0,at));};
  h.before_spell_effect=[&](auto used,auto event,auto at){check(used==113);f.w.tick(at);before_health=f.w.resources(1)->health;before_energy=f.w.resources(1)->energy;admitted=event;return f.w.before_spell_effect(f.caster,used,event,at);};
  h.simple_enchantment_completed=[&](auto target,auto used,auto rank,auto,auto event,auto at)->casting::EffectCompletion{
   check(used==113&&target==f.target&&event==admitted&&event);++calls;const auto quote=f.quote(rank,at);
   const bool ok=quote&&f.complete(f.caster,*tainted_flesh(rank),event,at,&*quote);return {ok,{},{},{},false,true};};
  Policy policy;policy.primary=4;policy.secondary=6;policy.campaigns=255;policy.learned[113]=1;Slots bar{};bar[0]={113,0};attributes::Ranks ranks{};
  if(mode==7)h.enchantment_duration_ready={};if(mode==8)h.before_spell_effect={};
  check(runtime.start(100,{113,0,f.target.actor},bar,policy,ranks,true,h)==(mode>=7?casting::Reason::unsupported:casting::Reason::accepted));
  if(mode>=7){check(calls==0&&f.w.resources(1)->energy==50);++paid;continue;}
  if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(101,h);if(mode==3)h.simple_enchantment_completed={};if(mode==4)h.before_spell_effect={};if(mode==5)h.enchantment_duration_ready={};
  if(mode==6){f.w.leave(1);f.w.join(1,"Replacement",0,0,f.box,0,{}, {},200,1);f.w.set_vitals(1,{100,1000,40,100},100);}
  f.box->drain();runtime.update(1100,h);const auto packets=f.box->drain();runtime.update(1200,h);check(calls==unsigned(mode==0));
  if(mode==0){check(f.w.resources(1)->health==before_health+(version?5:0)&&f.w.resources(1)->energy==before_energy+(version?1:0));check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);}
  ++paid;
 }
 for(unsigned kind=0;kind<3;++kind){
  context="future clock kind="+std::to_string(kind);Fixture f(kind,181);
  f.w.tick(200);check(f.w.before_spell_effect(f.caster,113,1,200));const auto old=f.quote(0,200);check(bool(old));
  f.w.tick(300);f.box->drain();const auto before=*f.w.resources(1);
  check(!f.complete(f.caster,*tainted_flesh(0),1,200,&*old));check(f.box->drain().empty());
  check(f.w.resources(1)->health==before.health&&f.w.resources(1)->energy==before.energy);
  const auto fresh=f.quote(0,300);check(bool(fresh));check(f.complete(f.caster,*tainted_flesh(0),1,300,&*fresh));
  check(f.w.resources(1)->health==before.health+5&&f.w.resources(1)->energy==before.energy+1);
 }
 for(unsigned kind=0;kind<2;++kind){
  context="lethal cleanse kind="+std::to_string(kind);Fixture f(kind,181);
  f.w.join(3,"Foe",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);f.w.set_vitals(3,{1000,1000,100,100},100);
  const auto foe=*f.w.effect_scope(3);check(f.w.rotting_flesh(foe,f.target,*rotting_flesh(0),100));
  check(f.w.degeneration_hex(foe,f.target,*degeneration_hex(19,12),100));f.w.set_vitals(f.target.actor,{1,1000,50,100},100);
  check(f.w.before_spell_effect(f.caster,113,1,150));const auto duration=f.quote(0,150);check(bool(duration));f.box->drain();
  check(f.complete(f.caster,*tainted_flesh(0),1,150,&*duration));
  check(f.w.resources(f.target.actor)->health==0&&!f.w.has_condition(f.target,combat::conditions::Kind::disease,150));
  check(f.w.status_snapshot(f.target,150)->empty());const auto packets=f.box->drain();
  check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
  check(!f.complete(f.caster,*tainted_flesh(0),1,150,&*duration));
 }
 std::cout<<"PASS "<<manual<<" Tainted Flesh admitted reward/receipt/replay/replacement cases and "<<paid<<" paid completion/cancel/interrupt/hook-loss/lifetime cases;3 future-clock refusal/retry and2 lethal cleanse cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
