#include "gw_skill_casting.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
void check(bool ok){if(!ok)throw std::runtime_error("Insidious Parasite casting regression");}
int main()try{
 for(unsigned mode=0;mode<4;++mode){
  using namespace skills::casting;Hooks h;unsigned energy=100,commits=0;std::uint64_t generation=9;
  std::vector<skills::Packet> sent;
  h.events=[&](const auto& batch){sent.insert(sent.end(),batch.begin(),batch.end());};
  h.resources=[&](unsigned)->std::optional<vitals::Resources>{return vitals::Resources{500,500,energy,100};};
  h.consume_energy=[&](unsigned cost){check(cost==15);if(cost>energy)return false;energy-=cost;return true;};
  h.projectile_target_valid=[](unsigned id,float range){return id==2&&range==1248;};
  h.target_scope=[&](unsigned id)->std::optional<skills::effects::Scope>{return id==2?std::optional<skills::effects::Scope>{{1,2,generation}}:std::nullopt;};
  h.degeneration_hex=[&](auto target,const auto& plan,auto now){check(target.generation==9&&plan.skill==123&&plan.rank==12&&plan.pips==0&&plan.duration==13000&&plan.radius==0&&now==1100);++commits;return true;};
  skills::Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[123]=1;
  skills::Slots slots{};slots[0]={123,0};attributes::Ranks ranks{};ranks[7]=12;Runtime cast(1);
  check(cast.start(100,{123,0,2},slots,policy,ranks,true,h)==Reason::accepted&&energy==85);
  cast.update(1099,h);check(commits==0);
  if(mode==1)cast.cancel(h);if(mode==2)cast.interrupt(500,h);if(mode==3)++generation;
  cast.update(1100,h);cast.update(1100,h);check(commits==(mode?0u:1u));
  const auto recharges=std::count(sent.begin(),sent.end(),slot_event(228,1,{123,0},12));check(recharges==(mode==0||mode==2?1:0));
 }
 std::cout<<"PASS Insidious Parasite actual Runtime1second cast,15Energy,12second recharge,cancel,interrupt and target replacement gates\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
