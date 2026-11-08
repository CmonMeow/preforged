#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("pet owner death World line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();skills::effects::Scope owner,pet,foe,other;
    Fixture(){
        std::vector<world::Instance::Npc> npcs;
        for(unsigned id:{100u,101u}){world::Instance::Npc n;n.id=n.definition.id=id;n.definition.level=5;n.definition.visual_adjustment={0,0,0,100};n.x=80;
            combat::npcs::Definition d;d.binding={162,id,id,0,0,0,0,5};d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health=combat::Health{1000,1000};d.damageable=true;d.physical_armor=d.general_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=0;n.combat_definition=d;npcs.push_back(n);}
        w.configure_npcs(npcs);w.join(1,"Pet owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        owner=*w.effect_scope(1);foe=*w.effect_scope(100);other=*w.effect_scope(101);
        check(w.restore_pet_profile(owner,1,pets::Profile{pets::Pet{}},1)&&w.pet_owner_combat(owner,1,0,1000));
        check(w.reconcile_pet_bar_mode(owner,162,true,false,std::array{436u},1000));pet=*w.owned_pet_scope(owner);
        combat::weapon::Owned sword{1,1,{1,1,2},0,false,false,27};check(w.ordinary_weapon(owner,sword,1));box->drain();
    }
};
int main()try{
    {
        Fixture f;check(f.w.ordinary_attack(f.owner,100,1000));
        std::uint64_t decision=1000;
        for(;decision<=3000;decision+=50){f.w.tick(decision);if(f.w.npc_ordinary_pending(f.pet))break;}
        check(decision<=3000);const auto first=*f.w.npc_ordinary_pending(f.pet);
        const auto death=decision+100;check(death<first.token.completes);
        f.w.cancel_ordinary_attack(f.owner,death);check(f.w.kill(f.owner,death));
        const auto before=f.w.npc_combat_snapshot(f.foe)->health->current;
        check(f.w.npc_ordinary_pending(f.pet)==std::optional(first));
        bool repeated=false;
        for(auto now=death+50;now<=death+4500;now+=50){f.w.tick(now);
            if(const auto active=f.w.npc_ordinary_pending(f.pet);active&&active->token.sequence>first.token.sequence)repeated=true;
            check(f.w.resources(1)->health==0&&f.w.owned_pet_scope(f.owner)==f.pet);
        }
        check(repeated&&f.w.npc_combat_snapshot(f.foe)->health->current<before);
        check(!f.w.pet_comfort_ready(f.owner,f.pet,death+4501));
        // Dead-owner requests cannot replace the retained target. Exact scope
        // ownership remains valid, but new targeting has a separate alive gate.
        f.w.pet_offensive_completed(f.owner,f.other,312,1000000,death+4501);
        check(!f.w.provoke_pet(f.owner,f.other,ai::companion::Trigger::owner_attack,death+4501));
        check(!f.w.command_pet(f.owner,{pets::wire::Command::Kind::target,f.pet.actor,101},death+4501));
        f.w.join(2,"Living ally",0,0,f.box,0,{}, {},death+4502,1);f.w.set_vitals(2,{1000,1000,100,100},100);
        const auto ally=*f.w.effect_scope(2);
        check(f.w.damage_npc(ally,f.foe,{combat::Kind::armor_ignoring,10000,20,194},1248,death+4503).reason==combat::Reason::accepted);
        for(auto now=death+4600;now<=death+7000;now+=100)f.w.tick(now);
        check(!f.w.npc_ordinary_pending(f.pet)&&f.w.npc_combat_snapshot(f.other)->health->current==1000);
        const auto pet_health=f.w.npc_combat_snapshot(f.pet)->health->current;
        check(f.w.resurrect(f.owner,death+7100));
        check(f.w.owned_pet_scope(f.owner)==f.pet&&f.w.npc_combat_snapshot(f.pet)->health->current==pet_health);
        f.w.leave(1);check(!f.w.npc_combat_snapshot(f.pet));
        f.w.join(1,"New Owner",0,0,f.box,0,{}, {},death+7200,1);f.w.set_vitals(1,{1000,1000,100,100},100);
        const auto next=*f.w.effect_scope(1);check(next!=f.owner);
        check(f.w.restore_pet_profile(next,1,pets::Profile{pets::Pet{}},1)&&f.w.pet_owner_combat(next,1,0,death+7200));
        check(f.w.reconcile_pet_bar_mode(next,162,true,false,std::array{436u},death+7200));const auto replacement=*f.w.owned_pet_scope(next);check(replacement!=f.pet);
        f.w.pet_offensive_completed(f.owner,f.other,312,1000001,death+7201);f.w.tick(death+7300);check(!f.w.npc_ordinary_pending(replacement));
    }
    {
        Fixture f;check(f.w.kill(f.owner,1001));f.w.pet_offensive_completed(f.owner,f.foe,312,1,1002);
        check(!f.w.command_pet(f.owner,{pets::wire::Command::Kind::target,f.pet.actor,100},1002));
        for(unsigned now=1100;now<=5000;now+=100)f.w.tick(now);
        check(!f.w.npc_ordinary_pending(f.pet)&&f.w.npc_combat_snapshot(f.foe)->health->current==1000);
    }
    std::cout<<"Owned pet keeps retained combat after owner death, with no new dead-owner targeting or lifetime reuse\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
