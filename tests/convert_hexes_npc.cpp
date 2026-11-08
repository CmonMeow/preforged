#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <string>
using namespace fw::gw;using namespace skills::effects;
void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Convert Hexes NPC line "+std::to_string(at.line()));
}
combat::npcs::Definition definition(bool ally=false){
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
    d.relation=ally?combat::npcs::Relation::allied_to_players:combat::npcs::Relation::hostile_to_players;
    d.spawn_health=combat::Health{500,1000};d.damageable=d.conditions_allowed=true;
    d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=d.physical_armor=60;d.healing_allowed=true;d.healing_evidence="authored fixture";return d;
}
int main()try{
    const Scope source{1,1,1};
    for(unsigned rank=0;rank<=20;++rank){
        combat::npcs::LiveActor n(definition(),{1,1,1,146,100});const auto life=n.lifetime();
        check(n.apply_single_degeneration_hex(life,*degeneration_hex(101,0),3,1,100).has_value());
        check(n.apply_single_degeneration_hex(life,*degeneration_hex(31,0),3,2,100).has_value());
        check(n.apply_single_degeneration_hex(life,*degeneration_hex(108,0),3,4,100).has_value());
        const auto result=n.apply_convert_hexes(life,source,*convert_hexes(rank),3,100);check(result&&result->removed.size()==3&&result->necromancer_hexes==2);
        check(n.convert_hexes_armor(life,100)==10&&n.final_physical_armor(life,100)==70&&n.final_earth_armor(life,100)==70);
        for(unsigned kind:{3u,4u,5u,11u})check(n.final_caster_armor(life,kind,100)==70);
        auto plain=n;check(plain.remove_hex_effect(life,3,100));const auto hit=n.apply_damage(life,{combat::Kind::earth,100,20,174},100);
        const auto unbuffed=plain.apply_damage(life,{combat::Kind::earth,100,20,174},100);
        check(hit.reason==combat::Reason::accepted&&unbuffed.reason==combat::Reason::accepted&&std::abs(hit.applied/unbuffed.applied-std::exp2(-10./40))<1e-7);
        const auto expiry=100+convert_hexes(rank)->duration;check(n.convert_hexes_armor(life,expiry-1)==10&&n.convert_hexes_armor(life,expiry)==0);
    }
    {combat::npcs::LiveActor n(definition(),{1,1,1,146,100});const auto life=n.lifetime();
        check(n.apply_single_degeneration_hex(life,*degeneration_hex(101,0),3,1,100).has_value());check(n.apply_convert_hexes(life,source,*convert_hexes(0),2,100).has_value());
        auto preview=n;check(!n.apply_convert_hexes(life,source,*convert_hexes(0),3,101));check(n.convert_hexes_armor(life,101)==10);
        check(n.apply_single_degeneration_hex(life,*degeneration_hex(101,0),3,3,200).has_value());check(n.apply_convert_hexes(life,source,*convert_hexes(12),4,200).has_value());
        check(preview.preserve_current_hexes(n,life,200)&&preview.convert_hexes_armor(life,9000)==10);
        auto stale=life;++stale.actor_generation;check(!n.apply_convert_hexes(stale,source,*convert_hexes(0),5,200));
        check(n.apply_damage(life,{combat::Kind::armor_ignoring,10000,20,49},201).killed&&n.convert_hexes_armor(life,201)==0);}
    {auto d=definition();d.earth_armor=500;combat::npcs::LiveActor n(d,{1,1,1,146,100});const auto life=n.lifetime();
        check(n.apply_single_degeneration_hex(life,*degeneration_hex(101,0),3,1,100).has_value());check(n.apply_convert_hexes(life,source,*convert_hexes(0),2,100).has_value());
        check(n.final_earth_armor(life,100)==510);const auto hit=n.apply_damage(life,{combat::Kind::earth,1000000,20,174},100);
        check(hit.reason==combat::Reason::accepted&&std::abs(hit.damage-1000000*std::exp2((60.-510)/40))<1e-7);
        d.earth_armor=501;bool rejected=false;try{combat::npcs::LiveActor invalid(d,{1,1,1,146,100});}catch(const std::invalid_argument&){rejected=true;}check(rejected);}
    for(unsigned mode=0;mode<5;++mode){
        world::Instance w;auto d=definition(mode!=4);if(mode==2)d.healing_allowed=false;if(mode==3)d.healing_allowed.reset();
        world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=100;npc.definition.visual_adjustment={0,0,0,100};npc.combat_definition=d;w.configure_npcs({npc});
        auto box=std::make_shared<world::Outbox>();w.join(1,"Monk",0,0,box,0,{{165,0,1,0,0,0,3,0}},{},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        const auto owner=*w.effect_scope(1),target=*w.effect_scope(100);box->drain();const unsigned divine=mode==0?0:12;
        check(w.complete_convert_hexes(owner,target,*convert_hexes(12),divine,100)==(mode<3));
        check(w.npc_combat_snapshot(target)->health->current==(mode==1?538:500));
        const auto packets=box->drain();const auto body=*skills::particles::targeted_body_2022(100,1,303);
        check(std::count(packets.begin(),packets.end(),body)==unsigned(mode<3));check(w.convert_hexes_armor(target,100)==0);
    }
    std::cout<<"PASS21 NPC owned Convert Hexes ranks,armor getters/earth damage,expiry/replacement/preserved-current ownership and death;five World ally/healing/hostility cases;allied-NPC hostile-hex integration still pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
