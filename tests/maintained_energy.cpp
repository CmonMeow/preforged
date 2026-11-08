#include "gw_maintained_energy.h"
#include <iostream>
#include <string>
#include <stdexcept>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){if(!ok)throw std::runtime_error("Maintained Energy line "+std::to_string(at.line()));}
MaintainedEnchantments ledger(unsigned count){MaintainedEnchantments result;for(unsigned n=0;n<count;++n){Scope target{1,500-n,1};Registry registry(target);check(registry.apply(target,{n+1,1,290,12,Category::enchantment,100,{}},Replace::independent).accepted);check(result.add_mending({1,1,1},target,n+1,registry,100));}return result;}
int main()try{
 const Scope caster{1,1,1};unsigned cases=0;
 for(unsigned base:{0u,1u,2u,4u,7u,10u})for(unsigned count=0;count<=22;++count)for(unsigned initial:{0u,1u,10u})for(unsigned elapsed:{0u,1u,2999u,6000u}){
  ++cases;auto owned=ledger(count);overcast::CeilingCurve curve(caster,100,33);energy::Regeneration fraction;vitals::Resources resources{100,100,initial,100};
  const auto proposal=prepare_maintained_energy(owned,curve,caster,resources,fraction,0,100+elapsed,base);check(proposal.has_value());
  std::int64_t units=initial*overcast::energy_unit;unsigned remaining=count;std::vector<std::pair<unsigned,std::uint64_t>> ends;
  for(unsigned ms=0;ms<=elapsed;++ms){if(ms)units=std::clamp<std::int64_t>(units+(int(base)-int(remaining))*100,0,100*overcast::energy_unit);
   while(remaining&&(int(base)-int(remaining)< (units<=0?0:-10))){ends.emplace_back(500-(remaining-1),100+ms);--remaining;}}
  check(overcast::precise_current(proposal->energy)==units);check(proposal->ownership.count(caster,100+elapsed)==remaining);check(proposal->ended.size()==ends.size());
  for(unsigned n=0;n<ends.size();++n)check(proposal->ended[n].release.target.actor==ends[n].first&&proposal->ended[n].at==ends[n].second);
  check(owned.count(caster,100)==count&&curve.cursor()==100&&resources.energy==initial&&fraction.remainder()==0);
 }
 // Timed positive pips must remain uncapped before subtracting upkeep.
 {auto owned=ledger(12);overcast::CeilingCurve curve(caster,100,33);check(curve.apply_energy_modifier(caster,1,178,10,10000,100));energy::Regeneration fraction;vitals::Resources resources{100,100,1,100};
  const auto result=prepare_maintained_energy(owned,curve,caster,resources,fraction,0,600,10);check(result&&result->ended.empty()&&result->final_pips==8&&overcast::precise_current(result->energy)==700000);}
 // A modifier expiry changes the drain;zero is not postponed to the final tick.
 {auto owned=ledger(5);overcast::CeilingCurve curve(caster,100,33);check(curve.apply_energy_modifier(caster,1,178,4,1000,100));energy::Regeneration fraction;vitals::Resources resources{100,100,1,100};
  const auto result=prepare_maintained_energy(owned,curve,caster,resources,fraction,0,6100,2);check(result&&result->ended.size()==3&&result->ended.front().at==2434&&result->ownership.count(caster,6100)==2&&result->final_pips==0&&overcast::precise_current(result->energy)==0);}
 // Prepared removal is exact and cannot be forged,replayed or committed after replacement.
 {auto owned=ledger(6);const auto quote=*owned.prepare_upkeep(caster,4,true,200);check(quote.releases.size()==2);auto forged=quote;forged.releases.pop_back();check(!owned.commit_upkeep(forged)&&owned.size()==6);check(owned.commit_upkeep(quote)&&owned.size()==4&&!owned.commit_upkeep(quote));}
 {auto owned=ledger(6);const auto quote=*owned.prepare_upkeep(caster,4,true,200);check(owned.ended({1,500,1},1,200));check(!owned.commit_upkeep(quote));}
 {auto owned=ledger(1);overcast::CeilingCurve curve(caster,100,33);energy::Regeneration fraction;vitals::Resources resources{100,100,1,100};check(!prepare_maintained_energy(owned,curve,{1,1,2},resources,fraction,0,200,4));resources.health=0;check(!prepare_maintained_energy(owned,curve,caster,resources,fraction,0,200,4));}
 {auto owned=ledger(2);const Scope other{1,2,1},recipient{1,600,1};Registry registry(recipient);check(registry.apply(recipient,{1,2,290,12,Category::enchantment,100,{}},Replace::independent).accepted);check(owned.add_mending(other,recipient,1,registry,100));check(owned.ended({1,500,1},1,200));
  overcast::CeilingCurve curve(other,100,33);energy::Regeneration fraction;vitals::Resources resources{100,100,1,100};const auto result=prepare_maintained_energy(owned,curve,other,resources,fraction,0,300,4);check(result&&result->ownership.count(other,300)==1&&result->ownership.count(caster,200)==1);}
 // Sub-pip Energy remains positive until its actual first millisecond crossing.
 {auto owned=ledger(5);overcast::CeilingCurve curve(caster,100,33);energy::Regeneration fraction;vitals::Resources resources{100,100,0,100};
  const auto before=prepare_maintained_energy(owned,curve,caster,resources,fraction,0,100,4,1);check(before&&before->ended.empty()&&overcast::precise_current(before->energy)==1);
  const auto after=prepare_maintained_energy(owned,curve,caster,resources,fraction,0,101,4,1);check(after&&after->ended.size()==1&&after->ended.front().at==101&&overcast::precise_current(after->energy)==0);}
 std::cout<<"PASS "<<cases<<" independent millisecond-oracle upkeep cases plus modifier expiry,pre-cap boost,atomic release/replay and scope guards;pure settlement oracle; World callers checked separately\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
