#include "gw_skill_casting.h"
#include "gw_world_simulation.h"
#include "gw_quickening_zephyr_spawn.h"
#include <iostream>
#include <source_location>
#include <stdexcept>

using namespace fw::gw;
static void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Quickening Zephyr cast line "+std::to_string(at.line()));
}
int main()try{
    const auto plan=*skills::effects::quickening_zephyr(0);
    const auto candidate=world::quickening_zephyr_candidate_template(146,plan);
    check(candidate&&candidate->definition.model_file==141876);
    check(candidate->definition.id==60000&&candidate->definition.level==1);
    check(candidate->definition.skin_file==0&&candidate->definition.appearance==0);
    check(candidate->definition.flags==16384&&candidate->definition.primary==2);
    check(candidate->definition.visual_adjustment==std::array<std::uint8_t,4>{0,0,0,80});
    check(candidate->model_files.empty()&&candidate->definition.encoded_name.empty());
    check(candidate->literal_display_name==u"Spirit of Quickening Zephyr");
    check(npc::literal_name_2022(97,*candidate->literal_display_name).size()>0);
    check(candidate->combat_definition->evidence.health.find("provisional")!=std::string::npos);
    check(!candidate->combat_definition->general_armor);
    check(!world::quickening_zephyr_candidate_template(0,plan));
    auto forged=plan;++forged.level;
    check(!world::quickening_zephyr_candidate_template(146,forged));
    check(world::quickening_zephyr_candidate_template(146,*skills::effects::quickening_zephyr(20))->definition.id==60020);
    for(unsigned mode=0;mode<5;++mode){
        world::Instance world;world.configure_npc_visibility(5020);
        auto box=std::make_shared<world::Outbox>();
        world.join(1,"Ranger",0,0,box,0,{}, {},100,1);
        world.set_vitals(1,{500,500,mode==2?20u:100u,100},100);
        const auto owner=*world.effect_scope(1);
        box->drain();
        skills::casting::Runtime runtime(1);
        skills::casting::Hooks hooks;
        unsigned payments=0,completions=0;bool stale=false;
        hooks.events=[&](const auto& packets){box->enqueue(packets);};
        hooks.resources=[&](auto actor){return world.resources(actor);};
        hooks.target_scope=[&](auto actor){auto scope=world.effect_scope(actor);
            if(scope&&stale&&actor==1)++scope->generation;
            return scope;};
        hooks.target_valid=[](auto actor,float,bool self){return actor==1&&self;};
        hooks.consume_energy_at=[&](unsigned cost,auto now){
            ++payments;check(cost==25);
            return world.consume_energy(1,cost,owner,now);
        };
        if(mode!=1)hooks.quickening_zephyr_spawn=[&](auto source,const auto& plan,auto now){
            ++completions;
            return world::stage_quickening_zephyr_candidate(world,source,plan,146,now);
        };
        skills::Policy policy;policy.primary=2;policy.campaigns=255;policy.learned[475]=1;
        skills::Slots bar{};bar[0]={475,0};attributes::Ranks ranks{};ranks[24]=0;
        const auto result=runtime.start(1000,{475,0,0},bar,policy,ranks,true,hooks);
        if(mode==1){check(result==skills::casting::Reason::unsupported);
            check(payments==0&&completions==0);continue;}
        if(mode==2){check(result==skills::casting::Reason::energy&&payments==1&&completions==0);continue;}
        check(result==skills::casting::Reason::accepted&&runtime.active()&&payments==1);
        check(world.resources(1)->energy==75&&completions==0);
        runtime.update(5999,hooks);check(completions==0);
        if(mode==3){runtime.cancel(hooks);runtime.update(6000,hooks);check(completions==0);continue;}
        if(mode==4)stale=true;
        runtime.update(6000,hooks);
        if(mode==4){check(completions==0&&world.npc_packets().empty());continue;}
        check(completions==1);
        const auto packets=box->drain();
        const auto finish=std::find(packets.begin(),packets.end(),skills::casting::terminal(58,1));
        const auto spawn=std::find_if(packets.begin(),packets.end(),[](const auto& p){
            return p.size()>=2&&p[0]==32&&p[1]==0;});
        check(finish!=packets.end()&&spawn!=packets.end()&&finish<spawn);
        check(std::count(packets.begin(),packets.end(),
            skills::casting::slot_event(228,1,{475,0},60))==1);
        check(world.quickening_zephyr_affects_player(owner,6000)==true);
        check(runtime.start(6001,{475,0,0},bar,policy,ranks,true,hooks)==
              skills::casting::Reason::busy);
        check(runtime.start(6751,{475,0,0},bar,policy,ranks,true,hooks)==
              skills::casting::Reason::recharge);
    }
    std::cout<<"PASS provisional production-template Quickening Zephyr payment, five-second cast, cancel, scoped spawn and finish-before-publication\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
