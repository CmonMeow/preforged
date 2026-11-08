#include "gw_world_simulation.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Patrol visibility line "+std::to_string(at.line()));
}
unsigned word(const world::Packet& p,unsigned at){
    return unsigned(p.at(at))|(unsigned(p.at(at+1))<<8)|(unsigned(p.at(at+2))<<16)|(unsigned(p.at(at+3))<<24);
}
std::shared_ptr<const navigation::Mesh> obstacle(){
    const auto path=std::filesystem::temp_directory_path()/("patrol-visibility-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".fwnav");
    std::ofstream out(path,std::ios::binary);check(bool(out));
    const auto u32=[&](unsigned n){for(unsigned i=0;i<4;++i)out.put(char(n>>(8*i)));};
    const auto f32=[&](float n){u32(std::bit_cast<unsigned>(n));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);
    for(float n:{0.f,0.f,1000.f,400.f})f32(n);u32(1);u32(3);
    for(unsigned i=0;i<3;++i){u32(i?i-1:0xffffffffu);u32(i<2?i+1:0xffffffffu);
        for(unsigned j=0;j<3;++j)u32(0xffffffffu);
        const float left=i==0?0.f:i==1?180.f:220.f,right=i==0?180.f:i==1?220.f:1000.f;
        for(float n:{i==1?300.f:0.f,400.f,left,right,left,right})f32(n);
    }
    out.close();auto result=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path));
    std::filesystem::remove(path);return result;
}
struct Observer {
    std::shared_ptr<world::Outbox> box=std::make_shared<world::Outbox>();
    bool visible=false;unsigned spawns=0,despawns=0,moves=0,max_batch_moves=0;
    void consume(const std::vector<world::Packet>& packets){
        const auto facing=agent::facing_2022(100,0);const unsigned facing_opcode=(facing[0]|unsigned(facing[1])<<8)&0x7fff;
        unsigned batch_moves=0;
        for(const auto& p:packets){if(p.size()<6||word(p,2)!=100)continue;
            const unsigned op=(p[0]|unsigned(p[1])<<8)&0x7fff;
            if(op==32){check(!visible);visible=true;++spawns;}
            else if(op==33){check(visible);visible=false;++despawns;}
            else if(op==44||op==facing_opcode){check(visible);if(op==44){++moves;++batch_moves;}}
        }
        max_batch_moves=std::max(max_batch_moves,batch_moves);
    }
    void drain(){consume(box->drain());}
};
int main()try{
    world::Instance w;w.configure_navigation(obstacle());w.configure_npc_visibility(250);
    world::Instance::Npc n;n.id=n.definition.id=100;n.definition.level=1;n.x=160;n.y=100;
    n.definition.visual_adjustment={0,0,0,100};combat::npcs::Definition d;
    d.binding.map=160;d.binding.actor=d.binding.definition=100;d.binding.level=1;
    d.relation=combat::npcs::Relation::hostile_to_players;d.spawn_health=combat::Health{80,80};d.damageable=true;
    n.combat_definition=d;w.configure_npcs({n});
    Observer near,far;
    w.join(1,"Near",100,100,near.box,0,{}, {},100,1,false);
    w.join(2,"Far",500,100,far.box,0,{}, {},100,1,false);
    near.drain();far.drain();near.consume(w.npc_entry_packets(1));far.consume(w.npc_entry_packets(2));
    check(near.visible&&!far.visible&&near.spawns==1&&far.spawns==0);
    const auto source=*w.effect_scope(100);combat::ai::FlarePolicy policy;
    policy.source=source;policy.level=1;policy.speed=220;policy.acquire=1;policy.follow=1500;policy.leash=600;
    policy.flight={1000,32};policy.evidence="synthetic authored visibility patrol";
    check(w.configure_npc_hostile_ordinary(policy,{5,5,0,0,false,combat::ai::ordinary::Delivery::melee,
        750,2000,128,{1000,32},policy.evidence},1000));
    check(w.configure_npc_wander(source,{{{280,0},{0,0}},100,10000,policy.evidence},1000));
    const auto tick=[&](unsigned at){w.tick(at);near.drain();far.drain();check(!w.npc_flare_ai_snapshot(source)->engaged);};
    tick(1000);tick(4000); // Full outward detour: visible peer gets every corner.
    check(!near.visible&&far.visible);check(near.max_batch_moves>=3);
    check(far.moves==0&&far.spawns==1); // Hidden movement suppressed before entry spawn.
    tick(4100);tick(7100); // Reverse route and visibility roles.
    check(near.visible&&!far.visible);check(far.max_batch_moves>=3);
    check(near.spawns==2&&near.despawns==1&&far.despawns==1);
    const auto hidden_moves=far.moves;tick(7200);check(far.moves==hidden_moves);
    check(near.moves>0&&far.moves>0);
    std::cout<<"Real multi-corner patrol respects per-peer spawn/movement/despawn/re-entry order\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
