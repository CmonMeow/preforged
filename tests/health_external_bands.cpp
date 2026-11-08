#include "gw_health_regeneration.h"
#include "gw_ward_harm_exposure.h"
#include <iostream>
#include <source_location>
#include <string>
#include <limits>
using namespace fw::gw::skills;using namespace effects;
using regeneration::State;using regeneration::ExternalPips;
void check(bool ok,std::source_location at=std::source_location::current()){
 if(!ok)throw std::runtime_error("External Health line "+std::to_string(at.line()));
}
std::vector<ExternalPips> convert(std::span<const WardRegenerationBand> bands,double from,double through){
 std::vector<ExternalPips> out;for(const auto& b:bands){const double a=std::max(from,b.from),z=std::min(through,b.through);if(a<z)out.push_back({a,z,int(b.pips)});}return out;
}
int main()try{unsigned cases=0;const Scope owner{1,30,1};
 for(unsigned rank=0;rank<=20;++rank)for(bool dense:{false,true}){
  State state(owner,0);double health=100;
  check(state.apply(owner,{1,30,886,10,0,5000}));
  GroundAreaRecipient r{owner,2,0,0,0,true,false};
  const std::array wards{*ward_against_harm({1,25,1},80,2,0,0,0,*ward_harm_plan(239,rank),0)};
  std::vector<TimedGroundAreaLeg> legs{{r,0,0,0,1000},{r,600,0,1000,1000}};r.x=600;legs.push_back({r,600,0,1000,2000});
  const auto bands=ward_harm_regeneration_bands(wards,legs);check(bool(bands));
  const auto step=dense?100:2000;
  for(unsigned end=step;end<=2000;end+=step){const auto selected=convert(*bands,state.cursor(),end);
   const auto result=state.advance_external(owner,end,health,1000,selected,-11);check(result&&!result->death_at);}
  // Restful10 plus Ward1..4 and degeneration-11 must be summed BEFORE cap.
  const double expected=100+2*(int(wards[0].plan.regeneration)-1)-2;
  check(std::abs(health-expected)<1e-8&&state.cursor()==2000);++cases;
 }
 {State state(owner,0);double health=100;GroundAreaRecipient r{owner,2,0,-323,0,true,false};
  const std::array wards{*ward_against_harm({1,25,1},80,2,0,0,0,*ward_harm_plan(239,0),0)};
  const std::array legs{TimedGroundAreaLeg{r,-321,0,0,1},TimedGroundAreaLeg{{owner,2,0,-321,0,true,false},-321,0,1,1000}};
  const auto bands=ward_harm_regeneration_bands(wards,legs);check(bool(bands));const auto ext=convert(*bands,0,1000);
  check(state.advance_external(owner,1000,health,1000,ext).has_value());check(std::abs(health-101.999)<1e-8);
  State dense(owner,0);double sampled=100;for(unsigned end=100;end<=1000;end+=100){const auto slice=convert(*bands,dense.cursor(),end);check(dense.advance_external(owner,end,sampled,1000,slice).has_value());}
  check(std::abs(sampled-health)<1e-8);++cases;
 }
 {State state(owner,0);double health=100;check(state.apply(owner,{1,30,288,8,0,1500}));
  const std::array ext{ExternalPips{0,1000,4},ExternalPips{1000,2000,0}};
  check(state.advance_external(owner,2000,health,1000,ext).has_value());check(health==128); //20 capped +8 until owned expiry1500.
  check(!state.lookup(owner,1));++cases;
 }
 {State state(owner,0);double health=999;const std::array ext{ExternalPips{0,1000,10},ExternalPips{1000,2000,-1}};
  check(state.advance_external(owner,2000,health,1000,ext).has_value());check(health==998);++cases;
 }
 {State state(owner,0);double health=1;const std::array ext{ExternalPips{0,1000,-1},ExternalPips{1000,2000,10}};
  const auto result=state.advance_external(owner,2000,health,1000,ext);check(result&&result->death_at==500&&health==0&&state.cursor()==2000);++cases;
 }
 {State state(owner,0);double health=.001;const std::array ext{ExternalPips{0,1,-1}};
  const auto result=state.advance_external(owner,1,health,1000,ext);check(result&&result->death_at&&std::abs(*result->death_at-.5)<1e-9&&health==0);++cases;
 }
 {State state(owner,0);double health=0;const std::array ext{ExternalPips{0,1000,10}};
  const auto result=state.advance_external(owner,1000,health,1000,ext);check(result&&!result->death_at&&health==0);++cases;
 }
 for(unsigned mode=0;mode<10;++mode){State state(owner,0);double health=100;check(state.apply(owner,{1,30,886,10,0,5000}));
  std::vector<ExternalPips> ext{{0,500,1},{500,1000,2}};auto scope=owner;unsigned maximum=1000;int other=0;
  if(mode==0)ext[1].begins=501;if(mode==1)ext[1].begins=499;if(mode==2)ext[1].ends=999;
  if(mode==3)ext[0].pips=1001;if(mode==4)++scope.generation;if(mode==5)other=-1001;
  if(mode==6)ext[1].ends=std::numeric_limits<double>::quiet_NaN();if(mode==7)maximum=0;
  if(mode==8)ext.clear();if(mode==9)ext.resize(65537);
  check(!state.advance_external(scope,1000,health,maximum,ext,other));
  check(health==100&&state.cursor()==0&&state.lookup(owner,1).has_value());++cases;
 }
 {State state(owner,0);double health=100;check(state.advance_external(owner,0,health,1000,{}).has_value());
  const std::array ext{ExternalPips{0,1000,0}};check(state.advance_external(owner,1000,health,1000,ext).has_value());
  check(!state.advance_external(owner,999,health,1000,{}));check(state.cursor()==1000&&health==100);++cases;
 }
 // A speculative copy gives World an actual death boundary without mutating
 // its live owner. Replaying the accepted prefix is the owner's responsibility.
 {State state(owner,0);double health=1;auto preview=state;double proposed=health;const std::array ext{ExternalPips{0,1000,-1}};
  const auto result=preview.advance_external(owner,1000,proposed,1000,ext);check(result&&result->death_at==500);
  check(state.cursor()==0&&health==1);const std::array prefix{ExternalPips{0,500,-1}};
  const auto committed=state.advance_external(owner,500,health,1000,prefix);
  check(committed&&committed->death_at==500&&health==0&&state.cursor()==500);++cases;
 }
 {State state(owner,0);double health=100;const std::array ext{ExternalPips{0,1000,10}};
  check(state.advance_external(owner,1000,health,1000,ext).has_value());check(health==120&&state.pips(999)==0&&state.pips(1000)==0);++cases;
 }
 {State state(owner,0);double health=100;const std::array ext{ExternalPips{0,9007199254740992.,1}};
  check(!state.advance_external(owner,9007199254740992ull,health,1000,ext));check(health==100&&state.cursor()==0);++cases;
 }
 std::cout<<cases<<" external Health all-rank Ward exposure, pre-cap composition, fractional dense/sparse, expiry/cap/death, transactional rejection and preflight-copy cases passed; World adapter pending\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
