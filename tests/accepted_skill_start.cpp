#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){if(!b)throw std::runtime_error("Accepted skill start line "+std::to_string(at.line()));}
bool has_berserker(world::Instance& w,Scope scope,std::uint64_t at){const auto entries=w.status_snapshot(scope,at);return entries&&std::any_of(entries->begin(),entries->end(),[](const auto& e){return e.skill==370;});}
int main()try{
 for(unsigned mode=0;mode<4;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={344,0};bar[1]={377,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,377,123,0,100,1100,true));
  const auto active=[&](auto at){return wary_stance_active(*w.status_snapshot(f.source,at),at);};
  casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0;
  h.resources=[&](auto id){return w.resources(id);};h.events=[](const auto&){};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.consume_energy=[&](auto n){if(mode==1)return false;return w.consume_energy(1,n,f.source);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};h.status_mutation=[&](auto s,auto e,auto rank,auto at,auto until,bool present){return w.status_mutation(f.source,s,e,rank,at,until,present);};
  h.begin_action=[&](Slot used,auto at){++calls;check(w.resources(1)->energy==25&&at==500);check(w.accepted_skill_start(f.source,used,at));check(!active(at));};
  Policy p;p.primary=1;p.campaigns=255;p.learned[344]=p.learned[377]=1;attributes::Ranks ranks{};
  if(mode==2)p.learned[344]=0;
  if(mode==3){check(w.ordinary_attack(f.source,2,200));check(active(200));continue;}
  const auto result=runtime.start(500,{344,0,2},bar,p,ranks,true,h);
  if(mode==0)check(result==casting::Reason::accepted&&calls==1&&!active(500));
  else check(result!=casting::Reason::accepted&&calls==0&&active(500));
 }
 for(unsigned mode=0;mode<4;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={344,0};bar[1]={370,0};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,370,123,0,100,5100,true));
  casting::Runtime runtime(1);casting::Hooks h;unsigned calls=0;
  h.resources=[&](auto id){return w.resources(id);};h.events=[](const auto&){};h.target_valid=[](auto id,float,bool self){return id==1&&self;};
  h.consume_energy=[&](auto n){check(n==5);if(mode==1)return false;return w.consume_energy(1,n,f.source);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};h.status_mutation=[&](auto s,auto e,auto rank,auto at,auto until,bool active){return w.status_mutation(f.source,s,e,rank,at,until,active);};
  h.begin_action=[&](Slot used,auto at){++calls;check(used==Slot{344,0}&&at==1234&&w.resources(1)->energy==25);check(w.accepted_skill_start(f.source,used,at));check(!has_berserker(w,f.source,at));};
  Policy p;p.primary=1;p.campaigns=255;p.learned[344]=p.learned[370]=1;attributes::Ranks ranks{};
  if(mode==2)p.learned[344]=0;
  if(mode==3){check(w.ordinary_attack(f.source,2,200));check(has_berserker(w,f.source,200));continue;}
  const auto result=runtime.start(1234,{344,0,2},bar,p,ranks,true,h);
  if(mode==0){check(result==casting::Reason::accepted&&calls==1);check(w.accepted_skill_start(f.source,{344,0},1234));}
  else{check(result!=casting::Reason::accepted&&calls==0);check(has_berserker(w,f.source,1234));}
 }
 {warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={344,1};check(w.configure_adrenaline(f.source,bar,100));
  check(w.status_mutation(f.source,370,123,0,100,5100,true));
  auto stale=f.source;++stale.generation;check(!w.accepted_skill_start(stale,{344,1},200));
  check(!w.accepted_skill_start(f.source,{344,0},200));check(has_berserker(w,f.source,200));
  check(w.accepted_skill_start(f.source,{344,1},200));check(!has_berserker(w,f.source,200));
 }
 std::cout<<"PASS paid accepted-use exact slot/timestamp callback for instant skill,denial preservation,ordinary-attack preservation,World lifetime/copy validation and duplicate cleanup\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
