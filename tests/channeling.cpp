#include "gw_channeling.h"
#include <limits>
#include <stdexcept>
void check(bool v){if(!v)throw std::runtime_error("Channeling count regression");}
int main(){using namespace fw::gw::skills::effects;
 const auto plan=*channeling(12);check(plan.duration==46000&&plan.radius==322&&plan.per_foe==1);
 check(channeling()->duration==8000&&channeling(20)->duration==72000&&!channeling(21));
 Scope caster{1,1,7};
 std::vector<ChannelingFoe> actors{
    {{1,2,1},0,0,0,true,true},{{1,3,1},322,0,0,true,true},
    {{1,4,1},322.01,0,0,true,true},{{1,5,1},0,0,0,true,false},
    {{1,6,1},0,0,0,false,true},{{2,7,1},0,0,0,true,true},
    {{1,8,1},0,0,1,true,true},{caster,0,0,0,true,true},
    {{1,2,1},0,0,0,true,true}};
 check(channeling_energy(plan,caster,0,0,0,actors)==2);
 check(channeling_energy(plan,caster,0,0,0,{})==0);
 check(!channeling_energy(plan,{},0,0,0,actors));
 auto forged=plan;++forged.per_foe;check(!channeling_energy(forged,caster,0,0,0,actors));
 check(!channeling_energy(plan,caster,std::numeric_limits<double>::quiet_NaN(),0,0,actors));
 ChannelingCompletion completion(caster);
 Entry observer{8,1,38,12,Category::enchantment,100,46100};
 check(completion.prepare(caster,1,281,200,observer,0,0,0,actors));
 check(!completion.prepare(caster,2,281,200,observer,0,0,0,actors));
 check(!completion.complete({1,1,8},1,true,200));
 check(!completion.complete(caster,1,true,199));
 check(completion.complete(caster,1,true,200)==2);
 check(!completion.complete(caster,1,true,200));
 check(!completion.prepare(caster,1,281,201,observer,0,0,0,actors));
 check(completion.prepare(caster,2,281,300,observer,0,0,0,actors));
 check(completion.complete(caster,2,false,300)==0);
 check(!completion.complete(caster,2,true,300));
 check(!completion.prepare(caster,3,132,400,observer,0,0,0,actors));
 check(!completion.prepare(caster,3,397,400,observer,0,0,0,actors));
 check(completion.prepare(caster,3,281,400,{},0,0,0,actors));
 check(completion.complete(caster,3,true,400)==0);
}
