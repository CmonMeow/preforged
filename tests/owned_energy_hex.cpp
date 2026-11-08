#include "gw_owned_energy_hex.h"
#include "gw_overcast_settlement.h"
#include <iostream>
#include <string>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
namespace regeneration=fw::gw::skills::regeneration;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Owned Energy hex line "+std::to_string(at.line()));
}
EnergyHexOwnership owner(unsigned skill,Scope caster,Scope target,unsigned effect=1,unsigned caster_effect=10,Time at=100){
 const auto plan=*energy_degeneration_hex(skill,12);
 return {caster,target,effect,skill==140?caster_effect:0u,skill,12,20,at,at+plan.duration};
}
int main()try{
 const Scope a{1,1,1},b{1,3,1},target{1,2,1};
 {
  Registry registry(target);regeneration::State health(target,100);overcast::CeilingCurve energy(target,100,33);
  std::map<Scope,regeneration::State> sources;sources.emplace(a,regeneration::State(a,100));
  check(sources.at(a).apply(a,{10,a.actor,115,1,100,10000}));
  check(!prepare_energy_hex(registry,health,energy,sources,owner(140,a,target)));
  sources.clear();check(health.apply(target,{1,a.actor,115,1,100,10000}));
  check(!prepare_energy_hex(registry,health,energy,sources,owner(125,a,target)));
 }
 for(unsigned skill:{125u,140u}){
  Registry registry(target);regeneration::State health(target,100);overcast::CeilingCurve energy(target,100,33);
  std::map<Scope,regeneration::State> sources;if(skill==140)sources.emplace(a,regeneration::State(a,100));
  const auto link=owner(skill,a,target);
  const auto applied=prepare_energy_hex(registry,health,energy,sources,link);check(bool(applied));
  check(matches_energy_hex(applied->registry,link));check(applied->health.pips(100)==(skill==125?-4:0));
  check(applied->energy.effective_energy_pips(4,100)==3);
  check(registry.active(target,100).empty()&&!health.lookup(target,1)&&!energy.has_energy_modifiers());
  auto forged=link;++forged.level;forged.caster.generation=0;check(!prepare_energy_hex(registry,health,energy,sources,forged));
  auto state=*applied;double target_health=1000;
  check(state.health.advance(target,1000,target_health,1000));
  vitals::Resources resources{1000,1000,50,100};energy::Regeneration regen;
  const auto settled=overcast::settle(state.energy,target,resources,regen,0,1000,4);check(bool(settled));state.energy=settled->curve;
  const MindWrackTransition zero_event{target,1,1000,{0,100,0,1},{0,100,0,0}};
  const auto zero_reaction=prepare_energy_hex_zero(state.registry,state.health,state.energy,state.caster_health,link,zero_event);
  check(zero_reaction&&zero_reaction->damage==(skill==125?63u:41u));
  const auto& zero_end=zero_reaction->change;
  check(!prepare_energy_hex_zero(zero_end.registry,zero_end.health,zero_end.energy,zero_end.caster_health,link,zero_event));
  auto false_zero=zero_event;false_zero.before.fractional_units=0;
  check(!prepare_energy_hex_zero(state.registry,state.health,state.energy,state.caster_health,link,false_zero));
  false_zero=zero_event;false_zero.after.fractional_units=1;
  check(!prepare_energy_hex_zero(state.registry,state.health,state.energy,state.caster_health,link,false_zero));
  false_zero=zero_event;++false_zero.target.generation;
  check(!prepare_energy_hex_zero(state.registry,state.health,state.energy,state.caster_health,link,false_zero));
  false_zero=zero_event;false_zero.at=link.expires;
  check(!prepare_energy_hex_zero(state.registry,state.health,state.energy,state.caster_health,link,false_zero));
  check(matches_energy_hex(state.registry,link));
  // Generate the zero event from real settlement, rather than using the
  // interval's initial UI value or a result already advanced beyond zero.
  auto one=resources;one.energy=1;
  const auto crossing=overcast::settle_until_zero(applied->energy,target,one,regen,0,5000,0);
  check(bool(crossing));
  const auto observed=energy_zero_observation(*crossing,target,2);
  check(observed&&observed->at==3100&&observed->before.fractional_units==100);
  auto actual=*applied;double actual_hp=1000;
  check(actual.health.advance(target,observed->at,actual_hp,1000));
  const auto actual_end=prepare_energy_hex_zero(actual.registry,actual.health,crossing->curve,
      actual.caster_health,link,*observed);
  check(actual_end&&actual_end->damage==(skill==125?63u:41u));
  const auto overshot=overcast::settle(applied->energy,target,one,regen,0,5000,0);
  check(overshot&&!energy_zero_observation(*overshot,target,2));
  auto stale_target=target;++stale_target.generation;
  check(!energy_zero_observation(*crossing,stale_target,2)&&!energy_zero_observation(*crossing,target,0));
  const auto ended=prepare_energy_hex_end(state.registry,state.health,state.energy,state.caster_health,link,1000);check(bool(ended));
  check(ended->registry.active(target,1000).empty()&&!ended->energy.has_energy_modifiers()&&!ended->health.lookup(target,1));
  check(!prepare_energy_hex_end(ended->registry,ended->health,ended->energy,ended->caster_health,link,1000));
  if(skill==140){auto old=ended->caster_health.at(a);double hp=1000;check(old.advance(a,2000,hp,1000));check(std::abs(hp-998.2)<1e-8);}
  // Natural expiry may already have pruned timed sources,while registry cleanup
  // still needs its original ownership record. This must not fabricate damage.
  auto expired=*applied;double hp=1000;check(expired.health.advance(target,30000,hp,1000));
  const auto later=overcast::settle(expired.energy,target,resources,regen,0,30000,4);check(bool(later));expired.energy=later->curve;
  check(expired.energy.prune_energy_modifiers(target,30000));
  if(skill==140){double caster_hp=1000;check(expired.caster_health.at(a).advance(a,30000,caster_hp,1000));}
  check(bool(prepare_energy_hex_end(expired.registry,expired.health,expired.energy,expired.caster_health,link,30000)));
 }
 {
  Registry registry(target);regeneration::State health(target,100);overcast::CeilingCurve energy(target,100,33);
  std::map<Scope,regeneration::State> sources;sources.emplace(a,regeneration::State(a,100));
  const auto original=owner(140,a,target);const auto initial=prepare_energy_hex(registry,health,energy,sources,original);check(bool(initial));auto state=*initial;
  double hp=1000;check(state.health.advance(target,1000,hp,1000));
  vitals::Resources resources{1000,1000,50,100};energy::Regeneration regen;const auto settled=overcast::settle(state.energy,target,resources,regen,0,1000,4);check(bool(settled));state.energy=settled->curve;
  state.caster_health.emplace(b,regeneration::State(b,1000));const auto replacement=owner(140,b,target,2,11,1000);
  check(!prepare_energy_hex(state.registry,state.health,state.energy,state.caster_health,replacement)); // No unowned renewal.
  const auto renewed=prepare_energy_hex(state.registry,state.health,state.energy,state.caster_health,replacement,original);check(bool(renewed));
  check(renewed->removed.size()==1&&renewed->removed[0].instance==1&&matches_energy_hex(renewed->registry,replacement));
  check(!renewed->energy.has_energy_modifier(target,1)&&renewed->energy.has_energy_modifier(target,2));
  auto old=renewed->caster_health.at(a),next=renewed->caster_health.at(b);double old_hp=1000,new_hp=1000;
  check(old.advance(a,2000,old_hp,1000)&&next.advance(b,2000,new_hp,1000));check(std::abs(old_hp-998.2)<1e-8&&new_hp==998);
  auto wrong=original;wrong.caster_health_effect=99;
  check(!prepare_energy_hex(state.registry,state.health,state.energy,state.caster_health,replacement,wrong));
  auto missing=state.caster_health;missing.erase(a);check(!prepare_energy_hex(state.registry,state.health,state.energy,missing,replacement,original));
  auto stale=a;++stale.generation;missing.emplace(stale,regeneration::State(stale,1000));
  check(!prepare_energy_hex(state.registry,state.health,state.energy,missing,replacement,original));
  auto dead_sources=state.caster_health;dead_sources.at(a)=regeneration::State(a,500);
  check(!prepare_energy_hex(state.registry,state.health,state.energy,dead_sources,replacement,original));
  check(!prepare_energy_hex(state.registry,state.health,state.energy,dead_sources,replacement,original,{{a,99}}));
  check(!prepare_energy_hex(state.registry,state.health,state.energy,dead_sources,replacement,original,{{a,1001}}));
  check(!prepare_energy_hex(state.registry,state.health,state.energy,dead_sources,replacement,original,{{a,600}}));
  check(bool(prepare_energy_hex(state.registry,state.health,state.energy,dead_sources,replacement,original,{{a,500}})));
  dead_sources.at(a)=regeneration::State(a,1001);
  check(!prepare_energy_hex(state.registry,state.health,state.energy,dead_sources,replacement,original,{{a,500}}));
  // Refusing a forged old source never mutates the target or linked timer.
  check(matches_energy_hex(state.registry,original)&&state.energy.has_energy_modifier(target,1));
 }
 {
  Registry registry(target);regeneration::State health(target,100);overcast::CeilingCurve energy(target,100,33);
  std::map<Scope,regeneration::State> sources;sources.emplace(b,regeneration::State(b,100));
  const auto wither=owner(125,a,target),malaise=owner(140,b,target,2,11);
  const auto first=prepare_energy_hex(registry,health,energy,sources,wither);check(bool(first));
  auto both=prepare_energy_hex(first->registry,first->health,first->energy,first->caster_health,malaise);check(bool(both));
  vitals::Resources resources{1000,1000,1,100};energy::Regeneration regen;
  const auto boundary=overcast::settle_until_zero(both->energy,target,resources,regen,0,5000,0);check(bool(boundary));
  const auto event=energy_zero_observation(*boundary,target,77);check(event&&event->at==1600);
  double hp=1000;check(both->health.advance(target,event->at,hp,1000));
  for(bool reverse:{false,true}){
   auto owners=reverse?std::vector{malaise,wither}:std::vector{wither,malaise};
   const auto ended=prepare_energy_hex_zero_batch(both->registry,both->health,boundary->curve,both->caster_health,owners,*event);
   check(ended&&ended->damage.size()==2&&ended->damage[0].damage==63&&ended->damage[1].damage==41);
   check(ended->registry.active(target,event->at).empty()&&!ended->energy.has_energy_modifiers());
   auto caster=ended->caster_health.at(b);double caster_hp=1000;
   check(caster.advance(b,5000,caster_hp,1000)&&std::abs(caster_hp-997.)<1e-8);
   check(!prepare_energy_hex_zero_batch(ended->registry,ended->health,ended->energy,ended->caster_health,owners,*event));
  }
  check(!prepare_energy_hex_zero_batch(both->registry,both->health,boundary->curve,both->caster_health,{wither},*event));
  check(!prepare_energy_hex_zero_batch(both->registry,both->health,boundary->curve,both->caster_health,{wither,wither},*event));
  auto forged=malaise;++forged.caster.generation;
  check(!prepare_energy_hex_zero_batch(both->registry,both->health,boundary->curve,both->caster_health,{wither,forged},*event));
  check(matches_energy_hex(both->registry,wither)&&matches_energy_hex(both->registry,malaise));
  const auto mind=prepare_mind_wrack(both->registry,a,target,3,*mind_wrack(12),100);check(bool(mind));
  MindWrackEvents events(target);
  const auto unified=prepare_energy_zero_observers(mind->registry,both->health,boundary->curve,both->caster_health,
      {wither,malaise},mind->owner,events,*event);
  check(unified&&unified->hexes.damage.size()==2&&unified->mind_wrack&&unified->mind_wrack->zero_damage==83);
  check(unified->hexes.registry.active(target,event->at).empty());
  check(!prepare_energy_zero_observers(mind->registry,both->health,boundary->curve,both->caster_health,
      {wither,malaise},{},events,*event)); // Cannot silently omit a live observer.
  check(!prepare_energy_zero_observers(mind->registry,both->health,boundary->curve,both->caster_health,
      {wither},mind->owner,events,*event));
  const auto renewed=prepare_mind_wrack(unified->hexes.registry,a,target,4,*mind_wrack(12),event->at);check(bool(renewed));
  check(!prepare_energy_zero_observers(renewed->registry,unified->hexes.health,unified->hexes.energy,
      unified->hexes.caster_health,{},renewed->owner,unified->events,*event));
  auto fresh=*event;++fresh.event;
  const auto mind_only=prepare_energy_zero_observers(renewed->registry,unified->hexes.health,unified->hexes.energy,
      unified->hexes.caster_health,{},renewed->owner,unified->events,fresh);
  check(mind_only&&mind_only->hexes.damage.empty()&&mind_only->mind_wrack->zero_damage==83);
  // Registry records caster actor, so generation liveness remains the World
  // owner's responsibility; lifetime fences must be checked before staging.
  check(mind->registry.active(target,event->at).size()==3);
 }
 std::cout<<"PASS owned Wither/Malaise atomic proposals,linked histories,expiry,renewal,shared zero batch,forged/missing/stale owners and replay;World/zero-event integration pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
