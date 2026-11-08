#include "gw_npc_catalog.h"
#include "gw_overcast.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
using namespace skills;
void check(bool value,std::source_location where=std::source_location::current()){
    if(!value)throw std::runtime_error("captured skill regression line "+std::to_string(where.line()));
}
int main()try{
    using combat::conditions::Kind;
    for(unsigned skill:{397u,404u})for(bool prepared:{false,true}){
        world::Instance w;w.join(1,"Archer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);w.join(2,"Foe",900,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{480,480,40,40},100);w.set_vitals(2,{480,480,40,40},100);
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);combat::weapon::Owned bow{7,1,{20,20,1},12,false,false,5};
        check(w.ordinary_weapon(a,bow,20)&&w.physical_armor(b,60));const auto plan=*combat::weapon::attack_skill(skill,12);
        if(prepared)check(w.status_mutation(a,432,1,12,100,24100,true));
        check(plan.projectile_speed_percent==(skill==397?200u:100u));
        if(skill==397)check(plan.windup_ms==1000);
        auto bad=plan;bad.projectile_speed_percent=300;check(!w.melee_attack(a,b,bow,bad,100));
        const unsigned hit_health=prepared?451u:460u;
        check(w.melee_attack(a,b,bow,plan,100));
        w.tick(190);check(w.resources(2)->health==480); // A wrongly stacked 4x Quick Shot would already hit.
        w.tick(290);check(w.resources(2)->health==(skill==397?hit_health:480u));
        w.tick(490);check(w.resources(2)->health==hit_health);
    }

    {
        using namespace casting;world::Instance w;w.join(1,"Runner",0,0,std::make_shared<world::Outbox>());w.set_vitals(1,{100,100,40,40},100);
        const auto scope=*w.effect_scope(1);Hooks h;h.events=[](const auto&){};
        h.resources=[&](auto id){return w.resources(id);};h.consume_energy=[&](auto amount){return w.consume_energy(1,amount);};
        h.target_valid=[](auto id,float,bool){return id==1;};h.speed_multiplier=[&](double speed){return w.movement_multiplier(1,float(speed),100);};
        h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto ends,bool active){return w.status_mutation(scope,skill,effect,rank,now,ends,active);};
        h.effect_snapshot=[&](auto now){return w.status_snapshot(scope,now);};
        Policy policy;policy.primary=1;policy.secondary=7;policy.campaigns=0x1e;policy.learned[349]=policy.learned[1043]=1;
        Slots bar{};bar[0]={349,0};bar[1]={1043,0};attributes::Ranks ranks{};ranks[17]=12;
        Runtime cast(1);check(cast.start(100,{349,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(100,h);
        check(w.status_snapshot(scope,100)->front().expires==13100);
        w.move(1,5000,0,100);w.tick(1100);check(std::abs(w.position(1,1100).first-360)<.2);
        check(cast.start(1100,{1043,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(1100,h);
        w.tick(2100);check(std::abs(w.position(1,2100).first-792)<.3);
        w.tick(5100);check(std::abs(w.position(1,5100).first-1944)<.5);
        effects::MovementEffects speed;speed.apply({1,1,349,12,effects::Category::stance,100,13100});speed.apply({2,2,76,0,effects::Category::hex,100,8100});
        check(speed.at(100)==.625f&&speed.integral(100,1100)==625);
    }



    for(unsigned id:{286u,958u,1120u}){
        using namespace casting;std::vector<Packet> sent;Hooks h;
        h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
        h.resources=[](auto)->std::optional<vitals::Resources>{return vitals::Resources{100,1000,40,40};};
        h.consume_energy=[](auto){return true;};h.target_valid=[](auto id,float,bool){return id==43;};
        h.heal=[](auto id,auto,float,bool){return id==43;};
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[id]=1;Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};
        Runtime cast(42);check(cast.start(100,{id,0,43},bar,policy,ranks,true,h)==casting::Reason::accepted);
        cast.update(100+std::uint64_t(find_skill(id)->activation*1000),h);
        const auto correct=*particles::targeted_body_2022(43,42,id),wrong=*particles::target_body_2022(43,id);
        check(std::count(sent.begin(),sent.end(),correct)==1&&std::count(sent.begin(),sent.end(),wrong)==0);
    }

    {
        using namespace casting;world::Instance w;auto infuse_out=std::make_shared<world::Outbox>();
        w.join(1,"Monk",0,0,infuse_out,0,{}, {},100,1);
        w.join(2,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.set_vitals(1,{400,500,40,40},100);w.set_vitals(2,{100,1000,40,40},100);w.public_professions(1,3,0);
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);Hooks h;
        h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};
        h.consume_energy=[&](auto n){return w.consume_energy(1,n);};
        h.target_valid=[](auto id,float,bool self){return id==2&&!self;};h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.infuse_target_valid=[&](auto target,auto now){return w.valid_infuse_target(a,target,now);};
        h.infuse_health=[&](auto target,const auto& plan,auto now){return w.infuse_health(a,target,plan,now);};
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[292]=1;Slots bar{};bar[0]={292,0};attributes::Ranks ranks{};ranks[find_skill(292)->attribute]=12;ranks[16]=10;
        Runtime cast(1);check(cast.start(100,{292,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);
        const auto end=100+std::uint64_t(find_skill(292)->activation*1000);cast.update(end,h);
        const auto published=infuse_out->drain();
        const auto particle=particles::infuse_health_target_2022(2,1);
        check(std::count(published.begin(),published.end(),particle)==1);
        const auto heal_number=particles::health_change_2022(2,1,290,1000),loss_number=particles::health_change_2022(1,1,-200,500);
        check(std::count(published.begin(),published.end(),heal_number)==1&&std::count(published.begin(),published.end(),loss_number)==1);

        check(w.resources(1)->health==200&&w.resources(2)->health==390); // 200*1.29 +32.
        auto stale=b;++stale.generation;check(!w.infuse_health(a,stale,*effects::infuse_health(12,10),end));
        check(!w.infuse_health(a,a,*effects::infuse_health(12,10),end)&&w.resources(1)->health==200);
        auto forged=*effects::infuse_health(12,10);forged.percent=999;check(!w.infuse_health(a,b,forged,end));
        Runtime cancel(1);check(cancel.start(end+1000,{292,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);cancel.cancel(h);cancel.update(end+2000,h);check(w.resources(1)->health==200);
    }

    for(unsigned rank=0;rank<=20;++rank){
        const auto plan=effects::shadow_of_fear(rank);check(plan&&plan->pips==0&&plan->skill==136&&plan->radius==166&&plan->slower_percent==50);
        check(effects::valid_faintheartedness(*plan));
        const effects::Scope scope{1,2,3};effects::Registry registry(scope);skills::regeneration::State regeneration(scope,100);
        const auto applied=effects::apply_faintheartedness(scope,registry,regeneration,99,1,*plan,100);
        check(applied&&applied->entry.skill==136);
        const auto active=applied->registry.active(scope,100);
        check(effects::faintheartedness_active(active,100));
        check(!effects::faintheartedness_active(active,100+plan->duration));
        check(effects::reviewed_attack_duration(1000,false,effects::faintheartedness_active(active,100))==1500);
    }

    {
        using namespace casting;world::Instance w;auto feast_out=std::make_shared<world::Outbox>();
        for(unsigned id=1;id<=6;++id){w.join(id,"Feast test",float(id*30),0,id==1?feast_out:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==6?1:2);w.set_vitals(id,{id==1?10u:100u,100,40,40},100);}
        const auto scope=*w.effect_scope(1);Hooks h;
        h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};
        h.consume_energy=[&](auto n){return w.consume_energy(1,n);};h.target_valid=[](auto id,float,bool){return id==1;};
        h.area_life_steal=[&](const auto& plan,auto now){return w.area_life_steal(scope,plan,now);};
        Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.learned[110]=1;Slots bar{};bar[0]={110,0};attributes::Ranks ranks{};ranks[find_skill(110)->attribute]=12;
        Runtime cast(1);check(cast.start(100,{110,0,5},bar,policy,ranks,true,h)==casting::Reason::accepted);
        cast.update(100+std::uint64_t(find_skill(110)->activation*1000),h);
        check(w.resources(1)->health==100);
        const auto packets=feast_out->drain();
        for(unsigned id=2;id<=4;++id){
            const auto expected=combat::detail::integer(20,id,210);
            check(std::count(packets.begin(),packets.end(),expected)==1);
            const auto wrong=combat::detail::integer(21,id,210);
            check(std::count(packets.begin(),packets.end(),wrong)==0);
        }
        for(unsigned id=2;id<=4;++id)check(w.resources(id)->health==46);
        check(w.resources(5)->health==100&&w.resources(6)->health==100);
        auto bad=*effects::area_life_steal(110,12);++bad.maximum_targets;check(!w.area_life_steal(scope,bad,3000));
    }

    {
        const unsigned amount[]={10,14,17,21,25,28,32,36,39,43,47,50,54,58,61,65,69,72,76,80,83};
        const unsigned count[]={1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5};
        for(unsigned rank=0;rank<=20;++rank){const auto p=effects::area_life_steal(110,rank);check(p&&p->amount==amount[rank]&&p->maximum_targets==count[rank]&&p->radius==322.f);}
        check(!effects::area_life_steal(110,21)&&!effects::area_life_steal(153,0));
    }

    for(unsigned id:{16u,17u}){
        using namespace casting;
        world::Instance w;w.join(1,"Protected",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Enemy",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{100,100,40,40},100);w.set_vitals(2,{100,100,0,40},100);
        auto scope=*w.effect_scope(1),foe=*w.effect_scope(2);
        Hooks h;h.events=[](const auto&){};h.resources=[&](auto actor){return w.resources(actor);};
        h.consume_energy=[&](unsigned n){return w.consume_energy(1,n);};
        h.target_valid=[](auto id,float,bool){return id==1;};
        h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto ends,bool active){return w.status_mutation(scope,skill,effect,rank,now,ends,active);};
        h.effect_snapshot=[&](auto now){return w.status_snapshot(scope,now);};
        Policy policy;policy.primary=5;policy.campaigns=0x1e;policy.learned[id]=1;policy.learned[6]=1;
        Slots bar{};bar[0]={id,0};bar[1]={6,0};attributes::Ranks ranks{};
        Runtime cast(1);check(cast.start(100,{id,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);
        cast.update(100,h);auto effects=w.status_snapshot(scope,100);check(effects&&effects->size()==1&&effects->front().skill==id);
        check(w.cast_activity(scope,281,100,1100));
        check(w.interrupt_skill(foe,scope,*effects::interrupt_skill(61,0),20,100));
        check(w.casting_skill(scope,100)==281);
        // A new stance replaces Resolve; consumed Concentration must not linger.
        check(cast.start(200,{6,0,0},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(200,h);
        check(w.interrupt_skill(foe,scope,*effects::interrupt_skill(61,0),20,200));
        check(!w.casting_skill(scope,200));
        effects=w.status_snapshot(scope,200);check(effects&&effects->size()==1&&effects->front().skill==6);
        cast.update(30200,h);check(w.status_snapshot(scope,30200)->empty());
    }
    {
        world::Instance w;w.join(1,"Protected",0,0,std::make_shared<world::Outbox>());
        w.set_vitals(1,{100,100,40,40},100);const auto scope=*w.effect_scope(1);
        check(w.status_mutation(scope,16,123,0,100,1100,true));
        check(w.cast_activity(scope,281,100,1100));
        auto stale=scope;++stale.generation;
        check(!w.interrupt_cast(stale,281,200));
        check(!w.interrupt_cast(scope,281,200)&&w.casting_skill(scope,200)==281);
        check(w.interrupt_cast(scope,281,201)); // First legitimate attempt consumed protection.
        check(w.status_mutation(scope,16,124,0,300,1300,true));
        check(w.cast_activity(scope,281,1000,2000));
        check(w.interrupt_cast(scope,281,1300)); // Exact expiry is not protected.
    }
    for(unsigned stance:{16u,17u})for(unsigned energy:{0u,4u,20u}){
        world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        w.join(2,"Protected",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        w.set_vitals(1,{100,100,0,40},100);w.set_vitals(2,{100,100,energy,40},100);
        const auto a=*w.effect_scope(1),b=*w.effect_scope(2);
        check(w.status_mutation(b,stance,123,15,100,10000,true));
        check(w.cast_activity(b,281,100,1100));
        check(w.interrupt_skill(a,b,*effects::interrupt_skill(23,0),20,100));
        check(w.casting_skill(b,100)==281&&w.control_epoch(b)==0);
        check(w.resources(2)->health==70); // Damage survives protection.
        check(w.resources(2)->energy==(stance==16?energy:energy>4?energy-4:0));
        check(w.interrupt_skill(a,b,*effects::interrupt_skill(61,0),20,100));
        check(w.resources(1)->energy==3); // Leech gain survives protection too.
        check(bool(w.casting_skill(b,100))==(stance==17&&energy>=4));
    }

    for(unsigned rank=0;rank<=20;++rank){
        const auto touch=*effects::shadow_damage(155,rank);
        check(touch.damage==20+3*rank&&touch.range==144&&!touch.steal_above_half);
        const auto armor=*effects::condition_spell(159,rank);
        check(armor.duration==(5+rank)*1000&&armor.radius==166&&armor.kind==Kind::cracked_armor);
        const auto comfort=*effects::status_heal(1396,rank,0);
        check(effects::status_heal_amount(comfort,0,0,0)==15+3*rank);
        check(effects::status_heal_amount(comfort,0,0,1)==30+5*rank);
        check(effects::status_heal_amount(comfort,2,3,10)==30+5*rank); // once, not per condition.
        const auto mending=*effects::condition_removal(1401,rank,0);
        check(mending.count==2&&mending.range==144&&mending.self&&effects::condition_heal(mending,2,1)==2*(15+3*rank));
    }
    check(!effects::shadow_damage(155,21)&&!effects::condition_spell(159,21)&&!effects::status_heal(1396,21,0));
    {
        world::Instance w;auto out=std::make_shared<world::Outbox>();w.join(1,"Monk",0,0,out);w.public_professions(1,3,0);
        w.join(2,"Ally",10,0,std::make_shared<world::Outbox>());
        check(w.party_action(1,158,2,0)&&w.party_action(2,154,1,0));
        w.set_vitals(1,{100,1000,50,50},100);w.set_vitals(2,{100,1000,50,50},100);
        const auto source=*w.effect_scope(1),ally=*w.effect_scope(2);const auto heal=*effects::status_heal(1396,12,10);
        check(w.status_heal(source,2,heal,100)&&w.resources(2)->health==183); // 51+32 Divine Favor
        check(w.apply_condition(source,ally,Kind::blind,100,100));
        check(w.status_heal(source,2,heal,100)&&w.resources(2)->health==305); // +39 once
        check(w.status_heal(source,2,heal,200)&&w.resources(2)->health==388); // expired condition excluded
        check(w.status_heal(source,1,heal,200)&&w.resources(1)->health==183); // self is legal
        auto forged=heal;++forged.per_status;check(!w.status_heal(source,2,forged,200));
        auto stale=source;++stale.generation;check(!w.status_heal(stale,2,heal,200));
        check(w.apply_condition(source,source,Kind::blind,200,1000)&&w.apply_condition(source,source,Kind::weakness,200,1000)&&w.apply_condition(source,source,Kind::dazed,200,1000));
        const auto mending=*effects::condition_removal(1401,12,10);
        check(w.remove_conditions(source,1,mending,200)&&w.resources(1)->health==317);
        check(w.has_condition(source,Kind::blind,200)&&!w.has_condition(source,Kind::weakness,200)&&!w.has_condition(source,Kind::dazed,200));
    }
    {
        const std::string hash(64,'a');
        std::string placements="gw-npcs 1\n",metadata="gw-npc-combat 2 "+hash+"\nsource test authored "+hash+" tests/captured_skills.cpp\n";
        for(unsigned id:{100u,101u,102u}){
            const auto x=id==100?100:id==101?250:500;
            placements+="npc 146 "+std::to_string(id)+" 1 1234 0 1677721600 0 8 1 1 0 "+std::to_string(x)+" 0 0 - -\n";
            metadata+="npc 146 "+std::to_string(id)+" 1 1234 0 0 8 1 hostile-to-players - 100 100 80 1 1 test test test - test 60 60 test test\n";
        }
        std::istringstream appearance(placements),reviewed(metadata);auto population=world::load_npc_catalog(appearance);
        world::bind_combat_catalog(population,combat::npcs::load_catalog(reviewed,hash),hash);
        {
            std::array<unsigned,2> after_wave{},after_crystal{};
            for(unsigned enabled=0;enabled<2;++enabled){
                world::Instance frag;frag.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
                frag.join(2,"Victim",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
                frag.set_vitals(1,{1000,1000,100,100},100);frag.set_vitals(2,{1000,1000,100,100},100);
                const auto caster=*frag.effect_scope(1),target=*frag.effect_scope(2);check(frag.combat_armor(target,60));
                if(enabled)check(frag.degeneration_hex(caster,target,*effects::degeneration_hex(19,12),100));
                check(frag.shockwave(caster,*effects::caster_earth_aoe(937,12),20,200));after_wave[enabled]=frag.resources(2)->health;
                check(frag.crystal_wave(caster,*effects::crystal_wave(12),300));after_crystal[enabled]=frag.resources(2)->health;
            }
            check(after_wave[0]==after_wave[1]+51&&after_crystal[0]==after_crystal[1]+102);
            world::Instance lethal;auto box=std::make_shared<world::Outbox>();
            lethal.join(1,"Caster",0,0,box,0,{}, {},100,1);lethal.join(2,"Victim",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            lethal.set_vitals(1,{1000,1000,100,100},100);lethal.set_vitals(2,{10,1000,100,100},100);
            const auto a=*lethal.effect_scope(1),b=*lethal.effect_scope(2);
            check(lethal.degeneration_hex(a,b,*effects::degeneration_hex(19,12),100));
            check(lethal.apply_condition(a,b,Kind::blind,200,1000)&&lethal.resources(2)->health==0);
            lethal.tick(1500);check(lethal.resources(2)->health==0);
        }
        {
            world::Instance energy;auto box=std::make_shared<world::Outbox>();
            energy.join(1,"Mesmer",0,0,box,0,{}, {},100,1);energy.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            energy.set_vitals(1,{1000,1000,40,100},100);energy.set_vitals(2,{1000,1000,40,100},100);
            check(energy.energy_capacity(1,5,0,100,{80,-2}));const auto scope=*energy.effect_scope(1);
            using namespace casting;Hooks h;bool heal=true;std::vector<Packet> output;
            h.events=[&](const auto& packets){output.insert(output.end(),packets.begin(),packets.end());};
            h.resources=[&](auto id){return energy.resources(id);};h.consume_energy=[&](auto cost){return energy.consume_energy(1,cost);};
            h.target_valid=[](auto id,float,bool){return id==1;};h.heal=[&](auto,auto,float,bool){return heal;};
            h.status_mutation=[&](auto skill,auto effect,auto rank,auto now,auto ends,bool active){return energy.status_mutation(scope,skill,effect,rank,now,ends,active);};
            h.effect_snapshot=[&](auto now){return energy.status_snapshot(scope,now);};
            h.prepare_channeling=[&](auto skill,auto event,auto now){return energy.prepare_channeling(scope,skill,event,now);};
            h.complete_channeling=[&](auto event,bool success,auto now){return energy.complete_channeling(scope,event,success,now);};
            Policy policy;policy.primary=5;policy.secondary=3;policy.campaigns=255;policy.learned[38]=policy.learned[281]=1;
            Slots bar{};bar[0]={38,0};bar[1]={281,0};attributes::Ranks ranks{};ranks[3]=12;Runtime cast(1);
            check(cast.start(100,{38,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(1100,h);
            check(energy.resources(1)->energy==35);
            check(std::count(output.begin(),output.end(),generic(21,1,64))==1);
            check(cast.start(2000,{281,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(3000,h);
            check(energy.resources(1)->energy==31);
            check(cast.start(5000,{281,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.cancel(h);cast.update(6000,h);
            check(energy.resources(1)->energy==26);
            heal=false;check(cast.start(7000,{281,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(8000,h);
            check(energy.resources(1)->energy==21);
            heal=true;check(cast.start(9000,{281,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.update(10000,h);
            check(energy.resources(1)->energy==17);
            check(cast.start(12000,{281,0,1},bar,policy,ranks,true,h)==casting::Reason::accepted);cast.interrupt(12100,h);cast.update(13000,h);
            check(energy.resources(1)->energy==12);
        }
        {
            world::Instance channeling;channeling.configure_npcs(population.for_map(146));
            channeling.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            channeling.join(2,"Boundary",322,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            channeling.join(3,"Outside",323,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            channeling.join(4,"Ally",10,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            for(unsigned id:{1u,2u,3u,4u})channeling.set_vitals(id,{1000,1000,40,100},100);
            check(channeling.energy_capacity(1,5,0,100,{80,-2}));const auto scope=*channeling.effect_scope(1);
            check(channeling.status_mutation(scope,38,10,12,100,46100,true));
            check(channeling.prepare_channeling(scope,281,1,200)&&channeling.resources(1)->energy==40);
            check(channeling.complete_channeling(scope,1,false,200)&&channeling.resources(1)->energy==40);
            check(!channeling.complete_channeling(scope,1,true,200));
            check(channeling.prepare_channeling(scope,281,2,300));auto stale=scope;++stale.generation;
            check(!channeling.complete_channeling(stale,2,true,300));
            check(channeling.complete_channeling(scope,2,true,300)&&channeling.resources(1)->energy==43);
            check(!channeling.complete_channeling(scope,2,true,300));
            check(channeling.restore_energy(1,56));
            check(channeling.prepare_channeling(scope,281,3,400)&&channeling.complete_channeling(scope,3,true,400));
            check(channeling.resources(1)->energy==100);
            check(channeling.prepare_channeling(scope,281,4,500));
            check(channeling.energy_capacity(1,5,0,600,{80,0})&&channeling.consume_energy(1,10));
            check(!channeling.complete_channeling(scope,4,true,500));
            check(channeling.complete_channeling(scope,4,false,500)&&channeling.resources(1)->energy==90);
            check(channeling.prepare_channeling(scope,281,5,700)&&channeling.complete_channeling(scope,5,true,700));
            check(channeling.resources(1)->energy==93);
            check(channeling.energy_capacity(1,5,0,700,{80,-2})&&channeling.consume_energy(1,3));
            check(channeling.prepare_channeling(scope,281,6,46100)&&channeling.complete_channeling(scope,6,true,46100));
            check(channeling.resources(1)->energy==90);
            channeling.leave(1);check(!channeling.complete_channeling(scope,6,true,46100));
        }
        {
            world::Instance shackles;shackles.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            shackles.join(2,"Warrior",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            shackles.set_vitals(1,{1000,1000,100,100},100);shackles.set_vitals(2,{1000,1000,40,100},100);
            check(shackles.energy_capacity(2,1,0,100,{80,-2}));
            const auto a=*shackles.effect_scope(1),b=*shackles.effect_scope(2);
            using namespace casting;Hooks h;std::uint64_t at=100;
            h.events=[](const auto&){};h.resources=[&](auto id){return shackles.resources(id);};h.consume_energy=[&](auto n){return shackles.consume_energy(1,n);};
            h.target_scope=[&](auto id){return shackles.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return shackles.projectile_target_valid(a,id,range,at);};
            h.degeneration_hex=[&](auto target,const auto& plan,auto now){return shackles.degeneration_hex(a,target,plan,now,20);};
            Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[66]=1;Slots bar{};bar[0]={66,0};attributes::Ranks ranks{};ranks[3]=12;Runtime cast(1);
            check(cast.start(at,{66,0,2},bar,policy,ranks,true,h)==casting::Reason::accepted);at=3100;cast.update(at,h);
            check(shackles.resources(1)->energy==90);
            combat::weapon::Owned sword{7,2,{20,20,1},12,false,false,27};check(shackles.ordinary_weapon(b,sword,20)&&shackles.physical_armor(a,60));
            check(!shackles.ordinary_attack(b,999,3200)&&shackles.resources(2)->energy==40);
            check(shackles.ordinary_attack(b,1,3200)&&shackles.resources(2)->energy==35);shackles.cancel_ordinary_attack(b);
            check(shackles.ordinary_attack(b,1,3300)&&shackles.resources(2)->energy==30);shackles.cancel_ordinary_attack(b);
            check(shackles.consume_energy(2,28));
            check(shackles.ordinary_attack(b,1,3400)&&shackles.resources(2)->energy==0);shackles.cancel_ordinary_attack(b);
            check(shackles.ordinary_attack(b,1,3500)&&shackles.resources(2)->energy==0);shackles.cancel_ordinary_attack(b);
            check(shackles.restore_energy(2,10));
            check(shackles.ordinary_attack(b,1,20100)&&shackles.resources(2)->energy==10);shackles.cancel_ordinary_attack(b);
        }
        for(unsigned skill:{43u,47u})for(unsigned victim:{2u,100u})for(unsigned protection:{0u,16u,17u}){
            if((victim==100||skill==47)&&protection)continue;
            world::Instance hex;hex.configure_attack_miss_draws([]{return .5;});auto targets=population.for_map(146);for(auto& n:targets)n.combat_definition->spawn_health=combat::Health{1000,1000};
            hex.configure_npcs(targets);auto box=std::make_shared<world::Outbox>();hex.join(1,"Mesmer",0,0,box,0,{}, {},100,1);
            hex.join(2,"Warrior",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            hex.set_vitals(1,{1000,1000,100,100},100);hex.set_vitals(2,{1000,1000,100,100},100);
            const auto caster=*hex.effect_scope(1),target=*hex.effect_scope(victim);
            using namespace casting;Hooks h;std::uint64_t at=200;
            h.events=[](const auto&){};h.resources=[&](auto id){return hex.resources(id);};h.consume_energy=[&](auto cost){return hex.consume_energy(1,cost);};
            h.target_scope=[&](auto id){return hex.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return hex.projectile_target_valid(caster,id,range,at);};
            h.degeneration_hex=[&](auto receiver,const auto& plan,auto now){return hex.degeneration_hex(caster,receiver,plan,now,20);};
            Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[1]=12;Runtime cast(1);
            check(cast.start(at,{skill,0,victim},bar,policy,ranks,true,h)==casting::Reason::accepted);at=skill==43?2200:1200;cast.update(at,h);
            const unsigned damage=skill==43?76:114;check(hex.resources(1)->energy==90);
            if(victim==2){
                combat::weapon::Owned axe{7,2,{20,20,1},12,false,false,2};check(hex.ordinary_weapon(target,axe,20)&&hex.physical_armor(caster,60));
                if(protection)check(hex.status_mutation(target,protection,99,12,at,at+30000,true));
                check(!hex.ordinary_attack(target,999,at+1)&&hex.resources(2)->health==1000);
                check(hex.ordinary_attack(target,1,at+100));check(hex.resources(2)->health==1000-damage);
                check(hex.has_condition(target,Kind::blind,at+100)==(skill==47));
                if(skill==43)check(*hex.control_epoch(target)==1);
                if(protection==16){const auto state=hex.status_snapshot(target,at+100);check(state&&std::none_of(state->begin(),state->end(),[](const auto& e){return e.skill==16;}));}
                if(protection==17)check(hex.resources(2)->energy==95);
                hex.tick(at+1100);check(hex.resources(1)->health==1000);
            }else{
                const auto action=hex.begin_npc_action(target,combat::npcs::control::Action::attack,at+100,at+600);check(bool(action));
                check(hex.npc_combat_snapshot(target)->health->current==1000-damage);
                check(hex.complete_npc_action(*action,at+600)==(skill==47));
            }
        }
        for(bool pvp:{false,true})for(unsigned victim:{2u,100u}){
            world::Instance frag;auto targets=population.for_map(146);
            for(auto& npc:targets){npc.combat_definition->spawn_health=combat::Health{1000,1000};if(npc.id==101)npc.x=100;}
            frag.configure_npcs(targets);auto box=std::make_shared<world::Outbox>();
            frag.join(1,"Mesmer",0,0,box,0,{}, {},100,1);
            frag.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            frag.join(3,"Colocated",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            for(unsigned id:{1u,2u,3u})frag.set_vitals(id,{1000,1000,100,100},100);
            const auto caster=*frag.effect_scope(1),player=*frag.effect_scope(2),npc=*frag.effect_scope(100);
            using namespace casting;Hooks h;std::uint64_t at=200;std::vector<Packet> output;
            h.events=[&](const auto& packets){output.insert(output.end(),packets.begin(),packets.end());};
            h.resources=[&](auto id){return frag.resources(id);};h.consume_energy=[&](auto cost){return frag.consume_energy(1,cost);};
            h.target_scope=[&](auto id){return frag.effect_scope(id);};
            h.projectile_target_valid=[&](auto id,float range){return frag.projectile_target_valid(caster,id,range,at);};
            h.degeneration_hex=[&](auto target,const auto& plan,auto now){return frag.degeneration_hex(caster,target,plan,now,20);};
            Policy policy;policy.primary=5;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[19]=1;
            Slots bar{};bar[0]={19,0};attributes::Ranks ranks{};ranks[1]=12;Runtime cast(1);
            check(cast.start(200,{19,0,victim},bar,policy,ranks,true,h)==casting::Reason::accepted);
            at=1200;cast.update(at,h);check(frag.resources(1)->energy==95);
            check(std::find(output.begin(),output.end(),slot_event(228,1,{19,0},pvp?8:5))!=output.end());
            const bool player_hexed=!pvp||victim==2,npc_hexed=!pvp||victim==100;
            const auto colocated=frag.status_snapshot(*frag.effect_scope(3),at);
            check(colocated&&effects::active_control_hex(*colocated,pvp?2998:19,at)==!pvp);
            check(bool(frag.npc_combat_snapshot(*frag.effect_scope(101))->effects&0x800)==!pvp);
            check(frag.apply_condition(caster,player,Kind::blind,1300,1000));
            check(frag.resources(2)->health==(player_hexed?983u:1000u));
            check(frag.apply_condition(caster,caster,Kind::blind,1300,1000));
            check(frag.condition_transfer(caster,npc,*effects::condition_transfer(154,12),1400));
            check(frag.npc_combat_snapshot(npc)->health->current==(npc_hexed?983:1000));
            check(frag.apply_condition(caster,player,Kind::blind,1500,1000));
            check(frag.resources(2)->health==(player_hexed?983u:1000u));
            frag.tick(2600);
            check(frag.resources(2)->health==(player_hexed?966u:1000u));
            check(frag.npc_combat_snapshot(npc)->health->current==(npc_hexed?966:1000));
            frag.tick(2700);check(frag.resources(2)->health==(player_hexed?966u:1000u));
        }
        for(bool reconnect:{false,true}){
            world::Instance frag;frag.configure_npcs(population.for_map(146));auto box=std::make_shared<world::Outbox>();
            frag.join(1,"Mesmer",0,0,box,0,{}, {},100,1);frag.set_vitals(1,{1000,1000,100,100},100);
            const auto caster=*frag.effect_scope(1),target=*frag.effect_scope(100);
            check(frag.degeneration_hex(caster,target,*effects::degeneration_hex(2998,12),100,20));
            auto before=*frag.npc_combat_snapshot(target);
            const auto expected=combat::apply(before,{combat::Kind::armor_ignoring,17,20,2998},true);
            const auto packets=combat::damage_packets_2022(1,expected);
            check(frag.apply_condition(caster,caster,Kind::blind,110,1000));
            check(frag.condition_transfer(caster,target,*effects::condition_transfer(154,12),200));
            check(frag.npc_combat_snapshot(target)->health->current==83);
            if(reconnect){
                frag.leave(1);frag.join(1,"Replacement",0,0,box,0,{}, {},210,1);
                frag.set_vitals(1,{1000,1000,100,100},100);check(*frag.effect_scope(1)!=caster);
            }
            box->drain();frag.tick(300);const auto output=box->drain();
            check(std::count(output.begin(),output.end(),packets[0])==(reconnect?0:1));
            check(std::count(output.begin(),output.end(),packets[1])==(reconnect?0:1));
            frag.tick(400);const auto repeated=box->drain();
            check(std::count(repeated.begin(),repeated.end(),packets[1])==0);
        }
        for(unsigned skill:{35u,56u})for(unsigned victim:{2u,100u})for(bool remove:{false,true}){
            world::Instance controls;controls.configure_npcs(population.for_map(146));auto box=std::make_shared<world::Outbox>();
            controls.join(1,"Mesmer",0,0,box,0,{}, {},100,1);
            controls.join(2,"Warrior",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            controls.join(3,"Boundary",266,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            controls.join(4,"Outside",267,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            for(unsigned id:{1u,2u,3u,4u})controls.set_vitals(id,{1000,1000,100,100},100);
            const auto caster=*controls.effect_scope(1),warrior=*controls.effect_scope(2),target=*controls.effect_scope(victim);
            Slots adrenaline_bar{};adrenaline_bar[0]={335,0};
            check(controls.configure_adrenaline(warrior,adrenaline_bar,100));
            combat::weapon::Owned axe{7,2,{20,20,1},12,false,false,2};
            check(controls.ordinary_weapon(warrior,axe,20)&&controls.physical_armor(caster,60));
            check(controls.melee_attack(warrior,caster,axe,*combat::weapon::attack_skill(322,12),110));
            check(controls.adrenaline(warrior)->at(0).units==25);
            using namespace casting;Hooks h;std::uint64_t at=200;std::vector<Packet> cast_packets;
            h.events=[&](const auto& packets){cast_packets.insert(cast_packets.end(),packets.begin(),packets.end());};h.resources=[&](auto id){return controls.resources(id);};
            h.consume_energy=[&](auto cost){return controls.consume_energy(1,cost);};
            h.target_scope=[&](auto id){return controls.effect_scope(id);};
            h.projectile_target_valid=[&](auto id,float range){return controls.projectile_target_valid(caster,id,range,at);};
            h.degeneration_hex=[&](auto receiver,const auto& plan,auto now){return controls.degeneration_hex(caster,receiver,plan,now);};
            Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[skill]=1;
            Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[1]=ranks[2]=12;Runtime cast(1);
            check(cast.start(200,{skill,0,victim},bar,policy,ranks,true,h)==casting::Reason::accepted);
            at=skill==35?1200:2200;cast.update(at,h);check(controls.resources(1)->energy==85);
            if(victim==100)check(controls.npc_combat_snapshot(target)->effects&0x800);
            if(skill==35)check(std::count(cast_packets.begin(),cast_packets.end(),generic(21,1,60))==1);
            if(skill==56){
                const auto boundary=controls.status_snapshot(*controls.effect_scope(3),at);
                const auto outside=controls.status_snapshot(*controls.effect_scope(4),at);
                check(boundary&&effects::adrenaline_forbidden(*boundary,at));
                check(outside&&!effects::adrenaline_forbidden(*outside,at));
            }
            if(skill==35&&victim==100)continue;
            const auto expiry=at+18000;
            if(skill==35){
                Hooks receiver;unsigned costs=0;
                receiver.events=[](const auto&){};receiver.resources=[&](auto id){return controls.resources(id);};
                receiver.effect_snapshot=[&](auto now){return controls.status_snapshot(warrior,now);};
                receiver.target_valid=[](auto id,float,bool){return id==2;};
                receiver.consume_energy=[&](auto){++costs;return true;};
                receiver.heal=[](auto,auto,float,bool){return true;};
                receiver.healing_signet_activity=[](auto,auto){return true;};
                Policy healing;healing.primary=1;healing.campaigns=255;healing.learned[1]=1;
                Slots signet_bar{};signet_bar[0]={1,0};Runtime signet(2);
                check(signet.start(at,{1,0,2},signet_bar,healing,ranks,true,receiver)==casting::Reason::policy&&costs==0);
                const auto retry=remove?at+100:expiry;
                if(remove)check(controls.shatter_delusions(caster,warrior,*effects::shatter_delusions(0),20,retry));
                check(signet.start(retry,{1,0,2},signet_bar,healing,ranks,true,receiver)==casting::Reason::accepted);
            }else{
                // Existing charge survives application. Neither outgoing hits
                // nor incoming damage can add charge while the hex is active.
                check(controls.adrenaline(warrior)->at(0).units==25);
                check(controls.melee_attack(warrior,caster,axe,*combat::weapon::attack_skill(322,12),at+1));
                check(controls.adrenaline(warrior)->at(0).units==25);
                auto other_axe=axe;other_axe.item=8;
                check(controls.ordinary_weapon(caster,other_axe,20)&&controls.physical_armor(warrior,60));
                check(controls.melee_attack(caster,warrior,other_axe,*combat::weapon::attack_skill(322,12),at+2));
                check(controls.adrenaline(warrior)->at(0).units==25);
                const auto retry=remove?at+100:expiry;
                if(remove)check(controls.shatter_delusions(caster,warrior,*effects::shatter_delusions(0),20,retry));
                const auto before_retry=controls.adrenaline(warrior)->at(0).units;
                check(controls.melee_attack(warrior,caster,axe,*combat::weapon::attack_skill(322,12),retry));
                check(controls.adrenaline(warrior)->at(0).units==std::min(80u,before_retry+25));
            }
        }
        for(unsigned victim:{2u,100u})for(bool source_dies:{false,true})for(unsigned ending:{0u,1u,2u,3u}){
            if(source_dies&&ending)continue;
            world::Instance conundrum;conundrum.configure_npcs(population.for_map(146));
            auto box=std::make_shared<world::Outbox>();
            conundrum.join(1,"Mesmer",0,0,box,0,{}, {},100,1);
            conundrum.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            conundrum.join(3,"Boundary",266,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            conundrum.join(4,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            for(unsigned id:{1u,2u,3u,4u})conundrum.set_vitals(id,{1000,1000,40,100},100);
            check(conundrum.energy_capacity(1,5,0,100,{80,-2}));
            const auto caster=*conundrum.effect_scope(1),target=*conundrum.effect_scope(victim);
            using namespace casting;Hooks h;std::uint64_t at=100;
            h.events=[](const auto&){};h.resources=[&](auto id){return conundrum.resources(id);};
            h.consume_energy=[&](auto cost){return conundrum.consume_energy(1,cost);};
            h.target_scope=[&](auto id){return conundrum.effect_scope(id);};
            h.projectile_target_valid=[&](auto id,float range){return conundrum.projectile_target_valid(caster,id,range,at);};
            h.activation_multiplier=[&](auto id,auto now){return conundrum.activation_multiplier(caster,id,now);};
            h.degeneration_hex=[&](auto receiver,const auto& plan,auto now){return conundrum.degeneration_hex(caster,receiver,plan,now);};
            Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[36]=1;
            Slots bar{};bar[0]={36,0};attributes::Ranks ranks{};ranks[1]=12;Runtime cast(1);
            check(cast.start(100,{36,0,victim},bar,policy,ranks,true,h)==casting::Reason::accepted);
            at=2100;cast.update(at,h);check(conundrum.resources(1)->energy==30);
            for(unsigned id:{2u,100u,101u}){
                const auto scope=*conundrum.effect_scope(id);
                check(conundrum.activation_multiplier(scope,281,2100)==2);
                check(conundrum.activation_multiplier(scope,132,2100)==1);
            }
            check(conundrum.activation_multiplier(*conundrum.effect_scope(4),281,2100)==1);
            check(conundrum.activation_multiplier(*conundrum.effect_scope(102),281,2100)==1);
            // The adjacent circle follows the selected target, not the caster.
            const unsigned recipients=4;
            check(conundrum.activation_multiplier(*conundrum.effect_scope(3),281,2100)==2);
            if(source_dies)check(conundrum.kill(caster,3000));
            if(ending==1){
                check(conundrum.shatter_delusions(caster,target,*effects::shatter_delusions(0),20,3100));
                check(conundrum.resources(1)->energy==36);
            }
            if(ending==2)check(conundrum.degeneration_hex(caster,target,*effects::degeneration_hex(36,12),3100));
            if(ending==3){
                conundrum.leave(1);
                conundrum.join(1,"Replacement",0,0,box,0,{}, {},3100,1);
                conundrum.set_vitals(1,{1000,1000,40,100},100);
                check(conundrum.energy_capacity(1,5,0,3100,{80,-2}));
                check(*conundrum.effect_scope(1)!=caster);
                conundrum.tick(15100);check(conundrum.resources(1)->energy==40);
                continue;
            }
            conundrum.tick(15000);check(conundrum.resources(1)->energy==(ending==1?36u:30u));
            conundrum.tick(15100);
            if(ending==2){check(conundrum.resources(1)->energy==30);conundrum.tick(16100);}
            check(conundrum.resources(1)->energy==30+6*recipients);
            conundrum.tick(16200);check(conundrum.resources(1)->energy==30+6*recipients);
            check(conundrum.activation_multiplier(target,281,16200)==1);
        }
        for(bool pvp:{false,true})for(unsigned victim:{2u,100u})for(bool dazed:{false,true})for(bool remove:{false,true}){
            world::Instance migraine;auto targets=population.for_map(146);for(auto& npc:targets)npc.combat_definition->spawn_health=combat::Health{1000,1000};
            migraine.configure_npcs(targets);auto box=std::make_shared<world::Outbox>();migraine.join(1,"Mesmer",0,0,box,0,{}, {},100,1);
            migraine.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            migraine.set_vitals(1,{1000,1000,40,40},100);migraine.set_vitals(2,{1000,1000,40,40},100);
            const auto a=*migraine.effect_scope(1),b=*migraine.effect_scope(victim);
            if(dazed){check(migraine.apply_condition(a,a,Kind::dazed,100,60000));check(migraine.condition_transfer(a,b,*effects::condition_transfer(132,12),100));}
            using namespace casting;Hooks h;std::uint64_t at=100;
            h.events=[](const auto&){};h.resources=[&](auto id){return migraine.resources(id);};h.consume_energy=[&](auto amount){return migraine.consume_energy(1,amount);};
            h.target_scope=[&](auto id){return migraine.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return migraine.projectile_target_valid(a,id,range,at);};
            h.activation_multiplier=[&](auto id,auto now){return migraine.activation_multiplier(a,id,now);};
            h.degeneration_hex=[&](auto target,const auto& plan,auto now){return migraine.degeneration_hex(a,target,plan,now);};
            Policy policy;policy.primary=5;policy.campaigns=255;policy.pvp_match=pvp;policy.learned[53]=1;
            Slots bar{};bar[0]={53,0};attributes::Ranks ranks{};ranks[1]=12;Runtime cast(1);
            check(cast.start(100,{53,0,victim},bar,policy,ranks,true,h)==casting::Reason::accepted);at=2100;cast.update(at,h);
            check(migraine.resources(1)->energy==30);
            check(migraine.activation_multiplier(b,281,2100)==(dazed?2.5:2.)&&migraine.activation_multiplier(b,132,2100)==2.);
            check(migraine.activation_multiplier(b,154,2100)==2.&&migraine.activation_multiplier(b,397,2100)==1.&&migraine.attack_duration(b,1000,2100)==1000);
            auto published=box->drain();check(std::count(published.begin(),published.end(),*particles::targeted_body_2022(victim,1,pvp?3183:53))==1);
            std::uint64_t finished=3100;
            if(victim==2){
                Hooks receiver;unsigned heals=0;std::vector<Packet> output;
                receiver.events=[&](const auto& packets){output.insert(output.end(),packets.begin(),packets.end());};receiver.resources=[&](auto id){return migraine.resources(id);};
                receiver.consume_energy=[&](auto amount){return migraine.consume_energy(2,amount);};receiver.target_scope=[&](auto id){return migraine.effect_scope(id);};receiver.target_valid=[](auto id,float,bool){return id==2;};
                receiver.activation_multiplier=[&](auto id,auto now){return migraine.activation_multiplier(b,id,now);};
                receiver.heal_scoped=[&](auto target,unsigned amount,float range,bool self,auto now){++heals;return migraine.heal_target_scoped(b,target,amount,range,self,now);};
                Policy monk;monk.primary=3;monk.campaigns=255;monk.learned[281]=1;Slots healing{};healing[0]={281,0};attributes::Ranks healing_rank{};healing_rank[13]=12;Runtime heal(2);
                check(heal.start(2200,{281,0,2},healing,monk,healing_rank,true,receiver)==casting::Reason::accepted);
                finished=2200+(dazed?2500:2000);heal.update(finished-1,receiver);check(!heals);heal.update(finished,receiver);check(heals==1);
                Packet duration{161,0};dword(duration,61);dword(duration,2);dword(duration,std::bit_cast<std::uint32_t>(float(dazed?2.5:2.)));
                check(std::find(output.begin(),output.end(),duration)!=output.end());
            }else {migraine.tick(3100);check(migraine.npc_combat_snapshot(b)->health->current==986);}
            if(remove){check(migraine.shatter_delusions(a,b,*effects::shatter_delusions(0),20,finished));}
            const auto before=victim==2?double(migraine.resources(2)->health):migraine.npc_combat_snapshot(b)->health->current;
            migraine.tick(pvp?13100:20100);
            check(migraine.activation_multiplier(b,281,pvp?13100:20100)==(dazed?2.:1.)&&migraine.activation_multiplier(b,132,pvp?13100:20100)==1.);
            const auto after=victim==2?double(migraine.resources(2)->health):migraine.npc_combat_snapshot(b)->health->current;
            if(remove)check(after==before);else if(victim==100)check(after==(pvp?860:762));
        }
        {
            auto definition=*population.for_map(146).front().combat_definition;definition.spawn_health=combat::Health{50,1000};
            combat::npcs::LiveActor npc(definition,{1,1,1,146,100});const auto life=npc.lifetime();
            check(bool(npc.apply_single_degeneration_hex(life,*effects::degeneration_hex(129,12),1,1,100))&&npc.apply_condition(life,Kind::deep_wound,1,100,60000));
            const auto token=npc.begin_action(life,combat::npcs::control::Action::spell,100,1100);check(bool(token));
            check(npc.heal(life,50,100)&&npc.actor(life)->health->current==0&&(npc.actor(life)->effects&0x10));
            check(!npc.complete_action(life,*token,1100)); // Healing settles pending lethal wound debt and invalidates the action.
        }
        for(unsigned skill:{129u,142u})for(unsigned primary:{2u,100u})for(bool wounded:{false,true}){
            world::Instance reduction;auto targets=population.for_map(146);for(auto& npc:targets)npc.combat_definition->spawn_health=combat::Health{1000,1000};
            reduction.configure_npcs(targets);auto box=std::make_shared<world::Outbox>();reduction.join(1,"Necromancer",0,0,box,0,{}, {},100,1);
            reduction.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            reduction.set_vitals(1,{400,1000,40,40},100);reduction.set_vitals(2,{500,1000,40,40},100);
            const auto a=*reduction.effect_scope(1),b=*reduction.effect_scope(primary),self=*reduction.effect_scope(2);
            if(wounded)check(reduction.apply_condition(a,self,Kind::deep_wound,100,60000));
            const auto plan=*effects::degeneration_hex(skill,12);check(plan.duration==(skill==129?29000u:25000u));
            using namespace casting;Hooks h;std::uint64_t at=100;
            h.events=[](const auto&){};h.resources=[&](auto id){return reduction.resources(id);};h.consume_energy=[&](auto amount){return reduction.consume_energy(1,amount);};
            h.target_scope=[&](auto id){return reduction.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return reduction.projectile_target_valid(a,id,range,at);};
            h.health_sacrifice=[&](auto id,auto now){return reduction.health_sacrifice(a,id,now);};h.degeneration_hex=[&](auto target,const auto& hit,auto now){return reduction.degeneration_hex(a,target,hit,now);};
            Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[7]=12;Runtime cast(1);
            check(cast.start(100,{skill,0,primary},bar,policy,ranks,true,h)==casting::Reason::accepted);at=1100;cast.update(at,h);
            check(reduction.resources(1)->health==(skill==129?300u:400u)&&reduction.resources(1)->energy==(skill==129?30u:25u));
            const auto published=box->drain();check(std::count(published.begin(),published.end(),*particles::targeted_body_2022(primary,1,skill))==1);
            check(reduction.heal_target_scoped(self,self,300,0,true,1100));
            const bool affected=skill==142||primary==2;
            const unsigned expected=wounded?(affected?(skill==129?580u:592u):640u):(affected?(skill==129?698u:740u):800u);
            check(reduction.resources(2)->health==expected);
            const auto gain=reduction.resource_gain(self,a,*effects::resource_gain(1365,0),1100);check(gain&&gain->health_delta==10);
            check(reduction.ally_cleansing(self,self,*effects::ally_cleansing(295),1100));const auto before=reduction.resources(2)->health;
            check(reduction.heal_target_scoped(self,self,100,0,true,1101)&&reduction.resources(2)->health==before+100);
            reduction.tick(2100);check(reduction.npc_combat_snapshot(*reduction.effect_scope(100))->health->current==(skill==142?996:1000));
        }
        for(bool defile:{false,true})for(bool lingering:{false,true})for(bool wounded:{false,true})for(bool cleanse:{false,true}){
            auto definition=*population.for_map(146).front().combat_definition;definition.spawn_health=combat::Health{500,1000};
            combat::npcs::LiveActor npc(definition,{1,1,1,146,100});const auto life=npc.lifetime();
            if(defile)check(bool(npc.apply_single_degeneration_hex(life,*effects::degeneration_hex(129,12),1,1,100)));
            if(lingering)check(bool(npc.apply_single_degeneration_hex(life,*effects::degeneration_hex(142,12),1,2,100)));
            if(wounded)check(npc.apply_condition(life,Kind::deep_wound,1,100,60000));
            check(npc.heal(life,100,1100));
            const double before=500-(wounded?100:0)-(lingering?4:0);
            const double factor=std::max(.6,(defile?.66:1.)*(lingering?.8:1.)*(wounded?.8:1.));
            check(std::abs(npc.actor(life)->health->current-(before+100*factor))<1e-7);
            if(cleanse){if(defile)check(npc.remove_hex_effect(life,1,1100));if(lingering)check(npc.remove_hex_effect(life,2,1100));if(wounded)check(npc.remove_condition(life,Kind::deep_wound,1100));}
            npc.advance_conditions(life,30100);const auto prior=npc.actor(life)->health->current;
            check(npc.heal(life,100,30100)&&std::abs(npc.actor(life)->health->current-prior-(wounded&&!cleanse?80:100))<1e-7);
        }
        {
            world::Instance authority;authority.configure_npcs(population.for_map(146));authority.join(1,"Caster",0,0,std::make_shared<world::Outbox>());
            authority.set_vitals(1,{1,1000,40,40},100);const auto a=*authority.effect_scope(1),b=*authority.effect_scope(100);
            check(authority.apply_condition(a,a,Kind::poison,100,10000));const auto plan=*effects::condition_transfer(132,0);
            check(!authority.condition_transfer(a,b,plan,1100)&&authority.resources(1)->health==1); // Missing trusted roll is rejected.
            check(!authority.condition_transfer(a,b,plan,1100,.25)&&authority.resources(1)->health==0); // Failed cast still settles caster death.
            check(!authority.has_condition(b,Kind::poison,1100));
        }
        for(unsigned victim:{2u,100u}){
            world::Instance all;auto targets=population.for_map(146);for(auto& npc:targets)npc.combat_definition->spawn_health=combat::Health{1000,1000};
            all.configure_npcs(targets);all.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            all.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);all.set_vitals(1,{1000,1000,40,40},100);all.set_vitals(2,{1000,1000,40,40},100);
            const auto a=*all.effect_scope(1),b=*all.effect_scope(victim);
            for(unsigned i=0;i<10;++i)check(all.apply_condition(a,a,static_cast<Kind>(i),100,10000));
            check(all.resources(1)->max_health==900&&all.condition_transfer(a,b,*effects::condition_transfer(132,12),100));
            for(unsigned i=0;i<10;++i)check(!all.has_condition(a,static_cast<Kind>(i),100)&&all.has_condition(b,static_cast<Kind>(i),100));
            check(all.resources(1)->max_health==1000);
            if(victim==2)check(all.resources(2)->max_health==900);else check(all.npc_combat_snapshot(b)->health->maximum==900);
            all.tick(19100);for(unsigned i=0;i<10;++i)check(!all.has_condition(b,static_cast<Kind>(i),19100));
            if(victim==2)check(all.resources(2)->max_health==1000&&all.resources(2)->health==640);
            else check(all.npc_combat_snapshot(b)->health->maximum==1000&&all.npc_combat_snapshot(b)->health->current==640);
        }
        {
            const auto low=*effects::condition_transfer(132,4),high=*effects::condition_transfer(132,5);
            check(effects::condition_transfer_failed(low,.499999)==true&&effects::condition_transfer_failed(low,.5)==false);
            check(effects::condition_transfer_failed(high,.1)==false&&!effects::condition_transfer_failed(low,-.1)&&!effects::condition_transfer_failed(low,1));
        }
        for(unsigned skill:{132u,154u})for(unsigned victim:{2u,100u})for(unsigned rank:{0u,4u,5u,12u})for(double roll:{.25,.75}){
            world::Instance transfer;auto targets=population.for_map(146);for(auto& npc:targets)npc.combat_definition->spawn_health=combat::Health{1000,1000};
            transfer.configure_npcs(targets);auto box=std::make_shared<world::Outbox>();transfer.join(1,"Necromancer",0,0,box,0,{}, {},100,1);
            transfer.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            transfer.set_vitals(1,{1000,1000,40,40},100);transfer.set_vitals(2,{1000,1000,40,40},100);
            const auto a=*transfer.effect_scope(1),b=*transfer.effect_scope(victim);
            check(transfer.apply_condition(a,a,Kind::crippled,100,20000));
            check(transfer.condition_transfer(a,b,*effects::condition_transfer(154,0),100));
            check(transfer.apply_condition(a,a,Kind::poison,100,10000)&&transfer.apply_condition(a,a,Kind::crippled,100,6000)&&
                  transfer.apply_condition(a,a,Kind::weakness,100,7000)&&transfer.apply_condition(a,a,Kind::blind,100,8000));
            box->drain();const auto plan=*effects::condition_transfer(skill,rank);auto forged=plan;++forged.duration_percent;
            check(!transfer.condition_transfer(a,b,forged,100));auto stale=b;++stale.generation;check(!transfer.condition_transfer(a,stale,plan,100));
            using namespace casting;Hooks h;std::uint64_t at=100;unsigned rolls=0,commits=0,failures=0;std::vector<Packet> sent;
            h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](auto id){return transfer.resources(id);};
            h.consume_energy=[&](auto amount){return transfer.consume_energy(1,amount);};h.target_scope=[&](auto id){return transfer.effect_scope(id);};
            h.projectile_target_valid=[&](auto id,float range){return transfer.projectile_target_valid(a,id,range,at);};
            h.skill_roll=[&]()->std::optional<double>{++rolls;return roll;};h.diagnostic=[&](auto,const char* state){failures+=std::string_view(state)=="attribute_failure";};
            h.condition_transfer=[&](auto target,const auto& hit,auto now,std::optional<double> value){++commits;return transfer.condition_transfer(a,target,hit,now,value);};
            Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[skill]=1;Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};ranks[7]=rank;Runtime cast(1);
            if(skill==132&&rank<=4){auto no_roll=h;no_roll.skill_roll={};Runtime unavailable(1);
                check(unavailable.start(100,{skill,0,victim},bar,policy,ranks,true,no_roll)==casting::Reason::unsupported);}
            check(cast.start(100,{skill,0,victim},bar,policy,ranks,true,h)==casting::Reason::accepted);
            const bool failed=skill==132&&rank<=4&&roll<.5;at=skill==132?1100:850;cast.update(at,h);
            check(rolls==(skill==132&&rank<=4?1u:0u)&&commits==1&&failures==unsigned(failed));
            check(transfer.resources(1)->energy==(skill==132?40u:35u));
            check(transfer.has_condition(a,Kind::blind,at)==failed&&transfer.has_condition(b,Kind::blind,at)==!failed);
            check(std::count(sent.begin(),sent.end(),slot_event(228,1,{skill,0},skill==132?4:0))==1);
            const auto published=box->drain();check(std::count(published.begin(),published.end(),*particles::targeted_body_2022(victim,1,skill))==(failed?0:1));
            check(transfer.has_condition(b,Kind::crippled,10000)); // Longer pre-existing timer is retained.
            if(!failed){
                const unsigned percent=skill==154?100:rank==0?100:rank==4?127:rank==5?133:180;
                const auto end=at+((8100-at)*percent+50)/100;
                check(transfer.has_condition(b,Kind::blind,end-1)&&!transfer.has_condition(b,Kind::blind,end));
            }
            check(transfer.has_condition(a,Kind::poison,at)==(failed||skill==154));
            const bool kept_cripple=failed||(skill==154&&rank<12);
            transfer.move(1,1000,0,at);transfer.tick(at+1000);check(std::abs(transfer.position(1,at+1000).first-(kept_cripple?144.f:288.f))<.3);
        }
        for(unsigned primary:{2u,100u})for(bool renew:{false,true})for(bool cleanse:{false,true}){
            world::Instance suffering;auto targets=population.for_map(146);
            targets[1].x=352;targets[2].x=353;
            for(auto& npc:targets)npc.combat_definition->spawn_health=combat::Health{1000,1000};
            auto neutral=targets[0];neutral.id=103;neutral.combat_definition->binding.actor=103;neutral.combat_definition->relation=combat::npcs::Relation::neutral;targets.push_back(neutral);
            auto layered=targets[0];layered.id=104;layered.plane=1;layered.combat_definition->binding.actor=104;targets.push_back(layered);
            suffering.configure_npcs(targets);auto box=std::make_shared<world::Outbox>();
            for(unsigned id=1;id<=5;++id){
                const float x=id==1?0:id==3?352:id==4?353:100;
                suffering.join(id,"Actor",x,0,id==1?box:std::make_shared<world::Outbox>(),0,{}, {},100,id==1||id==5?1:2);
                suffering.set_vitals(id,{1000,1000,40,40},100);
            }
            const auto a=*suffering.effect_scope(1),b=*suffering.effect_scope(primary);const auto plan=*effects::degeneration_hex(108,12);
            auto stale=b;++stale.generation;check(!suffering.degeneration_hex(a,stale,plan,100));
            check(!suffering.degeneration_hex(a,*suffering.effect_scope(5),plan,100));
            check(!suffering.degeneration_hex(a,*suffering.effect_scope(103),plan,100));
            check(!suffering.degeneration_hex(a,*suffering.effect_scope(104),plan,100));
            auto forged=plan;++forged.radius;check(!suffering.degeneration_hex(a,b,forged,100));
            using namespace casting;Hooks h;std::uint64_t at=100;
            h.events=[](const auto&){};h.resources=[&](auto id){return suffering.resources(id);};h.consume_energy=[&](auto amount){return suffering.consume_energy(1,amount);};
            h.target_scope=[&](auto id){return suffering.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return suffering.projectile_target_valid(a,id,range,at);};
            h.degeneration_hex=[&](auto target,const auto& hit,auto now){return suffering.degeneration_hex(a,target,hit,now);};
            Policy policy;policy.primary=4;policy.campaigns=255;policy.learned[108]=1;Slots bar{};bar[0]={108,0};attributes::Ranks ranks{};ranks[7]=12;Runtime cast(1);
            check(cast.start(100,{108,0,primary},bar,policy,ranks,true,h)==casting::Reason::accepted);
            at=1100;cast.update(at,h);check(suffering.resources(1)->energy==25);
            const auto published=box->drain();for(unsigned id:{2u,3u,100u,101u})
                check(std::count(published.begin(),published.end(),*particles::targeted_body_2022(id,1,108))==1);
            suffering.tick(2100);for(unsigned id:{2u,3u})check(suffering.resources(id)->health==996);
            for(unsigned id:{100u,101u})check(suffering.npc_combat_snapshot(*suffering.effect_scope(id))->health->current==996);
            for(unsigned id:{1u,4u,5u})check(suffering.resources(id)->health==1000);
            for(unsigned id:{102u,103u,104u})check(suffering.npc_combat_snapshot(*suffering.effect_scope(id))->health->current==1000);
            if(renew)check(suffering.degeneration_hex(a,b,plan,2100));
            if(cleanse){const auto self=*suffering.effect_scope(2);check(suffering.ally_cleansing(self,self,*effects::ally_cleansing(295),2100));}
            suffering.tick(renew?28100:27100);
            check(suffering.resources(2)->health==(cleanse?996u:renew?896u:900u));
            check(suffering.resources(3)->health==(renew?896u:900u));
            for(unsigned id:{100u,101u})check(suffering.npc_combat_snapshot(*suffering.effect_scope(id))->health->current==(renew?896:900));
        }
        {
            auto definition=*population.for_map(146).front().combat_definition;definition.spawn_health=combat::Health{1000,1000};
            combat::npcs::LiveActor npc(definition,{1,1,1,146,100});const auto life=npc.lifetime();
            check(bool(npc.apply_single_degeneration_hex(life,*effects::degeneration_hex(108,12),1,7,100)));
            check(npc.remove_hex_effect(life,7,1100)&&npc.actor(life)->health->current==996);
            npc.advance_conditions(life,2100);check(npc.actor(life)->health->current==996);
            check(bool(npc.apply_single_degeneration_hex(life,*effects::degeneration_hex(108,0),1,8,2100)));
            npc.advance_conditions(life,8100);check(npc.actor(life)->health->current==996);
        }
        {
            world::Instance pve;auto targets=population.for_map(146);for(auto& npc:targets)npc.combat_definition->spawn_health=combat::Health{1000,1000};
            pve.configure_npcs(targets);pve.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>());pve.set_vitals(1,{1000,1000,40,40},100);
            const auto a=*pve.effect_scope(1),b=*pve.effect_scope(100);
            check(pve.degeneration_hex(a,b,*effects::degeneration_hex(108,12),100));pve.tick(1100);
            check(pve.npc_combat_snapshot(b)->health->current==996); // NPC hostility needs no PvP team assignment.
        }
        for(bool pvp:{false,true})for(unsigned victim:{2u,100u})for(bool cleanse:{false,true}){
            world::Instance anguish;auto targets=population.for_map(146);
            for(auto& npc:targets)npc.combat_definition->spawn_health=combat::Health{1000,1000};
            auto box=std::make_shared<world::Outbox>();anguish.configure_npcs(targets);
            anguish.join(1,"Mesmer",0,0,box,0,{}, {},100,1);anguish.join(2,"Enemy",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            anguish.set_vitals(1,{1000,1000,40,40},100);anguish.set_vitals(2,{1000,1000,40,40},100);
            const auto a=*anguish.effect_scope(1),b=*anguish.effect_scope(victim);
            const auto plan=*effects::movement_hex(54,12,pvp);
            check(plan.skill==(pvp?3152u:54u)&&plan.pips==(pvp?4u:7u)&&plan.duration==17000);
            auto forged=plan;++forged.pips;check(!anguish.movement_hex(a,b,forged,100));
            using namespace casting;Hooks h;std::vector<Packet> sent;std::uint64_t at=100;
            h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};
            h.resources=[&](auto id){return anguish.resources(id);};h.consume_energy=[&](auto amount){return anguish.consume_energy(1,amount);};
            h.target_scope=[&](auto id){return anguish.effect_scope(id);};h.projectile_target_valid=[&](auto id,float range){return anguish.projectile_target_valid(a,id,range,at);};
            h.movement_hex=[&](auto target,const auto& hit,auto now){check(hit==plan);return anguish.movement_hex(a,target,hit,now);};
            Policy policy;policy.primary=5;policy.campaigns=255;policy.learned[54]=1;policy.pvp_match=pvp;
            Slots bar{};bar[0]={54,0};attributes::Ranks ranks{};ranks[1]=12;Runtime cast(1);
            {
                auto interrupted_hooks=h;unsigned spent=0;std::vector<Packet> output;
                interrupted_hooks.consume_energy=[&](auto cost){spent+=cost;return true;};
                interrupted_hooks.events=[&](const auto& packets){output.insert(output.end(),packets.begin(),packets.end());};
                Runtime interrupted(1);check(interrupted.start(100,{54,0,victim},bar,policy,ranks,true,interrupted_hooks)==casting::Reason::accepted);
                interrupted.interrupt(200,interrupted_hooks);interrupted.update(1100,interrupted_hooks);
                check(spent==(pvp?15u:5u)&&std::count(output.begin(),output.end(),slot_event(228,1,{54,0},pvp?20:12))==1);
                check(interrupted.start(1000,{54,0,victim},bar,policy,ranks,true,interrupted_hooks)==casting::Reason::recharge);
                check(anguish.attack_duration(b,1000,1100)==1000);
            }
            check(cast.start(100,{54,0,victim},bar,policy,ranks,true,h)==casting::Reason::accepted);
            check(anguish.resources(1)->energy==(pvp?25u:35u));
            at=1100;cast.update(at,h);check(anguish.attack_duration(b,1000,1100)==(pvp?1000u:1500u));
            const auto expected_particle=*particles::targeted_body_2022(victim,1,plan.skill);const auto published=box->drain();
            check(std::count(published.begin(),published.end(),expected_particle)==1);
            check(std::count(sent.begin(),sent.end(),slot_event(228,1,{54,0},pvp?20:12))==1);
            if(victim==2)anguish.move(2,5000,0,1100);
            anguish.tick(2100);
            auto health=[&](){return victim==2?double(anguish.resources(2)->health):anguish.npc_combat_snapshot(b)->health->current;};
            check(health()==(pvp?992:986));
            if(victim==2)check(std::abs(anguish.position(2,2100).first-244)<.2);
            if(cleanse){
                check(anguish.shatter_delusions(a,b,*effects::shatter_delusions(0),20,2100));const auto after=health();
                anguish.tick(3100);check(health()==after&&anguish.attack_duration(b,1000,3100)==1000);
                if(victim==2)check(std::abs(anguish.position(2,3100).first-532)<.2);
            }else{
                anguish.tick(19100);check(health()==(pvp?864:762)&&anguish.attack_duration(b,1000,19100)==1000);
                if(victim==2)check(std::abs(anguish.position(2,19100).first-2836)<.5);
            }
        }
        for(unsigned skill:{339u,1136u})for(unsigned victim:{2u,100u})for(unsigned strength:{0u,12u,20u})for(bool blind:{false,true}){
            world::Instance penetrating;penetrating.configure_attack_miss_draws([]{return .5;});auto targets=population.for_map(146);
            for(auto& npc:targets){auto& d=*npc.combat_definition;d.physical_armor=100;d.physical_armor_evidence="test";d.spawn_health=combat::Health{500,500};}
            penetrating.configure_npcs(targets);penetrating.join(1,"Warrior",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            penetrating.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            penetrating.set_vitals(1,{100,100,40,40},100);penetrating.set_vitals(2,{500,500,40,40},100);
            const auto a=*penetrating.effect_scope(1),b=*penetrating.effect_scope(victim);combat::weapon::Owned axe{7,1,{20,20,2},12,false,false,2};
            check(penetrating.ordinary_weapon(a,axe,20)&&penetrating.physical_armor(*penetrating.effect_scope(2),100));
            if(blind)check(penetrating.apply_condition(a,a,Kind::blind,100,5000));
            const auto plan=*combat::weapon::attack_skill(skill,12,strength);
            check(penetrating.melee_attack(a,b,axe,plan,100));
            const double expected=blind?500:500-(10*std::sqrt(2.)+17);
            if(victim==2)check(penetrating.resources(2)->health==std::ceil(expected));
            else check(std::abs(penetrating.npc_combat_snapshot(b)->health->current-expected)<1e-9);
        }
        for(unsigned victim:{2u,100u})for(unsigned starting:{499u,500u,501u})for(bool degeneration:{false,true}){
            world::Instance thrust;auto targets=population.for_map(146);
            for(auto& npc:targets){auto& d=*npc.combat_definition;d.physical_armor=60;d.physical_armor_evidence="test";d.spawn_health=combat::Health{double(starting),1000};}
            thrust.configure_npcs(targets);thrust.join(1,"Warrior",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            thrust.join(2,"Enemy",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            thrust.set_vitals(1,{100,100,40,40},100);thrust.set_vitals(2,{starting,1000,40,40},100);
            const auto a=*thrust.effect_scope(1),b=*thrust.effect_scope(victim);
            combat::weapon::Owned sword{7,1,{20,20,2},12,false,false,27};
            check(thrust.ordinary_weapon(a,sword,20)&&thrust.physical_armor(*thrust.effect_scope(2),60));
            if(degeneration){
                if(victim==2)check(thrust.apply_condition(a,b,Kind::bleeding,100,5000));
                else check(thrust.melee_attack(a,b,sword,*combat::weapon::attack_skill(382,0),100));
            }
            const auto plan=*combat::weapon::attack_skill(385,12,0);check(plan.bonus==32&&plan.required_type==27);
            check(thrust.melee_attack(a,b,sword,plan,1100));
            const double before=starting-(degeneration?(victim==100?26:6):0);
            const double expected=before-20-(before<500?64:32);
            if(victim==2)check(thrust.resources(2)->health==expected);
            else check(thrust.npc_combat_snapshot(b)->health->current==expected);
        }
        {
            world::Instance adrenaline;adrenaline.join(1,"Warrior",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            adrenaline.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            adrenaline.set_vitals(1,{100,100,40,40},100);adrenaline.set_vitals(2,{2000,2000,40,40},100);
            const auto a=*adrenaline.effect_scope(1),b=*adrenaline.effect_scope(2);combat::weapon::Owned sword{7,1,{20,20,2},12,false,false,27};
            check(adrenaline.ordinary_weapon(a,sword,20)&&adrenaline.physical_armor(b,60));
            Slots bar{};bar[0]={385,0};bar[1]={335,0};check(adrenaline.configure_adrenaline(a,bar,100));
            for(unsigned n=0;n<10;++n)check(adrenaline.melee_attack(a,b,sword,*combat::weapon::attack_skill(322,0),100+n));
            const auto charged=*adrenaline.adrenaline(a);check(charged.size()==2&&charged[0].units&&charged[1].units);
            check(adrenaline.consume_adrenaline(a,{385,0},110));
            for(const auto& pool:*adrenaline.adrenaline(a))check(pool.units==0);
        }
        for(unsigned skill:{337u,338u})for(unsigned victim:{2u,100u})for(bool blind:{false,true})for(bool lethal:{false,true}){
            world::Instance axe;axe.configure_attack_miss_draws([]{return .5;});auto targets=population.for_map(146);
            for(auto& npc:targets){npc.combat_definition->physical_armor=60;npc.combat_definition->physical_armor_evidence="test";}
            axe.configure_npcs(targets);axe.join(1,"Warrior",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            axe.join(2,"Enemy",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            axe.set_vitals(1,{100,100,40,40},100);axe.set_vitals(2,{100,100,40,40},100);
            const auto source=*axe.effect_scope(1),target=*axe.effect_scope(victim);
            combat::weapon::Owned weapon{7,1,{lethal?500u:20u,lethal?500u:20u,2},12,false,false,2};
            check(axe.ordinary_weapon(source,weapon,20)&&axe.physical_armor(*axe.effect_scope(2),60));
            if(blind)check(axe.apply_condition(source,source,Kind::blind,100,5000));
            const auto plan=*combat::weapon::attack_skill(skill,12,0);
            check(plan.required_type==2&&plan.bonus==(skill==338?25u:0u)&&combat::weapon::condition_duration(plan)==17000);
            check(combat::weapon::condition_duration(*combat::weapon::attack_skill(skill,0))==5000&&combat::weapon::condition_duration(*combat::weapon::attack_skill(skill,15))==20000);
            auto bad=plan;++bad.bonus;check(!axe.melee_attack(source,target,weapon,bad,100));
            auto sword=weapon;sword.item_type=27;check(!axe.melee_attack(source,target,sword,plan,100));
            check(axe.melee_target(source,victim,100,skill)&&axe.melee_attack(source,target,weapon,plan,100));
            const bool wounded=!blind&&!lethal;
            check(axe.has_condition(target,Kind::deep_wound,100)==wounded);
            const double expected=blind?100:lethal?0:skill==338?35:60;
            if(victim==2){check(axe.resources(2)->health==expected&&axe.resources(2)->max_health==(wounded?80u:100u));}
            else {const auto health=*axe.npc_combat_snapshot(target)->health;check(health.current==expected&&health.maximum==(wounded?80:100));}
            if(wounded){
                axe.tick(17100);check(!axe.has_condition(target,Kind::deep_wound,17100));
                if(victim==2)check(axe.resources(2)->max_health==100);
                else check(axe.npc_combat_snapshot(target)->health->maximum==100);
            }
        }
        for(unsigned skill:{391u,392u,397u,404u})for(unsigned victim:{2u,100u})for(bool blind:{false,true})for(bool lethal:{false,true}){
            world::Instance arrows;arrows.configure_attack_miss_draws([]{return .5;});auto bow_targets=population.for_map(146);
            for(auto& npc:bow_targets){npc.combat_definition->physical_armor=60;npc.combat_definition->physical_armor_evidence="test";}
            arrows.configure_npcs(bow_targets);arrows.join(1,"Ranger",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            arrows.join(2,"Enemy",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            arrows.set_vitals(1,{100,100,40,40},100);arrows.set_vitals(2,{100,100,40,40},100);
            const auto a=*arrows.effect_scope(1),b=*arrows.effect_scope(victim);
            combat::weapon::Owned bow{7,1,{lethal?500u:20u,lethal?500u:20u,1},12,false,false,5};check(arrows.ordinary_weapon(a,bow,20));check(arrows.physical_armor(*arrows.effect_scope(2),60));
            if(blind)check(arrows.apply_condition(a,a,Kind::blind,100,5000));
            const auto plan=*combat::weapon::attack_skill(skill,12);check(plan.required_type==5&&combat::weapon::condition_duration(plan)==(skill==391?21000u:skill==392?13000u:skill==404?17000u:0u));
            using namespace casting;Hooks h;std::uint64_t at=100;std::vector<Packet> sent;
            h.events=[&](const auto& packets){sent.insert(sent.end(),packets.begin(),packets.end());};h.resources=[&](auto id){return arrows.resources(id);};h.consume_energy=[&](auto n){return arrows.consume_energy(1,n);};
            h.weapon_attack_timing=[&](auto now){return arrows.weapon_attack_timing(a,now);};
            h.weapon_snapshot=[&](){return std::optional(bow);};h.target_scope=[&](auto id){return arrows.effect_scope(id);};
            h.melee_target=[&](auto id,auto now){return arrows.melee_target(a,id,now,skill);};
            h.attack_skill_target=[&](auto id,auto skill,auto now){return arrows.melee_target(a,id,now,skill);};
            h.attack_activity=[&](auto until){check(arrows.attack_activity(a,until,at));};
            h.melee_attack=[&](auto to,const auto& gear,const auto& p,auto now){return arrows.melee_attack(a,to,gear,p,now);};
            Policy policy;policy.primary=2;policy.campaigns=0x1e;policy.learned[skill]=1;Slots bar{};bar[0]={skill,0};attributes::Ranks ranks{};if(find_skill(skill)->attribute<ranks.size())ranks[find_skill(skill)->attribute]=12;
            Runtime cast(1);check(cast.start(100,{skill,0,victim},bar,policy,ranks,true,h)==casting::Reason::accepted);
            const auto activation=casting::activation(1,victim,skill,true);
            const auto timing=combat::weapon::timing_2022(1,2.475f,1.f);
            check(std::find(sent.begin(),sent.end(),timing)<std::find(sent.begin(),sent.end(),activation));
            check(std::count(sent.begin(),sent.end(),activation)==1&&std::count(sent.begin(),sent.end(),combat::weapon::start_2022(1,victim))==0);
            const auto release=100+plan.windup_ms;
            at=release-1;cast.update(at,h);arrows.tick(at);check(!arrows.has_condition(b,(skill==391?Kind::bleeding:skill==392?Kind::crippled:Kind::poison),at));
            at=release;cast.update(at,h);check(!arrows.has_condition(b,(skill==391?Kind::bleeding:skill==392?Kind::crippled:Kind::poison),at));
            arrows.tick(release+100);check(arrows.has_condition(b,(skill==391?Kind::bleeding:skill==392?Kind::crippled:Kind::poison),release+100)==(!blind&&!lethal&&skill!=397));
            const auto recharge=casting::slot_event(228,1,{skill,0},find_skill(skill)->recharge);
            check(std::count(sent.begin(),sent.end(),recharge)==1);
            // Frozen bow windows release227 without a melee46 terminal.
            check(std::count(sent.begin(),sent.end(),casting::generic(46,1,0))==0);
            check(std::count(sent.begin(),sent.end(),casting::terminal(46,1))==0);
            check(std::count(sent.begin(),sent.end(),casting::generic(58,1,0))==0);
            check(std::count(sent.begin(),sent.end(),casting::terminal(58,1))==0);
            check(std::count(sent.begin(),sent.end(),skills::cancellation_packet(1,{skill,0}))==1);
            check(cast.start(release+1,{skill,0,victim},bar,policy,ranks,true,h)==(find_skill(skill)->aftercast>0?casting::Reason::busy:casting::Reason::recharge));
            check(!arrows.has_condition(b,(skill==391?Kind::bleeding:skill==392?Kind::crippled:Kind::poison),release+25100));
        }
        for(unsigned victim:{2u,100u}){
            world::Instance gaze;auto out=std::make_shared<world::Outbox>();gaze.configure_npcs(population.for_map(146));
            gaze.join(1,"Caster",0,0,out,0,{}, {},100,1);gaze.join(2,"Enemy",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            gaze.set_vitals(1,{50,100,40,40},100);gaze.set_vitals(2,{100,100,40,40},100);
            const auto a=*gaze.effect_scope(1),b=*gaze.effect_scope(victim);out->drain();
            check(gaze.life_steal(a,b,*effects::life_steal(153,12),100));
            const auto sent=out->drain();const auto correct=*particles::targeted_body_2022(victim,1,153);
            check(std::count(sent.begin(),sent.end(),correct)==1);
        }
        {
            using namespace casting;
            std::string ally_meta=metadata;
            const std::string from="hostile-to-players - 100 100 80 1 1 test test test - test",to="protected - 100 20 80 0 0 test test test - -";
            for(std::size_t at=0;(at=ally_meta.find(from,at))!=std::string::npos;at+=to.size())ally_meta.replace(at,from.size(),to);
            std::istringstream p(placements),m(ally_meta);auto allies=world::load_npc_catalog(p);
            world::bind_combat_catalog(allies,combat::npcs::load_catalog(m,hash),hash);
            world::Instance w;w.configure_npcs(allies.for_map(146));w.join(1,"Monk",0,0,std::make_shared<world::Outbox>());
            w.set_vitals(1,{100,100,40,40},100);const auto caster=*w.effect_scope(1),target=*w.effect_scope(100);
            Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return w.resources(id);};
            h.consume_energy=[&](auto n){return w.consume_energy(1,n);};h.target_scope=[&](auto id){return w.effect_scope(id);};
            h.infuse_target_valid=[&](auto id,auto now){return w.valid_infuse_target(caster,id,now);};
            h.infuse_health=[&](auto target,const auto& plan,auto now){return w.infuse_health(caster,target,plan,now);};
            Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[292]=1;Slots bar{};bar[0]={292,0};attributes::Ranks ranks{};
            Runtime cast(1);check(cast.start(100,{292,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted);
            cast.update(350,h);check(w.resources(1)->health==50&&w.npc_combat_snapshot(target)->health->current==70);
            auto stale=target;++stale.generation;check(!w.infuse_health(caster,stale,*effects::infuse_health(0),350));
            check(w.resources(1)->health==50);
        }
        {
            using namespace casting;world::Instance fear;auto fear_out=std::make_shared<world::Outbox>();fear.configure_npcs(population.for_map(146));
            fear.join(1,"Caster",0,0,fear_out,0,{}, {},100,1);
            fear.join(2,"Enemy",150,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            fear.join(3,"Ally",150,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            for(unsigned id=1;id<=3;++id)fear.set_vitals(id,{100,100,40,40},100);
            const auto caster=*fear.effect_scope(1),target=*fear.effect_scope(100);
            Hooks h;h.events=[](const auto&){};h.resources=[&](auto id){return fear.resources(id);};
            h.consume_energy=[&](auto n){return fear.consume_energy(1,n);};
            h.projectile_target_valid=[](auto id,float){return id==100;};h.target_scope=[&](auto id){return fear.effect_scope(id);};
            h.faintheartedness=[&](auto to,const auto& plan,auto now){return fear.faintheartedness(caster,to,plan,now);};
            Policy policy;policy.primary=4;policy.campaigns=0x1e;policy.learned[136]=1;Slots bar{};bar[0]={136,0};attributes::Ranks ranks{};ranks[7]=12;
            Runtime cast(1);check(cast.start(100,{136,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted);
            const auto end=100+std::uint64_t(find_skill(136)->activation*1000);cast.update(end,h);
            check(fear.attack_duration(*fear.effect_scope(2),1000,end)==1500);
            check(fear.attack_duration(target,1000,end)==1500);
            const auto sent=fear_out->drain();
            const auto visual=particles::shadow_of_fear_target_2022(2,1);
            check(std::count(sent.begin(),sent.end(),visual)==1);
            check(fear.attack_duration(*fear.effect_scope(101),1000,end)==1500);
            check(fear.attack_duration(*fear.effect_scope(102),1000,end)==1000);
            check(fear.attack_duration(*fear.effect_scope(3),1000,end)==1000);
            check(fear.status_snapshot(*fear.effect_scope(2),end)->front().skill==136);
            fear.tick(end+1000);check(fear.resources(2)->health==100);
            check(fear.npc_combat_snapshot(target)->health->current==100);
            check(fear.attack_duration(*fear.effect_scope(2),1000,end+effects::shadow_of_fear(12)->duration)==1000);
            check(fear.attack_duration(target,1000,end+effects::shadow_of_fear(12)->duration)==1000);
            check(fear.faintheartedness(caster,*fear.effect_scope(2),*effects::faintheartedness(12),end+1001));
            check(fear.attack_duration(*fear.effect_scope(2),1000,end+1001)==1500); // Slow effects do not multiply.
            auto stale=target;++stale.generation;check(!fear.attack_duration(stale,1000,end));
        }
        {
            world::Instance feast;feast.configure_npcs(population.for_map(146));
            feast.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            feast.join(2,"Foe",150,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            feast.join(3,"Ally",50,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            feast.set_vitals(1,{10,200,40,40},100);feast.set_vitals(2,{20,100,40,40},100);feast.set_vitals(3,{100,100,40,40},100);
            const auto caster=*feast.effect_scope(1);
            auto wrong=caster;++wrong.generation;check(!feast.area_life_steal(wrong,*effects::area_life_steal(110,12),100));
            check(feast.area_life_steal(caster,*effects::area_life_steal(110,12),100));
            check(feast.npc_combat_snapshot(*feast.effect_scope(100))->health->current==46);
            check(feast.npc_combat_snapshot(*feast.effect_scope(101))->health->current==46);
            check(feast.npc_combat_snapshot(*feast.effect_scope(102))->health->current==100);
            check(feast.resources(2)->health==0&&feast.resources(3)->health==100);
            check(feast.resources(1)->health==139); // 54 + remaining20 +1 +54.
            check(feast.area_life_steal(caster,*effects::area_life_steal(110,12),100));
            check(feast.npc_combat_snapshot(*feast.effect_scope(100))->health->current==0);
            check(feast.npc_combat_snapshot(*feast.effect_scope(101))->health->current==0);
            check(feast.resources(1)->health==200);
            check(feast.area_life_steal(caster,*effects::area_life_steal(110,12),100)); // Empty area succeeds.
        }
        world::Instance w;w.configure_npcs(population.for_map(146));w.join(1,"Necromancer",0,0,std::make_shared<world::Outbox>());
        w.set_vitals(1,{100,100,40,40},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(100);const auto plan=*effects::condition_spell(159,12);
        check(w.condition_spell(source,target,plan,100));
        check(w.has_condition(target,Kind::cracked_armor,100)); // Actual condition state, not just the conditioned visual flag.
        check(w.has_condition(*w.effect_scope(101),Kind::cracked_armor,100));
        check(!w.has_condition(*w.effect_scope(102),Kind::cracked_armor,100)); // outside adjacent radius
        check(!w.has_condition(target,Kind::cracked_armor,17100));
        auto stale=target;++stale.generation;check(!w.condition_spell(source,stale,plan,17100));
        // Existing condition-family casts use the same scoped NPC pipeline.
        check(w.condition_spell(source,target,*effects::condition_spell(131,0),17100));
        w.tick(18100);check(w.npc_combat_snapshot(target)->health->current==94);
        {
            world::Instance blind;auto out=std::make_shared<world::Outbox>();blind.configure_npcs(population.for_map(146));
            blind.join(1,"Mesmer",0,0,out);blind.set_vitals(1,{100,100,40,40},100);
            const auto caster=*blind.effect_scope(1),foe=*blind.effect_scope(100),near=*blind.effect_scope(101),far=*blind.effect_scope(102);
            auto stale_foe=foe;++stale_foe.generation;
            check(!blind.blindness(caster,stale_foe,*effects::blindness(58),100)&&!blind.has_condition(caster,Kind::blind,100));
            check(!blind.blindness(caster,far,*effects::blindness(58),100)&&!blind.has_condition(caster,Kind::blind,100));
            out->drain();check(blind.blindness(caster,foe,*effects::blindness(58),100));
            check(blind.has_condition(caster,Kind::blind,15099)&&blind.has_condition(foe,Kind::blind,15099));
            check(!blind.has_condition(near,Kind::blind,100)&&!blind.has_condition(foe,Kind::blind,15100));
            const auto packets=out->drain();const auto particle=particles::midnight_target_2022(100,1);
            check(std::count(packets.begin(),packets.end(),particle)==1);
            check(blind.npc_combat_snapshot(foe)->health->current==100&&blind.resources(1)->health==100);
            check(blind.blindness(caster,foe,*effects::blindness(424,12),15100));
            check(blind.has_condition(foe,Kind::blind,28099)&&blind.has_condition(near,Kind::blind,28099)&&!blind.has_condition(far,Kind::blind,15100));
            check(blind.blindness(caster,far,*effects::blindness(220,12),15100)&&blind.has_condition(far,Kind::blind,22099));
            // Exercise the real casting dispatcher with no fabricated NPC Resources.
            for(unsigned id:{58u,220u,424u}){
                using namespace casting;Policy policy;policy.primary=id==58?5:id==424?2:6;policy.campaigns=0x1e;policy.learned[id]=1;
                Slots bar{};bar[0]={id,0};attributes::Ranks ranks{};Hooks hooks;unsigned calls=0;
                hooks.resources=[&](auto actor){return blind.resources(actor);};hooks.events=[](const auto&){};
                hooks.consume_energy=[](unsigned){return true;};hooks.target_scope=[&](auto actor){return blind.effect_scope(actor);};
                hooks.projectile_target_valid=[&](auto actor,float range){return blind.projectile_target_valid(caster,actor,range,30000);};
                hooks.blindness=[&](auto victim,const auto& plan,auto now){++calls;return blind.blindness(caster,victim,plan,now);};
                Runtime runtime(1);check(runtime.start(30000,{id,0,100},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
                runtime.update(32000,hooks);check(calls==1&&blind.has_condition(foe,Kind::blind,32000));
            }
            auto immune_population=population.for_map(146);immune_population[0].combat_definition->conditions_allowed=false;
            world::Instance immune;immune.configure_npcs(immune_population);immune.join(1,"Caster",0,0,std::make_shared<world::Outbox>());immune.set_vitals(1,{100,100,40,40},100);
            const auto immune_source=*immune.effect_scope(1);
            check(immune.blindness(immune_source,*immune.effect_scope(100),*effects::blindness(58),100));
            check(immune.has_condition(immune_source,Kind::blind,100)&&!immune.has_condition(*immune.effect_scope(100),Kind::blind,100));
        }
        for(unsigned selected:{2u,100u}){
            world::Instance mixed;mixed.configure_npcs(population.for_map(146));
            mixed.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            mixed.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            mixed.join(3,"Ally",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            for(unsigned id:{1u,2u,3u})mixed.set_vitals(id,{100,100,40,40},100);
            check(mixed.blindness(*mixed.effect_scope(1),*mixed.effect_scope(selected),*effects::blindness(424,12),100));
            for(unsigned id:{2u,100u,101u})check(mixed.has_condition(*mixed.effect_scope(id),Kind::blind,13099));
            for(unsigned id:{1u,3u,102u})check(!mixed.has_condition(*mixed.effect_scope(id),Kind::blind,100));
        }
        world::Instance aoe;auto aoe_out=std::make_shared<world::Outbox>();aoe.configure_npcs(population.for_map(146));
        aoe.join(1,"Elementalist",0,0,aoe_out);aoe.set_vitals(1,{100,100,40,40},100);
        const auto aoe_source=*aoe.effect_scope(1),victim=*aoe.effect_scope(100);
        check(aoe.shockwave(aoe_source,*effects::caster_earth_aoe(174,0),20,100));
        check(std::abs(aoe.npc_combat_snapshot(victim)->health->current-(100.-26./std::sqrt(2.)))<1e-9);
        check(aoe.npc_combat_snapshot(*aoe.effect_scope(101))->health->current==100);
        const auto shake=particles::aftershock_screen_shake_2022(100);const auto first_packets=aoe_out->drain();
        check(std::count(first_packets.begin(),first_packets.end(),shake)==1);
        check(aoe.shockwave(aoe_source,*effects::caster_earth_aoe(937,0),20,200));
        check(aoe.has_condition(victim,Kind::weakness,200)&&aoe.has_condition(victim,Kind::cracked_armor,200)&&aoe.has_condition(victim,Kind::blind,200));
        check(aoe.has_condition(*aoe.effect_scope(101),Kind::weakness,200)&&!aoe.has_condition(*aoe.effect_scope(101),Kind::blind,200));
        // Cast Runtime -> NPC scope validation -> real authoritative condition transaction.
        using namespace casting;Policy necro;necro.primary=4;necro.campaigns=0x1e;necro.learned[159]=1;
        Slots bar{};bar[0]={159,0};attributes::Ranks ranks{};ranks[7]=12;Hooks hooks;
        hooks.resources=[&](auto id){return aoe.resources(id);};hooks.events=[](const auto&){};
        hooks.consume_energy=[](auto amount){return amount==5;};hooks.target_scope=[&](auto id){return aoe.effect_scope(id);};
        hooks.projectile_target_valid=[&](auto id,float range){return aoe.projectile_target_valid(aoe_source,id,range,300);};
        unsigned condition_casts=0;hooks.condition_spell=[&](auto scope,const auto& skill,auto now){++condition_casts;return aoe.condition_spell(aoe_source,scope,skill,now);};
        Runtime runtime(1);check(runtime.start(300,{159,0,100},bar,necro,ranks,true,hooks)==casting::Reason::accepted);
        runtime.update(1300,hooks);check(condition_casts==1&&aoe.has_condition(victim,Kind::cracked_armor,1300));
        // Unknown armor must reject the whole area transaction before any hit.
        auto uncertain=population.for_map(146);uncertain[1].x=150;
        uncertain[1].combat_definition->earth_armor.reset();
        world::Instance denied;denied.configure_npcs(uncertain);denied.join(1,"Caster",0,0,std::make_shared<world::Outbox>());
        denied.set_vitals(1,{100,100,40,40},100);const auto denied_source=*denied.effect_scope(1),healthy=*denied.effect_scope(100);
        check(!denied.shockwave(denied_source,*effects::caster_earth_aoe(174,0),20,100));
        check(denied.npc_combat_snapshot(healthy)->health->current==100);
    }
    {
        using namespace casting;Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[1396]=1;
        Slots bar{};bar[0]={1396,0};attributes::Ranks ranks{};ranks[13]=12;ranks[16]=10;
        Hooks hooks;std::vector<Packet> packets;unsigned calls=0;
        hooks.events=[&](const auto& p){packets.insert(packets.end(),p.begin(),p.end());};
        hooks.resources=[](auto)->std::optional<vitals::Resources>{return vitals::Resources{100,1000,50,50};};
        hooks.target_valid=[](auto actor,float range,bool self){return actor==42&&range==1248&&self;};
        hooks.consume_energy=[](unsigned amount){return amount==5;};
        hooks.status_heal=[&](auto actor,const auto& plan,auto at){check(actor==42&&plan.skill==1396&&plan.rank==12&&plan.divine_heal==32&&at==1100);++calls;return true;};
        Runtime runtime(42);check(runtime.start(100,{1396,0,42},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
        runtime.update(1099,hooks);check(!calls);runtime.update(1100,hooks);check(calls==1);
        const auto body=particles::targeted_body_2022(42,42,1396);check(body&&std::count(packets.begin(),packets.end(),*body)==1);
        policy.learned[1401]=1;bar[0]={1401,0};ranks[15]=12;unsigned removals=0;
        hooks.target_valid=[](auto actor,float range,bool self){return actor==42&&range==144&&self;};
        hooks.remove_conditions=[&](auto actor,const auto& plan,auto at){check(actor==42&&plan.skill==1401&&plan.rank==12&&plan.divine_heal==32&&at==2750);++removals;return true;};
        Runtime touch(42);check(touch.start(2000,{1401,0,42},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
        touch.update(2749,hooks);check(!removals);touch.update(2750,hooks);check(removals==1);
    }
    {
        using namespace casting;world::Instance world;world.join(1,"Casting monk",0,0,std::make_shared<world::Outbox>());
        world.set_vitals(1,{50,100,40,40},100);const auto scope=*world.effect_scope(1);
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[281]=1;Slots bar{};bar[0]={281,0};attributes::Ranks ranks{};
        Hooks hooks;unsigned healed=0;hooks.resources=[&](auto id){return world.resources(id);};hooks.events=[](const auto&){};
        hooks.consume_energy=[](unsigned){return true;};hooks.target_valid=[](auto id,float,bool){return id==1;};
        hooks.heal=[&](unsigned,unsigned,float,bool){++healed;return true;};
        hooks.control_epoch=[&]{return world.control_epoch(scope);};
        hooks.cast_activity=[&](unsigned id,auto begins,auto ends){check(world.cast_activity(scope,id,begins,ends));};
        Runtime runtime(1);check(runtime.start(100,{281,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
        check(!world.casting_skill(scope,99)&&world.casting_skill(scope,100)==281&&!world.casting_skill(scope,1100));
        auto stale=scope;++stale.generation;check(!world.interrupt_cast(stale,281,500));
        check(!world.interrupt_cast(scope,282,500)&&world.casting_skill(scope,500)==281);
        check(world.interrupt_cast(scope,281,500)&&!world.casting_skill(scope,500));
        runtime.update(500,hooks);check(!runtime.active()&&!healed);
        const auto ready=500+std::uint64_t(find_skill(281)->recharge)*1000;
        check(ready>500);
        check(runtime.start(ready-1,{281,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
        runtime.update(ready,hooks);
        check(runtime.start(ready,{281,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
        runtime.cancel(hooks);check(!world.casting_skill(scope,ready));
        check(runtime.start(ready+1,{281,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
        runtime.update(ready+1001,hooks);check(healed==1&&!world.casting_skill(scope,ready+1001));
        check(!world.interrupt_cast(scope,281,ready+1001));
    }
    {
        for(unsigned id:{23u,25u,61u})for(unsigned rank:{0u,12u,20u}){
            const auto plan=*effects::interrupt_skill(id,rank);
            check(plan.damage==(id==23?30+6*rank:0));
            check(effects::interrupt_matches(plan,281,false));
            check(effects::interrupt_matches(plan,1553,false)); // Chants qualify; Leech gains no Energy for them.
            check(effects::interrupt_matches(plan,0,true)==(id==61));
            world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(1,{500,500,0,100},100);w.set_vitals(2,{500,500,40,40},100);
            const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
            check(w.interrupt_skill(source,target,plan,20,100)); // Idle enemy: successful cast, no secondary effect.
            check(w.resources(1)->energy==0&&w.resources(2)->health==500);
            check(w.cast_activity(target,281,100,2000));const auto old=w.control_epoch(target);
            auto stale=target;++stale.generation;check(!w.interrupt_skill(source,stale,plan,20,200));
            check(w.casting_skill(target,200)==281);
            check(w.interrupt_skill(source,target,plan,20,200)&&w.control_epoch(target)!=old&&!w.casting_skill(target,200));
            check(w.resources(2)->health==500-plan.damage&&w.resources(1)->energy==plan.energy);
            const auto energy=w.resources(1)->energy;
            check(w.cast_activity(target,1553,300,2000)&&w.interrupt_skill(source,target,plan,20,400));
            check(w.resources(1)->energy==energy+(id==25?plan.energy:0));
            check(w.cast_activity(target,1,500,2000));
            check(w.interrupt_skill(source,target,plan,20,600));
            check(bool(w.casting_skill(target,600))==(id!=61));
            auto forged=plan;++forged.energy;check(!w.interrupt_skill(source,target,forged,20,700));
        }
        using namespace casting;
        for(unsigned id:{23u,24u,25u,61u,860u,931u}){
            Policy policy;policy.primary=5;policy.campaigns=0x1e;policy.learned[id]=1;Slots bar{};bar[0]={id,0};attributes::Ranks rank{};
            Hooks h;unsigned calls=0;effects::Scope foe{1,2,1};h.events=[](const auto&){};
            h.resources=[](auto)->std::optional<vitals::Resources>{return vitals::Resources{100,100,40,40};};h.consume_energy=[](unsigned){return true;};
            h.target_scope=[&](auto)->std::optional<effects::Scope>{return foe;};h.projectile_target_valid=[](auto,float){return true;};
            h.interrupt_skill=[&](auto target,const auto& plan,auto now){check(target==foe&&plan.skill==id&&now==350);++calls;return true;};
            Runtime runtime(1);check(runtime.start(100,{id,0,2},bar,policy,rank,true,h)==casting::Reason::accepted);
            runtime.update(349,h);check(calls==0);runtime.update(350,h);check(calls==1);
        }
    }
    {
        const auto disruption=*effects::interrupt_skill(860,12);
        check(disruption.damage==43&&effects::interrupt_matches(disruption,281,false));
        check(!effects::interrupt_matches(disruption,1553,false)&&effects::interrupt_matches(disruption,1553,false,true));
        check(!effects::interrupt_matches(disruption,0,true,true)&&!effects::interrupt_matches(disruption,1,false));
        check(effects::interrupt_matches(disruption,1,false,true));
        for(unsigned id:{24u,931u}){
            world::Instance w;w.join(1,"Mesmer",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
            w.join(2,"Foe",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
            w.set_vitals(1,{500,500,20,40},100);w.set_vitals(2,{500,500,2,40},100);
            const auto a=*w.effect_scope(1),b=*w.effect_scope(2);const auto plan=*effects::interrupt_skill(id,12);
            check(plan.target_loss==(id==24?14:0)&&plan.target_gain==(id==931?6:0));
            check(w.interrupt_skill(a,b,plan,20,100)&&w.resources(2)->energy==2);
            check(w.cast_activity(b,281,100,1000)&&w.interrupt_skill(a,b,plan,20,200));
            check(w.resources(2)->energy==(id==24?0:8)&&w.resources(1)->energy==20);
        }
    }
    std::cout<<"Captured skills: rank bounds, Vile Touch, NPC adjacent conditions, Words of Comfort timing/statuses and native body effect PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

