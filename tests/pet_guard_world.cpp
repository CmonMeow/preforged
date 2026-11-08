#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills;
unsigned mode=0;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("pet Guard mode "+std::to_string(mode)+" line "+std::to_string(at.line()));
}
struct Fixture {
    world::Instance w;std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();effects::Scope owner,pet,foe;
    explicit Fixture(float x=80){
        world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=5;n.definition.visual_adjustment={0,0,0,100};n.x=x;
        combat::npcs::Definition d;d.binding={162,100,100,0,0,0,0,5};d.relation=combat::npcs::Relation::hostile_to_players;
        d.damageable=true;d.conditions_allowed=true;d.holy_traits.light_sensitive=false;d.light_sensitive_evidence="synthetic known holy traits";d.spawn_health=combat::Health{1000,1000};d.physical_armor=d.general_armor=d.earth_armor=d.fire_armor=d.cold_armor=d.lightning_armor=0;n.combat_definition=d;
        w.configure_npcs({n});w.join(1,"Guard owner",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        owner=*w.effect_scope(1);foe=*w.effect_scope(100);check(w.physical_armor(owner,0));
        check(w.restore_pet_profile(owner,1,pets::Profile{pets::Pet{}},1)&&w.pet_owner_combat(owner,1,0,1000));
        check(w.reconcile_pet_bar_mode(owner,162,true,false,std::array{436u},1000));pet=*w.owned_pet_scope(owner);
        box->drain();
    }
    void heel(){check(w.command_pet(owner,{pets::wire::Command::Kind::behavior,pet.actor,2},1000).has_value());}
    void enemy_attack(bool ranged=false){
        combat::ai::FlarePolicy p;p.source=foe;p.level=5;p.speed=220;p.acquire=1000;p.follow=1500;p.leash=2000;p.flight={1000,32};p.evidence="synthetic committed Guard threat";
        combat::ai::ordinary::Reviewed attack{1,1,0,2,false,ranged?combat::ai::ordinary::Delivery::ranged:combat::ai::ordinary::Delivery::melee,900,2000,ranged?1000.:128.,{1000,32},p.evidence};
        check(w.configure_npc_hostile_ordinary(p,attack,1000));
    }
};
int main()try{
    for(mode=0;mode<5;++mode){Fixture f;
        if(mode==1)f.heel();
        if(mode==3){f.w.configure_attack_miss_draws([]{return 0.;});check(f.w.apply_condition(f.owner,f.owner,combat::conditions::Kind::blind,1000,10000));}
        if(mode==4){f.w.configure_attack_block_draws([]{return 0.;});check(f.w.stage_npc_shield_deflection(f.foe,*effects::shield_deflection(12),1000));}

        combat::weapon::Owned sword{1,1,{1,1,2},0,false,false,27};check(f.w.ordinary_weapon(f.owner,sword,1));
        check(f.w.ordinary_attack(f.owner,100,1000));check(!f.w.npc_ordinary_pending(f.pet));
        if(mode==2)f.w.cancel_ordinary_attack(f.owner,1001);
        bool pet_attack=false;
        for(unsigned now=1100;now<=5000;now+=100){f.w.tick(now);pet_attack|=f.w.npc_ordinary_pending(f.pet).has_value();}
        check(pet_attack==(mode==0||mode==3||mode==4));
        if(mode==3||mode==4){const auto outcomes=f.w.take_attack_outcomes(f.owner);
            check(std::any_of(outcomes.begin(),outcomes.end(),[&](const auto& result){return result.outcome==(mode==3?combat::weapon::AttackOutcome::miss:combat::weapon::AttackOutcome::block);}));}
    }
    for(mode=3;mode<6;++mode){Fixture f(mode==5?500.f:80.f);if(mode==4)f.heel();f.enemy_attack(mode==5);
        f.w.tick(1000);check(f.w.npc_ordinary_pending(f.foe).has_value()&&!f.w.npc_ordinary_pending(f.pet));
        bool pet_attack=false;for(unsigned now=1100;now<=5000;now+=100){f.w.tick(now);pet_attack|=f.w.npc_ordinary_pending(f.pet).has_value();}
        check(pet_attack==(mode==3)&&f.w.resources(1)->health<1000);
    }
    for(mode=6;mode<10;++mode){Fixture f;
        if(mode==7)f.heel();casting::Hooks h;h.resources=[&](unsigned id){return f.w.resources(id);};
        h.events=[](const auto&){};h.actor_generation=[&]{return std::optional(f.owner.generation);};
        h.target_scope=[&](unsigned id){return f.w.effect_scope(id);};
        h.projectile_target_valid=[](unsigned id,float range){return id==100&&range==144;};
        h.consume_energy_at=[&](unsigned cost,auto now){return f.w.consume_energy(1,cost,f.owner,now);};
        h.holy_damage=[&](auto target,const auto& plan,auto now)->casting::EffectCompletion{return f.w.holy_damage(f.owner,target,plan,1,now);};
        h.pet_offensive_completed=[&](auto owner,auto target,unsigned skill,auto sequence,auto now){f.w.pet_offensive_completed(owner,target,skill,sequence,now);};
        casting::Runtime runtime(1);casting::ActorContext actor{1,f.owner.generation,3};
        check(runtime.start_trusted(1000,{312,0,100},actor,true,h)==casting::Reason::accepted);
        if(mode==8)runtime.cancel(h,1100);
        if(mode==9){auto stale=f.foe;++stale.generation;f.w.pet_offensive_completed(f.owner,stale,312,1,1100);runtime.cancel(h,1100);}
        runtime.update(1749,h);check(!f.w.npc_ordinary_pending(f.pet));runtime.update(1750,h);
        bool pet_attack=false;for(unsigned now=1800;now<=4000;now+=100){f.w.tick(now);pet_attack|=f.w.npc_ordinary_pending(f.pet).has_value();}
        check(pet_attack==(mode==6));
    }
    std::cout<<"Production Guard committed ordinary/skill acquisition, adjacent threats, Heel, cancel and stale scopes passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
