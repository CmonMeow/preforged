#include "gw_necrosis.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main()try{
    for(unsigned rank=0;rank<=10;++rank){
        const auto plan=skills::effects::necrosis(rank);
        check(bool(plan)&&plan->damage==60+3*rank,"earned title scaling");
        combat::Actor actor{2,1,combat::Health{1000,1000},{},0,true};
        const auto no_effect=skills::effects::necrosis_target(*plan,actor,false,20);
        check(no_effect.accepted&&!no_effect.hit&&no_effect.target.health->current==1000,
              "unafflicted target does not take damage");
        const auto afflicted=skills::effects::necrosis_target(*plan,actor,true,20);
        check(afflicted.accepted&&afflicted.hit&&
              afflicted.target.health->current==1000-plan->damage,
              "afflicted target takes armor-ignoring damage");
    }
    auto forged=*skills::effects::necrosis(5);forged.damage+=1;
    combat::Actor actor{2,1,combat::Health{1000,1000},{},0,true};
    check(!skills::effects::necrosis_target(forged,actor,true,20).accepted&&
          !skills::effects::necrosis(11),"forged plan or unearned rank rejected");
    actor.health.reset();check(!skills::effects::necrosis_target(*skills::effects::necrosis(0),actor,true,20).accepted,
                               "unknown target Health rejected");
    std::cout<<"Necrosis detached rank and affliction policy passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
