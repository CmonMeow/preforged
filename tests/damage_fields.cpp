#include "gw_npc_catalog.h"
#include "gw_skill_casting.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace world;using namespace skills;using namespace effects;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    check(damage_field(247,12)->damage==27&&damage_field(1113,15)->damage==32&&damage_field(195,12)->damage==41&&damage_field(195,12,true)->radius==252,"field plans");
    check(!damage_field(247,21)&&!damage_field(247,0,true)&&casting::Runtime::effect_supported(195),"Lava Font capability missing");
    {
        DamageFieldSchedule schedule;const Scope first{1,1,1},second{1,2,1};
        check(schedule.add(first,20,10,20,0,*damage_field(247),100)&&schedule.add(second,20,30,40,0,*damage_field(1113),600),"field scheduling");
        std::vector<std::pair<std::uint64_t,unsigned>> order;
        schedule.advance(1099,[&](const auto&){throw std::runtime_error("early pulse");return true;});
        schedule.advance(5600,[&](const auto& p){order.emplace_back(p.at,p.caster.actor);check(p.x==(p.caster==first?10:30)&&p.plan.radius==166,"fixed field geometry changed");return true;});
        check(order.size()==10&&std::is_sorted(order.begin(),order.end())&&schedule.empty(),"overlapping fields not chronological");
        schedule.advance(1200,[&](const auto&){throw std::runtime_error("backwards replay");return true;});
        check(!schedule.add(first,20,0,0,0,*damage_field(247),100),"backdated field accepted");
        DamageFieldSchedule tied;check(tied.add(second,20,0,0,0,*damage_field(247),100)&&tied.add(first,20,0,0,0,*damage_field(247),100),"equal-time fixture");
        std::vector<unsigned> actors;tied.advance(1100,[&](const auto& p){actors.push_back(p.caster.actor);return false;});check(actors==std::vector<unsigned>{2,1}&&tied.empty(),"equal-time creation order/cancellation");
    }
    {
        const auto packets=particles::lava_font_ground_2022(25,std::bit_cast<float>(0xc5b45c52u),std::bit_cast<float>(0x44e8a1f2u),0);
        const combat::Packet expected{0xa0,0x80,0x52,0x5c,0xb4,0xc5,0xf2,0xa1,0xe8,0x44,0,0,25,0,0,0,0x5c,1,0,0};
        check(packets.size()==2&&packets[0]==expected,"Lava ground packet differs from native serializer");
        auto second=expected;second[16]=0x5d;check(packets[1]==second,"Lava second ground resource");
        Instance w;auto out=std::make_shared<Outbox>();w.join(1,"Caster",0,0,out,0,{}, {},100,1);w.set_vitals(1,{100,100,40,40},100);out->drain();
        const auto caster=*w.effect_scope(1);
        check(!w.damage_field(caster,*damage_field(195,12,true),20,1000)&&out->drain().empty(),"rejected Lava published visuals");
        check(w.damage_field(caster,*damage_field(195,12),20,1000),"Lava visual commit");
        std::vector<combat::Packet> ground;for(const auto& packet:out->drain())if(packet.size()==20&&packet[0]==160&&packet[1]==0x80)ground.push_back(packet);
        check(ground==particles::lava_font_ground_2022(1,0,0,0),"accepted Lava missing positional visuals");
    }
    const auto population=[](){
        using namespace combat::npcs;const std::string hash(64,'a');
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 100 0 0 - -\n");auto result=load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 1 "+hash+"\nsource test authored "+hash+" tests/damage_fields.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 500 500 60 1 1 test test test - test\n");const auto catalog=load_catalog(metadata,hash);bind_combat_catalog(result,catalog,hash);
        auto npcs=result.for_map(146);npcs.front().combat_definition->fire_armor=60;npcs.front().combat_definition->holy_traits={false,true};return npcs;
    };
    const auto join=[](Instance& w){w.join(1,"Caster",0,0,std::make_shared<Outbox>(),0,{}, {},100,1);w.set_vitals(1,{100,200,40,40},100);return *w.effect_scope(1);};
    for(unsigned variant=0;variant<3;++variant){
        auto npcs=population();if(variant==1)npcs.front().combat_definition->holy_traits.light_sensitive=true;if(variant==2)npcs.front().combat_definition->holy_traits.light_sensitive.reset();
        Instance w;w.configure_npcs(npcs);const auto caster=join(w),npc=*w.effect_scope(100);
        check(w.damage_field(caster,*damage_field(247,12),20,100),"Symbol field creation");w.move(1,1000,0,100);w.tick(5100);
        check(w.npc_combat_snapshot(npc)->health->current==500-(variant==2?0:135*(variant==1?2:1)),"Symbol did not remain at initial location or used summoned multiplier");
        const auto health=w.npc_combat_snapshot(npc)->health->current;w.tick(6100);check(w.npc_combat_snapshot(npc)->health->current==health,"sixth Symbol pulse");
        check(w.damage_field(caster,*damage_field(195,12),20,6100),"Lava Font without Overcast rejected");
    }
    {
        // An interleaved spell commit must settle the earlier field pulse before
        // evaluating Shadow Strike's above-half-health condition.
        std::vector<double> results;
        for(bool explicit_tick:{false,true}){
            auto npcs=population();npcs.front().combat_definition->spawn_health=combat::Health{270,500};Instance w;w.configure_npcs(npcs);const auto caster=join(w),npc=*w.effect_scope(100);
            check(w.fire_storm(caster,npc,*fire_storm(12),20,100),"interleaved storm");if(explicit_tick)w.tick(1100);
            check(w.shadow_damage(caster,npc,*shadow_damage(102,0),20,1600),"interleaved spell");results.push_back(w.npc_combat_snapshot(npc)->health->current);results.push_back(w.resources(1)->health);
        }
        check(results[0]==results[2]&&results[1]==results[3],"interleaved health commit overtook due field");
    }
    for(bool leaving:{false,true}){
        auto npcs=population();auto guardian=npcs.front();guardian.x=leaving?1000.f:0.f;guardian.combat_definition->relation=combat::npcs::Relation::neutral;
        Instance w;w.configure_npcs({guardian});const auto caster=join(w),guard=*w.effect_scope(100);npcs.front().x=leaving?-100.f:300.f;
        check(w.begin_wave_objective(caster,guard,73,1,3,1)&&w.spawn_quest_wave(caster,73,1,npcs,[]{return true;}),"field movement fixture");const auto enemy=*w.effect_scope(60000);
        check(w.chase_quest_guardian(enemy,100,100)&&w.damage_field(caster,*damage_field(247,0),20,100),"moving field creation");w.tick(5100);
        check(w.npc_combat_snapshot(enemy)->health->current==500-8*(leaving?2:4),"delayed pulse used final NPC position");
    }
    {
        auto npcs=population();npcs.front().combat_definition->spawn_health=combat::Health{20,500};Instance w;w.configure_npcs(npcs);const auto caster=join(w),npc=*w.effect_scope(100);
        check(w.damage_field(caster,*damage_field(247,0),20,100),"lethal field");w.tick(5100);const auto deaths=w.take_npc_deaths(caster);
        check(w.npc_combat_snapshot(npc)->health->current==0&&deaths.size()==1&&deaths.front().time==3100,"death time or repeated later pulse kill");
    }
    {
        using namespace casting;Hooks h;unsigned commits=0,cost=0;h.events=[](const auto&){};h.resources=[](auto)->std::optional<vitals::Resources>{return vitals::Resources{100,100,40,40};};h.consume_energy=[&](unsigned c){cost+=c;return true;};h.target_valid=[](auto id,float,bool){return id==1;};
        h.damage_field=[&](const auto& p,auto now){check(p.skill==247&&p.rank==12&&now==2100,"Symbol casting plan");++commits;return true;};
        Policy policy;policy.primary=3;policy.campaigns=0x1e;policy.learned[247]=1;Slots bar{};bar[0]={247,0};attributes::Ranks ranks{};ranks[14]=12;
        Runtime cast(1);check(cast.start(100,{247,0,100},bar,policy,ranks,true,h)==casting::Reason::accepted&&cost==5,"Symbol normalizes to caster and pays energy");cast.update(2099,h);check(!commits,"early Symbol activation");cast.update(2100,h);cast.update(2100,h);check(commits==1,"Symbol commit replay");
    }
    {
        auto npcs=population();npcs.front().combat_definition->spawn_health=combat::Health{28,500};Instance w;w.configure_npcs(npcs);const auto caster=join(w),npc=*w.effect_scope(100);
        check(w.degeneration_hex(caster,npc,*degeneration_hex(31,0),100)&&w.damage_field(caster,*damage_field(247,0),20,100),"degeneration/field ordering fixture");w.tick(5100);
        const auto deaths=w.take_npc_deaths(caster);check(deaths.size()==1&&deaths.front().time==2100,"late degeneration overtook earlier field pulses");
    }
    for(bool depart:{false,true}){
        Instance w;w.configure_npcs(population());const auto caster=join(w),npc=*w.effect_scope(100);w.set_vitals(1,{20,200,40,40},100);
        check(w.damage_field(caster,*damage_field(1113,0),20,100),"Kirin lifecycle fixture");
        if(depart){w.leave(1);join(w);}else{const auto loss=w.health_sacrifice(caster,115,100);check(loss&&loss->killed,"caster death fixture");}
        w.tick(5100);check(w.npc_combat_snapshot(npc)->health->current==(depart?500:460),"field lifecycle transferred to replacement or stopped at caster death");
    }
    {
        auto npcs=population();auto guardian=npcs.front();guardian.x=0;guardian.combat_definition->relation=combat::npcs::Relation::neutral;
        Instance w;w.configure_npcs({guardian});const auto caster=join(w),guard=*w.effect_scope(100);npcs.front().x=300;
        check(w.begin_wave_objective(caster,guard,73,1,3,1)&&w.spawn_quest_wave(caster,73,1,npcs,[]{return true;}),"moving storm center fixture");const auto enemy=*w.effect_scope(60000);
        check(w.chase_quest_guardian(enemy,100,100)&&w.fire_storm(caster,enemy,*fire_storm(0),20,1600),"cast between NPC movement ticks");
        // Cast at x150, not stored spawn x300; first pulse sees x50 within166.
        w.tick(2600);check(w.npc_combat_snapshot(enemy)->health->current==495,"Fire Storm captured stale target center");
    }
    {
        auto npcs=population();auto guardian=npcs.front();guardian.x=0;guardian.combat_definition->relation=combat::npcs::Relation::neutral;
        Instance w;w.configure_npcs({guardian});const auto caster=join(w),guard=*w.effect_scope(100);npcs.front().x=300;
        check(w.begin_wave_objective(caster,guard,73,1,3,1)&&w.spawn_quest_wave(caster,73,1,npcs,[]{return true;}),"field retarget fixture");const auto enemy=*w.effect_scope(60000);
        check(w.chase_quest_guardian(enemy,100,100)&&w.damage_field(caster,*damage_field(247,0),20,100),"retarget initial route");w.move(1,1000,0,100);
        check(w.chase_quest_guardian(enemy,100,2600,caster)&&w.npc_combat_snapshot(enemy)->health->current==492,"retarget overtook earlier pulse");w.tick(5100);
        check(w.npc_combat_snapshot(enemy)->health->current==484,"retarget lost intermediate position/history");
    }
    // Lava Font snapshots authoritative Overcast and caster position at completion.
    for(unsigned mode=0;mode<4;++mode){
        auto npcs=population();npcs.front().x=220;Instance w;w.configure_npcs(npcs);
        const auto caster=join(w),npc=*w.effect_scope(100);
        w.join(2,"Enemy",220,0,std::make_shared<Outbox>(),0,{}, {},100,2);
        w.set_vitals(2,{500,500,40,40},100);check(w.elemental_armor(*w.effect_scope(2),60,60),"player armor");
        if(mode==1||mode==2)check(w.activate_overcast_energy(caster,219,0,1,200),"Overcast setup");
        const auto completion=mode==2?40000u:mode==1?14000u:1000u;
        check(w.damage_field(caster,*damage_field(195,12),20,completion),"Lava Font commit");
        if(mode==3)check(w.activate_overcast_energy(caster,219,0,2,1100),"late Overcast");
        w.move(1,1000,0,completion);w.tick(completion+5000);
        check(w.npc_combat_snapshot(npc)->health->current==(mode==1?295:500),"Lava radius changed after activation");
        check(w.resources(2)->health==(mode==1?295:500),"Lava player radius mismatch");
        check(!w.damage_field(caster,*damage_field(195,12,true),20,completion+5000),"caller forged expanded radius");
    }
    for(unsigned mode=0;mode<5;++mode){
        using namespace casting;Instance w;const auto caster=join(w);unsigned commits=0;
        Slots bar{};bar[0]={195,0};Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[195]=1;
        attributes::Ranks ranks{};ranks[10]=12;Hooks hooks;
        std::vector<combat::Packet> emitted;hooks.resources=[&](unsigned id){return w.resources(id);};
        hooks.events=[&](const auto& packets){emitted.insert(emitted.end(),packets.begin(),packets.end());};
        hooks.target_valid=[](unsigned id,float,bool self){return id==1&&self;};
        hooks.consume_energy_at=[&](unsigned cost,auto at){return w.consume_energy(1,cost,caster,at);};
        if(mode!=2)hooks.complete_damage_field=[&](const auto& plan,auto at)->std::optional<std::vector<combat::Packet>>{
            ++commits;if(mode==3)return {};std::vector<combat::Packet> visuals;
            if(!w.damage_field(caster,plan,20,at,&visuals))return {};return visuals;
        };
        Runtime cast(1);const auto result=cast.start(1000,{195,0,999},bar,policy,ranks,true,hooks);
        check(result==(mode==2?casting::Reason::unsupported:casting::Reason::accepted),"Lava cast admission");
        check(w.resources(1)->energy==(mode==2?40u:30u),"Lava ordinary cost");
        if(mode==1)cast.cancel(hooks);if(mode==4)cast.interrupt(1500,hooks);cast.update(2499,hooks);check(!commits,"early Lava commit");
        cast.update(2500,hooks);check(commits==unsigned(mode==0||mode==3),"Lava cancel/complete");
        std::vector<combat::Packet> completed;
        for(const auto& packet:emitted){
            if(packet.size()==20&&packet[0]==160&&packet[1]==0x80)completed.push_back(packet);
            else if(packet.size()>=14&&packet[0]==158&&(packet[2]==58||packet[2]==21))completed.push_back(packet);
        }
        if(mode==0){
            auto expected=std::vector<combat::Packet>{generic(58,1,0),*success_body_effect(1,195)};
            const auto ground=particles::lava_font_ground_2022(1,0,0,0);expected.insert(expected.end(),ground.begin(),ground.end());
            check(completed==expected,"Lava completion/body/ground order differs from capture");
        }else check(completed.empty(),"unsuccessful Lava published completion effects");
    }
    std::cout<<"Fixed damage fields: chronological pulses, movement, interleaved commits, death and trait gates verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
