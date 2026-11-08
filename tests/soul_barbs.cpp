#include "gw_soul_barbs.h"
#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace skills::effects;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    check(soul_barbs(0)->damage==15&&soul_barbs(12)->damage==27&&soul_barbs(15)->damage==30&&!soul_barbs(21),"reviewed ranks");
    Scope victim{1,43,2};Registry registry(victim);const auto plan=*soul_barbs(12);
    check(!soul_barbs_reaction(registry,victim,100,BarbsTrigger::spell_applied,true,0),"initial application triggered itself");
    check(apply_soul_barbs(registry,victim,1,42,plan,0).has_value(),"hex application");
    auto reaction=soul_barbs_reaction(registry,victim,135,BarbsTrigger::spell_applied,true,1000);
    check(reaction&&reaction->cause.cause==42&&reaction->plan.damage==27,"hex reaction");
    combat::Actor actor{43,2,combat::Health{100,100},{},0,true};
    auto hit=soul_barbs_damage(*reaction,actor,1);check(hit&&hit->damage==27&&hit->after.current==73,"armor-independent damage");
    check(soul_barbs_reaction(registry,victim,288,BarbsTrigger::spell_applied,true,1000).has_value(),"enchantment cast missing");
    check(soul_barbs_reaction(registry,victim,288,BarbsTrigger::enchantment_renewed,true,1000).has_value(),"enchantment self-renewal missing");
    check(!soul_barbs_reaction(registry,victim,135,BarbsTrigger::enchantment_renewed,true,1000),"invented generic hex renewal");
    check(!soul_barbs_reaction(registry,victim,281,BarbsTrigger::spell_applied,true,1000),"ordinary heal triggered");
    check(!soul_barbs_reaction(registry,victim,135,BarbsTrigger::spell_applied,false,1000),"failed spell triggered");
    reaction=soul_barbs_reaction(registry,victim,100,BarbsTrigger::spell_applied,true,1000);
    check(reaction&&reaction->plan.damage==27,"reapplication prior-rank reaction");
    const auto replaced=apply_soul_barbs(registry,victim,2,44,*soul_barbs(0),1000);
    check(replaced&&replaced->removed.size()==1&&replaced->removed[0].instance==1,"replacement stacks");
    check(soul_barbs_reaction(registry,victim,135,BarbsTrigger::spell_applied,true,1000)->plan.damage==15,"new rank not active");
    check(!soul_barbs_reaction(registry,victim,135,BarbsTrigger::spell_applied,true,31000),"expired reaction");
    auto stale=victim;++stale.generation;check(!soul_barbs_reaction(registry,stale,135,BarbsTrigger::spell_applied,true,1000),"recycled target reaction");
    ++actor.generation;check(!soul_barbs_damage(*reaction,actor,20),"recycled damage target");

    {
        Registry committed(victim);check(apply_soul_barbs(committed,victim,1,42,plan,0).has_value(),"journal initial hex");check(committed.take_applications(victim).empty(),"initial hex journal self-trigger");
        auto abandoned=committed;check(abandoned.apply(victim,{2,43,288,0,Category::enchantment,1000,16000},Replace::same_skill).accepted,"speculative application");
        check(committed.take_applications(victim).empty(),"rollback leaked journal");
        committed=std::move(abandoned);const auto events=committed.take_applications(victim);
        check(events.size()==1&&events[0].target==victim&&events[0].applied.skill==288&&events[0].observers.size()==1&&events[0].observers[0].rank==12,"committed pre-application snapshot");
        check(committed.take_applications(victim).empty(),"journal replay");
        check(apply_soul_barbs(committed,victim,3,44,*soul_barbs(0),1000).has_value(),"journal replacement");const auto renewal=committed.take_applications(victim);
        check(renewal.size()==1&&renewal[0].sequence>events[0].sequence&&renewal[0].observers[0].cause==42&&renewal[0].observers[0].rank==12,"replacement lost original observer");
        check(committed.apply(victim,{4,43,288,0,Category::enchantment,1000,16000},Replace::same_skill).accepted,"death event fixture");committed.clear(victim);check(committed.take_applications(victim).empty(),"death retained pending journal");
        Registry bounded(victim);check(apply_soul_barbs(bounded,victim,1,42,plan,0).has_value(),"bounded journal fixture");
        for(unsigned i=0;i<128;++i)check(bounded.apply(victim,{2,43,288,0,Category::enchantment,1000,16000},Replace::same_skill).accepted,"bounded journal rejected valid entry");
        check(!bounded.apply(victim,{3,43,288,0,Category::enchantment,1000,16000},Replace::same_skill).accepted&&bounded.lookup(victim,2).has_value()&&!bounded.lookup(victim,3),"full journal mutated effect state");
        check(bounded.take_applications(victim).size()==128&&bounded.apply(victim,{3,43,288,0,Category::enchantment,1000,16000},Replace::same_skill).accepted,"journal did not recover after drain");
    }
    {
        using namespace world;using namespace skills;using namespace casting;
        Instance w;w.join(1,"Necromancer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.join(2,"Target",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{200,200,40,40},100);w.set_vitals(2,{200,200,40,40},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.learned[100]=1;Slots bar{};bar[0]={100,0};attributes::Ranks ranks{};ranks[7]=12;Hooks h;unsigned energy=0;
        h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};h.consume_energy=[&](auto cost){energy+=cost;return true;};h.target_valid=[](auto,float,bool){return false;};h.target_scope=[&](auto id){return w.effect_scope(id);};h.projectile_target_valid=[](auto id,float range){return id==2&&range==1248;};
        h.soul_barbs=[&](auto who,const auto& p,auto now){return w.soul_barbs(source,who,p,20,now);};
        Runtime cast(1);check(cast.start(100,{100,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted&&energy==10,"Soul Barbs casting cost");cast.update(2099,h);check(w.status_snapshot(target,2099)->empty(),"Soul Barbs early activation");cast.update(2100,h);cast.update(2100,h);check(w.resources(2)->health==200,"initial Soul Barbs triggered");
        check(w.soul_barbs(source,target,*soul_barbs(0),20,2100)&&w.resources(2)->health==173,"Soul Barbs recast did not use old rank");
        check(w.status_mutation(target,180,9,0,2100,12100,true)&&w.resources(2)->health==158,"enchantment application missing reaction");
        check(w.status_mutation(target,180,10,0,2100,12100,true)&&w.resources(2)->health==143,"enchantment renewal missing reaction");
        check(w.status_mutation(target,180,10,0,2100,{},false)&&w.resources(2)->health==143,"removal triggered Soul Barbs");
        check(!w.status_mutation(target,180,0,0,2100,12100,true)&&w.resources(2)->health==143,"failed application triggered Soul Barbs");
        check(w.backfire(source,target,*effects::backfire(28),20,2100)&&w.resources(2)->health==128,"Backfire application missing Soul Barbs");
        check(w.movement_hex(source,target,*effects::movement_hex(76,0),2100)&&w.resources(2)->health==113,"movement hex missing reaction");
        auto stale=target;++stale.generation;check(!w.soul_barbs(source,stale,*soul_barbs(12),20,2100)&&w.resources(2)->health==113,"stale target applied Soul Barbs");
        w.tick(32100);const auto after=w.resources(2)->health;check(w.status_mutation(target,180,11,0,32100,42100,true)&&w.resources(2)->health==after,"expired Soul Barbs triggered");
    }
    {
        using namespace world;using namespace skills;
        Instance w;w.join(1,"Necromancer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.join(2,"Target",100,0,std::make_shared<Outbox>(),0,{}, {},100,2);w.set_vitals(1,{200,200,40,40},100);w.set_vitals(2,{100,200,40,40},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);check(w.soul_barbs(source,target,*soul_barbs(0),20,100),"modifier fixture");
        check(w.protective_enchantment(target,target,*regeneration::reviewed(299,0),0,100)&&w.resources(2)->health==88,"new Shielding application reduction");
        check(w.status_mutation(target,346,9,0,100,8100,true)&&w.resources(2)->health==88,"stance triggered Soul Barbs");
        check(w.status_mutation(target,180,10,0,100,8100,true)&&w.resources(2)->health==61,"Frenzy then Shielding reduction");
        check(w.protective_enchantment(target,target,*regeneration::reviewed(288,0),0,100)&&w.resources(2)->health==34,"Breeze application reaction");
        check(w.status_mutation(target,180,11,0,100,8100,true)&&w.resources(2)->health==7,"renewal reaction");
        check(w.status_mutation(target,180,12,0,100,8100,true)&&w.resources(2)->health==0,"Soul Barbs lethal application cleanup");
        const auto remaining=*w.status_snapshot(target,100);check(std::none_of(remaining.begin(),remaining.end(),[](const auto& e){return e.skill==100||e.skill==288||e.skill==299;}),"death retained target-owned effects");
    }
    {
        using namespace world;using namespace skills;using namespace combat::npcs;const std::string hash(64,'a');
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto population=load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 1 "+hash+"\nsource test authored "+hash+" tests/soul_barbs.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 - 1 0 test test - - -\n");const auto catalog=load_catalog(metadata,hash);bind_combat_catalog(population,catalog,hash);
        Instance w;w.configure_npcs(population.for_map(146));w.join(1,"Necromancer",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.set_vitals(1,{200,200,40,40},100);
        w.join(2,"Bonded",500,0,std::make_shared<Outbox>(),0,{}, {},100,2);w.set_vitals(2,{200,200,40,40},100);
        const auto source=*w.effect_scope(1),npc=*w.effect_scope(100),bonded=*w.effect_scope(2);
        check(w.elemental_armor(bonded,60,60),"Bonded cold armor");
        check(w.soul_barbs(source,npc,*soul_barbs(12),20,100)&&w.npc_combat_snapshot(npc)->health->current==100,"initial NPC Soul Barbs");
        check(w.stage_npc_bonds_of_torment(npc,bonded,*bonds_of_torment_npc(),100),"NPC Bonds");
        check(w.soul_barbs(source,npc,*soul_barbs(0),20,100)&&w.npc_combat_snapshot(npc)->health->current==73,"NPC prior-rank recast");
        check(w.resources(2)->health==190,"Soul Barbs NPC damage did not reach bonded player");
        check(w.backfire(source,npc,*effects::backfire(28),20,100)&&w.npc_combat_snapshot(npc)->health->current==58,"NPC Backfire application");
        check(w.degeneration_hex(source,npc,*effects::degeneration_hex(31,0),100)&&w.npc_combat_snapshot(npc)->health->current==43,"NPC degeneration application");
        check(w.movement_hex(source,npc,*effects::movement_hex(76,0),100)&&w.npc_combat_snapshot(npc)->health->current==28,"NPC movement application");
        check(w.empathy_npc(source,npc,26,0,100)&&w.npc_combat_snapshot(npc)->health->current==13,"NPC empathy application");
        w.leave(1);w.join(1,"Replacement",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.set_vitals(1,{200,200,40,40},100);const auto replacement=*w.effect_scope(1);
        check(w.faintheartedness(replacement,npc,*effects::faintheartedness(0),100)&&w.npc_combat_snapshot(npc)->health->current==0,"NPC departed original source/lethal reaction");
    }
    check(skills::casting::Runtime::effect_supported(100),"Soul Barbs binding absent");
    std::cout<<"Soul Barbs committed applications, renewal, rollback, lifetime, modifiers and NPC reactions verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
