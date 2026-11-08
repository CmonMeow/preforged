#include "gw_freezing_gust.h"
#include <iostream>
using namespace fw::gw;
static void check(bool okay,const char* why){if(!okay)throw std::runtime_error(why);}
int main()try{
    for(unsigned rank:{0u,12u,15u,20u}){
        const auto plan=skills::effects::freezing_gust(rank);
        check(bool(plan)&&plan->damage==20+4*rank,"ranked cold damage");
        combat::Actor actor{2,1,combat::Health{1000,1000},{},{},true};actor.cold_armor=80;
        const auto snare=skills::effects::freezing_gust_target(*plan,actor,{},20,false,100);
        check(snare.accepted&&!snare.damaged&&!snare.hit&&snare.snare_ms==plan->snare_ms&&
              snare.actor.health->current==1000,"unhexed target snares without damage");
        const auto damage=skills::effects::freezing_gust_target(*plan,actor,{},20,true,100);
        check(damage.accepted&&damage.damaged&&damage.hit&&damage.snare_ms==0&&
              damage.actor.health->current<1000,"Water-hexed target takes cold hit only");
        actor.cold_armor.reset();
        check(!skills::effects::freezing_gust_target(*plan,actor,{},20,true,100).accepted&&
              skills::effects::freezing_gust_target(*plan,actor,{},20,false,100).accepted,
              "unknown armor blocks only the damage branch");
    }
    auto forged=*skills::effects::freezing_gust(12);forged.multiplier=1;
    combat::Actor actor{2,1,combat::Health{1000,1000},{},{},true};actor.cold_armor=80;
    check(!skills::effects::freezing_gust_target(forged,actor,{},20,false,100).accepted&&
          !skills::effects::freezing_gust(21),"forged plan and rank rejected");
    std::cout<<"Freezing Gust branch policy passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
