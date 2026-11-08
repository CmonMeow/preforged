#include "gw_condition_spells.h"
#include <stdexcept>
void check(bool ok){if(!ok)throw std::runtime_error("condition spell regression");}
int main(){using namespace fw::gw::skills::effects;using namespace fw::gw::combat::conditions;
 check(condition_spell(117,12)->duration==26000&&condition_spell(2859,12)->duration==26000&&condition_spell(78)->spread&&condition_spell(78)->radius==166&&condition_spell(118,12)->duration==17000&&condition_spell(2885,12)->radius==252&&condition_spell(2885,12)->duration==13000&&!condition_spell(117,21));
 check(condition_spell(131,12)->duration==13000&&condition_spell(131)->radius==166&&condition_spell(131)->kind==Kind::bleeding);
 const unsigned durations[]={13,14,16,17,19,20,20,20,20,20,20};
 for(unsigned rank=0;rank<=10;++rank){const auto plan=signet_infection(rank);check(plan&&plan->skill==2229&&plan->ebon_rank==rank&&plan->duration==1000ull*durations[rank]);}
 for(unsigned rank=0;rank<=10;++rank){const auto plan=signet_infection_condition(rank);check(plan&&valid_condition_spell(*plan)&&plan->duration==1000ull*durations[rank]&&plan->kind==Kind::disease&&plan->requires_bleeding&&!plan->spread&&!plan->radius);}
 auto forged_infection=*signet_infection_condition(7);forged_infection.requires_bleeding=false;check(!valid_condition_spell(forged_infection));
 check(!signet_infection(11));State infected;check(!signet_infection_bleeding_target(infected,100));
 check(infected.apply(Kind::bleeding,25,100,500));check(!signet_infection_bleeding_target(infected,99));
 check(signet_infection_bleeding_target(infected,100)&&signet_infection_bleeding_target(infected,599));
 check(!signet_infection_bleeding_target(infected,600));
 auto forged=*condition_spell(78);forged.radius=9999;check(!valid_condition_spell(forged));
 State source,target;source.apply(Kind::blind,1,100,5000);source.apply(Kind::weakness,1,200,10000);target.apply(Kind::blind,2,200,20000);
 auto spread=spread_conditions(source,target,42,1000);check(spread&&source.count(1000)==2&&spread->kinds.size()==2&&spread->state.lookup(Kind::blind)->expires==20200&&spread->state.lookup(Kind::weakness)->expires==10200&&spread->state.newest(1000)==Kind::weakness);
 check(spread->state.lookup(Kind::weakness)->cause==42&&!spread_conditions(source,target,0,1000));
 auto expired=spread_conditions(source,{},42,11000);check(expired&&expired->kinds.empty());
}
