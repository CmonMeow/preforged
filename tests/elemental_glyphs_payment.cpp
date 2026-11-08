#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool yes,std::source_location at=std::source_location::current()){
 if(!yes)throw std::runtime_error("Glyph payment line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned skill:{198u,199u,203u}){
  world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
  w.set_vitals(1,{500,500,30,30},100);const auto owner=*w.effect_scope(1);
  check(w.apply_elemental_glyph(owner,1,*elemental_glyph(skill,0),1000));
  check(!w.pay_elemental_glyph_use(owner,{198,0},5,1,1000));
  check(!w.pay_elemental_glyph_use(owner,{194,0},100,1,1000));
  check(w.resources(1)->energy==30);
  const auto quote=w.pay_elemental_glyph_use(owner,{185,0},15,1,1000);
  check(quote&&quote->energy_cost==(skill==199?5u:15u)&&quote->base_overcast==(skill==199?0u:5u));
  const auto paid=*w.overcast_energy_snapshot(owner);
  check(paid.curve.burden_units()==(skill==199?0u:500000u));
  check(w.elemental_glyph_snapshot(owner,1000)->remaining==(skill==198?10u:1u));
  const auto before=overcast::precise_current(paid);
  check(!w.pay_elemental_glyph_use(owner,{194,0},5,2,1000));
  check(overcast::precise_current(*w.overcast_energy_snapshot(owner))==before);
  check(w.settle_elemental_glyph_use(owner,1,GlyphOutcome::completed,1000).accepted);
  check(!w.pay_elemental_glyph_use(owner,{194,0},5,1,1000));
 }
 for(bool lethal:{false,true}){
  world::Instance w;
  for(unsigned id:{1u,2u}){w.join(id,"Actor",id==1?0:60,0,std::make_shared<world::Outbox>(),0,{}, {},100,id);
   w.set_vitals(id,{id==1&&lethal?1u:500u,500,id==1?5u:30u,30},100);}
  const auto owner=*w.effect_scope(1),foe=*w.effect_scope(2);
  check(w.apply_elemental_glyph(owner,1,*elemental_glyph(198),1000));
  w.set_vitals(1,{lethal?1u:500u,500,5,30},100);
  check(w.apply_mind_wrack(foe,owner,12,false,1000));
  const auto quote=w.pay_elemental_glyph_use(owner,{194,0},5,1,1000);
  check(bool(quote)!=lethal&&w.resources(1)->energy==0);
  if(lethal){check(w.resources(1)->health==0&&!w.elemental_glyph_snapshot(owner,1000));
   check(!w.settle_elemental_glyph_use(owner,1,GlyphOutcome::completed,1000).accepted);}
  else check(w.resources(1)->health==417&&w.settle_elemental_glyph_use(owner,1,GlyphOutcome::completed,1000).accepted);
 }
 {world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{500,500,30,30},100);const auto owner=*w.effect_scope(1);
  check(w.apply_elemental_glyph(owner,1,*elemental_glyph(199),1000));const auto old=*w.elemental_glyph_snapshot(owner,1000);
  check(w.apply_elemental_glyph(owner,2,*elemental_glyph(198),1000));
  check(!w.pay_elemental_glyph_use(owner,{194,0},5,1,1000,old)&&w.resources(1)->energy==30);
  const auto current=*w.elemental_glyph_snapshot(owner,1000);
  check(w.pay_elemental_glyph_use(owner,{194,0},5,1,1000,current).has_value()&&w.resources(1)->energy==25);
 }
 std::cout<<"PASS atomic glyph payment,reservation,base Overcast,replay and zero-Energy reaction\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
