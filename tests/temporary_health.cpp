#include "gw_temporary_health.h"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace fw::gw::skills::effects;
void check(bool value){if(!value)throw std::runtime_error("temporary Health regression");}
int main()try{
    const Scope owner{1,42,1};unsigned cases=0;
    for(unsigned base:{100u,400u,500u,1000u})for(unsigned rank:{0u,12u,20u})for(bool wound:{false,true})for(unsigned life=0;life<4;++life){
        const bool dead=life==0;
        TemporaryHealth ledger(owner,base,100);TemporaryHealthSnapshot snapshot{owner,100,dead?0.:life==1?1.:life==2?double(base)/2:double(base),{}};snapshot.capacity.reset(base);snapshot.capacity.capacity(snapshot.current,base,wound);
        const auto applied=prepare_to_limit_health(ledger,snapshot,*to_limit(rank),7,wound);check(applied.has_value());check(ledger.capacity()==base&&!ledger.source());
        check(applied->ledger.permanent()==base&&applied->ledger.capacity()==base+to_limit(rank)->health_bonus);
        const double expected=dead?0.:snapshot.current-snapshot.capacity.debt+to_limit(rank)->health_bonus-(applied->health.capacity.reduction-snapshot.capacity.reduction);
        check(applied->health.current-applied->health.capacity.debt==expected);
        auto at_end=applied->health;at_end.at=*applied->ledger.deadline();const auto ended=prepare_temporary_health_end(applied->ledger,at_end,7,wound);check(ended&&ended->ledger.capacity()==base&&ended->health.current==snapshot.current&&ended->health.capacity.debt==snapshot.capacity.debt);++cases;
    }
    // Independent sources stack and refresh only their own contribution.
    for(bool wound:{false,true})for(unsigned rank:{0u,12u,20u}){
        TemporaryHealth base(owner,500,100);TemporaryHealthSnapshot state{owner,100,100,{}};state.capacity.reset(500);state.capacity.capacity(state.current,500,wound);
        const auto pain=prepare_endure_pain_health(base,state,*endure_pain(rank),10,wound);check(pain.has_value());
        auto second=pain->health;second.at=101;
        const auto both=prepare_to_limit_health(pain->ledger,second,*to_limit(0),11,wound);check(both&&both->ledger.capacity()==500+endure_pain(rank)->health_bonus+10);
        auto refreshed=both->health;refreshed.at=102;
        const auto again=prepare_endure_pain_health(both->ledger,refreshed,*endure_pain(0),12,wound);check(again&&again->removed==10&&again->ledger.source()->effect==11&&again->ledger.capacity()==600);
        auto end=again->health;end.at=*again->ledger.deadline();const auto first=prepare_temporary_health_expiry(again->ledger,end,wound);check(first&&first->removed==12&&first->ledger.capacity()==510);
        end=first->health;end.at=*first->ledger.deadline();const auto last=prepare_temporary_health_expiry(first->ledger,end,wound);check(last&&last->removed==11&&last->ledger.capacity()==500&&std::abs(last->health.current-state.current)<1e-8&&std::abs(last->health.capacity.debt-state.capacity.debt)<1e-8);
        check(!base.endure_pain_source()&&!base.source());
    }
    for(unsigned pain_rank:{4u,20u}){TemporaryHealth base(owner,500,100);TemporaryHealthSnapshot state{owner,100,5,{}};state.capacity.reset(500);
        auto pain=*prepare_endure_pain_health(base,state,*endure_pain(pain_rank),10,false);
        auto both=*prepare_to_limit_health(pain.ledger,pain.health,*to_limit(0),11,false);
        auto end=both.health;end.at=*both.ledger.deadline();auto first=*prepare_temporary_health_expiry(both.ledger,end,false);
        check(first.removed==11&&first.ledger.endure_pain_source()->effect==10&&!first.ledger.source());
        end=first.health;end.at=*first.ledger.deadline();auto last=*prepare_temporary_health_expiry(first.ledger,end,false);
        check(last.removed==10&&last.ledger.capacity()==500&&last.health.current==5&&!last.ledger.deadline());
        auto invalid=*endure_pain(0);++invalid.health_bonus;check(!base.apply_endure_pain(invalid,10,100));
        check(!both.ledger.apply_endure_pain(*endure_pain(0),12,99));
    }
    for(bool pvp:{false,true})for(bool wound:{false,true})for(unsigned rank:{0u,12u,20u}){
        TemporaryHealth base(owner,500,100);TemporaryHealthSnapshot state{owner,100,100,{}};state.capacity.reset(500);state.capacity.capacity(state.current,500,wound);
        auto pain=*prepare_endure_pain_health(base,state,*endure_pain(0),10,wound);
        auto limit=*prepare_to_limit_health(pain.ledger,pain.health,*to_limit(0),11,wound);
        auto defy=prepare_defy_pain_health(limit.ledger,limit.health,*defy_pain(rank,pvp),12,wound);check(defy&&defy->ledger.capacity()==600+defy_pain(rank,pvp)->health_bonus);
        auto next=defy->health;next.at=101;auto refresh=prepare_defy_pain_health(defy->ledger,next,*defy_pain(0,!pvp),13,wound);
        check(refresh&&refresh->removed==12&&refresh->ledger.source()->effect==11&&refresh->ledger.endure_pain_source()->effect==10&&refresh->ledger.capacity()==690);
        auto resized=prepare_permanent_health_resize(refresh->ledger,refresh->health,530,wound);check(resized&&resized->ledger.capacity()==720);
        auto current=*resized;
        for(unsigned effect:{10u,11u,13u}){auto at=current.health;at.at=*current.ledger.deadline();auto end=prepare_temporary_health_expiry(current.ledger,at,wound);check(end&&end->removed==effect);current=*end;}
        check(current.ledger.capacity()==530&&!current.ledger.deadline()&&current.health.current-current.health.capacity.debt==state.current-state.capacity.debt+30);
        check(!base.defy_pain_source());auto invalid=*defy_pain(0);++invalid.armor;check(!base.apply_defy_pain(invalid,10,100));
    }
    TemporaryHealth ledger(owner,500,100);TemporaryHealthSnapshot snapshot{owner,100,5,{}};snapshot.capacity.reset(500);
    auto applied=*prepare_to_limit_health(ledger,snapshot,*to_limit(0),7,false);check(applied.health.current==15);
    applied.health.capacity.event(applied.health.current,-10);check(applied.health.current==5);applied.health.at=*applied.ledger.deadline();
    auto ended=*prepare_temporary_health_end(applied.ledger,applied.health,7,false);check(ended.health.current==1&&ended.health.capacity.debt==6);
    auto healed=ended.health;healed.capacity.event(healed.current,4,true);check(healed.current==0);
    healed=ended.health;healed.capacity.event(healed.current,10,true);check(healed.current==5);
    auto recovered=ended.health;recovered.capacity.regeneration(recovered.current,6);check(recovered.current==1&&recovered.capacity.debt==0);
    auto wounded=*prepare_temporary_health_condition(ended.ledger,ended.health,true);check(wounded.health.current==1&&wounded.health.capacity.debt==106);
    auto cleared=*prepare_temporary_health_condition(wounded.ledger,wounded.health,false);check(cleared.health.current==1&&cleared.health.capacity.debt==6);
    auto changed=applied.health;changed.at=101;auto permanent=*prepare_permanent_health_resize(applied.ledger,changed,530,false);check(permanent.ledger.permanent()==530&&permanent.ledger.capacity()==540&&permanent.health.current==35);
    permanent.health.at=*permanent.ledger.deadline();auto removed=*prepare_temporary_health_end(permanent.ledger,permanent.health,7,false);check(removed.ledger.capacity()==530&&removed.health.current==25);
    auto stale=snapshot;++stale.owner.generation;check(!prepare_to_limit_health(ledger,stale,*to_limit(0),7,false));
    check(!prepare_to_limit_health(applied.ledger,applied.health,*to_limit(0),7,false));
    auto late=applied.health;late.at=*applied.ledger.deadline()+1;check(!prepare_temporary_health_end(applied.ledger,late,7,false));
    auto early=applied.health;early.at=*applied.ledger.deadline()-1;check(!prepare_temporary_health_expiry(applied.ledger,early,false));
    early.at=*applied.ledger.deadline();check(prepare_temporary_health_expiry(applied.ledger,early,false).has_value());
    auto initial=snapshot;initial.current=400;auto first=*prepare_to_limit_health(ledger,initial,*to_limit(0),7,false);first.health.at=101;
    auto refreshed=*prepare_to_limit_health(first.ledger,first.health,*to_limit(12),8,false);check(refreshed.removed==7&&refreshed.ledger.capacity()==550&&refreshed.health.current==450);
    refreshed.health.at=102;auto again=*prepare_to_limit_health(refreshed.ledger,refreshed.health,*to_limit(12),9,false);check(again.health.current==450&&again.ledger.capacity()==550);
    again.health.at=103;check(!prepare_to_limit_health(again.ledger,again.health,*to_limit(12),8,false));
    auto invalid=snapshot;invalid.current=501;check(!prepare_to_limit_health(ledger,invalid,*to_limit(0),7,false));
    invalid=snapshot;invalid.current=std::numeric_limits<double>::quiet_NaN();check(!prepare_to_limit_health(ledger,invalid,*to_limit(0),7,false));
    invalid=snapshot;invalid.capacity.base=501;check(!prepare_to_limit_health(ledger,invalid,*to_limit(0),7,false));
    invalid=snapshot;invalid.capacity.debt=-1;check(!prepare_to_limit_health(ledger,invalid,*to_limit(0),7,false));
    check(!prepare_permanent_health_resize(first.ledger,first.health,1000000,false));
    std::cout<<"PASS"<<cases<<" capacity/rank/DeepWound/death round trips,latent negative Health,healing/regen,permanent resize and identity/clock guards;World integration pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
