#include "gw_dancing_daggers.h"
#include "gw_skill_particle_wire.h"
#include <iostream>
#include <stdexcept>

using namespace fw::gw;
static void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
int main()try{
 const auto low=skills::effects::dancing_daggers(0);
 const auto mid=skills::effects::dancing_daggers(12);
 const auto high=skills::effects::dancing_daggers(15);
 check(low&&mid&&high&&low->damage==5&&mid->damage==29&&high->damage==35,"ranked damage");
 check(low->projectiles==3&&low->range==624&&low->launch_model==854&&low->impact_resource==855,"three half-range projectiles");
 check(!skills::effects::dancing_daggers(21),"rank bounds");
 combat::Actor foe;foe.id=2;foe.generation=1;foe.health=combat::Health{500,500};foe.earth_armor=60;foe.damageable=true;
 const combat::conditions::State conditions;
 const auto hit=skills::effects::dancing_dagger_hit(*mid,foe,conditions,20,1000);
 check(hit.accepted&&hit.hit.reason==combat::Reason::accepted&&hit.hit.after.current<500,"earth armor impact");
 auto forged=*mid;forged.projectiles=2;
 check(!skills::effects::dancing_dagger_hit(forged,foe,conditions,20,1000).accepted,"reject forged projectile count");
 const auto launch=skills::particles::spell_projectile_launch_2022(42,-1440.f,1980.f,0,.03f,858);
 check(launch.size()==29&&launch[0]==163&&launch[1]==128&&launch[20]==86&&launch[21]==3,"2022 model854 launch");
 const auto impact=skills::particles::dancing_dagger_impact_2022(43,42);
 check(impact.size()==18&&impact[0]==159&&impact[1]==0&&impact[14]==87&&impact[15]==3,"2022 resource855 impact");
 std::cout<<"PASS Dancing Daggers detached three-projectile damage policy\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
