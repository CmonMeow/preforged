#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("Selfless payment line "+std::to_string(at.line()));
}
struct Fixture{
 world::Instance w;Scope owner,ally,foe;
 Fixture(unsigned alias=1952,unsigned ally_team=1){
  for(unsigned id:{1u,2u,3u}){
   w.join(id,"Actor",float(id*10),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:id==2?ally_team:1);
   w.set_vitals(id,{1000,1000,100,100},100);
  }
  owner=*w.effect_scope(1);ally=*w.effect_scope(2);foe=*w.effect_scope(3);
  check(w.complete_selfless_spirit(owner,*selfless_spirit(alias),0,1000));
 }
};
int main()try{
 for(unsigned alias:{1952u,2095u}){
  {Fixture f(alias,0);
   check(f.w.consume_selfless_energy(f.owner,{194,0},f.ally,10,1100));
   check(f.w.resources(1)->energy==90); // Missing party membership is not an alliance.
  }
  for(unsigned mode=0;mode<6;++mode){
   Fixture f(alias);auto target=std::optional<Scope>{f.ally};
   if(mode==1)target=f.owner;if(mode==2)target=f.foe;if(mode==3)target.reset();
   const unsigned skill=mode==4?198:194;const unsigned cost=mode==5?2:10;
   check(f.w.consume_selfless_energy(f.owner,{skill,0},target,cost,1100));
   check(f.w.resources(1)->energy==100-((mode==0||mode==5)?(cost>3?cost-3:0):cost));
  }
  {Fixture f(alias);auto stale=f.ally;++stale.generation;
   check(!f.w.consume_selfless_energy(f.owner,{194,0},stale,10,1100));
   check(f.w.resources(1)->energy==100);
   check(f.w.consume_selfless_energy(f.owner,{194,0},f.ally,10,16000));
   check(f.w.resources(1)->energy==90);
  }
  {Fixture f(alias);
   check(f.w.activate_selfless_overcast_energy(f.owner,219,f.ally,10,1,1100));
   check(f.w.resources(1)->energy==93);
   check(f.w.overcast_energy_snapshot(f.owner)->curve.burden_units()==500000);
   check(!f.w.activate_selfless_overcast_energy(f.owner,219,f.ally,10,1,1100));
  }
  for(unsigned glyph:{198u,199u,203u}){
   Fixture f(alias);check(f.w.apply_elemental_glyph(f.owner,50,*elemental_glyph(glyph),1001));
   const auto state=f.w.elemental_glyph_snapshot(f.owner,1100);check(bool(state));
   const auto paid=f.w.pay_selfless_elemental_glyph_use(f.owner,{194,0},f.ally,20,1,1100,state);
   check(paid&&paid->energy_cost==(glyph==199?7u:17u));
   check(f.w.resources(1)->energy==100-paid->energy_cost);
   check(!f.w.pay_selfless_elemental_glyph_use(f.owner,{194,0},f.ally,20,2,1100,state));
   check(f.w.settle_elemental_glyph_use(f.owner,1,GlyphOutcome::completed,1200).accepted);
   check(!f.w.settle_elemental_glyph_use(f.owner,1,GlyphOutcome::completed,1200).accepted);
  }
 }
 std::cout<<"PASS Selfless World payment alias/semantic target/floor/expiry/stale/Overcast/glyph lease cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


