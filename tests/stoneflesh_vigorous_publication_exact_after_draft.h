#pragma once
// Exact BEFORE assertions retained; only the completed_action callback opts into the typed publication API.
#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
static void stoneflesh_after_check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Stoneflesh/Vigorous publication line "+std::to_string(at.line()));
}
inline void run_stoneflesh_publication_exact_after(){
 auto store=std::make_unique<world::Instance>();auto& world=*store;auto box=std::make_shared<world::Outbox>();
 world.join(1,"Stone Vigor",0,0,box,0,{}, {},100,1);world.set_vitals(1,{100,1000,40,40},100);
 const auto owner=*world.effect_scope(1);const auto vigor=*effects::vigorous_spirit(0);
 stoneflesh_after_check(world.complete_vigorous_spirit(owner,owner,vigor,0,100));
 Slots bar{};bar[0]={1375,0};stoneflesh_after_check(world.configure_adrenaline(owner,bar,100));
 Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[1375]=1;attributes::Ranks ranks{};
 const auto equipment=*items::weapon_passives::equipped_enchanting(inventory::Profile{},1);
 auto quote=[&](unsigned skill,std::uint64_t base,std::uint64_t at){
  const auto status=world.status_snapshot(owner,at);
  return status?effects::enchanting_duration(skill,base,owner,equipment,*status,at):std::nullopt;
 };
 casting::Runtime runtime(1);casting::Hooks hooks;unsigned payments=0,paid=0,completed=0,effects_committed=0;
 hooks.resources=[&](auto actor){return world.resources(actor);};
 hooks.events=[&](const auto& batch){box->enqueue(batch);};
 hooks.target_scope=[&](auto actor){return world.effect_scope(actor);};
 hooks.target_valid=[](auto actor,float,bool self){return actor==1&&self;};
 hooks.consume_energy_at=[&](unsigned cost,auto at){++payments;paid+=cost;return world.consume_energy(1,cost,owner,at);};
 hooks.effect_snapshot=[&](auto at){return world.status_snapshot(owner,at);};
 hooks.before_spell_effect=[&](auto skill,auto event,auto at){return world.before_spell_effect(owner,skill,event,at);};
 hooks.enchantment_duration_ready=[&](auto skill,auto base,auto at){return bool(quote(skill,base,at));};
 hooks.simple_enchantment_completed=[&](auto target,auto skill,auto rank,unsigned,auto event,auto at)->casting::EffectCompletion{
  stoneflesh_after_check(target==owner&&skill==1375);const auto plan=effects::stoneflesh_aura(rank);
  const auto duration=plan?quote(skill,plan->duration,at):std::nullopt;
  casting::EffectCompletion result;std::uint64_t completed_at=0;
  result.accepted=duration&&world.stoneflesh_aura_enchantment(owner,rank,at,event,&*duration,&completed_at,&result.publication);
  if(result.accepted){++effects_committed;result.effect_snapshot_at=completed_at;}return result;
 };
 hooks.completed_action=[&](auto skill,auto at)->casting::ActionCompletion{++completed;casting::ActionCompletion result;result.accepted=world.completed_skill_action(owner,skill,at,nullptr,&result.publication);return result;};
 hooks.begin_action=[&](auto slot,auto at){stoneflesh_after_check(world.accepted_skill_start(owner,slot,at));};
 box->drain();stoneflesh_after_check(runtime.start(1000,{1375,0,999},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
 stoneflesh_after_check(payments==1&&paid==10&&world.resources(1)->energy==30);box->drain();
 runtime.update(2999,hooks);stoneflesh_after_check(effects_committed==0&&completed==0&&world.resources(1)->health==100);box->drain();
 runtime.update(3000,hooks);const auto packets=box->drain();
 stoneflesh_after_check(effects_committed==1&&completed==1&&payments==1&&paid==10);
 stoneflesh_after_check(world.resources(1)->health==100+vigor.healing);
 const auto heal_packet=particles::health_change_2022(1,1,vigor.healing,1000);
 stoneflesh_after_check(std::count(packets.begin(),packets.end(),heal_packet)==1);
 const auto recharge=std::find(packets.begin(),packets.end(),casting::slot_event(228,1,{1375,0},15));
 const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
 const auto body=std::find(packets.begin(),packets.end(),casting::generic(21,1,1184));
 const auto heal=std::find(packets.begin(),packets.end(),heal_packet);
 stoneflesh_after_check(recharge!=packets.end()&&finish!=packets.end()&&body!=packets.end()&&heal!=packets.end());
 stoneflesh_after_check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
 stoneflesh_after_check(std::count(packets.begin(),packets.end(),casting::generic(21,1,1184))==1);
 runtime.update(3000,hooks);runtime.update(3001,hooks);const auto replay=box->drain();
 stoneflesh_after_check(effects_committed==1&&completed==1&&payments==1&&world.resources(1)->health==100+vigor.healing);
 stoneflesh_after_check(std::find(replay.begin(),replay.end(),heal_packet)==replay.end());
 std::cout<<"positions recharge="<<recharge-packets.begin()<<" finish="<<finish-packets.begin()<<" body="<<body-packets.begin()<<" heal="<<heal-packets.begin()<<'\n';
 // Existing deferred-recipient publication contract; no new retail timing claim.
 stoneflesh_after_check(recharge<finish&&finish<body&&body<heal);
 std::cout<<"PASS typed1375 paid completion254 Health reaction follows native prefix once\n";
}
