#include "gw_quest_marksmanship.h"
#include "gw_quest_catalog.h"
#include "gw_quest_timed_state.h"
#include "gw_quest_event_time.h"
#include "gw_ranger_marksmanship.h"
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv)try{
 using Q=fw::gw::quests::MarksmanshipObjective;using S=Q::Scope;
 const auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
 {
  using namespace fw::gw::skills::ranger;
  check(point_blank_shot(0,1200)->bonus==10&&point_blank_shot(15,1200)->bonus==40,
        "Point Blank Shot Expertise progression differs from wiki");
  check(point_blank_shot(12,1200)->range==600&&!point_blank_shot(21,1200)&&
        !point_blank_shot(0,0),"Point Blank Shot range validation failed");
  ReadTheWind preparation;
  check(!preparation.active(0)&&preparation.prepare(12,1000)&&preparation.bonus(1000)==9,
        "Read the Wind preparation or Marksmanship progression failed");
  const auto normal=fw::gw::projectiles::intercept({2500,32},0,0,1000,0,0,0);
  const auto policy=preparation.flight({2500,32},1000);
  const auto fast=fw::gw::projectiles::intercept(*policy,0,0,1000,0,0,0);
  check(normal->flight_ms==400&&fast->flight_ms==200,"preparation did not double arrow speed");
  check(preparation.active(24999)&&!preparation.active(25000)&&preparation.bonus(25000)==0&&
        preparation.flight({2500,32},25000)->speed==2500,"preparation expiry retained arrow effects");
  check(!preparation.prepare(21,0)&&!preparation.prepare(0,UINT64_MAX-23999),"invalid preparation accepted");
  check(preparation.prepare(15,100)&&preparation.bonus(100)==10,"rank15 bonus failed");
  preparation.remove();check(!preparation.active(100),"preparation removal failed");
 }
 check(argc==2,"captured quest42 catalog required");
 const auto catalog=fw::gw::quests::load_catalog(std::filesystem::path(argv[1]));
 check(catalog.size()==1&&catalog.contains(42),"quest42 capture identity lost");
 const auto& text=catalog.at(42);
 check(text.entry.map_from==162&&text.entry.map_to==162&&text.objective_variants.size()==4&&text.completed_variant,
       "captured map or five-kill progress variants missing");
 check(text.description&&text.objectives&&text.completed_variant->objectives,"captured quest text missing");
 {
  using namespace fw::gw::quests;
  auto saved=*accept(Profile{},42);saved=*arm_marksmanship(saved,1,100000);
  check(event_utc_ms(5000,10000,115000)==110000,"queued event used processing time");
  check(!event_utc_ms(10001,10000,115000)&&!event_utc_ms(1,10000,100),"invalid clock mapping accepted");
  const auto bytes=encode(saved);check(bytes[3]=='3'&&decode(bytes)==saved,"timed deadline codec round trip failed");
  check(marksmanship_elapsed(decode(bytes).records.at(42),135000)==35000,"reconnect reset elapsed time");
  check(!arm_marksmanship(saved,1,140000),"active timed attempt implicitly restarted");
  check(!credit_objective(saved,42,1,10,5)&&!credit_objective(saved,42,1,10,5,190000),"missing timestamp or late durable credit accepted");
  check(!credit_objective(saved,42,1,6,5,110000)&&
        !credit_objective(saved,42,1,0x10000000aull,5,110000),"nonlogical durable worm credit accepted");
  auto opaque=saved;opaque.records.at(42).objective_credits={10};
  opaque=decode(encode(opaque));
  check(!marksmanship_credit_mask(opaque.records.at(42)),"legacy opaque credits mapped to guessed worms");
  check(!credit_objective(opaque,42,1,1,5,110000),"legacy opaque attempt mixed with logical worm credits");
  auto recovered=*accept(*abandon(opaque,42,1),42);
  recovered=*arm_marksmanship(recovered,2,200000);
  check(bool(credit_objective(recovered,42,2,1,5,210000)),"legacy opaque attempt could not restart explicitly");
  saved=*credit_objective(saved,42,1,1,5,110000);
  check(marksmanship_credit_mask(decode(encode(saved)).records.at(42))==1,"new durable credit cannot restore worm identity");
  auto logical=*accept(Profile{},42);logical=*arm_marksmanship(logical,1,100000);
  logical=*credit_objective(logical,42,1,1,5,110000);
  logical=*credit_objective(logical,42,1,3,5,110001);
  check(marksmanship_credit_mask(logical.records.at(42))==5,"persisted logical worm credits lost");
  const auto delayed=event_utc_ms(60000,110000,200000);
  check(delayed==150000&&bool(credit_objective(saved,42,1,2,5,delayed)),"timely queued kill rejected solely because processing was after deadline");
  auto expired=*expire_marksmanship(saved,1,190000);check(decode(encode(expired))==expired,"expiry persistence lost");
  check(!credit_objective(expired,42,1,2,5,120000)&&!arm_marksmanship(expired,1,200000),"expired attempt revived");
  auto restarted=*accept(*abandon(expired,42,1),42);check(restarted.records.at(42).cycle==2&&!restarted.records.at(42).timed,"new attempt retained old deadline");
  check(bool(arm_marksmanship(restarted,2,200000)),"explicit new attempt could not arm");
  for(unsigned n=0;n<bytes.size();++n){bool rejected=false;try{(void)decode(std::span(bytes).first(n));}catch(const std::invalid_argument&){rejected=true;}check(rejected,"truncated timed profile accepted");}
  auto legacy=*accept(Profile{},43);check(encode(legacy)[3]=='1'&&decode(encode(legacy))==legacy,"legacy uncounted format changed");
  legacy=*credit_objective(legacy,43,1,1,3);check(encode(legacy)[3]=='2'&&decode(encode(legacy))==legacy,"legacy counted format changed");
 }
 S owner{1,1,1};std::array<S,5> worms{{{1,10,1},{1,11,1},{1,12,1},{1,13,1},{1,14,1}}};
 auto objective=Q::create(owner,2,1000,worms);check(bool(objective),"valid timed encounter rejected");
 {
  const auto restored=Q::restore(owner,2,100,35000,5,worms);
  check(restored&&restored->deadline()==55100&&restored->remaining()==3,"reconnect reset timer or kill credits");
  auto resumed=*restored;
  check(!resumed.death(owner,2,worms[0],200)&&!resumed.death(owner,2,worms[2],200),"reconnect credited already killed logical worm");
  check(resumed.logical_credit(worms[1])==2&&resumed.death(owner,2,worms[1],200),"logical credit changed across restore");
  check(!resumed.death(owner,2,worms[3],55100)&&resumed.expire(owner,2,55100),"restored deadline got a new ninety seconds");
  check(!Q::restore(owner,2,100,90001,0,worms)&&!Q::restore(owner,2,100,0,32,worms),"invalid persisted timer or mask accepted");
  check(Q::restore(owner,2,100,90000,0,worms)->state()==Q::State::failed,"expired reconnect revived challenge");
  check(Q::restore(owner,2,100,90000,31,worms)->state()==Q::State::completed,"completed challenge lost on late reconnect");
 }
 check(!objective->death({1,1,2},2,worms[0],2000)&&!objective->death(owner,1,worms[0],2000),"stale owner or attempt credited");
 check(!objective->death(owner,2,worms[0],999),"preacceptance death credited");
 check(objective->death(owner,2,worms[0],2000)&&!objective->death(owner,2,worms[0],2001),"duplicate death credited");
 S replacement{1,11,2};check(objective->replace_actor(owner,2,worms[1],replacement,3000),"trusted worm lifetime replacement refused");
 check(!objective->death(owner,2,worms[1],4000)&&objective->death(owner,2,replacement,4000),"old lifetime credited after replacement");
 check(!objective->replace_actor(owner,2,replacement,worms[1],5000),"credited worm respawn authorized");
 for(unsigned i=2;i<5;++i)check(objective->death(owner,2,worms[i],90999),"valid final deaths refused");
 check(objective->state()==Q::State::completed&&objective->remaining()==0&&!objective->expire(owner,2,91000),"completion lost at deadline");
 auto late=*Q::create(owner,3,1000,worms);check(!late.death(owner,3,worms[0],91000)&&late.expire(owner,3,91000),"deadline boundary accepted late kill");
 check(!late.death(owner,3,worms[0],5000)&&late.state()==Q::State::failed,"expired attempt revived");
 auto bad=worms;bad[4]=bad[0];check(!Q::create(owner,1,0,bad),"duplicate logical assignments accepted");
 bad[4].generation=2;check(!Q::create(owner,1,0,bad),"one agent ID assigned to two logical worms");
 auto distinct=*Q::create(owner,4,1000,worms);check(!distinct.replace_actor(owner,4,worms[0],{1,11,2},2000),"replacement reused another logical worm ID");
 // Replacement must preserve create's actor-ID separation from the owner,
 // even when the proposed lifetime has a different generation.
 check(!distinct.replace_actor(owner,4,worms[0],owner,2000)&&
       !distinct.replace_actor(owner,4,worms[0],{1,1,2},2000),"replacement reused the owner actor ID");
 check(distinct.logical_credit(worms[0])==1&&!distinct.logical_credit({1,1,2})&&
       distinct.remaining()==5,"rejected owner alias mutated logical worm state");
 check(distinct.replace_actor(owner,4,worms[0],{1,10,2},2001)&&
       distinct.logical_credit({1,10,2})==1&&distinct.death(owner,4,{1,10,2},2002),
       "owner alias rejection prevented a valid worm lifetime replacement");
 check(!Q::create(owner,1,UINT64_MAX,worms),"deadline overflow accepted");
 std::cout<<"PASS timed five-worm objective, owner/attempt/lifetime fences and expiry\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
