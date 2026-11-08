#include "gw_stamina_signet.h"
#include <stdexcept>
#include <iostream>

using namespace fw::gw::skills::effects;
using fw::gw::combat::weapon::AttackDelivery;
using fw::gw::combat::weapon::QualifiedHit;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}

int main()try{
    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=stamina_signet(rank);
        check(plan&&plan->health_bonus==interpolate(50,300,rank)&&
              plan->activation==250&&plan->aftercast==750&&plan->recharge==20000,
              "native ranked plan");
    }
    check(!stamina_signet(21),"invalid rank");
    const Scope owner{7,25,3},foe{7,30,8};
    Registry registry(owner);
    check(registry.apply(owner,{80,25,1411,12,Category::other,1000,{}},Replace::same_skill).accepted,
          "effect application");
    check(active_stamina_signet(registry,owner,1200)->instance==80,"owned status");
    const QualifiedHit old{owner,foe,1,900,1200,AttackDelivery::melee};
    check(!end_stamina_on_hit(registry,owner,old),"preapplication release cannot consume");
    const QualifiedHit stale{{7,25,4},foe,2,1100,1200,AttackDelivery::melee};
    check(!end_stamina_on_hit(registry,owner,stale),"stale attacker lifetime");
    const QualifiedHit unclassified{owner,foe,3,1100,1200,AttackDelivery::unknown};
    check(!end_stamina_on_hit(registry,owner,unclassified),"unclassified delivery");
    const QualifiedHit valid{owner,foe,4,1100,1200,AttackDelivery::ranged};
    const auto ended=end_stamina_on_hit(registry,owner,valid);
    check(ended&&ended->instance==80&&!active_stamina_signet(registry,owner,1200),
          "qualified hit ends exact effect");
    check(!end_stamina_on_hit(registry,owner,valid),"duplicate hit cannot consume twice");
    check(registry.apply(owner,{81,25,1411,12,Category::other,1300,{}},Replace::same_skill).accepted,
          "recast");
    check(!end_stamina_on_hit(registry,owner,valid)&&active_stamina_signet(registry,owner,1400),
          "old hit cannot consume refreshed effect");
    std::cout<<"PASS Signet of Stamina ranked plan and scoped qualified-hit ending\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
