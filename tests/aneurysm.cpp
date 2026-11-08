#include "gw_aneurysm.h"
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <vector>
using namespace fw::gw;
static void check(bool okay,std::source_location at=std::source_location::current()){
    if(!okay)throw std::runtime_error("Aneurysm line "+std::to_string(at.line()));
}
int main()try{
    const auto low=skills::effects::aneurysm(0),mid=skills::effects::aneurysm(12),
        high=skills::effects::aneurysm(15);
    check(low&&mid&&high&&!skills::effects::aneurysm(21));
    check(low->damage_per_energy==1&&low->adjacent_loss_cap==1);
    check(mid->damage_per_energy==3&&mid->adjacent_loss_cap==24);
    check(high->damage_per_energy==3&&high->adjacent_loss_cap==30);
    combat::Actor target{42,7,combat::Health{100,100},{},0,true};
    const auto result=skills::effects::aneurysm_target(*mid,target,10,30,30,20);
    check(result&&result->restored_energy==20&&result->damage_energy==20&&
          result->adjacent_loss==20&&result->hit&&result->hit->applied==60&&
          result->hit->target==42&&result->hit->generation==7);
    check(target.health->current==100); // Pure proposal, no committed mutation.
    const auto capped=skills::effects::aneurysm_target(*mid,target,0,100,100,20);
    check(capped&&capped->adjacent_loss==24&&capped->hit&&
          capped->hit->applied==100&&capped->hit->killed);
    const auto full=skills::effects::aneurysm_target(*mid,target,30,30,30,20);
    check(full&&!full->hit&&!full->restored_energy&&!full->adjacent_loss);
    const auto overcast=skills::effects::aneurysm_target(*mid,target,10,20,30,20);
    check(overcast&&overcast->restored_energy==10&&overcast->damage_energy==20&&
          overcast->adjacent_loss==20&&overcast->hit->applied==60);
    const auto sealed=skills::effects::aneurysm_target(*mid,target,10,5,30,20);
    check(sealed&&!sealed->restored_energy&&sealed->damage_energy==20&&
          sealed->hit->applied==60);
    check(!skills::effects::aneurysm_target(*mid,target,10,31,30,20));
    auto forged=*mid;++forged.adjacent_loss_cap;
    check(!skills::effects::aneurysm_target(forged,target,10,30,30,20));
    target.damageable=false;
    check(!skills::effects::aneurysm_target(*mid,target,10,30,30,20));
    target.damageable=true;
    using skills::effects::AneurysmNearby;
    const combat::Actor foe{43,8,combat::Health{80,80},{},0,true};
    const combat::Actor ally{44,9,combat::Health{80,80},{},0,true};
    const std::vector<AneurysmNearby> near{{foe,120,true,7},{ally,100,false,20}};
    const auto area=skills::effects::aneurysm_area(*mid,target,0,30,30,20,near);
    check(area&&area->primary.hit&&area->adjacent.size()==1&&
          area->adjacent[0].actor==43&&area->adjacent[0].energy==7);
    auto unknown=near;unknown[0].current_energy.reset();
    check(!skills::effects::aneurysm_area(*mid,target,0,30,30,20,unknown));
    auto duplicate=near;duplicate.push_back(near[0]);
    check(!skills::effects::aneurysm_area(*mid,target,0,30,30,20,duplicate));
    auto distant=near;distant[0].distance=167;
    check(skills::effects::aneurysm_area(*mid,target,0,30,30,20,distant)->adjacent.empty());
    std::cout<<"PASS Aneurysm catalog, Energy restoration, damage, adjacent cap and rejection\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
