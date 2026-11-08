#include "gw_npc_catalog.h"
#include "gw_movement_effects.h"
#include <iostream>
int main()try{
    using namespace fw::gw;using namespace skills::effects;
    auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
    check(movement_hex(76,0)->duration==8000&&movement_hex(76,12)->duration==18000&&movement_hex(76,15)->duration==20000,"wiki duration mistaken for native slowdown scale");
    MovementEffects timeline;timeline.apply({1,1,1043,0,Category::stance,100,3100});timeline.apply({2,2,76,0,Category::hex,1100,9100});
    check(timeline.at(100)==1.5f&&timeline.at(1100)==.75f&&timeline.at(3100)==.5f&&timeline.at(9100)==1.f,"movement composition/expiry");
    check(timeline.integral(100,10100)==7000,"late tick movement integral");
    timeline.remove(2,4100);check(timeline.integral(100,5100)==4500,"removed snare interval lost");
    {
        combat::conditions::State state;MovementEffects slow;
        check(state.apply(combat::conditions::Kind::crippled,1,100,1000),"Crippled fixture");slow.crippled(state,100);
        check(slow.at(100)==.5f&&slow.integral(100,2100)==1500,"Crippled expiry distance");
        check(state.apply(combat::conditions::Kind::crippled,1,600,1),"short Crippled renewal");slow.crippled(state,600);
        check(slow.integral(100,2100)==1500,"short Crippled shortened timer or stacked slow");
        slow.apply({0,1,1043,0,Category::stance,100,3100});slow.remove(0,600);
        check(slow.at(600)==.5f,"stance effect ID removal cleared Crippled");
        state.remove(combat::conditions::Kind::crippled);slow.crippled(state,800);
        check(slow.at(800)==1.f&&slow.at(799)==.5f,"cleanse discarded pre-removal movement");
    }
    for(bool cleanse:{false,true}){
        world::Instance cripple;cripple.join(1,"Runner",0,0,std::make_shared<world::Outbox>());cripple.set_vitals(1,{100,100,30,30},100);
        const auto runner=*cripple.effect_scope(1);
        check(cripple.apply_condition(runner,runner,combat::conditions::Kind::crippled,100,1000),"player Crippled application");
        cripple.move(1,5000,0,100);cripple.tick(600);check(std::abs(cripple.position(1,600).first-72)<.2,"player Crippled speed");
        if(cleanse)check(cripple.remove_conditions(runner,1,*condition_removal(278,0,0),600),"Crippled cleanse");
        else check(cripple.apply_condition(runner,runner,combat::conditions::Kind::crippled,600,1),"short player Crippled renewal");
        cripple.tick(2100);check(std::abs(cripple.position(1,2100).first-(cleanse?504.f:432.f))<.5,"player Crippled cleanse/expiry route distance");
    }
    auto world=std::make_unique<world::Instance>();auto box=std::make_shared<world::Outbox>();
    world->join(1,"Mesmer",0,0,box,0,{}, {},100,1);world->join(2,"Runner",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
    world->set_vitals(1,{100,100,30,30},100);world->set_vitals(2,{100,100,30,30},100);
    const auto caster=*world->effect_scope(1),target=*world->effect_scope(2);
    check(world->status_mutation(target,1043,1,0,100,3100,true)&&world->movement_multiplier(2,1.5f,100),"Dash apply");
    check(world->movement_hex(caster,target,*movement_hex(76,0),100),"player movement hex");
    world->move(2,5000,0,100);world->tick(1100);check(std::abs(world->position(2,1100).first-316)<.2,"Dash/snare distance");
    world->tick(9100);check(std::abs(world->position(2,9100).first-1756)<.5,"delayed Dash/hex expiry distance");
    check(world->movement_multiplier(2,1.5f,9100),"delayed activation acknowledgement");
    world->tick(9600);check(std::abs(world->position(2,9600).first-1900)<.5,"late callback resurrected expired Dash");
    check(world->movement_multiplier(2,1.f,9600),"expired Dash callback");
    world->tick(10100);check(std::abs(world->position(2,10100).first-2044)<.5,"post-expiry speed");
    check(!world->movement_hex(caster,caster,*movement_hex(76),10100),"friendly movement hex");
    {
        auto removed=std::make_unique<world::Instance>();removed->join(1,"Mesmer",0,0,box,0,{}, {},100,1);removed->join(2,"Runner",100,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);
        removed->set_vitals(1,{100,100,30,30},100);removed->set_vitals(2,{100,100,30,30},100);
        const auto a=*removed->effect_scope(1),b=*removed->effect_scope(2);
        check(removed->status_mutation(b,1043,1,0,100,3100,true)&&removed->movement_multiplier(2,1.5f,100)&&removed->movement_hex(a,b,*movement_hex(76),100),"removal fixture");
        removed->move(2,5000,0,100);removed->tick(1100);
        check(removed->shatter_delusions(a,b,*shatter_delusions(0),1,1100),"Shatter removal of movement hex");
        removed->tick(2100);check(std::abs(removed->position(2,2100).first-748)<.5,"removing Burden also removed Dash or retained snare");
        removed->tick(4100);check(std::abs(removed->position(2,4100).first-1468)<.5,"Dash expiry after hex removal");
    }
    combat::npcs::Definition definition;definition.binding.map=146;definition.binding.actor=100;
    definition.relation=combat::npcs::Relation::hostile_to_players;definition.spawn_health=combat::Health{100,100};definition.damageable=true;
    combat::npcs::LiveActor npc(definition,{1,1,1,146,100});const auto life=npc.lifetime();
    {
        auto slowed_definition=definition;slowed_definition.conditions_allowed=true;
        combat::npcs::LiveActor slowed(slowed_definition,life);
        check(slowed.apply_condition(life,combat::conditions::Kind::crippled,1,100,1000),"NPC Crippled");
        check(slowed.movement_multiplier(life,100)==.5f&&slowed.movement_integral(life,100,2100)==1500,"NPC Crippled expiry integration");
        check(slowed.remove_condition(life,combat::conditions::Kind::crippled,600),"NPC Crippled cleanse");
        check(slowed.movement_integral(life,100,2100)==1750&&slowed.movement_multiplier(life,600)==1.f,"NPC cleanse lost movement history");
    }
    check(npc.apply_movement_hex(life,*movement_hex(76),1,1,100),"NPC movement hex");
    check(npc.movement_integral(life,100,9100)==5000,"NPC expiry integration");
    npc.refresh_hexes(life,9100);check(npc.movement_integral(life,100,9100)==5000,"NPC registry expiry lost movement history");
    check(npc.apply_movement_hex(life,*movement_hex(76),1,2,9100)&&npc.remove_mesmer_hex(life,2,10100),"NPC real hex removal");
    check(npc.movement_integral(life,9100,11100)==1500,"removed NPC hex still slows");
    {
        using namespace combat::npcs;using namespace world;
        const std::string hash(64,'a');
        std::istringstream appearance("gw-npcs 1\nnpc 146 100 1 1234 0 1677721600 0 8 1 1 0 0 0 0 - -\n");
        auto population=load_npc_catalog(appearance);
        std::istringstream metadata("gw-npc-combat 1 "+hash+"\nsource test authored "+hash+
            " tests/movement_effects.cpp\nnpc 146 100 1 1234 0 0 8 1 hostile-to-players - 100 100 60 1 1 test test test - test\n");
        const auto catalog=load_catalog(metadata,hash);bind_combat_catalog(population,catalog,hash);
        auto guardian=population.for_map(146).front();guardian.combat_definition->relation=Relation::neutral;
        Instance chase;chase.configure_npcs({guardian});chase.join(2,"Mesmer",0,0,std::make_shared<Outbox>());chase.set_vitals(2,{100,100,30,30},100);
        const auto owner=*chase.effect_scope(2),guard=*chase.effect_scope(100);
        auto enemies=population.for_map(146);enemies[0].x=1000;
        check(chase.begin_wave_objective(owner,guard,73,1,3,1)&&chase.spawn_quest_wave(owner,73,1,enemies,[]{return true;}),"moving NPC fixture");
        const auto enemy=*chase.effect_scope(60000);
        check(chase.movement_hex(owner,enemy,*movement_hex(76),100)&&chase.chase_quest_guardian(enemy,100,100),"snared NPC chase");
        chase.tick(9100);
        check(chase.projectile_target_valid(owner,60000,501,9100)&&!chase.projectile_target_valid(owner,60000,499,9100),"NPC route lost late-expiry snare interval");
        chase.tick(10100);
        check(chase.projectile_target_valid(owner,60000,401,10100)&&!chase.projectile_target_valid(owner,60000,399,10100),"NPC route failed to restore speed");
    }
    std::cout<<"Movement composition, player expiry paths and NPC movement history verified\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
