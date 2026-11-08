#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Gift of Health World line "+std::to_string(at.line()));
}
int main()try{
    // Resource reward commits only after the target and disable proposal pass.
    // Keep the spell reaction identity distinct from the skill-use/disable event.
    for(unsigned version:{0u,181u,2860u})for(bool npc:{false,true}){
        world::Instance world;
        auto box=std::make_shared<world::Outbox>();
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;
            n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
            d.binding.map=146;d.binding.actor=d.binding.definition=100;
            d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={100,1000};
            d.damageable=true;d.healing_allowed=true;d.healing_evidence="Gift completion test";
            n.combat_definition=d;world.configure_npcs({n});}
        world.join(1,"Caster",0,0,box,0,{}, {},100,1);
        world.set_vitals(1,{100,1000,50,100},100);
        if(!npc){world.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            world.set_vitals(2,{100,1000,50,100},100);}
        const auto source=*world.effect_scope(1),target=*world.effect_scope(npc?100:2);
        Slots slots{};slots[0]={1121,0};slots[1]={1120,0};
        check(world.configure_adrenaline(source,slots,100));
        if(version){const auto renewal=*effects::ether_renewal(version,15);
            check(world.status_mutation(source,version,80,15,100,100+renewal.duration,true));}
        check(world.before_spell_effect(source,1121,91,200));box->drain();
        const auto plan=*effects::gift_health(0);std::function<void()> publish;
        auto complete=[&](effects::Scope recipient,std::uint64_t reaction){
            return npc?world.stage_gift_health_npc(source,recipient,plan,0,7,200,&publish,reaction):
                world.stage_gift_health_player(source,recipient,plan,0,7,200,&publish,reaction);};
        auto stale=target;++stale.generation;
        check(!complete(stale,91));check(world.resources(1)->health==100);
        check(!complete(target,90));check(world.resources(1)->energy==50);
        check(complete(target,91));check(bool(publish));check(box->drain().empty());
        check(world.resources(1)->health==100+(version?20:0));
        check(world.resources(1)->energy==50+(version?effects::ether_renewal(version,15)->energy_per_enchantment:0));
        check(npc?world.npc_combat_snapshot(target)->health->current==115:world.resources(2)->health==115);
        check(world.skill_disabled_slot(source,{1120,0},200)==10200);
        publish();check(!box->drain().empty());publish();check(box->drain().empty());
        check(!complete(target,91));check(world.resources(1)->health==100+(version?20:0));
    }
    world::Instance w;
    for(unsigned id:{1u,2u,3u}){
        w.join(id,"Actor",id==3?1800.f:float(id*40),0,
            std::make_shared<world::Outbox>(),0,{}, {},100,id==3?2:1);
        w.set_vitals(id,{id==1?500u:200u,500,100,100},100);
    }
    const auto source=*w.effect_scope(1),target=*w.effect_scope(2),foe=*w.effect_scope(3);
    Slots bar{};bar[0]={1121,0};bar[1]={1119,0};bar[2]={1120,0};bar[3]={887,0};
    check(w.configure_adrenaline(source,bar,100));
    const auto plan=*skills::effects::gift_health(0);
    check(!w.stage_gift_health_player(source,source,plan,0,1,200));
    check(!w.stage_gift_health_player(source,foe,plan,0,1,200));
    check(w.resources(2)->health==200);
    check(w.stage_gift_health_player(source,target,plan,0,1,200));
    check(w.resources(2)->health==215);
    check(w.skill_disabled_slot(source,{1119,0},200)==10200);
    check(w.skill_disabled_slot(source,{1120,0},200)==10200);
    check(w.skill_disabled_slot(source,{887,0},200)==10200);
    check(w.skill_disabled_slot(source,{1121,0},200)==0);
    check(!w.stage_gift_health_player(source,target,plan,0,1,300));
    check(w.resources(2)->health==215);
    const auto revision=w.disable_reset_revision(source);check(revision&&(*revision)[7]>0);
    check(w.reset_skill_disables(source,{{1119,0}},*revision,300));
    check(w.skill_disabled_slot(source,{1119,0},300)==0);
    check(w.skill_disabled_slot(source,{1120,0},300)==10200);
    check(!w.reset_skill_disables(source,{{1120,0}},*revision,301));
    check(w.stage_gift_health_player(source,target,plan,0,2,400));
    check(w.resources(2)->health==230);
    check(w.skill_disabled_slot(source,{1119,0},400)==10400);
    w.leave(1);check(!w.skill_disabled_slot(source,{1119,0},400));
    {
        world::Instance party;
        party.join(10,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,0);
        party.join(11,"Ally",40,0,std::make_shared<world::Outbox>(),0,{}, {},100,0);
        party.set_vitals(10,{500,500,100,100},100);
        party.set_vitals(11,{200,500,100,100},100);
        const auto caster=*party.effect_scope(10),recipient=*party.effect_scope(11);
        check(!party.valid_gift_health_target(caster,recipient,200));
        check(party.party_action(10,158,11,100)&&party.party_action(11,154,10,100));
        check(party.configure_adrenaline(caster,bar,100));
        check(party.valid_gift_health_target(caster,recipient,200));
        check(party.stage_gift_health_player(caster,recipient,plan,0,1,200));
        check(party.resources(11)->health==215);
    }
    {
        world::Instance ordered;
        auto caster_box=std::make_shared<world::Outbox>();
        auto ally_box=std::make_shared<world::Outbox>();
        ordered.join(10,"Caster",0,0,caster_box,0,{}, {},100,1);
        ordered.join(11,"Ally",40,0,ally_box,0,{}, {},100,1);
        ordered.set_vitals(10,{500,500,100,100},100);
        ordered.set_vitals(11,{200,500,100,100},100);
        const auto a=*ordered.effect_scope(10),b=*ordered.effect_scope(11);
        check(ordered.configure_adrenaline(a,bar,100));
        caster_box->drain();ally_box->drain();
        std::function<void()> publish;
        check(ordered.stage_gift_health_player(a,b,plan,0,1,200,&publish));
        check(bool(publish)&&caster_box->drain().empty()&&ally_box->drain().empty());
        check(ordered.resources(11)->health==215);
        publish();
        const auto packets=ally_box->drain();
        check(std::find(packets.begin(),packets.end(),
            skills::particles::health_change_2022(11,10,15,500))!=packets.end());
        publish();check(ally_box->drain().empty());
    }
    for(unsigned mode=0;mode<6;++mode){
        world::Instance npc_world;
        world::Instance::Npc n;
        n.id=n.definition.id=100;n.x=mode==3?1800.f:60.f;
        n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.binding.level=20;
        d.relation=mode==1?combat::npcs::Relation::neutral:
            mode==2?combat::npcs::Relation::hostile_to_players:
            combat::npcs::Relation::allied_to_players;
        d.spawn_health=combat::Health{100,500};
        d.damageable=d.conditions_allowed=true;
        if(mode!=4){d.healing_allowed=true;d.healing_evidence="fixture";}
        n.combat_definition=d;npc_world.configure_npcs({n});
        auto box=std::make_shared<world::Outbox>();
        npc_world.join(1,"Caster",0,0,box,0,{}, {},100,1);
        npc_world.set_vitals(1,{480,480,50,100},100);
        const auto caster=*npc_world.effect_scope(1),npc=*npc_world.effect_scope(100);
        Slots npc_bar{};npc_bar[0]={1121,0};npc_bar[1]={1119,0};
        check(npc_world.configure_adrenaline(caster,npc_bar,100));
        auto selected=npc;if(mode==5)++selected.generation;
        const bool accepted=mode==0;
        box->drain();std::function<void()> publish;
        check(npc_world.stage_gift_health_npc(caster,selected,plan,0,1,200,&publish)==accepted);
        check(box->drain().empty());
        check(npc_world.npc_combat_snapshot(npc)->health->current==
            (accepted?115:100));
        check(npc_world.skill_disabled_slot(caster,{1119,0},200)==
            (accepted?10200:0));
        if(accepted){
            check(bool(publish));publish();
            const auto packets=box->drain();
            check(std::find(packets.begin(),packets.end(),
                skills::particles::health_change_2022(100,1,15,500))!=packets.end());
            check(!npc_world.stage_gift_health_npc(caster,npc,plan,0,1,300));
            check(npc_world.npc_combat_snapshot(npc)->health->current==115);
        }
    }
    std::cout<<"Gift of Health World player/NPC heals and exact-copy disable/reset pass; payment/cast binding pending\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
