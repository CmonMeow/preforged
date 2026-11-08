#include "gw_skill_casting.h"
#include <iostream>

int main()try{
    using namespace fw::gw;using namespace skills;using namespace skills::casting;
    auto check=[](bool ok){if(!ok)throw std::runtime_error("Selfless Runtime semantic target/payment");};
    // The legacy unscoped heal path still supplies a semantic ally scope.
    for(unsigned skill:{281u,283u,275u,301u,288u,287u})for(bool self:{false,true}){
        Policy policy;policy.primary=3;policy.campaigns=255;policy.learned[skill]=1;
        Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[13]=12;ranks[15]=12;
        Hooks h;unsigned legacy=0,payments=0;std::optional<effects::Scope> paid_target;
        h.events=[](const auto&){};
        h.resources=[](unsigned)->std::optional<vitals::Resources>{return vitals::Resources{100,100,50,50};};
        h.target_scope=[](unsigned actor)->std::optional<effects::Scope>{return effects::Scope{1,actor,1};};
        h.target_valid=[](unsigned,float,bool){return true;};
        h.consume_energy_at=[&](unsigned,auto){++legacy;return true;};
        h.consume_selfless_energy=[&](Slot slot,auto target,unsigned,auto){check(slot.skill==skill);++payments;paid_target=target;return true;};
        h.heal=[](unsigned,unsigned,float,bool){return true;};
        h.status_heal=[](unsigned,const auto&,auto){return true;};
        h.remove_conditions=[](unsigned,const auto&,auto){return true;};
        h.remove_effects=[](unsigned,const auto&,auto){return true;};
        h.apply_health_regeneration=[](unsigned,const auto&,auto,auto,float,bool){return true;};
        h.party_heal=[](const auto&,auto){return true;};
        Runtime runtime(42);
        check(runtime.start(100,{skill,0,self?42u:43u},bar,policy,ranks,true,h)==casting::Reason::accepted);
        check(payments==1&&legacy==0);
        check(bool(paid_target)==(!self&&skill!=287));
        if(paid_target)check(paid_target->actor==43);
    }
    std::cout<<"PASS Selfless Runtime legacy ally families and self/party exclusion\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
