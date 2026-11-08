#include "gw_putrid_bile_timeline.h"
#include <iostream>
#include <map>
#include <source_location>
using namespace fw::gw;
using namespace skills::effects;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Bile forecast line "+std::to_string(at.line()));
}
combat::npcs::LiveActor npc(unsigned id,double health){
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=id;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{health,100};
    d.damageable=d.conditions_allowed=true;return {d,{1,1,1,146,id}};
}
int main()try{
    const Scope caster{1,1,1};
    {
        PutridBileLinks links;const PutridBileLink link{caster,{1,100,1},7,0,20,100,5100};
        check(links.assign(link));auto saved=links.pending();check(saved.size()==1&&saved[0]==link);
        saved[0].effect=999;check(links.lookup(link.target)==link);
        check(links.remove(link.target,7)&&links.pending().empty()&&saved.size()==1);
    }
    for(bool reverse:{false,true}){
        std::map<unsigned,combat::npcs::LiveActor> live;
        live.emplace(100,npc(100,reverse?2:6));live.emplace(200,npc(200,reverse?6:2));
        std::vector<PutridBileLink> links;
        for(auto&[id,n]:live){check(n.apply_single_degeneration_hex(n.lifetime(),*degeneration_hex(2058,0),1,id,100).has_value());
            links.push_back({caster,n.hex_scope(),id,0,20,100,5100});}
        const auto lookup=[&](Scope scope)->std::optional<BileTimelineTarget>{
            const auto n=live.find(scope.actor);return n==live.end()?std::nullopt:std::optional(BileTimelineTarget{scope,n->second,{},{}});
        };
        auto result=forecast_putrid_bile(links,10100,lookup);
        check(result.reason==BileForecastReason::okay&&result.next&&result.next->at==1100&&
            result.next->kind==BileBoundaryKind::npc_death&&result.next->target.actor==(reverse?100u:200u));
        for(const auto&[id,n]:live)check(n.actor(n.lifetime())->health->current==(id==(reverse?100u:200u)?2:6));
        const auto dead=result.next->target;
        check(live.at(dead.actor).advance_conditions(live.at(dead.actor).lifetime(),1100)->killed);
        std::erase_if(links,[&](const auto& link){return link.target==dead;});
        result=forecast_putrid_bile(links,10100,lookup);check(result.next&&result.next->at==3100);
        // External events restrict forecasting before a previously predicted death.
        result=forecast_putrid_bile(links,2100,lookup);check(result.reason==BileForecastReason::okay&&!result.next);
        auto wrong=links;wrong[0].effect++;
        check(forecast_putrid_bile(wrong,10100,lookup).reason==BileForecastReason::effect);
        wrong=links;wrong[0].target.generation++;
        check(forecast_putrid_bile(wrong,10100,lookup).reason==BileForecastReason::lifetime);
        wrong=links;wrong[0].caster_level=0;
        check(forecast_putrid_bile(wrong,10100,lookup).reason==BileForecastReason::link);
    }
    for(double health:{10.,100.}){
        auto n=npc(100,health);check(n.apply_single_degeneration_hex(n.lifetime(),*degeneration_hex(2058,0),1,7,100).has_value());
        const std::vector<PutridBileLink> links{{caster,n.hex_scope(),7,0,20,100,5100}};
        const auto lookup=[&](Scope scope)->std::optional<BileTimelineTarget>{return BileTimelineTarget{scope,n,{},{}};};
        auto result=forecast_putrid_bile(links,10100,lookup);
        check(result.reason==BileForecastReason::okay&&result.next&&result.next->at==5100&&result.next->kind==BileBoundaryKind::expiry);
        auto forged=links;forged[0].expires=5101;
        check(forecast_putrid_bile(forged,10100,lookup).reason==BileForecastReason::effect);
        n.advance_conditions(n.lifetime(),6100);
        // Health has advanced beyond the old expiry; it is not old-time proof.
        check(forecast_putrid_bile(links,10100,lookup).reason==BileForecastReason::clock);
        n.refresh_hexes(n.lifetime(),6100);
        check(forecast_putrid_bile(links,10100,lookup).reason==BileForecastReason::effect);
    }
    {
        // Player expiry is validated without pretending to forecast player Health.
        const Scope player{1,2,1};Entry hex{7,1,2058,0,Category::hex,100,3100};
        const std::vector<PutridBileLink> links{{caster,player,7,0,20,100,3100}};
        const auto lookup=[&](Scope scope)->std::optional<BileTimelineTarget>{return BileTimelineTarget{scope,{},hex,{}};};
        const auto result=forecast_putrid_bile(links,5100,lookup);
        check(result.reason==BileForecastReason::okay&&result.next&&result.next->at==3100&&result.next->kind==BileBoundaryKind::expiry);
        hex.skill=99;check(forecast_putrid_bile(links,5100,lookup).reason==BileForecastReason::effect);
        check(forecast_putrid_bile(links,5100,[](Scope)->std::optional<BileTimelineTarget>{return {};}).reason==BileForecastReason::lifetime);
    }
    {
        // External pips must be the actual exposure used by World settlement.
        auto n=npc(100,6);check(n.apply_single_degeneration_hex(n.lifetime(),*degeneration_hex(2058,0),1,7,100).has_value());
        const std::vector<PutridBileLink> links{{caster,n.hex_scope(),7,0,20,100,5100}};
        combat::npcs::LiveActor::ExternalHealth external;external.cause=1;
        external.boundaries={1100};external.pips=[](Time at){return at<1100?-1:0;};
        const auto result=forecast_putrid_bile(links,10100,[&](Scope scope)->std::optional<BileTimelineTarget>{return BileTimelineTarget{scope,n,{},external};});
        check(result.reason==BileForecastReason::okay&&result.next&&result.next->at==2100&&result.next->kind==BileBoundaryKind::npc_death);
        check(n.actor(n.lifetime())->health->current==6&&n.health_clock(n.lifetime())==100);
        auto future=n;future.advance_conditions(future.lifetime(),4100);
        check(forecast_putrid_bile(links,2100,[&](Scope scope)->std::optional<BileTimelineTarget>{return BileTimelineTarget{scope,future,{},{}};}).reason==BileForecastReason::clock);
    }
    std::cout<<"Copy-only Bile death/expiry forecast, exact2058 ownership, both actor orders and external pips; World orchestration pending\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
