#include "gw_reversal_npc_wire.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("NPC Health-loss revision line "+std::to_string(at.line()));
}
combat::npcs::LiveActor actor(){
    combat::npcs::Definition d;d.binding.map=161;d.binding.actor=d.binding.definition=100;d.binding.level=4;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{100,100};
    d.damageable=true;d.conditions_allowed=true;d.fire_armor=0;d.earth_armor=0;
    return {d,{1,1,1,161,100}};
}
int main()try{
    auto n=actor();const auto life=n.lifetime();check(n.health_loss_revision(life)==0);
    auto preview=n;check(preview.apply_damage(life,{combat::Kind::armor_ignoring,1,20,194},1000).applied==1);
    check(n.health_loss_revision(life)==0&&preview.health_loss_revision(life)==1);
    n=preview;check(n.heal(life,1,1000));check(n.actor(life)->health->current==100&&n.health_loss_revision(life)==1);
    check(n.apply_damage(life,{combat::Kind::armor_ignoring,0,20,194},1000).applied==0);check(n.health_loss_revision(life)==1);
    auto stale=life;++stale.actor_generation;check(!n.health_loss_revision(stale));
    check(n.apply_damage(stale,{combat::Kind::armor_ignoring,1,20,194},1000).reason!=combat::Reason::accepted);
    check(n.health_loss_revision(life)==1);
    auto older=n;check(n.apply_damage(life,{combat::Kind::armor_ignoring,1,20,194},1000).applied==1);
    check(older.preserve_current_hexes(n,life,1000));check(older.health_loss_revision(life)==2);
    n=older;
    check(n.apply_condition(life,combat::conditions::Kind::poison,1,1000,2000));n.advance_conditions(life,1100);
    check(n.health_loss_revision(life)==3);const auto revision=*n.health_loss_revision(life);
    check(n.heal(life,100,1100));check(n.health_loss_revision(life)==revision);
    auto fresh=actor();check(fresh.health_loss_revision(fresh.lifetime())==0);
    auto spell=actor();const auto spell_life=spell.lifetime();
    check(spell.apply_elemental(spell_life,*skills::effects::elemental(194,0),4,1,1000).has_value());
    check(spell.health_loss_revision(spell_life)==1);
    auto loss=actor();const auto loss_life=loss.lifetime();
    check(loss.apply_grenths_balance(loss_life,{1,100},1000).has_value());check(loss.health_loss_revision(loss_life)==1);
    {
        combat::npcs::Definition d;d.binding.map=161;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{50,1000};
        d.damageable=d.conditions_allowed=true;
        combat::npcs::LiveActor wounded(d,{1,1,1,161,100});const auto life=wounded.lifetime();
        check(wounded.apply_condition(life,combat::conditions::Kind::deep_wound,1,100,10000));
        check(wounded.actor(life)->health->current==1&&wounded.actor(life)->health->maximum==900);
        check(wounded.apply_damage(life,{combat::Kind::armor_ignoring,0,20,194},100).reason==combat::Reason::accepted);
        check(wounded.actor(life)->health->current==1);
        const auto revision_before=*wounded.health_loss_revision(life);
        const auto hit=wounded.apply_damage(life,{combat::Kind::armor_ignoring,.5,20,194},100);
        check(hit.reason==combat::Reason::accepted);
        check(wounded.actor(life)->health->current==0&&hit.killed&&hit.applied==.5);
        check(hit.health_settlement.has_value()&&combat::valid_damage_health(hit));
        check(!combat::damage_packets_2022(1,hit).empty());
        for(unsigned mode=0;mode<7;++mode){
            auto forged=hit;
            if(mode==0)forged.health_settlement.reset();
            if(mode==1)forged.damage+=.1;
            if(mode==2)++forged.generation;
            if(mode==3)++forged.target;
            if(mode==4)forged.before.current+=.1;
            if(mode==5)forged.after.current=.5;
            if(mode==6)++forged.before.maximum;
            bool rejected=false;try{combat::damage_packets_2022(1,forged);}catch(const std::invalid_argument&){rejected=true;}
            check(rejected);
        }
        check(wounded.health_loss_revision(life)==revision_before+1);
        check(wounded.apply_damage(life,{combat::Kind::armor_ignoring,.5,20,194},100).reason!=combat::Reason::accepted);
        check(wounded.health_loss_revision(life)==revision_before+1);
        combat::npcs::LiveActor weapon(d,life);
        check(weapon.apply_condition(life,combat::conditions::Kind::deep_wound,1,100,10000));
        const auto contact=weapon.apply_weapon_damage(life,.5,100);
        check(contact&&contact->killed&&contact->health_settlement&&weapon.actor(life)->health->current==0);
        check(!combat::weapon::packets_2022(1,*contact).empty());
        auto forged=*contact;forged.health_settlement.reset();bool rejected=false;
        try{combat::weapon::packets_2022(1,forged);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
        auto area_definition=d;area_definition.earth_armor=400;
        combat::npcs::LiveActor area(area_definition,life);
        check(area.apply_condition(life,combat::conditions::Kind::deep_wound,1,100,10000));
        const auto blast=area.apply_earth_aoe(life,*skills::effects::caster_earth_aoe(937,0),0,20,1,100);
        check(blast&&blast->accepted&&area.actor(life)->health->current==0);
        check(blast->hits.size()==1&&blast->hits[0].killed&&blast->hits[0].health_settlement);
        check(!combat::damage_packets_2022(1,blast->hits[0]).empty());
        d.healing_allowed=true;d.healing_evidence="authored wounded conversion";
        combat::npcs::LiveActor conversion(d,life);
        check(conversion.apply_condition(life,combat::conditions::Kind::deep_wound,1,100,10000));
        check(conversion.apply_reversal_fortune(life,0,100,80,100));
        const auto receipt=conversion.apply_reversal_damage(life,{combat::Kind::armor_ignoring,.5,20,194},100,true);
        check(receipt&&!receipt->damage&&receipt->after_healing.current==0&&conversion.actor(life)->health->current==0);
        check(!combat::npcs::reversal_damage_packets_2022(100,1,*receipt).empty());
    }
    std::cout<<"NPC Health-loss receipts preserve restored hits, private previews and scoped lifetime\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
