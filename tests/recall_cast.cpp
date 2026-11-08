#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <source_location>
using namespace fw::gw;
unsigned scenario=0;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Recall cast case "+std::to_string(scenario)+" line "+std::to_string(at.line()));
}
void fixture(const std::filesystem::path& path,unsigned kind=0){
    std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);
    if(!out)throw std::runtime_error("fixture output unavailable");
    const auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(char(v>>(8*i)));};
    const auto f32=[&](float v){u32(std::bit_cast<std::uint32_t>(v));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);
    f32(-2000);f32(-2000);f32(kind==3?8000.f:kind==2?4000.f:2000.f);f32(2000);u32(2);
    const auto cell=[&](float left,float right){for(unsigned i=0;i<5;++i)u32(0xffffffff);
        f32(-2000);f32(2000);f32(left);f32(right);f32(left);f32(right);};
    u32(kind==1?2:1);if(kind==1){cell(-2000,200);cell(400,2000);}else cell(-2000,kind==3?8000.f:2000.f);
    u32(1);cell(kind==2?2000.f:-2000.f,kind==3?8000.f:kind==2?4000.f:2000.f);out.close();
    if(kind==2){std::ofstream links(path.string()+".links");links<<"FWLINK01 7 2 2\n0 0 1 0\n1 0 0 0\n";}
}
void join(world::Instance& w,unsigned id,float x,unsigned team,unsigned plane=0,std::shared_ptr<world::Outbox> box={}){
    w.join(id,"Actor",x,0,box?box:std::make_shared<world::Outbox>(),0,{},plane,100,team);
    w.set_vitals(id,{1000,1000,100,100},100);
}
int main(int argc,char** argv)try{
    if(argc!=2)throw std::runtime_error("fixture path required");fixture(argv[1]);
    auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(argv[1]));
    // Trusted opt-in casting exercises the real Runtime/World path;the host
    // deliberately leaves these callbacks unbound until all Recall gates close.
    for(unsigned mode=0;mode<14;++mode){
        scenario=mode;
        auto box=std::make_shared<world::Outbox>();world::Instance w;w.configure_navigation(mesh);

        if(mode==13){
            world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.x=600;npc.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition definition;definition.binding.map=146;definition.binding.actor=definition.binding.definition=100;
            definition.relation=combat::npcs::Relation::allied_to_players;definition.spawn_health=combat::Health{1000,1000};definition.damageable=true;
            npc.combat_definition=definition;w.configure_npcs({npc});
        }
        join(w,1,0,1,0,box);join(w,2,mode==5?1240.f:600.f,mode==9?2:1);
        const auto target_id=mode==13?100u:2u;
        const auto source=*w.effect_scope(1);const auto target=*w.effect_scope(target_id);
        check(!w.recall_target_valid(source,target,100)); // No published World bar yet.
        if(mode==11){w.set_vitals(1,{500,1000,100,100},100);w.set_vitals(2,{500,1000,100,100},100);}
        if(mode==7||mode==12)w.set_vitals(1,{1000,1000,mode==7?14u:15u,100},100);
        if(mode==12)check(w.energy_capacity(1,7,0,100,{80,-2}));
        skills::Slots bar{};bar[0]={925,0};bar[1]={876,0};check(w.configure_adrenaline(source,bar,100));
        skills::Policy policy;policy.primary=mode==11?3:7;policy.secondary=mode==11?7:0;
        policy.campaigns=255;policy.learned[925]=1;policy.learned[876]=1;
        attributes::Ranks ranks{};if(mode==11)ranks[16]=12;
        skills::casting::Runtime runtime(1);skills::casting::Hooks h;unsigned paid=0,completed=0;
        h.resources=[&](auto id){return w.resources(id);};h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.current_skillbar=[&](){return std::optional{bar};};
        h.events=[&](const auto& packets){box->enqueue(packets);};
        h.target_valid=[](auto,float,bool){return false;}; // Never use generic foe/self targeting.
        h.consume_energy_at=[&](unsigned cost,auto at){check(cost==15);const auto ok=w.consume_energy(1,cost,source,at);if(ok)++paid;return ok;};
        h.before_spell_effect=[&](auto skill,auto event,auto at){return w.before_spell_effect(source,skill,event,at);};
        h.skill_disabled_slot=[&](auto slot,auto at){return w.skill_disabled_slot(source,slot,at);};
        h.recall_feedback_pending=[&](auto slot,auto at){return w.recall_feedback_pending(source,slot,at);};
        h.recall_target_valid=[&](auto ally,auto at){return w.recall_target_valid(source,ally,at);};
        h.recall=[&](auto ally,auto at)->skills::casting::EffectCompletion{
            ++completed;const auto ok=w.complete_recall(source,ally,at);return {ok,{},{},{},false,ok};
        };
        if(mode==6)h.recall={};box->drain();
        const auto result=runtime.start(100,{925,0,mode==8?1u:target_id},bar,policy,ranks,true,h);
        using Reason=skills::casting::Reason;
        const auto expected=mode==6?Reason::unsupported:mode==7?Reason::energy:mode==8||mode==9?Reason::target:Reason::accepted;
        if(result!=expected)throw std::runtime_error("Recall admission mode "+std::to_string(mode)+" actual "+std::string(skills::casting::reason_text(result)));
        if(expected!=Reason::accepted){check(paid==0&&completed==0&&w.status_snapshot(source,100)->empty());continue;}
        check(paid==1&&w.resources(1)->energy==(mode==12?0:85));
        if(mode==5)w.move(2,1800,0,100);
        runtime.update(1099,h);check(completed==0&&w.status_snapshot(source,1099)->empty());
        if(mode==1)runtime.cancel(h);if(mode==2)runtime.interrupt(1099,h);
        if(mode==3)check(w.kill(target,1099));
        if(mode==4){w.leave(2);join(w,2,600,1);}
        if(mode==5)w.move(2,1800,0,100);
        if(mode==10)h.recall={};
        runtime.update(1100,h);runtime.update(1100,h);
        const bool success=mode==0||mode==11||mode==12||mode==13;check(completed==unsigned(success)&&paid==1&&!runtime.active());
        if(mode==12){
            check(w.status_snapshot(source,1100)->empty()&&w.position(1,1100).first==600);
            const auto notices=w.pending_recall_feedback(source);check(notices&&notices->size()==1);
            check(runtime.consume_recall_feedback(notices->front(),1100,h,[&](const auto& reply){return w.publish_recall_feedback(reply);}));
        }else check(w.status_snapshot(source,1100)->size()==unsigned(success));
        if(mode==11)check(w.resources(1)->health==500&&w.resources(2)->health==500);
        const auto packets=box->drain();const auto finish=combat::detail::integer(58,1,0);
        const auto body=*skills::particles::targeted_body_2022(target_id,1,925);
        check(std::count(packets.begin(),packets.end(),finish)==unsigned(success));
        check(std::count(packets.begin(),packets.end(),body)==unsigned(success));
        if(success)check(std::find(packets.begin(),packets.end(),finish)<std::find(packets.begin(),packets.end(),body));
        if(mode==0){
            check(w.status_snapshot(target,1100)->empty());
            check(runtime.start(1850,{925,0,2},bar,policy,ranks,true,h)==Reason::recharge);
        }
    }
    std::cout<<"PASS Recall opt-in casting lifecycle and captured completion prefix cases\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}







