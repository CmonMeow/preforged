#include "gw_world_simulation.h"
#include <iostream>
#include <stdexcept>
int main()try{
using namespace fw::gw::world;using namespace fw::gw::skills;
auto require=[](bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);};
        const auto blood_plan=*regeneration::reviewed(115,0);
        auto renewal_world=std::make_unique<Instance>();auto& renewed=*renewal_world;auto renewal_box=std::make_shared<Outbox>();
        renewed.join(1,"Blood Necromancer",0,0,renewal_box);renewed.set_vitals(1,{100,1000,20,20},100);
        const auto renewal_scope=*renewed.effect_scope(1);
        require(renewed.apply_health_regeneration(renewal_scope,1,blood_plan,1000,8000,0),"Blood Renewal authoritative self effect");
        renewed.tick(9000);
        require(renewed.resources(1)->health==182,"Blood Renewal delayed tick integrates42 regeneration and40 end heal");
        renewed.tick(10000);require(renewed.resources(1)->health==182,"Blood Renewal end heal fires once");
        require(renewed.apply_health_regeneration(renewal_scope,1,blood_plan,11000,18000,0)&&
            renewed.apply_health_regeneration(renewal_scope,1,blood_plan,12000,19000,0),"Blood Renewal replacement");
        // Local Renewal.html: renewing by recasting does not end the skill.
        require(renewed.resources(1)->health==188,"renewal incorrectly fired the previous enchantment's end heal");
        renewed.tick(18000);require(renewed.resources(1)->health==224,"old deadline ended the refreshed enchantment");
        renewed.tick(20000);require(renewed.resources(1)->health==270,"replacement contributes regeneration and its own end heal once");
        require(renewed.apply_health_regeneration(renewal_scope,1,blood_plan,21000,28000,0),"Blood Renewal removal fixture");
        const auto effect=renewed.status_snapshot(renewal_scope,21000)->front();
        require(renewed.status_mutation(renewal_scope,115,effect.instance,0,21000,{},false)&&renewed.resources(1)->health==310,"actual removal must trigger end healing");
        require(renewed.status_mutation(renewal_scope,115,effect.instance,0,21000,{},false)&&renewed.resources(1)->health==310,"duplicate removal repeated end healing");
        renewed.tick(29000);require(renewed.resources(1)->health==310,"removed enchantment retained regeneration or expiry healing");

std::cout<<"PASS Blood Renewal expiry and replacement\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
