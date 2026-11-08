#include "warrior_condition_fixture.h"
#include "gw_skill_casting.h"
#include <source_location>
#include <iostream>
using namespace fw::gw;using namespace skills;using namespace skills::effects;
void check(bool b,std::source_location at=std::source_location::current()){
 if(!b)throw std::runtime_error("Battle Rage cast line "+std::to_string(at.line()));
}
unsigned charge(world::Instance& w,Scope owner,unsigned skill){const auto pools=w.adrenaline(owner);check(pools.has_value());for(const auto& p:*pools)if(p.slot.skill==skill)return p.units;throw std::runtime_error("pool");}
int main()try{
 for(unsigned mode=0;mode<5;++mode){
  warrior_test::Fixture f(340,false);auto& w=f.world;Slots bar{};bar[0]={317,0};bar[1]={340,0};bar[2]={344,0};check(w.configure_adrenaline(f.source,bar,100));
  if(mode!=1)for(unsigned i=0;i<4;++i)check(w.melee_attack(f.source,f.target,f.weapon,*combat::weapon::attack_skill(340,12),200+i));
  casting::Runtime runtime(1);casting::Hooks h;std::vector<items::Packet> packets;unsigned paid=0,starts=0;
  h.events=[&](const auto& b){packets.insert(packets.end(),b.begin(),b.end());};h.resources=[&](auto id){return w.resources(id);};
  h.target_valid=[](auto id,float,bool self){return id==1&&self;};h.consume_energy_at=[&](auto n,auto at){paid+=n;return w.consume_energy(1,n,f.source,at);};
  h.consume_adrenaline=[&](auto slot,auto at){return w.consume_adrenaline(f.source,slot,at);};h.reconcile_adrenaline=[&](auto at){check(w.reconcile_adrenaline(f.source,at));};
  h.effect_snapshot=[&](auto at){return *w.status_snapshot(f.source,at);};h.status_mutation=[&](auto s,auto e,auto r,auto at,auto until,bool active){return w.status_mutation(f.source,s,e,r,at,until,active);};
  h.begin_action=[&](auto slot,auto at){++starts;check(w.accepted_skill_start(f.source,slot,at));};
  if(mode==2)h.begin_action={};if(mode==3)h.status_mutation={};
  Policy p;p.primary=1;p.campaigns=255;p.learned[317]=p.learned[340]=p.learned[344]=1;attributes::Ranks ranks{};ranks[17]=12;
  const auto result=runtime.start(1000,{317,0,2},bar,p,ranks,true,h);
  if(mode==1){check(result==casting::Reason::adrenaline&&starts==0);continue;}
  if(mode==2||mode==3){check(result==casting::Reason::unsupported&&charge(w,f.source,317)==80);continue;}
  if(result!=casting::Reason::accepted)throw std::runtime_error("Battle Rage admission reason="+std::to_string(static_cast<unsigned>(result)));
  check(paid==0&&charge(w,f.source,317)==0&&charge(w,f.source,340)==75&&starts==1);
  const auto entries=*w.status_snapshot(f.source,1000);check(entries.size()==1&&entries[0].skill==317&&entries[0].expires==18000);
  check(std::count(packets.begin(),packets.end(),casting::generic(48,1,317))==1&&std::count(packets.begin(),packets.end(),casting::generic(21,1,563))==1);
  if(mode==4){
   check(w.consume_energy(1,30,f.source,2000));check(runtime.start(2000,{344,0,1},bar,p,ranks,true,h)==casting::Reason::energy);
   check(battle_rage_active(*w.status_snapshot(f.source,2000),2000)==true);
   w.set_vitals(1,{1000,1000,30,30},100);check(runtime.start(2100,{344,0,1},bar,p,ranks,true,h)==casting::Reason::accepted);
   check(battle_rage_active(*w.status_snapshot(f.source,2100),2100)==false&&starts==2);
  }else{runtime.update(17999,h);check(battle_rage_active(*w.status_snapshot(f.source,17999),17999)==true);runtime.update(18000,h);check(w.status_snapshot(f.source,18000)->empty());}
 }
 std::cout<<"PASS Battle Rage normal instant80-unit payment/body563/rank duration,missing hooks,insufficient charge,failed-use preservation and accepted non-adrenal ending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
