#include "gw_finish_him.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
using namespace fw::gw;
using namespace fw::gw::skills::effects;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("Finish Him line "+std::to_string(at.line()));
}
int main()try{
    static constexpr unsigned damage[]{44,51,58,66,73,80};
    static constexpr unsigned duration[]{12,14,15,17,18,20};
    for(unsigned rank=0;rank<=10;++rank){
        const auto plan=finish_him(rank);check(plan&&plan->damage==damage[std::min(rank,5u)]&&
            plan->conditions_ms==1000ull*duration[std::min(rank,5u)]);
    }
    check(!finish_him(11));
    const auto plan=*finish_him(0);
    combat::Actor target;target.id=2;target.generation=1;target.health=combat::Health{49,100};target.damageable=true;
    combat::conditions::State conditions;
    const auto applied=resolve_finish_him(plan,target,conditions,20,1,100,true);
    check(applied&&applied->triggered&&applied->hit&&applied->target.health->current==0&&
          applied->target.health->maximum==80);
    check(applied->conditions.has(combat::conditions::Kind::cracked_armor,100)&&
          applied->conditions.has(combat::conditions::Kind::deep_wound,100));
    check(!conditions.has(combat::conditions::Kind::deep_wound,100));
    target.health=combat::Health{50,100};
    const auto half=resolve_finish_him(plan,target,conditions,20,1,100,true);
    check(half&&!half->triggered&&!half->hit&&half->target.health->current==50);
    target.health=combat::Health{540,1090};
    const auto bonus=resolve_finish_him(plan,target,conditions,20,1,100,true,1000);
    check(bonus&&!bonus->triggered&&resolve_finish_him(plan,target,conditions,20,1,100,true)->triggered);
    target.health=combat::Health{49,100};
    const auto immune=resolve_finish_him(plan,target,conditions,20,1,100,false);
    check(immune&&immune->triggered&&immune->hit&&immune->target.health->current==5&&
          !immune->conditions.has(combat::conditions::Kind::deep_wound,100));
    target.health=combat::Health{30,100};
    const auto lethal=resolve_finish_him(plan,target,conditions,20,1,100,true);
    check(lethal&&lethal->triggered&&lethal->hit->killed&&lethal->target.health->current==0&&
          lethal->conditions.has(combat::conditions::Kind::deep_wound,100));
    auto forged=plan;forged.damage=100;
    check(!resolve_finish_him(forged,target,conditions,20,1,100,true));
    std::cout<<"PASS Finish Him pinned title scaling, strict below-half gate, armor-ignoring damage and two conditions\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
