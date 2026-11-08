#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
#include <string>
#include <stdexcept>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Bull's Charge cast line "+std::to_string(at.line()));
}
int main()try{
 for(unsigned mode=0;mode<5;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={379,0};bar[1]={344,0};check(w.configure_adrenaline(f.source,bar,100));
  casting::Runtime runtime(1);casting::Hooks h;std::vector<items::Packet> packets;unsigned paid=0,starts=0;
  h.events=[&](const auto& b){packets.insert(packets.end(),b.begin(),b.end());};h.resources=[&](auto id){return w.resources(id);};
  h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy_at=[&](auto n,auto at){paid+=n;return w.consume_energy(1,n,f.source,at);};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};h.status_mutation=[&](auto s,auto e,auto r,auto at,auto until,bool active){return w.status_mutation(f.source,s,e,r,at,until,active);};
  h.begin_action=[&](auto slot,auto at){++starts;check(w.accepted_skill_start(f.source,slot,at));};
  if(mode==1)check(w.consume_energy(1,30,f.source,200));if(mode==2)h.begin_action={};if(mode==3)h.status_mutation={};
  Policy p;p.primary=1;p.campaigns=255;p.learned[379]=p.learned[344]=1;attributes::Ranks ranks{};ranks[17]=12;
  const auto result=runtime.start(1000,{379,0,2},bar,p,ranks,true,h);
  if(mode==1){check(result==casting::Reason::energy&&starts==0);continue;}
  if(mode==2||mode==3){check(result==casting::Reason::unsupported&&paid==0&&starts==0);continue;}
  check(result==casting::Reason::accepted&&paid==5&&starts==1);
  const auto entries=*w.status_snapshot(f.source,1000);check(entries.size()==1&&entries[0].skill==379&&entries[0].expires==11000);
  check(std::count(packets.begin(),packets.end(),casting::generic(48,1,379))==1&&std::count(packets.begin(),packets.end(),casting::generic(21,1,638))==1);
  check(runtime.start(1001,{379,0,1},bar,p,ranks,true,h)==casting::Reason::recharge);
  if(mode==4){
   check(w.consume_energy(1,25,f.source,2000));check(runtime.start(2000,{344,0,1},bar,p,ranks,true,h)==casting::Reason::energy);
   check(w.status_snapshot(f.source,2000)->size()==1);
   w.set_vitals(1,{1000,1000,30,30},100);check(runtime.start(2100,{344,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);
   const auto after=*w.status_snapshot(f.source,2100);check(std::none_of(after.begin(),after.end(),[](const auto& e){return e.skill==379;})&&starts==2);
  }else{runtime.update(10999,h);check(w.status_snapshot(f.source,10999)->size()==1);runtime.update(11000,h);check(w.status_snapshot(f.source,11000)->empty());}
 }
 std::cout<<"PASS Bull's Charge paid native cast/body638/pinned cooldown/rank duration,missing hooks,failed-use preservation and accepted ending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
