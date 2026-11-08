#include "gw_temporary_health.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace fw::gw::skills::effects;
static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}

int main()try{
    const Scope owner{4,25,6};
    for(const bool wounded:{false,true})for(const unsigned rank:{0u,12u,20u}){
        TemporaryHealth ledger(owner,400,100);
        TemporaryHealthSnapshot start{owner,100,250,{}};
        start.capacity.reset(400);
        start.capacity.capacity(start.current,400,wounded);
        const auto plan=*stamina_signet(rank);
        auto applied=prepare_stamina_signet_health(ledger,start,plan,10,wounded);
        check(applied&&applied->ledger.stamina_signet_source()&&
              applied->ledger.capacity()==400+plan.health_bonus&&
              !applied->ledger.deadline(),"indefinite bonus joins permanent capacity");
        auto removal=applied->health;removal.at=200;
        auto ended=prepare_temporary_health_end(applied->ledger,removal,10,wounded);
        check(ended&&ended->ledger.capacity()==400&&
              std::abs(ended->health.current-start.current)<1e-9&&
              std::abs(ended->health.capacity.debt-start.capacity.debt)<1e-9,
              "successful-hit ending restores prior Health and wound debt");
        applied->health.at=150;
        auto refreshed=prepare_stamina_signet_health(applied->ledger,applied->health,plan,11,wounded);
        check(refreshed&&refreshed->removed==10&&
              refreshed->ledger.capacity()==400+plan.health_bonus,
              "recast replaces only prior signet contribution");
        auto old=refreshed->health;old.at=160;
        check(!prepare_temporary_health_end(refreshed->ledger,old,10,wounded),
              "retired effect cannot remove refreshed bonus");
    }
    TemporaryHealth ledger(owner,400,100);
    TemporaryHealthSnapshot start{owner,100,250,{}};start.capacity.reset(400);
    auto stamina=*prepare_stamina_signet_health(ledger,start,*stamina_signet(0),10,false);
    auto timed_at=stamina.health;timed_at.at=101;
    auto timed=*prepare_endure_pain_health(stamina.ledger,timed_at,*endure_pain(0),11,false);
    check(timed.ledger.capacity()==400+stamina_signet(0)->health_bonus+endure_pain(0)->health_bonus&&
          timed.ledger.stamina_signet_source()->effect==10,
          "indefinite and timed bonuses stack");
    auto expiry=timed.health;expiry.at=*timed.ledger.deadline();
    auto expired=prepare_temporary_health_expiry(timed.ledger,expiry,false);
    check(expired&&expired->removed==11&&expired->ledger.capacity()==450&&
          !expired->ledger.deadline()&&expired->ledger.stamina_signet_source()->effect==10,
          "timed expiry leaves indefinite signet");
    auto permanent=expired->health;permanent.at=expiry.at+1;
    auto resized=prepare_permanent_health_resize(expired->ledger,permanent,430,false);
    check(resized&&resized->ledger.capacity()==480&&
          resized->ledger.stamina_signet_source()->effect==10,
          "equipment or morale capacity change preserves signet bonus");
    std::cout<<"PASS Signet of Stamina shared capacity, wound, recast, timed expiry and permanent resize\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
