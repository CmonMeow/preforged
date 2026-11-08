#include "gw_world_simulation.h"
#include <iostream>
#include <source_location>
#include <chrono>
#include <fstream>
using namespace fw::gw;
void check(bool ok,std::source_location at=std::source_location::current()){
    if(!ok)throw std::runtime_error("Signet Shadows World line "+std::to_string(at.line()));
}
world::Instance::Npc make_npc(double x=600,bool health=true){
    world::Instance::Npc n;n.id=n.definition.id=100;n.x=x;n.definition.visual_adjustment={0,0,0,100};
    combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;
    d.relation=combat::npcs::Relation::hostile_to_players;
    if(health)d.spawn_health=combat::Health{1000,1000};
    d.damageable=health;d.conditions_allowed=health;n.combat_definition=d;return n;
}
void join(world::Instance& w,unsigned id,unsigned team,double x,std::shared_ptr<world::Outbox> box){
    w.join(id,id==1?"Assassin":"Foe",float(x),0,box,0,{}, {},100,team);
    w.set_vitals(id,{1000,1000,0,100},100);
}
bool damage_packet(const world::Packet& p){return p.size()>=18&&p[0]==162&&p[2]==55;}
bool impact_packet(const world::Packet& p){return p.size()==18&&p[0]==159&&p[2]==20&&p[14]==0xb2&&p[15]==3;}
std::shared_ptr<const navigation::Mesh> disconnected_mesh(){
    const auto path=std::filesystem::temp_directory_path()/
        ("gw-signet-shadows-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".fwnav");
    std::ofstream out(path,std::ios::binary);check(bool(out));
    const auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)out.put(char(v>>(8*i)));};
    const auto f32=[&](float v){u32(std::bit_cast<std::uint32_t>(v));};
    out.write("FWNAV001",8);for(auto n:{1u,7u,0xeefe704cu,12u,1u,1u})u32(n);
    f32(-2000);f32(-2000);f32(2000);f32(2000);u32(1);u32(2);
    const auto cell=[&](float left,float right){for(unsigned i=0;i<5;++i)u32(0xffffffff);
        f32(-2000);f32(2000);f32(left);f32(right);f32(left);f32(right);};
    cell(-2000,200);cell(400,2000);out.close();
    const auto mesh=std::make_shared<const navigation::Mesh>(navigation::Mesh::load(path));
    std::filesystem::remove(path);return mesh;
}
int main()try{
    // Archived Line_of_sight describes projectile spells/ranged attacks. This
    // signet has no projectile, so a walking-path gap must not reject damage.
    const auto mesh=disconnected_mesh();navigation::LayeredMesh terrain(*mesh);
    check(!terrain.trace({0,0},{0,0},{600,0}));
    for(bool is_npc:{false,true}){
        world::Instance w;w.configure_navigation(mesh);
        if(is_npc)w.configure_npcs({make_npc()});else join(w,2,2,600,std::make_shared<world::Outbox>());
        join(w,1,1,0,std::make_shared<world::Outbox>());
        const auto source=*w.effect_scope(1),target=*w.effect_scope(is_npc?100:2);
        check(w.projectile_target_valid(source,target.actor,1248,1000));
        std::vector<world::Packet> visuals;
        check(w.signet_shadows(source,target,*skills::effects::signet_shadows(12),20,1000,&visuals));
        check(visuals.size()==1&&impact_packet(visuals[0]));
        check(is_npc?w.npc_combat_snapshot(target)->health->current==971:w.resources(2)->health==971);
    }
    for(bool is_npc:{false,true})for(unsigned rank=0;rank<=20;++rank)for(bool blind:{false,true}){
        world::Instance w;auto box=std::make_shared<world::Outbox>();
        if(is_npc)w.configure_npcs({make_npc()});else join(w,2,2,600,std::make_shared<world::Outbox>());
        join(w,1,1,0,box);const auto source=*w.effect_scope(1),target=*w.effect_scope(is_npc?100:2);
        if(blind){
            if(is_npc)check(w.blindness(source,target,*skills::effects::blindness(220,0),1000));
            else check(w.apply_condition(source,target,combat::conditions::Kind::blind,1000,10000));
        }
        box->drain();std::vector<world::Packet> visuals;std::function<void()> publication;
        check(w.signet_shadows(source,target,*skills::effects::signet_shadows(rank),20,1000,&visuals,&publication));
        check(visuals.size()==1&&impact_packet(visuals[0])&&bool(publication));
        const double expected=1000-(5+2*rank+(blind?15+3*rank:0));
        if(is_npc)check(w.npc_combat_snapshot(target)->health->current==expected);
        else check(w.resources(2)->health==expected);
        check(w.resources(1)->energy==0); // Signets require no Energy.
        const auto before=box->drain();check(std::none_of(before.begin(),before.end(),damage_packet));
        publication();const auto after=box->drain();check(std::count_if(after.begin(),after.end(),damage_packet)==1);
        check(std::none_of(after.begin(),after.end(),impact_packet)); // Caller owns the returned impact.
        publication();check(box->drain().empty());
    }
    for(unsigned mode=0;mode<7;++mode){
        world::Instance w;auto box=std::make_shared<world::Outbox>();join(w,1,1,0,box);
        join(w,2,mode==0?1:2,mode==1?1249:600,std::make_shared<world::Outbox>());
        auto source=*w.effect_scope(1),target=*w.effect_scope(2);auto plan=*skills::effects::signet_shadows(12);
        if(mode==2)++source.generation;if(mode==3)++target.generation;if(mode==4)++plan.blind_bonus;
        if(mode==5)target=source;
        box->drain();std::vector<world::Packet> visuals;std::function<void()> publication;
        check(!w.signet_shadows(source,target,plan,mode==6?0:20,1000,&visuals,&publication));
        check(visuals.empty()&&!publication&&w.resources(2)->health==1000);
        const auto packets=box->drain();check(std::none_of(packets.begin(),packets.end(),damage_packet));
    }
    for(unsigned mode=0;mode<3;++mode){
        world::Instance w;w.configure_npcs({make_npc(mode==0?1249:600,mode!=1)});
        join(w,1,1,0,std::make_shared<world::Outbox>());
        const auto source=*w.effect_scope(1);auto target=*w.effect_scope(100);if(mode==2)++target.generation;
        std::vector<world::Packet> visuals;
        check(!w.signet_shadows(source,target,*skills::effects::signet_shadows(12),20,1000,&visuals)&&visuals.empty());
    }
    // Blind expires exactly at its boundary, and is not consumed by a hit.
    for(std::uint64_t at:{1000u,1999u,2000u}){
        world::Instance w;join(w,1,1,0,std::make_shared<world::Outbox>());join(w,2,2,600,std::make_shared<world::Outbox>());
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.apply_condition(source,target,combat::conditions::Kind::blind,1000,1000));
        std::vector<world::Packet> visuals;
        check(w.signet_shadows(source,target,*skills::effects::signet_shadows(12),20,at,&visuals));
        check(w.resources(2)->health==1000-(at<2000?80:29));
        check(w.signet_shadows(source,target,*skills::effects::signet_shadows(12),20,at,&visuals));
        check(w.resources(2)->health==1000-2*(at<2000?80:29));
    }
    // Damage protection still applies to signets; Blind does not bypass caps.
    {
        world::Instance w;join(w,1,1,0,std::make_shared<world::Outbox>());join(w,2,2,600,std::make_shared<world::Outbox>());
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);
        check(w.apply_condition(source,target,combat::conditions::Kind::blind,1000,10000));
        skills::Slots bar{};bar[0]={340,0};check(w.configure_adrenaline(target,bar,1000));
        check(w.status_mutation(target,245,444,12,1000,20000,true));
        std::vector<world::Packet> visuals;
        check(w.signet_shadows(source,target,*skills::effects::signet_shadows(20),20,1000,&visuals));
        check(w.resources(2)->health==900); // 120 base+bonus capped to10% maxHP.
        check(w.adrenaline(target)->front().units==10); // Actual protected damage drives the victim observer.
    }
    // Direct World publication emits exactly one victim particle before damage.
    {
        world::Instance w;auto box=std::make_shared<world::Outbox>();join(w,1,1,0,box);join(w,2,2,600,std::make_shared<world::Outbox>());
        const auto source=*w.effect_scope(1),target=*w.effect_scope(2);box->drain();
        check(w.signet_shadows(source,target,*skills::effects::signet_shadows(0),20,1000,nullptr));
        const auto packets=box->drain();
        check(std::count_if(packets.begin(),packets.end(),impact_packet)==1);
        check(std::find_if(packets.begin(),packets.end(),impact_packet)<std::find_if(packets.begin(),packets.end(),damage_packet));
    }
    for(bool npc:{false,true})for(unsigned rr:{0u,12u,20u})for(unsigned hp:{10u,500u,1000u})for(bool blind:{false,true}){
        world::Instance w;
        if(npc){world::Instance::Npc n;n.id=n.definition.id=100;n.x=600;n.definition.visual_adjustment={0,0,0,100};
            combat::npcs::Definition d;d.binding.map=146;d.binding.actor=d.binding.definition=100;d.relation=combat::npcs::Relation::hostile_to_players;
            d.spawn_health={double(hp),1000};d.damageable=true;d.conditions_allowed=true;d.healing_allowed=true;d.healing_evidence="synthetic";
            n.combat_definition=d;w.configure_npcs({n});
        }else{w.join(2,"Foe",600,0,std::make_shared<world::Outbox>(),0,{}, {},100,2);w.set_vitals(2,{hp,1000,100,100},100);}
        auto box=std::make_shared<world::Outbox>();w.join(1,"Caster",0,0,box,0,{}, {},100,1);w.set_vitals(1,{1000,1000,100,100},100);
        const auto source=*w.effect_scope(1),target=*w.effect_scope(npc?100:2);
        if(blind){if(npc)check(w.blindness(source,target,*skills::effects::blindness(220,0),100));else check(w.apply_condition(source,target,combat::conditions::Kind::blind,100,10000));}
        if(npc)check(w.stage_npc_reversal(target,rr,100));else check(w.status_mutation(target,307,5,rr,100,8100,true));
        box->drain();
        const auto plan=*skills::effects::signet_shadows(12);std::vector<world::Packet> visuals;std::function<void()> publication;
        check(w.signet_shadows(source,target,plan,20,1000,&visuals,&publication)&&visuals.size()==1&&bool(publication));
        const double raw=plan.damage+(blind?plan.blind_bonus:0);
        const double converted=std::min(raw,double(skills::effects::reversal_fortune(rr)->maximum_healing));
        const double expected=std::max(0.,std::min(1000.,hp+converted)-(raw-converted));
        const double actual=npc?w.npc_combat_snapshot(target)->health->current:w.resources(2)->health;check(std::abs(actual-expected)<1.1);
        check(box->drain().empty());publication();check(!box->drain().empty());publication();check(box->drain().empty());
        if(npc)check(w.take_npc_deaths(source).size()==unsigned(expected<=0));
    }
    std::cout<<"PASS Signet of Shadows World84rank/Blind/player-NPC cases, deferred publication, lifetime/range/health guards, expiry and protection\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}


