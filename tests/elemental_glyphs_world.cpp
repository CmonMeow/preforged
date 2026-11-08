#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Glyph World line "+std::to_string(at.line()));}
int main()try{
 for(unsigned skill:{198u,199u,203u}){
  world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto owner=*w.effect_scope(1);
  const auto plan=*elemental_glyph(skill,0);check(w.apply_elemental_glyph(owner,1,plan,1000));
  auto state=w.elemental_glyph_snapshot(owner,1000);check(state&&state->remaining==plan.charges&&state->expires==(plan.duration?std::optional(1000+*plan.duration):std::nullopt));
  const auto wire=box->drain();check(std::any_of(wire.begin(),wire.end(),[&](const auto& p){return p.size()==20u&&p[0]==(skill==199?64:65);}));
  check(!w.has_enchantment(owner,1,1000));
  check(!w.reserve_elemental_glyph_use(owner,{198,0},5,0,1,1100));
  for(unsigned i=0;i<plan.charges;++i){
   const auto quote=w.reserve_elemental_glyph_use(owner,{194,0},10,5,i+1,1100+i*100);
   check(quote&&quote->energy_cost==(skill==199?0u:10u)&&quote->base_overcast==(skill==199?0u:5u));
   check(!w.reserve_elemental_glyph_use(owner,{194,0},10,5,99,1100+i*100));
   check(w.elemental_glyph_snapshot(owner,1100+i*100)->remaining==plan.charges-i);
   check(!w.settle_elemental_glyph_use(owner,99,GlyphOutcome::completed,1150+i*100).accepted);
   const auto result=w.settle_elemental_glyph_use(owner,i+1,GlyphOutcome::completed,1150+i*100);
   check(result.accepted&&result.consumed&&result.instant_recharge==(skill==203));
   check(!w.settle_elemental_glyph_use(owner,i+1,GlyphOutcome::completed,1150+i*100).accepted);
   check(!w.reserve_elemental_glyph_use(owner,{194,0},10,5,i+1,1160+i*100));
  }
  check(!w.elemental_glyph_snapshot(owner,3000));
  check(w.apply_elemental_glyph(owner,2,plan,4000));
  check(w.reserve_elemental_glyph_use(owner,{194,0},10,0,88,4100).has_value());
  check(w.status_mutation(owner,200,3,0,4200,19200,true));
  check(!w.elemental_glyph_snapshot(owner,4200)&&!w.settle_elemental_glyph_use(owner,88,GlyphOutcome::completed,4300).accepted);
  check(w.apply_elemental_glyph(owner,4,plan,5000));
  const auto loss=w.health_sacrifice(owner,115,5100); // nonlethal fixture, then explicitly lethal resource state
  w.set_vitals(1,{1,1000,100,100},100);check(w.health_sacrifice(owner,115,5300)->killed);
  check(!w.elemental_glyph_snapshot(owner,5300)&&!w.status_snapshot(owner,5300)->size());
 }
 {world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto owner=*w.effect_scope(1);
  check(w.apply_elemental_glyph(owner,1,*elemental_glyph(199,0),1000));check(w.elemental_glyph_snapshot(owner,1000000000).has_value());
  auto stale=owner;++stale.generation;check(!w.apply_elemental_glyph(stale,2,*elemental_glyph(198,0),2000));
 }
 {world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto owner=*w.effect_scope(1);
  check(w.status_mutation(owner,346,1,0,1000,9000,true));
  check(!w.apply_elemental_glyph(owner,1,*elemental_glyph(199,0),1100));
  check(w.status_snapshot(owner,1100)->front().skill==346);
  check(w.apply_elemental_glyph(owner,2,*elemental_glyph(199,0),1200));
  check(w.reserve_elemental_glyph_use(owner,{194,0},10,0,1,1300).has_value());
  check(w.status_mutation(owner,199,2,0,1400,{},false));
  check(w.status_mutation(owner,346,2,0,1500,9500,true));
  check(!w.elemental_glyph_snapshot(owner,1500)&&!w.reserve_elemental_glyph_use(owner,{194,0},10,0,2,1500));
  check(!w.settle_elemental_glyph_use(owner,1,GlyphOutcome::completed,1500).accepted);
  check(w.status_snapshot(owner,1500)->front().skill==346);
 }
 for(bool interrupted:{false,true}){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);const auto owner=*w.effect_scope(1);
  check(w.apply_elemental_glyph(owner,1,*elemental_glyph(203),1000));
  check(w.reserve_elemental_glyph_use(owner,{219,7},5,5,10,1100).has_value());
  const auto state=*w.elemental_glyph_snapshot(owner,1100);
  std::optional<std::uint64_t> control;const auto outcome=interrupted?GlyphOutcome::interrupted:GlyphOutcome::completed;
  if(interrupted){check(w.cast_activity(owner,fw::gw::skills::Slot{219,7},1100,3000));check(w.interrupt_cast(owner,219,1200));control=*w.control_epoch(owner);}
  else check(w.prepare_diversion_skill_use(owner,{219,7},20,1500));
  const auto use=interrupted?0:20;
  auto stale=owner;++stale.generation;
  check(!w.finish_renewal_spell(stale,10,use,{219,7},outcome,1500,5000,9000,control));
  check(!w.finish_renewal_spell(owner,11,use,{219,7},outcome,1500,5000,9000,control));
  check(!w.finish_renewal_spell(owner,10,use,{219,0},outcome,1500,5000,9000,control));
  check(!w.finish_renewal_spell(owner,10,use,{219,7},GlyphOutcome::failed,1500,5000,9000,control));
  check(w.elemental_glyph_snapshot(owner,1500)==state&&!w.cooldown_completion_stamp(owner,interrupted?10:20));
  const auto receipt=w.finish_renewal_spell(owner,10,use,{219,7},outcome,1500,5000,9000,control);
  check(receipt&&receipt->glyph_effect==1&&receipt->control_epoch==control.value_or(0)&&receipt->completion.instant_recharge&&
   receipt->completion.slot==fw::gw::skills::Slot{219,7}&&receipt->completion.base_ms==5000&&receipt->completion.existing_until==9000&&completion_deadline(receipt->completion)==1500);
  check(!w.elemental_glyph_snapshot(owner,1500)&&w.cooldown_completion_stamp(owner,interrupted?10:20)==receipt->completion);
  check(!w.finish_renewal_spell(owner,10,use,{219,7},outcome,1500,5000,9000,control));
 }
 std::cout<<"PASS authoritative glyph lifetime,untimed wire,reservation,terminal charge,replay,replacement,death and atomic Renewal terminal provenance\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
