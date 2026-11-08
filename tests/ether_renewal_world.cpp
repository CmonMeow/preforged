#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;using namespace skills::effects;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Ether Renewal World line "+std::to_string(at.line()));
}
int main()try{
    for(unsigned version:{181u,2860u}){
        world::Instance w;w.join(1,"Owner",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{100,1000,10,100},100);const auto scope=*w.effect_scope(1);
        const auto plan=*ether_renewal(version,15);
        check(!w.status_mutation(scope,version,80,15,100,100000,true));
        check(w.status_mutation(scope,version,80,15,100,100+plan.duration,true));
        check(w.status_mutation(scope,180,81,0,100,60100,true));
        check(!w.ether_renewal_completed(scope,174,1,100));
        check(w.before_spell_effect(scope,174,1,100));
        check(!w.ether_renewal_completed(scope,191,1,100));
        check(w.ether_renewal_completed(scope,174,1,100));
        check(w.resources(1)->health==140);
        check(w.resources(1)->energy==10+2*plan.energy_per_enchantment);
        check(!w.ether_renewal_completed(scope,174,1,100));
        check(w.resources(1)->health==140);
        auto stale=scope;++stale.generation;
        check(!w.ether_renewal_completed(stale,174,1,100));
        // Healing is committed before the caller's sacrifice boundary.
        w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(2,{100,1000,10,100},100);
        check(w.infuse_health(scope,*w.effect_scope(2),*infuse_health(0,0),100));
        check(w.resources(1)->health==70);
        check(w.status_mutation(scope,version,80,15,100,{},false));
        check(w.before_spell_effect(scope,174,2,100));
        check(!w.ether_renewal_completed(scope,174,2,100));
    }
    for(unsigned version:{181u,2860u}){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;
        n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={100,1000};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{10,30};d.energy_pips=0;
        d.energy_evidence=d.energy_pips_evidence="authored test";
        d.healing_allowed=true;d.healing_evidence="authored test";
        n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Observer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);const auto scope=*w.effect_scope(100);
        const auto plan=*ether_renewal(version,15);
        check(w.apply_npc_ether_renewal(scope,plan,100));
        const auto token=w.begin_npc_action(scope,combat::npcs::control::Action::spell,100,200,skills::Slot{174,0});
        check(token&&!w.complete_npc_action(*token,199));
        check(w.complete_npc_action(*token,200));
        check(w.npc_combat_snapshot(scope)->health->current==120);
        check(w.npc_energy_snapshot(scope)->current==10+plan.energy_per_enchantment);
        check(!w.complete_npc_action(*token,200));
        check(w.npc_combat_snapshot(scope)->health->current==120);
    }
    for(unsigned skill:{180u,1392u,31u}){
        world::Instance w;std::vector<world::Instance::Npc> population;
        for(unsigned id:{100u,101u}){
            world::Instance::Npc n;n.id=n.definition.id=id;n.x=float((id-100)*60);
            n.definition.level=20;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=id;d.binding.level=20;
            d.relation=id==100?combat::npcs::Relation::hostile_to_players:combat::npcs::Relation::allied_to_players;
            d.spawn_health={id==100?100.:1000.,1000};d.damageable=d.conditions_allowed=true;
            d.spawn_energy=combat::npcs::Energy{50,50};d.energy_pips=0;
            d.energy_evidence=d.energy_pips_evidence="authored terminal fixture";
            d.healing_allowed=true;d.healing_evidence="authored terminal fixture";
            n.combat_definition=d;population.push_back(n);
        }
        w.configure_npcs(population);w.join(1,"Observer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);const auto source=*w.effect_scope(100),target=*w.effect_scope(101);
        check(w.configure_npc_spellbook(source,{{{static_cast<std::uint16_t>(skill),0},skill==180?0.f:1248.f,"authored terminal fixture",
            skill==180?combat::ai::SpellTargetClass::self:combat::ai::SpellTargetClass::reviewed}}));
        check(w.apply_npc_ether_renewal(source,*ether_renewal(181,15),100));
        std::optional<combat::npcs::control::Token> token;
        if(skill==180)token=w.begin_npc_aura(source,*aura_restoration_plan(0),1000);
        else if(skill==1392)token=w.begin_npc_watchful(source,*watchful_healing(0),1000);
        else token=w.begin_npc_phantasm(source,target,0,1000);
        check(bool(token));w.tick(token->completes);
        check(w.npc_combat_snapshot(source)->health->current==120);
        check(w.npc_energy_snapshot(source)->current==50-skills::find_skill(skill)->energy_cost()+4);
        if(skill==31){const auto effects=w.status_snapshot(target,token->completes);
            check(effects&&std::any_of(effects->begin(),effects->end(),[](const auto& e){return e.skill==31;}));}
    }
    for(bool elemental_provider:{false,true}){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=20;
        n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={150,200};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
        d.healing_allowed=true;d.healing_evidence="authored projectile Renewal";
        n.combat_definition=d;w.configure_npcs({n});w.join(1,"Target",1000,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,0,100},100);const auto source=*w.effect_scope(100),target=*w.effect_scope(1);
        check(w.elemental_armor(target,60,60));combat::ai::FlarePolicy policy;
        policy.source=source;policy.level=20;policy.fire_magic=12;policy.speed=100;
        policy.acquire=1500;policy.follow=2000;policy.leash=2500;policy.flight={100,32};
        policy.evidence="authored projectile Renewal";
        if(elemental_provider)check(w.configure_npc_elemental_ai(policy,{{{194,0},*elemental(194,12),policy.evidence}},2,1000));
        else check(w.configure_npc_flare_ai(policy,1000));
        check(w.apply_npc_ether_renewal(source,*ether_renewal(181,15),1000));
        w.tick(2000);check(w.npc_energy_snapshot(source)->current==35);
        w.tick(3000);check(w.npc_combat_snapshot(source)->health->current==170);
        check(w.npc_energy_snapshot(source)->current==39&&w.resources(1)->health==1000);
        w.tick(3100);check(w.npc_combat_snapshot(source)->health->current==170);
    }
    for(unsigned mode=0;mode<3;++mode){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;
        n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={100,1000};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
        d.healing_allowed=d.knockdown_allowed=true;d.healing_evidence=d.knockdown_evidence="authored friendly Renewal";
        n.combat_definition=d;w.configure_npcs({n});
        for(unsigned id:{1u,2u}){w.join(id,"Ally",200,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(id,{50,1000,40,40},100);}
        const auto source=*w.effect_scope(100),target=*w.effect_scope(1);
        check(w.configure_npc_friendly_healer(source,{286,0,0,"authored friendly Renewal"}));
        check(w.apply_npc_ether_renewal(source,*ether_renewal(181,15),100));
        const auto token=w.begin_npc_friendly_heal(source,target,1000);check(bool(token));
        if(mode==1)check(w.knockdown_npc(source,1000,1100)==combat::npcs::control::Knockdown::accepted);
        if(mode==2)w.leave(1);
        w.tick(token->completes);
        check(w.npc_combat_snapshot(source)->health->current==(mode==0?120:100));
        check(w.npc_energy_snapshot(source)->current==(mode==0?34u:30u));
        if(mode!=2)check(w.resources(1)->health==(mode==0?85u:50u));
    }
    for(unsigned mode=0;mode<4;++mode){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;
        n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={1000,1000};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
        d.healing_allowed=true;d.healing_evidence="authored Renewal lifecycle";
        n.combat_definition=d;w.configure_npcs({n});auto box=std::make_shared<world::Outbox>();
        w.join(1,"Observer",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(100),observer=*w.effect_scope(1);
        check(w.apply_npc_ether_renewal(source,*ether_renewal(181,0),100));
        const auto effects=*w.status_snapshot(source,100);
        const auto entry=std::find_if(effects.begin(),effects.end(),[](const auto& e){return e.skill==181;});
        check(entry!=effects.end());world::Packet end{67,0};combat::detail::put(end,100);combat::detail::put(end,entry->instance);
        box->drain();
        if(mode==1)check(w.enchantment_removal(observer,source,*enchantment_removal(69,0),20,200));
        if(mode==2)check(w.damage_npc(observer,source,{combat::Kind::armor_ignoring,2000,20,23},1248,200).killed);
        if(mode==3)check(w.apply_npc_ether_renewal(source,*ether_renewal(2860,0),200));
        if(mode==0){w.tick(5099);const auto early=box->drain();check(std::count(early.begin(),early.end(),end)==0);w.tick(5299);}
        auto packets=box->drain();
        if(std::count(packets.begin(),packets.end(),end)!=1)
            throw std::runtime_error("Ether Renewal immediate end mode "+std::to_string(mode));
        w.tick(5400);packets=box->drain();check(std::count(packets.begin(),packets.end(),end)==0);
    }
    for(unsigned version:{181u,2860u})for(unsigned mode=0;mode<6;++mode){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;
        n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={100,1000};d.damageable=true;
        d.spawn_energy=combat::npcs::Energy{40,40};d.energy_pips=0;
        d.healing_allowed=d.knockdown_allowed=true;d.healing_evidence=d.knockdown_evidence="authored paid Renewal";
        if(mode==5)d.healing_allowed.reset();
        n.combat_definition=d;w.configure_npcs({n});
        w.join(1,"Observer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{1000,1000,100,100},100);const auto source=*w.effect_scope(100),observer=*w.effect_scope(1);
        check(w.configure_npc_spellbook(source,{{{static_cast<std::uint16_t>(version),0},0,"authored paid Renewal",combat::ai::SpellTargetClass::self}}));
        if(mode==3)check(w.apply_npc_ether_renewal(source,*ether_renewal(version,15),100));
        if(mode==4)check(w.backfire(observer,source,*backfire(28,12),20,100));
        const auto plan=*ether_renewal(version,15);
        if(mode==5){check(!w.begin_npc_ether_renewal(source,plan,1000));check(w.npc_energy_snapshot(source)->current==40);continue;}
        const auto token=w.begin_npc_ether_renewal(source,plan,1000);check(bool(token));
        check(token->completes==2000&&w.npc_energy_snapshot(source)->current==30);
        check(!w.begin_npc_ether_renewal(source,plan,1000));
        check(!w.complete_npc_ether_renewal(*token,1999));
        check(!w.complete_npc_action(*token,2000)); // Generic completion cannot steal the typed effect.
        check(!w.complete_npc_aura(*token,2000)); // Nor can the other self-enchantment wrapper.
        if(mode==1)check(w.knockdown_npc(source,1000,1100)==combat::npcs::control::Knockdown::accepted);
        if(mode==2)check(w.damage_npc(observer,source,{combat::Kind::armor_ignoring,2000,20,23},1248,1500).killed);
        w.tick(2000);const auto effects=*w.status_snapshot(source,2000);
        const auto effect=std::find_if(effects.begin(),effects.end(),[&](const auto& e){return e.skill==version;});
        check((effect!=effects.end())==(mode==0||mode==3));
        if(effect!=effects.end())check(effect->applied==2000&&effect->expires==2000+plan.duration);
        check(w.npc_combat_snapshot(source)->health->current==((mode==2||mode==4)?0:mode==3?120:100));
        check(w.npc_energy_snapshot(source)->current==30+(mode==3?plan.energy_per_enchantment:0));
        check(!w.complete_npc_ether_renewal(*token,2000));
        check(!w.begin_npc_ether_renewal(source,plan,2001));
    }
    for(unsigned spell:{292u,1526u})for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<6;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);
        w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{100,1000,40,40},100);w.set_vitals(2,{100,1000,40,40},100);w.public_professions(1,6,spell==292?3:10);
        const auto caster=*w.effect_scope(1),target=*w.effect_scope(2);
        if(version){const auto plan=*ether_renewal(version,15);check(w.status_mutation(caster,version,80,15,100,100+plan.duration,true));}
        if(mode==1)check(w.status_mutation(caster,180,81,0,100,60100,true));
        skills::casting::Hooks hooks;std::uint64_t admitted=0,completed=0;
        hooks.events=[](const auto&){};hooks.resources=[&](auto id){return w.resources(id);};
        hooks.consume_energy=[&](auto n){return w.consume_energy(1,n);};
        hooks.target_valid=[](auto id,float,bool self){return id==2&&!self;};
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.infuse_target_valid=[&](auto id,auto now){return w.valid_infuse_target(caster,id,now);};
        hooks.before_spell_effect=[&](auto skill,auto event,auto now){admitted=event;return w.before_spell_effect(caster,mode==3?174:skill,event,now);};
        hooks.infuse_health=[](auto,const auto&,auto)->bool{throw std::runtime_error("legacy Infuse path selected");};
        hooks.infuse_health_completed=[&](auto destination,const auto& plan,auto event,auto now){
            completed=event;return w.infuse_health(caster,destination,plan,now,event);};
        hooks.imbue_health=[](auto,const auto&,auto)->bool{throw std::runtime_error("legacy Imbue path selected");};
        hooks.imbue_health_completed=[&](auto destination,const auto& plan,auto event,auto now){
            completed=event;return w.imbue_health(caster,destination,plan,now,event);};
        skills::Policy policy;policy.primary=6;policy.secondary=spell==292?3:10;policy.campaigns=0x1e;policy.learned[spell]=1;
        skills::Slots bar{};bar[0]={static_cast<std::uint16_t>(spell),0};attributes::Ranks ranks{};skills::casting::Runtime runtime(1);
        check(runtime.start(100,{spell,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted);
        if(mode==2)w.leave(2);
        if(mode==4){hooks.infuse_health_completed={};hooks.imbue_health_completed={};}
        if(mode==5)hooks.before_spell_effect={};
        box->drain();
        const auto at=100+std::uint64_t(skills::find_skill(spell)->activation*1000);runtime.update(at,hooks);
        if(mode>=2){check(w.resources(1)->health==100&&w.resources(1)->energy==30);
            check(mode==3?admitted&&completed==admitted:completed==0);continue;}
        check(admitted&&completed==admitted);
        const unsigned enchantments=mode==1?2:1;
        const unsigned healed=version?20*enchantments:0;
        const unsigned caster_health=spell==292?(100+healed)/2:100+healed;
        const unsigned target_health=100+(spell==292?(100+healed)/2:(100+healed)/20);
        check(w.resources(1)->health==caster_health&&w.resources(2)->health==target_health);
        check(w.resources(1)->energy==30+(version?ether_renewal(version,15)->energy_per_enchantment*enchantments:0));
        const auto packets=box->drain();
        const auto loss=skills::particles::health_change_2022(1,1,-double((100+healed)/2),1000);
        check(std::count(packets.begin(),packets.end(),loss)==(spell==292?1:0));
        if(spell==292)check(std::count(packets.begin(),packets.end(),skills::particles::infuse_health_target_2022(2,1))==1);
        else check(std::count(packets.begin(),packets.end(),skills::particles::health_change_2022(2,1,target_health-100,1000))==1);
        if(version){const auto reward=skills::particles::health_change_2022(1,1,healed,1000);
            check(std::count(packets.begin(),packets.end(),reward)==1);
            if(spell==292)check(std::find(packets.begin(),packets.end(),reward)<std::find(packets.begin(),packets.end(),loss));}
        check(!(spell==292?w.infuse_health(caster,target,*infuse_health(0),at,completed):w.imbue_health(caster,target,*imbue_health(0),at,completed)));
        check(w.resources(1)->health==caster_health&&w.resources(2)->health==target_health);
    }
    for(unsigned spell:{292u,1526u})for(unsigned version:{181u,2860u})for(unsigned mode=0;mode<3;++mode){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;
        n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::allied_to_players;d.spawn_health={100,1000};d.damageable=true;
        if(mode!=2)d.healing_allowed=mode==0;d.healing_evidence="authored atomic Infuse recipient";
        n.combat_definition=d;w.configure_npcs({n});w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{100,1000,10,40},100);const auto source=*w.effect_scope(1),target=*w.effect_scope(100);
        const auto plan=*ether_renewal(version,15);check(w.status_mutation(source,version,80,15,100,100+plan.duration,true));
        check(w.before_spell_effect(source,spell,1,100));
        auto complete=[&](auto destination){return spell==292?w.infuse_health(source,destination,*infuse_health(0),100,1):
            w.imbue_health(source,destination,*imbue_health(0),100,1);};
        auto stale=target;++stale.generation;
        check(!complete(stale));check(!complete(source));
        check(w.resources(1)->health==100&&w.resources(1)->energy==10);
        // Preserve heal_ally's existing contract: explicit immunity rejects;
        // an otherwise healable ally with no immunity observation is allowed.
        const bool accepted=mode!=1;
        check(complete(target)==accepted);
        check(w.resources(1)->health==(accepted?(spell==292?60:120):100));
        check(w.resources(1)->energy==10+(accepted?plan.energy_per_enchantment:0));
        check(w.npc_combat_snapshot(target)->health->current==(accepted?(spell==292?160:106):100));
        if(accepted)check(!complete(target));
    }
    for(unsigned spell:{292u,1526u}){
        world::Instance w;w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Ally",60,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{100,1000,40,40},100);w.set_vitals(2,{100,1000,40,40},100);
        const auto caster=*w.effect_scope(1);skills::casting::Hooks hooks;unsigned legacy=0;
        hooks.events=[](const auto&){};hooks.resources=[&](auto id){return w.resources(id);};
        hooks.consume_energy=[&](auto n){return w.consume_energy(1,n);};
        hooks.target_valid=[](auto id,float,bool self){return id==2&&!self;};
        hooks.target_scope=[&](auto id){return w.effect_scope(id);};
        hooks.infuse_target_valid=[&](auto id,auto at){return w.valid_infuse_target(caster,id,at);};
        hooks.infuse_health=[&](auto target,const auto& plan,auto at){++legacy;return w.infuse_health(caster,target,plan,at);};
        hooks.imbue_health=[&](auto target,const auto& plan,auto at){++legacy;return w.imbue_health(caster,target,plan,at);};
        skills::Policy policy;policy.primary=6;policy.secondary=spell==292?3:10;policy.campaigns=0x1e;policy.learned[spell]=1;
        skills::Slots bar{};bar[0]={static_cast<std::uint16_t>(spell),0};attributes::Ranks ranks{};skills::casting::Runtime runtime(1);
        check(runtime.start(100,{spell,0,2},bar,policy,ranks,true,hooks)==skills::casting::Reason::accepted);
        hooks.infuse_health_completed=[](auto,const auto&,auto,auto)->bool{throw std::runtime_error("new Infuse route replaced admitted legacy route");};
        hooks.imbue_health_completed=[](auto,const auto&,auto,auto)->bool{throw std::runtime_error("new Imbue route replaced admitted legacy route");};
        runtime.update(350,hooks);check(legacy==1);
        check(w.resources(1)->health==(spell==292?50:100)&&w.resources(2)->health==(spell==292?150:105));
    }
    for(unsigned spell:{1227u,170u,1086u,1068u,866u,163u,188u,1081u,845u,194u,183u,186u,189u,191u,1379u,884u,2192u,1095u,846u,89u})
    for(unsigned version:{181u,2860u}){
        world::Instance w;const auto plan=*elemental(spell,0);
        {world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;n.definition.level=20;
            n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
            d.binding.map=146;d.binding.actor=d.binding.definition=100;d.binding.level=20;
            d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={spell==1068?110.:1000.,1000};d.damageable=d.conditions_allowed=true;
            d.fire_armor=d.cold_armor=d.lightning_armor=d.earth_armor=60;
            d.fire_armor_evidence=d.cold_armor_evidence=d.lightning_armor_evidence=d.evidence.armor="authored elemental completion";
            d.knockdown_allowed=true;d.knockdown_evidence="authored elemental completion";
            d.spawn_energy=combat::npcs::Energy{30,30};d.energy_pips=0;
            d.creature_type="human";d.creature_type_evidence="authored elemental completion";
            n.combat_definition=d;w.configure_npcs({n});}
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{100,1000,50,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(plan.caster_center?1:100);
        const auto renewal=*ether_renewal(version,15);check(w.status_mutation(source,version,80,15,100,100+renewal.duration,true));
        check(w.before_spell_effect(source,spell,1,100));std::vector<world::Packet> visuals;ElementalOutcome outcome;
        auto complete=[&](auto destination,unsigned event){return w.elemental(source,destination,plan,20,100,&visuals,nullptr,&outcome,event);};
        auto stale=target;++stale.generation;
        check(!complete(stale,1)&&!complete(target,2));
        check(w.resources(1)->health==100&&w.resources(1)->energy==50);
        if(!complete(target,1))throw std::runtime_error("elemental completion skill "+std::to_string(spell));
        check(w.resources(1)->health==120&&w.resources(1)->energy>=50+renewal.energy_per_enchantment);
        if(spell==1068)check(!outcome.instant_recharge); //110 >100 before Renewal,but110 <120 afterward.
        check(!complete(target,1));check(w.resources(1)->health==120);
        if(plan.projectile){check(w.npc_combat_snapshot(target)->health->current==1000);w.tick(1000);
            check(w.npc_combat_snapshot(target)->health->current<1000&&w.resources(1)->health==120);}
    }
    for(unsigned spell:{1227u,170u,1086u,1068u,866u,163u,188u,1081u,845u,194u,183u,186u,189u,191u,1379u,884u,2192u,1095u,846u,89u})for(unsigned version:{0u,181u,2860u})for(unsigned mode=0;mode<(spell==189?6u:4u);++mode){
        world::Instance w;world::Instance::Npc n;n.id=n.definition.id=100;n.x=60;
        n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
        d.binding.map=146;d.binding.actor=d.binding.definition=100;
        d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health={spell==1068?110.:1000.,1000};d.damageable=d.conditions_allowed=true;
        d.fire_armor=d.cold_armor=d.earth_armor=d.lightning_armor=60;
        d.creature_type="human";d.creature_type_evidence="authored paid elemental";
        d.knockdown_allowed=true;d.knockdown_evidence="authored paid elemental";
        n.combat_definition=d;std::vector<world::Instance::Npc> population{n};
        if(spell==189)for(unsigned id:{101u,102u}){auto other=n;other.id=other.definition.id=id;other.x=id==101?299.f:301.f;
            other.combat_definition->binding.actor=other.combat_definition->binding.definition=id;population.push_back(other);}
        w.configure_npcs(population);
        w.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.set_vitals(1,{100,1000,100,100},100);
        const auto source=*w.effect_scope(1);const auto renewal=version?*ether_renewal(version,15):EtherRenewalPlan{};
        if(version)check(w.status_mutation(source,version,80,15,100,100+renewal.duration,true));
        skills::casting::Hooks hooks;unsigned commits=0;bool target_replaced=false;std::vector<world::Packet> feedback;
        hooks.events=[&](const auto& packets){feedback.insert(feedback.end(),packets.begin(),packets.end());};
        hooks.resources=[&](auto id){return w.resources(id);};hooks.consume_energy=[&](auto cost){return w.consume_energy(1,cost);};
        hooks.consume_overcast_energy=[&](auto skill,auto cost,auto event,auto at){return w.activate_overcast_energy(source,skill,cost,event,at);};
        hooks.target_scope=[&](auto id){auto scope=w.effect_scope(id);if(scope&&id==100&&target_replaced)++scope->generation;return scope;};
        hooks.target_valid=[](auto id,float,bool self){return id==100||(id==1&&self);};
        hooks.projectile_target_valid=[](auto id,float){return id==100;};
        hooks.rodgort_target_valid=[&](auto target,auto at){return w.effect_scope(target.actor)==target&&w.projectile_target_valid(source,target.actor,1248,at,false,true);};
        hooks.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
        hooks.elemental=[](auto,const auto&,auto)->skills::casting::EffectCompletion{throw std::runtime_error("legacy elemental selected");};
        hooks.conditional_elemental=[](auto,const auto&,auto)->std::optional<skills::casting::ConditionalElementalCompletion>{throw std::runtime_error("legacy conditional selected");};
        hooks.elemental_completed=[&](auto target,const auto& plan,auto event,auto at)->std::optional<skills::casting::ConditionalElementalCompletion>{
            ++commits;std::vector<world::Packet> visuals;ElementalOutcome outcome;
            if(!w.elemental(source,target,plan,20,at,&visuals,nullptr,&outcome,event))return {};
            return skills::casting::ConditionalElementalCompletion{outcome,std::move(visuals)};};
        skills::Policy policy;policy.primary=6;policy.secondary=skills::find_skill(spell)->profession==6?4:skills::find_skill(spell)->profession;policy.campaigns=255;policy.learned[spell]=1;
        skills::Slots bar{};bar[0]={static_cast<std::uint16_t>(spell),0};attributes::Ranks ranks{};skills::casting::Runtime runtime(1);
        const auto plan=*elemental(spell,0);
        if(spell==189)check(plan.radius==240);
        if(runtime.start(100,{spell,0,plan.caster_center?1u:100u},bar,policy,ranks,true,hooks)!=skills::casting::Reason::accepted)
            throw std::runtime_error("paid elemental admission skill "+std::to_string(spell));
        const auto paid_energy=w.resources(1)->energy;
        if(mode==1)hooks.elemental_completed={};if(mode==2)hooks.before_spell_effect={};if(mode==3)runtime.cancel(hooks);
        if(mode==4){const auto target=*w.effect_scope(100);
            check(w.damage_npc(source,target,{combat::Kind::armor_ignoring,2000,20,23},1248,200).killed);
            check(!w.projectile_target_valid(source,100,1248,200));check(hooks.rodgort_target_valid(target,200));}
        if(mode==5)target_replaced=true;
        const auto at=100+std::uint64_t(skills::find_skill(spell)->activation*1000);runtime.update(at,hooks);
        const bool success=mode==0||mode==4;
        check(commits==unsigned(success));check(w.resources(1)->health==(success&&version?120:100));
        if(success)check(w.resources(1)->energy>=std::min(w.resources(1)->max_energy,paid_energy+(version?renewal.energy_per_enchantment:0)));
        else check(w.resources(1)->energy==paid_energy);
        if(mode==0&&spell==1068)check(std::count(feedback.begin(),feedback.end(),skills::casting::slot_event(228,1,{1068,0},version?10:0))==1);
        if(mode==0&&spell==189){const auto body=*skills::particles::targeted_body_2022(100,1,189);
            check(std::count(feedback.begin(),feedback.end(),body)==1);
            check(std::find(feedback.begin(),feedback.end(),skills::casting::terminal(58,1))<std::find(feedback.begin(),feedback.end(),body));}
        if(mode==4){check(w.npc_combat_snapshot(*w.effect_scope(101))->health->current==985);
            check(w.npc_combat_snapshot(*w.effect_scope(102))->health->current==1000);
            check(std::count(feedback.begin(),feedback.end(),*skills::particles::targeted_body_2022(100,1,189))==0);
            check(std::count(feedback.begin(),feedback.end(),*skills::particles::targeted_body_2022(101,1,189))==1);}
    }
    for(unsigned version:{0u,181u,2860u}){
        world::Instance w;for(unsigned id:{1u,2u,3u,4u}){w.join(id,"Player",id==3?299.f:id==4?301.f:float((id-1)*60),0,std::make_shared<world::Outbox>(),0,{}, {},100,id==1?1:2);
            w.set_vitals(id,{id==1?100u:1000u,1000,50,100},100);check(w.elemental_armor(*w.effect_scope(id),60,60));}
        const auto source=*w.effect_scope(1),center=*w.effect_scope(2);
        if(version){const auto renewal=*ether_renewal(version,15);check(w.status_mutation(source,version,80,15,100,100+renewal.duration,true));}
        w.set_vitals(2,{0,1000,50,100},100);
        check(!w.projectile_target_valid(source,2,1248,100));check(w.projectile_target_valid(source,2,1248,100,false,true));
        check(w.before_spell_effect(source,189,1,100));std::vector<world::Packet> visuals;ElementalOutcome outcome;
        check(w.elemental(source,center,*elemental(189,0),20,100,&visuals,nullptr,&outcome,1));
        check(w.resources(2)->health==0&&w.resources(3)->health==985&&w.resources(1)->health==(version?120:100));
        check(w.resources(4)->health==1000);
        check(std::count(visuals.begin(),visuals.end(),*skills::particles::targeted_body_2022(3,1,189))==1);
        check(std::count(visuals.begin(),visuals.end(),*skills::particles::targeted_body_2022(2,1,189))==0);
    }
    std::cout<<"PASS Ether Renewal scoped resources, atomic player spells, paid casts and NPC lifecycle\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
