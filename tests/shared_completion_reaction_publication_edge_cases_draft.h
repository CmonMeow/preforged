#pragma once
#include "shared_completion_reaction_publication_fixture_draft.h"
inline void run_completion_publication_edges(){
 // Actual outer owner: accepted state transition with no child publication.
 {CompletionPublicationFixture f;world::Outbox::DeferredQueue outer;std::function<void()> publication;
  {world::Outbox::Capture capture(*f.box,outer);publication_check(f.w.completed_skill_action(f.owner,1375,1000,nullptr,&publication));publication_check(!publication&&f.box->drain().empty());publication_check(f.w.resources(1)->health==100+f.vigor.healing);}
  publication_check(f.box->drain().empty()&&!outer.packets.empty());f.box->enqueue(std::move(outer.packets));const auto packets=f.box->drain();const auto heal=particles::health_change_2022(1,1,f.vigor.healing,1000);publication_check(std::count(packets.begin(),packets.end(),heal)==1);
 }
 // A REAL refused World reaction still owns causal resource/death output.
 {CompletionPublicationFixture f(1375,1);publication_check(f.w.apply_condition(f.owner,f.owner,combat::conditions::Kind::burning,100,10000));f.box->drain();std::function<void()> publication;
  publication_check(!f.w.completed_skill_action(f.owner,1375,1100,nullptr,&publication));publication_check(f.w.resources(1)->health==0&&bool(publication)&&f.box->drain().empty());publication();publication_check(!f.box->drain().empty());publication();publication_check(f.box->drain().empty());
 }
 // Genuine World glyph payment/lease; inject refusal at the LATER Renewal
 // finalizer after producer and real254 state committed. No success is forged.
 {CompletionPublicationFixture f;publication_check(f.w.energy_capacity(1,6,0,100,{20,-2}));publication_check(f.w.apply_elemental_glyph(f.owner,900,*effects::elemental_glyph(203,0),100));unsigned glyph_paid=0,terminal_attempts=0;
  f.h.elemental_glyph_snapshot=[&](auto at){return f.w.elemental_glyph_snapshot(f.owner,at);};
  f.h.pay_elemental_glyph=[&](auto slot,auto cost,auto event,auto at,const auto& expected){auto quote=f.w.pay_elemental_glyph_use(f.owner,slot,cost,event,at,expected);if(quote){++glyph_paid;publication_check(quote->energy_cost==10);}return quote;};
  f.h.settle_elemental_glyph=[&](auto event,auto outcome,auto at){return f.w.settle_elemental_glyph_use(f.owner,event,outcome,at);};f.h.glyph_clock=[&](){return f.clock;};
  f.h.control_epoch=[&](){return f.w.control_epoch(f.owner);};f.h.control_event_time=[&](auto epoch){return f.w.control_event_time(f.owner,epoch);};f.h.cooldown_control_source=[&](auto after){return f.w.cooldown_control_source(f.owner,after);};
  f.h.finish_renewal_spell=[&](auto,auto,auto,auto,auto,auto,auto,auto)->std::optional<effects::RenewalTerminal>{++terminal_attempts;publication_check(f.completed==1&&f.w.resources(1)->health==100+f.vigor.healing);f.collect_early();return {};};
  f.box->drain();f.clock=1000;publication_check(f.runtime.start(1000,{1375,0,1},f.bar,f.policy,f.ranks,true,f.h)==casting::Reason::accepted);publication_check(glyph_paid==1&&f.w.resources(1)->energy==30);f.box->drain();f.update(3000);auto packets=f.early;auto tail=f.box->drain();packets.insert(packets.end(),tail.begin(),tail.end());
  publication_check(terminal_attempts==1&&f.completed==1&&f.effects_committed==1&&!f.runtime.active());const auto heal=particles::health_change_2022(1,1,f.vigor.healing,1000);publication_check(std::count(packets.begin(),packets.end(),heal)==1);
  publication_check(std::find(packets.begin(),packets.end(),casting::terminal(58,1))==packets.end());const auto status=*f.w.status_snapshot(f.owner,3000);publication_check(std::any_of(status.begin(),status.end(),[](const auto& e){return e.skill==1375;}));
  f.update(3000);f.update(3001);publication_check(f.completed==1&&terminal_attempts==1&&f.box->drain().empty()&&f.w.resources(1)->health==100+f.vigor.healing);
 }
 std::cout<<"PASS nested empty receipt, real refused death settlement and late finalizer once-publication fallback\n";
}
