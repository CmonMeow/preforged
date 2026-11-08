#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace world;using namespace skills;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    const auto protection=*regeneration::reviewed(299,12);check(protection.pips==0&&protection.duration==8000&&regeneration::end_healing(299,12)==41,"Shielding plan");
    auto w=std::make_unique<Instance>();w->join(1,"Monk",0,0,std::make_shared<Outbox>(),0,{Packet{165,0,1,0,0,0,3,0}}, {},100,1);w->join(2,"Foe",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
    w->set_vitals(1,{100,400,20,20},100);w->set_vitals(2,{100,400,20,20},100);const auto monk=*w->effect_scope(1),foe=*w->effect_scope(2);
    auto stale=monk;++stale.generation;check(!w->protective_enchantment(monk,stale,protection,0,100),"replacement lifetime accepted");
    check(w->protective_enchantment(monk,monk,protection,0,100),"Shielding application");
    check(w->shadow_damage(foe,monk,*effects::shadow_damage(133,0),20,100)&&w->resources(1)->health==100,"small spell not fully absorbed");
    check(w->life_steal(foe,monk,*effects::life_steal(153,0),100)&&w->resources(1)->health==97&&w->resources(2)->health==103,"life-steal loss/gain reduction");
    check(w->shadow_damage(foe,monk,*effects::shadow_damage(102,0),20,100)&&w->resources(1)->health==97,"dual damage/life-steal reduction");
    combat::weapon::Owned axe{1,1,{20,20,1},12,false,false,2};check(w->ordinary_weapon(foe,axe,20)&&w->physical_armor(monk,60),"weapon fixture");
    check(w->melee_attack(foe,monk,axe,*combat::weapon::attack_skill(322,0),100)&&w->resources(1)->health==82,"weapon reduction");
    w->tick(8100);check(w->resources(1)->health==123,"expiry heal once");w->tick(9100);check(w->resources(1)->health==123,"end heal repeated");
    check(w->melee_attack(foe,monk,axe,*combat::weapon::attack_skill(322,0),9100)&&w->resources(1)->health==93,"expired reduction retained");
    check(w->protective_enchantment(monk,monk,protection,0,9100),"new shielding");
    const auto entry=w->status_snapshot(monk,9100)->front();
    check(w->status_mutation(monk,299,entry.instance,12,9100,{},false)&&w->resources(1)->health==134,"real removal did not heal");
    {
        Instance renewal;renewal.join(1,"Monk",0,0,std::make_shared<Outbox>());renewal.set_vitals(1,{100,400,20,20},100);const auto scope=*renewal.effect_scope(1);
        auto plan=*regeneration::reviewed(299,0);check(renewal.protective_enchantment(scope,scope,plan,0,100)&&renewal.protective_enchantment(scope,scope,plan,0,1100),"renew shielding");
        check(renewal.resources(1)->health==100,"renewal incorrectly triggered end healing");renewal.tick(9100);check(renewal.resources(1)->health==105,"renewed shielding expiry");
        plan=*regeneration::reviewed(115,0);check(renewal.apply_health_regeneration(scope,1,plan,9100,16100,0)&&renewal.apply_health_regeneration(scope,1,plan,10100,17100,0),"renew Blood Renewal");
        check(renewal.resources(1)->health==111,"Blood Renewal recast incorrectly fired end heal");renewal.tick(17100);check(renewal.resources(1)->health==193,"renewed Blood Renewal real end");
    }
    check(w->protective_enchantment(monk,monk,protection,12,9100)&&w->resources(1)->health==172,"Divine Favor must commit on original ally lifetime");
    combat::Actor actor{1,1,combat::Health{400,400},{},0,true,{},60,{}};actor.incoming_damage_multiplier=2;actor.incoming_reduction=15;
    check(combat::apply(actor,{combat::Kind::fire,50,20,197},true).damage==85,"protection must follow armor and amplification");
    actor.health=combat::Health{300,400};actor.incoming_damage_multiplier=1;
    const auto dual=effects::shadow_target(*effects::shadow_damage(102,0),{100,400},actor,20);
    check(dual&&dual->hit.damage==0&&dual->steal&&dual->steal->stolen==0&&dual->target.health->current==300&&dual->source.current==100,"damage and conditional steal need independent reduction");
    const auto blocked=effects::steal_life(*effects::life_steal(153,0),{100,400},{100,400},50);
    check(blocked&&blocked->stolen==0&&effects::life_steal_packets(2,1,*blocked).empty(),"zero life-steal publication");
    {
        using namespace casting;Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[299]=1;Slots bar{};bar[0]={299,0};attributes::Ranks ranks{};ranks[15]=12;ranks[16]=12;
        Hooks hooks;vitals::Resources body{100,400,20,20};unsigned commits=0,cost=0;
        hooks.events=[](const auto&){};hooks.resources=[&](unsigned id)->std::optional<vitals::Resources>{return id==42?std::optional(body):std::nullopt;};hooks.target_valid=[](unsigned id,float range,bool self){return id==42&&range==1248&&self;};
        hooks.consume_energy=[&](unsigned value){cost+=value;return true;};effects::Scope scope{1,42,1};hooks.target_scope=[&](unsigned id)->std::optional<effects::Scope>{return id==42?std::optional(scope):std::nullopt;};hooks.protective_enchantment=[&](auto target,const auto& p,unsigned divine,auto now){check(divine==12&&target==scope&&p.skill==299&&p.pips==0&&p.rank==12&&now==350,"Shielding cast plan");++commits;return true;};
        Runtime cast(42);check(cast.start(100,{299,0,42},bar,policy,ranks,true,hooks)==casting::Reason::accepted&&cost==5,"Shielding casting cost");cast.update(349,hooks);check(!commits,"Shielding before activation");cast.update(350,hooks);check(commits==1,"Shielding dispatch");
        Runtime replaced(42);check(replaced.start(1000,{299,0,42},bar,policy,ranks,true,hooks)==casting::Reason::accepted,"replacement cast start");++scope.generation;replaced.update(1250,hooks);check(commits==1,"cast crossed target lifetime");
    }
    std::cout<<"Shielding damage, life steal, expiry, removal, renewal and casting verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

