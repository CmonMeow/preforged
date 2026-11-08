#include "gw_world_simulation.h"
#include "gw_pet_lifecycle.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool yes,std::source_location at=std::source_location::current()){
    if(!yes)throw std::runtime_error("pet World line "+std::to_string(at.line()));
}
world::Instance::Npc stalker(){
    world::Instance::Npc n;n.id=100;n.definition.id=50;n.definition.model_file=128406;
    n.definition.flags=13;n.definition.primary=2;n.definition.level=5;n.definition.visual_adjustment={0,0,0,100};
    n.definition.encoded_name={u'\xf0d',u'\xa74a',u'\xbbf9',u'\x249a'};n.model_files={129924};n.x=500;n.captured_name="Melandru's Stalker";
    combat::npcs::Definition d;d.binding={162,100,50,128406,0,0,13,5};d.relation=combat::npcs::Relation::neutral;
    d.spawn_health=combat::Health{180,180};d.damageable=d.conditions_allowed=true;
    d.physical_armor=d.general_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=35;
    d.healing_allowed=true;d.healing_evidence="Comfort Animal local wiki and captured healing; synthetic test geometry";n.combat_definition=d;return n;
}
combat::ai::FlarePolicy motion(skills::effects::Scope source){
    combat::ai::FlarePolicy p;p.source=source;p.level=5;p.speed=220;p.acquire=1000;p.follow=5000;p.leash=5000;
    p.flight={1000,32};p.max_flights=4;p.proximity_acquisition=false;p.evidence="synthetic World test movement tuning";return p;
}
combat::ai::ordinary::Reviewed attack(unsigned damage){
    return {damage,damage,0,2,false,combat::ai::ordinary::Delivery::melee,750,2000,128,{1000,32},
        "synthetic test base damage/windup/reach; wiki pet2000ms cadence"};
}
struct CharmSplitFixture {
    world::Instance w;world::Instance::Npc wild=stalker();skills::effects::Scope owner,animal;
    std::shared_ptr<world::Outbox> output=std::make_shared<world::Outbox>();
    explicit CharmSplitFixture(bool pvp=false){
        w.configure_npcs({wild});w.join(1,"Reservation owner",0,0,output,0,{}, {},100,1);
        w.set_vitals(1,{10000,10000,40,40},100);owner=*w.effect_scope(1);animal=*w.effect_scope(100);
        check(w.restore_pet_profile(owner,0,{},1));check(w.pet_instance_mode(owner,pvp,100));check(w.pet_owner_combat(owner,1,0,100));check(w.configure_tameable_stalker(animal,wild.combat_definition->binding));
        check(w.configure_tameable_stalker_combat(animal,motion(animal),attack(2),{{{10,0}},1000,6000,"synthetic split patrol"}));
        output->drain();
    }
};
int main()try{
    {CharmSplitFixture f;check(f.w.pet_owner_combat(f.owner,2,1,1100));
        check(!f.w.reserve_pet_charm(f.owner,f.animal,1099));
        const auto updated=f.w.reserve_pet_charm(f.owner,f.animal,1100);check(bool(updated));check(f.w.cancel_pet_charm(*updated));}

    {CharmSplitFixture f(true);check(!f.w.pet_instance_mode(f.owner,false,1000));
        const auto reserved=f.w.reserve_pet_charm(f.owner,f.animal,1000);check(bool(reserved));check(f.w.cancel_pet_charm(*reserved));}

    {
        CharmSplitFixture f;const auto reserved=f.w.reserve_pet_charm(f.owner,f.animal,1000);check(bool(reserved));
        // Payment runs outside World: these nested World operations must neither
        // deadlock nor activate hostile AI before accepted-start.
        check(!f.w.npc_flare_ai_snapshot(f.animal));
        check(f.w.configure_tameable_stalker(f.animal,f.wild.combat_definition->binding));
        check(!f.w.reserve_pet_charm(f.owner,f.animal,1000));
        check(!f.w.accept_pet_charm(*reserved,0,1000,10000));
        check(!f.w.npc_flare_ai_snapshot(f.animal)&&!f.w.cancel_pet_charm(*reserved));
        const auto retry=*f.w.reserve_pet_charm(f.owner,f.animal,1001);
        check(!f.w.cancel_pet_charm(*reserved));
        const auto accepted=f.w.accept_pet_charm(retry,1,1001,10000);check(bool(accepted));
        check(f.w.npc_flare_ai_snapshot(f.animal).has_value());
        check(!f.w.accept_pet_charm(retry,2,1001,10000)&&!f.w.cancel_pet_charm(retry));
        check(f.w.cancel_pet_charm(*accepted));
        const auto again=f.w.reserve_pet_charm(f.owner,f.animal,1002);check(bool(again));
        check(f.w.cancel_pet_charm(*again)); // Cancelled hostile animal stays retryable.
    }
    {
        CharmSplitFixture f;const auto reserved=*f.w.reserve_pet_charm(f.owner,f.animal,1000);
        // A genuine payment reaction may kill the owner. World rejects accepted
        // start without restoring Health/Energy or converting the animal.
        check(f.w.kill(f.owner,1001));check(!f.w.accept_pet_charm(reserved,1,1001,10000));
        check(f.w.resources(1)->health==0&&!f.w.npc_flare_ai_snapshot(f.animal));
        check(!f.w.cancel_pet_charm(reserved));
    }
    {
        CharmSplitFixture f;const auto reserved=*f.w.reserve_pet_charm(f.owner,f.animal,1000);
        f.w.leave(1);check(!f.w.cancel_pet_charm(reserved));
        f.w.join(1,"New generation",0,0,f.output,0,{}, {},1001,1);
        f.w.set_vitals(1,{10000,10000,40,40},100);const auto next=*f.w.effect_scope(1);check(next!=f.owner);
        check(f.w.restore_pet_profile(next,0,{},1));check(f.w.pet_instance_mode(next,false,1001));check(f.w.pet_owner_combat(next,1,0,1001));
        const auto fresh=f.w.reserve_pet_charm(next,f.animal,1002);check(bool(fresh));
        check(!f.w.cancel_pet_charm(reserved)&&f.w.cancel_pet_charm(*fresh));
        check(!f.w.accept_pet_charm(reserved,1,1002,10000));
    }

    {
        auto wild=stalker();world::Instance idle;idle.configure_npcs({wild});auto output=std::make_shared<world::Outbox>();
        idle.join(1,"Neutral observer",0,0,output,0,{}, {},100,1);idle.set_vitals(1,{0,100,40,40},100);
        const auto source=*idle.effect_scope(100);check(idle.configure_tameable_stalker(source,wild.combat_definition->binding));
        check(idle.configure_tameable_stalker_combat(source,motion(source),attack(2),{{{10,0},{-10,0}},500,1000,"synthetic neutral patrol"},100));output->drain();
        unsigned moved=0;
        for(unsigned at=100;at<=3000;at+=100){idle.tick(at);for(const auto& packet:output->drain())if(packet.size()==16&&packet[0]==44&&packet[1]==0&&packet[2]==100)++moved;}
        check(moved>0&&idle.resources(1)->health==0);
    }
    auto wild=stalker();auto enemy=wild;enemy.id=enemy.definition.id=101;enemy.x=200;
    enemy.combat_definition->binding.actor=enemy.combat_definition->binding.definition=101;
    enemy.combat_definition->relation=combat::npcs::Relation::hostile_to_players;
    enemy.combat_definition->spawn_health=combat::Health{1000,1000};
    world::Instance w;w.configure_npcs({wild,enemy});auto output=std::make_shared<world::Outbox>();
    w.join(1,"Pet owner",0,0,output,0,{}, {},100,1);w.set_vitals(1,{10000,10000,40,40},100);
    const auto owner=*w.effect_scope(1),animal=*w.effect_scope(100),foe=*w.effect_scope(101);
    check(w.physical_armor(owner,0)&&w.restore_pet_profile(owner,0,{},1));
    check(w.configure_tameable_stalker(animal,wild.combat_definition->binding));
    check(w.configure_tameable_stalker_combat(animal,motion(animal),attack(2),{{{10,0}},1000,6000,"synthetic patrol"}));
    check(w.pet_owner_combat(owner,1,0,100));
    const auto receipt=w.begin_pet_charm(owner,animal,1,1000,10000);check(bool(receipt));
    unsigned writes=0;const pets::AdoptionCommit durable=[&](const pets::AdoptionWrite& write){
        ++writes;check(!write.expected.pet&&write.expected_revision==0&&write.replacement.pet==std::optional(pets::Pet{}));return pets::CommitStatus::committed;};
    check(w.complete_pet_charm(*receipt,true,durable,1001).status==pets::CharmCompletion::deferred&&writes==0);
    for(unsigned at=1100;at<=10900;at+=100){w.tick(at);output->drain();}
    check(w.resources(1)->health<10000); // The exact selected wild animal retaliates during the cast.
    const auto adopted=w.complete_pet_charm(*receipt,true,durable,11000);
    check(adopted.status==pets::CharmCompletion::adopted&&writes==1&&!adopted.packets.empty());
    check(w.owned_pet_scope(owner)==animal);
    check(w.complete_pet_charm(*receipt,true,durable,11001).status!=pets::CharmCompletion::adopted&&writes==1);
    ai::companion::Config behavior;behavior.follow_start=200;behavior.follow_stop=100;
    behavior.pet_guard_combat_break=1000;behavior.pet_attack_lock_range=2000;
    check(w.owned_pet_scope(owner)==animal); // Default AI installed atomically with adoption.
    auto stale_owner=owner;++stale_owner.generation;
    check(!w.pet_owner_combat(stale_owner,1,0,11000));
    check(w.pet_owner_combat(owner,1,0,11000));
    for(unsigned at=11100;at<=14000;at+=100){w.tick(at);output->drain();}
    check(w.nearby_npc_name(1,100,14000).has_value());
    check(w.provoke_pet(owner,foe,ai::companion::Trigger::owner_attack,14001));
    for(unsigned at=14100;at<=17000;at+=100){w.tick(at);output->drain();}
    check(w.npc_combat_snapshot(foe)->health->current<1000);
    const pets::wire::Command heel{pets::wire::Command::Kind::behavior,100,2};
    check(w.command_pet(owner,heel,17001).has_value());
    const auto enemy_health=w.npc_combat_snapshot(foe)->health->current;
    for(unsigned at=17100;at<=20000;at+=100){w.tick(at);output->drain();}
    check(w.npc_combat_snapshot(foe)->health->current==enemy_health);
    check(w.configure_npc_opposed_ordinary(foe,attack(10000),20001));
    check(w.begin_npc_opposed_ordinary(foe,animal,20002).has_value());
    w.tick(21002);output->drain();check(w.npc_combat_snapshot(animal)->health->current==0);
    check(w.pet_comfort_ready(owner,animal,21003));
    const auto revived=w.complete_pet_comfort(owner,animal,0,2,true,21003);
    check(revived.accepted&&revived.revived&&w.npc_combat_snapshot(animal)->health->current==18);
    check(!w.complete_pet_comfort(owner,animal,0,2,true,21004).accepted);
    const auto healed=w.complete_pet_comfort(owner,animal,0,3,true,21005,true);
    check(healed.accepted&&!healed.revived&&healed.healed==20&&w.npc_combat_snapshot(animal)->health->current==38);
    check(bool(healed.publication));healed.publication();healed.publication();
    check(w.reconcile_pet_bar(owner,162,true,std::array<unsigned,0>{},21100)&&!w.effect_scope(100)&&!w.owned_pet_scope(owner));
    check(w.reconcile_pet_bar(owner,162,true,std::array{436u},21200));
    const auto restored=*w.owned_pet_scope(owner);check(restored!=animal&&w.npc_combat_snapshot(restored)->health->current==180);
    check(!w.pet_comfort_ready(owner,animal,21201)&&!w.command_pet(owner,heel,21201));
    const auto entry=w.npc_packets();check(std::any_of(entry.begin(),entry.end(),[](const auto& p){return p.size()>=2&&p[0]==177&&p[1]==0;}));
    w.leave(1);check(!w.effect_scope(restored.actor));check(!w.pet_owner_combat(owner,1,0,21201));
    std::cout<<"World Stalker charm/adoption, controls/follow/ordinary and Comfort heal/revive passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
