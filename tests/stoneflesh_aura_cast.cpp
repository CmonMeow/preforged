#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_enchantment_duration.h"
#include <iostream>
#include <source_location>

using namespace fw::gw;
using namespace skills;
static void check(bool good,std::source_location at=std::source_location::current()){
    if(!good)throw std::runtime_error("Stoneflesh Aura cast line "+std::to_string(at.line()));
}
static inventory::Profile equipment_profile(){
    inventory::Profile profile;
    profile.bags.emplace(1,inventory::Bag{1,1,0,20});
    inventory::Item weapon;
    weapon.definition.id=100;weapon.definition.type=26;
    weapon.definition.quantity=1;weapon.definition.interaction=0x200000;
    weapon.definition.modifiers={0x22b80014u};weapon.bag=1;weapon.slot=0;
    profile.items.emplace(100,weapon);profile.weapons.sets[0]={100,0};
    return profile;
}
int main()try{
    for(bool extension:{false,true}){
        world::Instance world;auto out=std::make_shared<world::Outbox>();
        world.join(1,"Elementalist",0,0,out,0,{}, {},100,1);
        world.set_vitals(1,{480,480,40,40},100);
        const auto owner=*world.effect_scope(1);
        Slots bar{};bar[0]={1375,0};check(world.configure_adrenaline(owner,bar,100));
        Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[1375]=1;
        attributes::Ranks ranks{};ranks[9]=0;
        const auto equipment=*items::weapon_passives::equipped_enchanting(
            extension?equipment_profile():inventory::Profile{},7);
        auto quote=[&](unsigned skill,std::uint64_t base,std::uint64_t at){
            const auto active=world.status_snapshot(owner,at);
            return active?effects::enchanting_duration(skill,base,owner,equipment,*active,at):
                std::optional<effects::EnchantmentDuration>{};
        };
        casting::Runtime runtime(1);casting::Hooks hooks;
        std::vector<Packet> packets;unsigned paid=0;
        hooks.resources=[&](auto actor){return world.resources(actor);};
        hooks.target_valid=[](auto actor,float,bool self){return actor==1&&self;};
        hooks.events=[&](const auto& batch){packets.insert(packets.end(),batch.begin(),batch.end());};
        hooks.consume_energy_at=[&](unsigned cost,auto at){paid+=cost;return world.consume_energy(1,cost,owner,at);};
        hooks.effect_snapshot=[&](auto at){return world.status_snapshot(owner,at);};
        hooks.local_enchantment_duration=[&](unsigned skill,std::uint64_t base,std::uint64_t at)->std::optional<std::uint64_t>{
            const auto duration=quote(skill,base,at);
            return duration?duration->for_enchantment(skill,base,owner,at):std::nullopt;
        };
        hooks.status_mutation=[&](unsigned skill,unsigned effect,unsigned rank,
                std::uint64_t at,std::optional<std::uint64_t> ends,bool active){
            if(!active)return world.status_mutation(owner,skill,effect,rank,at,ends,false);
            const auto plan=effects::stoneflesh_aura(rank);
            const auto duration=plan?quote(skill,plan->duration,at):std::nullopt;
            return duration&&world.status_mutation(owner,skill,effect,rank,at,ends,true,{},&*duration);
        };
        hooks.begin_action=[&](auto slot,auto at){check(world.accepted_skill_start(owner,slot,at));};
        check(runtime.start(1000,{1375,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
        check(paid==10&&world.resources(1)->energy==30);
        runtime.update(2999,hooks);check(world.status_snapshot(owner,2999)->empty());
        runtime.update(3000,hooks);
        const auto active=*world.status_snapshot(owner,3000);
        check(active.size()==1&&active.front().skill==1375);
        check(active.front().expires==std::optional<std::uint64_t>{extension?9000u:8000u});
        check(std::count(packets.begin(),packets.end(),casting::generic(21,1,1184))==1);
        const auto finish=std::find(packets.begin(),packets.end(),casting::terminal(58,1));
        const auto body=std::find(packets.begin(),packets.end(),casting::generic(21,1,1184));
        const auto status=std::find_if(packets.begin(),packets.end(),[](const auto& packet){return packet.size()>1&&packet[0]==65&&packet[1]==0;});
        const auto recharge=std::find(packets.begin(),packets.end(),casting::slot_event(228,1,{1375,0},15));
        check(recharge!=packets.end()&&finish!=packets.end()&&body!=packets.end()&&status!=packets.end());
        check(recharge<finish&&finish<body&&body<status);
        check(std::count(packets.begin(),packets.end(),casting::terminal(58,1))==1);
        check(runtime.start(4000,{1375,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
        runtime.update(extension?9000u:8000u,hooks);
        check(world.status_snapshot(owner,extension?9000u:8000u)->empty());
    }
    std::cout<<"PASS Stoneflesh Aura paid cast, body1184, base/20% duration and recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

