#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Death's Charge cast line "+std::to_string(at.line()));
}
void fixture(const std::filesystem::path& path){
    std::filesystem::create_directories(path.parent_path());std::ofstream out(path,std::ios::binary);
    const auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(char(v>>(8*i)));};
    const auto f32=[&](float v){u32(std::bit_cast<std::uint32_t>(v));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);
    f32(-2000);f32(-2000);f32(2000);f32(2000);u32(1);u32(1);
    for(unsigned i=0;i<5;++i)u32(0xffffffff);
    for(unsigned i=0;i<6;++i)f32(i%2?2000:-2000);
    out.close();
}
int main(int argc,char** argv)try{
    check(argc==2);fixture(argv[1]);
    auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(argv[1]));
    for(unsigned mode=0;mode<3;++mode){
        world::Instance w;w.configure_navigation(mesh);
        auto box=std::make_shared<world::Outbox>();
        w.join(1,"Caster",0,0,box,0,{},0,100,1);w.set_vitals(1,{500,1000,100,100},100);
        w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{},0,100,2);
        w.set_vitals(2,{900,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        box->drain();
        skills::casting::Runtime runtime(1);skills::casting::Hooks h;
        std::vector<world::Packet> packets;
        h.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        h.resources=[&](auto id){return w.resources(id);};
        h.consume_energy_at=[&](unsigned cost,auto now){check(cost==5);return w.consume_energy(1,cost,source,now);};
        h.target_scope=[&](auto id){return w.effect_scope(id);};
        h.death_charge_target_valid=[&](auto foe,const auto& plan,auto now){return w.death_charge_target_valid(source,foe,plan,now);};
        h.death_charge=[&](auto foe,const auto& plan,auto now)->skills::casting::EffectCompletion{
            std::vector<world::Packet> visuals;std::function<void()> publish;
            const bool accepted=w.death_charge(source,foe,plan,now,&visuals,&publish);
            if(accepted){check(visuals.size()==4&&visuals.front()==combat::detail::integer(21,1,951));visuals.erase(visuals.begin());}
            return {accepted,std::move(visuals),{},std::move(publish),true};
        };
        skills::Policy policy;policy.primary=7;policy.campaigns=255;policy.learned[952]=1;
        skills::Slots bar{};bar[0]={952,0};attributes::Ranks ranks{};ranks[31]=12;
        check(runtime.start(1000,{952,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::accepted);
        check(w.resources(1)->energy==95&&w.resources(1)->health==500);
        if(mode==1)runtime.cancel(h);
        if(mode==2)check(w.kill(target,1100));
        runtime.update(1249,h);check(w.position(1,1249).first==0);
        runtime.update(1250,h);
        if(mode){check(w.position(1,1250).first==0);continue;}
        check(w.position(1,1250).first==600&&w.resources(1)->health==673);
        const auto body=combat::detail::integer(21,1,951);
        const auto it=std::find(packets.begin(),packets.end(),body);
        check(it!=packets.end()&&it+3<packets.end());
        check((it+1)->at(0)==160&&(it+2)->at(0)==160&&(it+3)->at(0)==44);
        check(std::count(packets.begin(),packets.end(),body)==1);
        check(!box->drain().empty());
        check(runtime.start(1251,{952,0,2},bar,policy,ranks,true,h)==skills::casting::Reason::busy);
    }
    std::cout<<"PASS paid Death's Charge, 250ms completion, native ordered relocation/healing, cancellation and lost target\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
