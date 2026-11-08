#include "gw_parasitic_npc_timeline.h"
#include "gw_health_regeneration.h"
#include "gw_damage_fields.h"
#include <iostream>
#include <source_location>
#include <string>
#include <stdexcept>
using namespace fw::gw;
using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Parasitic NPC timeline regression line "+std::to_string(at.line()));}
combat::npcs::LiveActor npc(unsigned id,unsigned hp){combat::npcs::Definition d;d.binding.map=146;d.binding.actor=id;d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{double(hp),100};d.damageable=true;d.conditions_allowed=true;return {d,{1,1,1,146,id}};}
int main()try{
 const Scope caster{1,1,1};
 for(bool reverse:{false,true}){
  std::map<unsigned,combat::npcs::LiveActor> live;
  live.emplace(100,npc(100,reverse?10:30));live.emplace(200,npc(200,reverse?30:10));
  std::vector<ParasiticBonds::Link> links;
  for(auto&[id,n]:live){check(n.apply_parasitic_bond(n.lifetime(),0,1,id,100).has_value());links.push_back({caster,n.hex_scope(),id,0,100,20100});}
  auto lookup=[&](Scope target)->std::optional<combat::npcs::LiveActor>{auto it=live.find(target.actor);return it==live.end()?std::nullopt:std::optional(it->second);};
  auto f=forecast_parasitic_npcs(links,40100,lookup);
  check(f.reason==BondForecastReason::okay&&f.next&&f.next->kind==BondBoundaryKind::npc_death&&f.next->at==5100&&f.next->target.actor==(reverse?100u:200u));
  check(live.at(100).actor(live.at(100).lifetime())->health->current==(reverse?10:30));
  // Commit only the selected death. Reforecast must find the later death,
  // without depending on NPC ID order or consuming the20second expiry first.
  const auto target=f.next->target;check(live.at(target.actor).advance_conditions(live.at(target.actor).lifetime(),f.next->at)->killed);
  links.erase(std::remove_if(links.begin(),links.end(),[&](const auto& l){return l.target==target;}),links.end());
  f=forecast_parasitic_npcs(links,40100,lookup);check(f.next&&f.next->kind==BondBoundaryKind::npc_death&&f.next->at==15100);
 }
 {
  auto n=npc(100,100);check(n.apply_parasitic_bond(n.lifetime(),0,1,1,100).has_value());
  std::vector<ParasiticBonds::Link> links{{caster,n.hex_scope(),1,0,100,20100}};
  auto lookup=[&](Scope)->std::optional<combat::npcs::LiveActor>{return n;};
  auto f=forecast_parasitic_npcs(links,10100,lookup);check(f.reason==BondForecastReason::okay&&!f.next);
  f=forecast_parasitic_npcs(links,40100,lookup);check(f.next&&f.next->kind==BondBoundaryKind::expiry&&f.next->at==20100);
  n.advance_conditions(n.lifetime(),30100);f=forecast_parasitic_npcs(links,40100,lookup);check(f.reason==BondForecastReason::clock);
 }
 {
  auto n=npc(100,40);check(n.apply_parasitic_bond(n.lifetime(),0,1,1,100).has_value());
  std::vector<ParasiticBonds::Link> links{{caster,n.hex_scope(),1,0,100,20100}};
  auto f=forecast_parasitic_npcs(links,40100,[&](Scope)->std::optional<combat::npcs::LiveActor>{return n;});
  check(f.next&&f.next->kind==BondBoundaryKind::npc_death&&f.next->at==20100);
  links[0].expires=20099;check(forecast_parasitic_npcs(links,40100,[&](Scope)->std::optional<combat::npcs::LiveActor>{return n;}).reason==BondForecastReason::link);
 }
 {
  auto n=npc(100,40);check(n.apply_parasitic_bond(n.lifetime(),0,1,1,1100).has_value());
  const Scope player{1,2,1};
  std::vector<ParasiticBonds::Link> links{{caster,n.hex_scope(),1,0,1100,21100},{caster,player,2,0,0,20000}};
  auto lookup=[&](Scope target)->std::optional<combat::npcs::LiveActor>{return target==n.hex_scope()?std::optional(n):std::nullopt;};
  auto f=forecast_parasitic_npcs(links,40100,lookup);
  check(f.next&&f.next->kind==BondBoundaryKind::expiry&&f.next->target==player&&f.next->at==20000);
  links.pop_back();f=forecast_parasitic_npcs(links,40100,lookup);
  check(f.next&&f.next->kind==BondBoundaryKind::npc_death&&f.next->at==21100);
  auto wrong=links;wrong[0].target.generation++;
  check(forecast_parasitic_npcs(wrong,40100,[&](Scope)->std::optional<combat::npcs::LiveActor>{return n;}).reason==BondForecastReason::lifetime);
  wrong=links;wrong[0].effect++;
  check(forecast_parasitic_npcs(wrong,40100,lookup).reason==BondForecastReason::effect);
  wrong=links;wrong[0].rank=12;
  check(forecast_parasitic_npcs(wrong,40100,lookup).reason==BondForecastReason::effect);
 }
 {
  // A field pulse precedes the degeneration death forecast. Stop at that
  // external boundary, commit the pulse, then reforecast the changed Health.
  auto n=npc(100,10);check(n.apply_parasitic_bond(n.lifetime(),0,1,1,100).has_value());
  ParasiticBonds bonds;check(bonds.add({caster,n.hex_scope(),1,0,100,20100}));
  DamageFieldSchedule fields;check(!fields.next_deadline());
  check(fields.add(caster,20,0,0,0,*damage_field(247,0),100));
  check(fields.next_deadline()==1100);
  const auto snapshot=bonds.pending();
  auto lookup=[&](Scope)->std::optional<combat::npcs::LiveActor>{return n;};
  const auto horizon=std::min(Time(40100),*fields.next_deadline());
  auto f=forecast_parasitic_npcs(snapshot,horizon,lookup);
  check(f.reason==BondForecastReason::okay&&!f.next&&bonds.size()==1);
  fields.advance(horizon,[&](const auto& pulse){
   check(pulse.at==1100);const auto update=n.advance_conditions(n.lifetime(),pulse.at);
   check(update&&!update->killed&&n.actor(n.lifetime())->health->current==8);
   // The incoming8damage pulse is lethal at1100,not the obsolete5100
   // degeneration prediction. Cancel subsequent pulses for this fixture.
   const auto hit=n.apply_damage(n.lifetime(),{combat::Kind::armor_ignoring,double(pulse.plan.damage),pulse.level,pulse.plan.skill},pulse.at);
   check(hit.killed&&n.actor(n.lifetime())->health->current==0);
   const auto returned=bonds.end(n.hex_scope(),1,pulse.at,true);
   check(returned&&returned->at==1100&&returned->healing==30);
   return false;
  });
  check(!fields.next_deadline()&&bonds.pending().empty());
  check(snapshot.size()==1&&snapshot[0].effect==1); // No dangling live view.
  check(bonds.expire(40100).empty());
 }
 std::cout<<"PASS read-only NPC death forecast before return expiry and external pulse boundaries,in both actor orders;world orchestration pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
