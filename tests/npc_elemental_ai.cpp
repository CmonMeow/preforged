#include "gw_npc_elemental_ai.h"
#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;namespace ai=combat::ai;namespace elemental=ai::elemental;
void check(bool yes,std::source_location at=std::source_location::current()){
 if(!yes)throw std::runtime_error("NPC elemental AI line "+std::to_string(at.line()));
}
const skills::effects::Scope source{1,100,1},first{1,2,1},second{1,3,1};
std::vector<elemental::Reviewed> book(){return {
 {{186,0},*skills::effects::elemental(186,12),"synthetic reviewed Fireball, captured skill identity"},
 {{194,0},*skills::effects::elemental(194,12),"synthetic reviewed Flare, captured skill identity"},
 {{191,17},*skills::effects::elemental(191,8),"synthetic reviewed Immolate, native plan"}};}
combat::npcs::Definition definition(){
 combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
 d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{500,500};d.damageable=true;
 d.conditions_allowed=true;d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;return d;
}
ai::SpellTarget target(skills::effects::Scope scope,float distance=100){return {scope,distance,true,true,true,true};}
elemental::Pose pose(skills::effects::Scope scope,float x,std::uint64_t clock=0,bool alive=true){return {scope,x,0,0,alive,clock};}
elemental::Recipient recipient(skills::effects::Scope scope,float x,std::optional<int> armor=60,bool hostile=true,std::uint64_t clock=0){
 combat::Actor a;a.id=scope.actor;a.generation=scope.generation;a.health=combat::Health{500,500};a.damageable=true;a.fire_armor=armor;
 return {pose(scope,x,clock),a,{},hostile};
}
struct Fixture {
 combat::npcs::LiveActor npc{definition(),{1,1,1,146,100}};
 elemental::Ledger ledger;combat::npcs::Lifetime life=npc.lifetime();
 Fixture(std::size_t capacity=4,std::vector<elemental::Reviewed> reviewed=book()):ledger(source,20,std::move(reviewed),{100,32},capacity){check(npc.configure_spellbook(life,ledger.reviewed_spells()));}
 std::optional<combat::npcs::control::Token> admit(ai::SpellTarget facts,std::uint64_t now,const ai::Spellbook::Availability& ready={}){
  const auto chosen=npc.choose_spell(life,facts,now,ready);if(!chosen)return {};
  const auto reservation=ledger.reserve(*chosen,facts,now);if(!reservation)return {};
  const auto before=*npc.energy(life);const auto token=npc.begin_spell(life,*chosen,facts,now,ready);
  if(!token){check(ledger.abort(*reservation));return {};}
  const auto after=*npc.energy(life);
  check(ledger.accept(*reservation,*token,{before.current,after.current,after.maximum,now,token->sequence}));return token;
 }
 elemental::ReleaseProposal finish(const combat::npcs::control::Token& token,skills::effects::Scope selected,float x,std::uint64_t now,bool survives=true){
  const auto p=ledger.prepare_release(token,pose(source,0),pose(selected,x),*npc.control_epoch(life),true,now);
  check(p.status==elemental::ReleaseStatus::ready&&npc.complete_action(life,token,now));check(ledger.commit_release(p,survives));return p;
 }
};
int main()try{
 {Fixture f(4,{{{183,0},*skills::effects::elemental(183,12),"native Inferno; authored AI fixture"}});
  check(!f.admit(target(first,167),1000)&&f.npc.energy(f.life)->current==50);
  const auto token=*f.admit(target(first,166),1000);check(token.completes==1750&&f.npc.energy(f.life)->current==40);
  // Selected foe is not a mechanical target: even its disappearance/future
  // clock cannot move or cancel an untargeted burst at the caster's location.
  elemental::Pose missing;missing.clock=token.completes+1000;
  const auto release=f.ledger.prepare_release(token,pose(source,50),missing,*f.npc.control_epoch(f.life),false,token.completes);
  check(release.status==elemental::ReleaseStatus::ready&&release.release->aim_x==50&&release.release->arrives==token.completes);
  check(f.npc.complete_action(f.life,token,token.completes)&&f.ledger.commit_release(release,true));
  const auto unknown=f.ledger.prepare_impact(release.cast.use.id,source,{recipient(second,100),recipient({1,4,1},150,{})},token.completes);
  check(unknown.status==elemental::ImpactStatus::unsupported&&unknown.hits.empty()&&!f.ledger.commit_impact(unknown));
  const auto future=f.ledger.prepare_impact(release.cast.use.id,source,{recipient(second,100,60,true,token.completes+1)},token.completes);
  check(future.status==elemental::ImpactStatus::deferred&&future.hits.empty()&&!f.ledger.commit_impact(future));
  const auto hit=f.ledger.prepare_impact(release.cast.use.id,source,{recipient(second,216),recipient({1,4,1},217)},token.completes);
  check(hit.status==elemental::ImpactStatus::hit&&hit.hits.size()==1&&hit.hits[0].target==second&&
      hit.hits[0].result.actor.health->current==386&&f.ledger.commit_impact(hit));
  check(!f.npc.choose_spell(f.life,target(second),token.completes+9999)&&
      f.npc.choose_spell(f.life,target(second),token.completes+10000)==skills::Slot{183,0});
  check(elemental::wire::release_body(100,183)==combat::detail::integer(21,100,328));
 }
 check(elemental::dead_primary_anchor(*skills::effects::elemental(186,12))&&
       !elemental::dead_primary_anchor(*skills::effects::elemental(194,12))&&
       !elemental::dead_primary_anchor(*skills::effects::elemental(191,12)));
 {Fixture f(2);const auto fireball=*f.admit(target(first),1000);check(fireball.skill==skills::Slot{186,0}&&f.npc.energy(f.life)->current==40);
  const auto one=f.finish(fireball,first,100,fireball.completes);check(one.release&&f.ledger.in_flight()==1);
  const auto next=fireball.completes+750;
  const auto flare=*f.admit(target(second,600),next);check(flare.skill==skills::Slot{194,0}&&f.npc.energy(f.life)->current==35);
  const auto two=f.finish(flare,second,600,flare.completes);check(f.ledger.in_flight()==2);
  const auto energy=f.npc.energy(f.life)->current;check(!f.admit(target(second),flare.completes+750)&&f.npc.energy(f.life)->current==energy);
  std::vector<elemental::Recipient> contacts{recipient(first,100),recipient({1,4,1},266),recipient({1,5,1},267),recipient({1,6,1},110,60,false)};
  const auto impact=f.ledger.prepare_impact(one.cast.use.id,source,contacts,flare.completes);
  check(impact.status==elemental::ImpactStatus::hit&&impact.hits.size()==2);
  check(impact.hits[0].target==first&&impact.release.cast.use.plan.skill()==186&&impact.release.cast.use.plan.rank()==12);
  check(f.ledger.commit_impact(impact)&&!f.ledger.commit_impact(impact));
  const auto later=f.ledger.prepare_impact(two.cast.use.id,source,{recipient(second,600)},two.release->arrives);
  check(later.status==elemental::ImpactStatus::hit&&later.hits.size()==1&&later.hits[0].target==second&&later.release.cast.use.plan.skill()==194);
  check(f.ledger.commit_impact(later)&&f.ledger.in_flight()==0);
 }
 {Fixture f;const ai::Spellbook::Availability first_disabled=[](skills::Slot s,std::uint64_t)->std::optional<std::uint64_t>{return s.skill==186?10000:0;};
  const auto token=*f.admit(target(first),1000,first_disabled);check(token.skill==skills::Slot{194,0}&&f.npc.energy(f.life)->current==45);
  check(f.npc.cancel_action(f.life,1100)&&f.ledger.cancel(token,1100)&&f.npc.energy(f.life)->current==45&&f.ledger.in_flight()==0);
  check(!f.ledger.cancel(token,1100));
 }
 {Fixture f;const ai::Spellbook::Availability only_immolate=[](skills::Slot s,std::uint64_t)->std::optional<std::uint64_t>{return s.skill==191?0:10000;};
  const auto token=*f.admit(target(first),1000,only_immolate);check(token.skill==skills::Slot{191,17}&&f.npc.energy(f.life)->current==40);
  const auto release=f.finish(token,first,100,token.completes);
  const auto hit=f.ledger.prepare_impact(release.cast.use.id,source,{recipient(first,100)},token.completes);
  check(hit.status==elemental::ImpactStatus::hit&&hit.hits.size()==1&&hit.hits[0].result.conditions.has(combat::conditions::Kind::burning,token.completes));
  check(hit.hits[0].result.additions.size()==1&&f.ledger.commit_impact(hit));
 }
 {Fixture f;const auto token=*f.admit(target(first),1000);
  const auto future=f.ledger.prepare_release(token,pose(source,0,token.completes+100),pose(first,100),*f.npc.control_epoch(f.life),true,token.completes);
  check(future.status==elemental::ReleaseStatus::deferred&&f.ledger.pending()&&f.ledger.in_flight()==0&&!f.ledger.commit_release(future,true));
  const auto blocked=f.ledger.prepare_release(token,pose(source,0),pose(first,100),*f.npc.control_epoch(f.life),false,token.completes+100);
  check(blocked.status==elemental::ReleaseStatus::stopped&&f.npc.complete_action(f.life,token,token.completes+100)&&f.ledger.commit_release(blocked,true));
  check(f.npc.energy(f.life)->current==40&&f.ledger.in_flight()==0);
 }
 {Fixture f;const auto token=*f.admit(target(first),1000);f.finish(token,first,100,token.completes,false);
  check(f.ledger.in_flight()==0&&f.npc.energy(f.life)->current==40); // authoritative lethal reaction before effect, no refund
 }
 {Fixture f;const auto token=*f.admit(target(first),1000);const auto release=f.finish(token,first,100,token.completes);
  const auto now=release.release->arrives;
  const auto future=f.ledger.prepare_impact(release.cast.use.id,source,{recipient(first,100),recipient({1,4,1},150,60,true,now+100)},now);
  check(future.status==elemental::ImpactStatus::deferred&&future.hits.empty()&&!f.ledger.commit_impact(future)&&f.ledger.in_flight()==1);
  const auto unknown=f.ledger.prepare_impact(release.cast.use.id,source,{recipient(first,100),recipient({1,4,1},150,{})},now+100);
  check(unknown.status==elemental::ImpactStatus::unsupported&&unknown.hits.empty()&&!f.ledger.commit_impact(unknown));
  auto recycled=source;++recycled.generation;const auto stale=f.ledger.prepare_impact(release.cast.use.id,recycled,{recipient(first,100)},now+100);
  check(stale.status==elemental::ImpactStatus::stale&&f.ledger.commit_impact(stale));
 }
 {Fixture f;const auto token=*f.admit(target(first),1000);const auto release=f.finish(token,first,100,token.completes);
  const auto miss=f.ledger.prepare_impact(release.cast.use.id,source,{recipient(first,133)},release.release->arrives);
  check(miss.status==elemental::ImpactStatus::miss&&f.ledger.commit_impact(miss));
 }
 {Fixture f;const auto token=*f.admit(target(first),1000);const auto release=f.finish(token,first,100,token.completes);
  check(f.npc.apply_damage(f.life,{combat::Kind::armor_ignoring,1000,20,194},token.completes+1).killed);
  const auto hit=f.ledger.prepare_impact(release.cast.use.id,f.npc.hex_scope(),{recipient(first,100)},release.release->arrives);
  check(hit.status==elemental::ImpactStatus::hit&&f.ledger.commit_impact(hit)); // released, same-generation source death
 }
 {Fixture f;const auto token=*f.admit(target(first),1000);
  const auto ready=f.ledger.prepare_release(token,pose(source,0),pose(first,100),*f.npc.control_epoch(f.life),true,token.completes);
  check(f.npc.cancel_action(f.life,token.completes)&&f.ledger.cancel(token,token.completes));
  check(!f.ledger.commit_release(ready,true)&&f.ledger.in_flight()==0);
 }
 {elemental::Ledger ledger(source,20,book(),{100,32},1);const auto reservation=*ledger.reserve({194,0},target(first),1000);
  check(!ledger.reserve({194,0},target(second),1000));
  const combat::npcs::control::Token token{source,0,1,combat::npcs::control::Action::spell,1000,2000,skills::Slot{194,0}};
  check(!ledger.accept(reservation,token,{50,50,50,1000,1})&&!ledger.accept(reservation,token,{50,45,50,1000,2}));
  check(ledger.abort(reservation)&&!ledger.abort(reservation)&&!ledger.accept(reservation,token,{50,45,50,1000,1}));
  check(!ledger.reserve({191,0},target(first),1001)); // only reviewed copy17 exists
 }
 {Fixture f;const auto reservation=*f.ledger.reserve({194,0},target(first),1000);
  check(f.npc.lose_energy(f.life,49)&&!f.npc.begin_spell(f.life,{194,0},target(first),1000));
  check(f.ledger.abort(reservation)&&f.npc.energy(f.life)->current==1&&f.ledger.in_flight()==0&&!f.ledger.pending());
 }
 {Fixture f;const auto token=*f.admit(target(first),1000);
  const auto released=f.ledger.prepare_release(token,pose(source,0),pose(first,100,0,false),*f.npc.control_epoch(f.life),true,token.completes);
  check(released.status==elemental::ReleaseStatus::ready&&f.npc.complete_action(f.life,token,token.completes)&&f.ledger.commit_release(released,true));
  auto corpse=recipient(first,100);corpse.pose.alive=false;corpse.actor.health->current=0;
  auto recycled=corpse;++recycled.pose.scope.generation;++recycled.actor.generation;
  const auto stale=f.ledger.prepare_impact(released.cast.use.id,source,{recycled,recipient(second,200)},released.release->arrives);
  check(stale.status==elemental::ImpactStatus::stale&&stale.hits.empty());
  const auto hit=f.ledger.prepare_impact(released.cast.use.id,source,{corpse,recipient(second,200)},released.release->arrives);
  check(hit.status==elemental::ImpactStatus::hit&&hit.hits.size()==1&&hit.hits[0].target==second&&f.ledger.commit_impact(hit));
 }
 std::cout<<"PASS multi-skill NPC paid casts,capacity-before-payment,independent flights,Fireball area,Immolate burning,retargeting and atomic proposals\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
