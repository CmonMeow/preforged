#include "gw_npc_combat_catalog.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Stoneflesh NPC line "+std::to_string(at.line()));
}
int main()try{
    using namespace combat;using namespace combat::npcs;using namespace skills::effects;
    const Lifetime life{1,1,1,146,100},stale{1,1,2,146,100};
    Definition d;d.binding.actor=100;d.binding.map=146;d.spawn_health=Health{1000,1000};
    d.damageable=d.conditions_allowed=true;d.relation=Relation::hostile_to_players;
    d.physical_armor=d.general_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=60;
    const auto plan=*stoneflesh_aura(12);
    for(auto kind:{Kind::earth,Kind::fire,Kind::cold,Kind::lightning,Kind::holy,
                  Kind::piercing,Kind::slashing,Kind::armor_ignoring}){
        LiveActor actor(d,life);
        check(!actor.apply_stoneflesh_aura(stale,plan,500,100));
        check(actor.apply_stoneflesh_aura(life,plan,500,100));
        auto copy=actor;
        auto hit=copy.apply_damage(life,{kind,100,20,1375},101);
        check(hit.reason==Reason::accepted&&hit.damage==75);
        auto expired=actor.apply_damage(life,{kind,100,20,1375},100+plan.duration);
        check(expired.reason==Reason::accepted&&expired.damage==100);
    }
    LiveActor actor(d,life);check(actor.apply_stoneflesh_aura(life,plan,500,100));
    weapon::Strike strike;strike.critical=true;
    check(actor.protect_stoneflesh_critical(life,strike,101));
    check(strike.critical&&strike.critical_damage_suppressed&&strike.critical_energy_suppressed);
    weapon::Strike expired_strike;
    check(actor.protect_stoneflesh_critical(life,expired_strike,100+plan.duration));
    check(!expired_strike.critical_damage_suppressed&&!expired_strike.critical_energy_suppressed);
    const auto weapon=actor.apply_weapon_damage(life,100,101);
    check(weapon&&weapon->damage==75);
    check(actor.remove_reviewed_enchantment(life,500,102));
    const auto unprotected=actor.apply_weapon_damage(life,100,103);
    check(unprotected&&unprotected->damage==100);
    check(actor.apply_stoneflesh_aura(life,plan,501,104));
    check(actor.apply_stoneflesh_aura(life,*stoneflesh_aura(0),502,105));
    check(!actor.hex_effect(life,501));
    const auto renewed=actor.apply_weapon_damage(life,100,106);
    check(renewed&&renewed->damage==99);
    LiveActor stripped(d,life);check(stripped.apply_stoneflesh_aura(life,plan,600,100));
    const auto registry=*stripped.owned_effect_snapshot(life);
    const Scope scope{life.world,life.actor,life.actor_generation};
    const auto ending=EndingStonefleshContext::capture(registry,scope,600,200);
    check(bool(ending));check(!ending->plan(scope,201));
    check(stripped.remove_reviewed_enchantment(life,600,200));
    const auto shatter=stripped.apply_removal_damage(life,{Kind::armor_ignoring,100,20,69},200,&*ending);
    check(shatter.reason==Reason::accepted&&shatter.damage==75);
    check(stripped.apply_removal_damage(life,{Kind::armor_ignoring,100,20,69},201,&*ending).reason!=Reason::accepted);
    for(unsigned mode:{0u,1u,2u}){
        auto known=d;known.healing_allowed=true;known.healing_evidence="authored context conversion";
        LiveActor live(known,life);
        check(live.apply_reversal_fortune(life,0,100,700,100));
        check(live.stage_shield_absorption(life,12,100,701,100));
        check(live.apply_stoneflesh_aura(life,plan,702,100));
        const auto owned=*live.owned_effect_snapshot(life);
        const auto context=EndingStonefleshContext::capture(owned,scope,702,200);check(bool(context));
        check(live.remove_reviewed_enchantment(life,702,200));
        if(mode==2)check(live.apply_stoneflesh_aura(life,*stoneflesh_aura(0),703,200));
        auto stale_context=live;
        check(stale_context.apply_damage(life,{Kind::armor_ignoring,100,20,69},201,nullptr,&*context).reason!=Reason::accepted);
        const Damage input{Kind::armor_ignoring,mode==1?20.:100.,20,69};
        auto raw=live;const auto hit=raw.apply_damage(life,input,200,nullptr,&*context);check(hit.reason==Reason::accepted);
        if(mode==1){
            check(hit.damage==0);live=std::move(raw);
            check(live.owned_effect_snapshot(life)->newest_skill(scope,307,200).has_value());
            const auto converted=live.apply_reversal_damage(life,{Kind::armor_ignoring,100,20,69},200,true);
            check(converted&&converted->damage&&converted->damage->damage==75);
        }else{
            const auto converted=live.apply_reversal_damage(life,input,200,true,nullptr,&*context);
            check(converted&&converted->damage&&converted->damage->damage==(mode==2?79:55));
            check(live.actor(life)->health->current==(mode==2?921:945));
            const auto next=live.apply_damage(life,{Kind::armor_ignoring,100,20,69},200);
            check(next.reason==Reason::accepted&&next.damage==(mode==2?89:90));
        }
    }
    std::cout<<"PASS Stoneflesh NPC damage, resolved weapon, renewal, removal and expiry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

