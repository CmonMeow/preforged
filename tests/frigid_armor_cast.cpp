#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include "gw_enchantment_duration.h"
#include <iostream>
#include <source_location>

using namespace fw::gw;
using namespace skills;
static void check(bool good,std::source_location at=std::source_location::current()){
    if(!good)throw std::runtime_error("Frigid Armor cast line "+std::to_string(at.line()));
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
        Slots bar{};bar[0]={1261,0};check(world.configure_adrenaline(owner,bar,100));
        Policy policy;policy.primary=6;policy.campaigns=255;policy.learned[1261]=1;
        attributes::Ranks ranks{};ranks[11]=0;
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
            const auto plan=effects::frigid_armor(rank);
            const auto duration=plan?quote(skill,plan->duration,at):std::nullopt;
            return duration&&world.status_mutation(owner,skill,effect,rank,at,ends,true,{},&*duration);
        };
        hooks.begin_action=[&](auto slot,auto at){check(world.accepted_skill_start(owner,slot,at));};
        check(runtime.start(1000,{1261,0,1},bar,policy,ranks,true,hooks)==casting::Reason::accepted);
        check(paid==5&&world.resources(1)->energy==35);
        runtime.update(1999,hooks);check(world.status_snapshot(owner,1999)->empty());
        runtime.update(2000,hooks);
        const auto active=*world.status_snapshot(owner,2000);
        check(active.size()==1&&active.front().skill==1261);
        check(active.front().expires==std::optional<std::uint64_t>{extension?14000u:12000u});
        check(std::count(packets.begin(),packets.end(),casting::generic(21,1,369))==1);
        check(runtime.start(3000,{1261,0,1},bar,policy,ranks,true,hooks)==casting::Reason::recharge);
        check(world.apply_condition(owner,owner,combat::conditions::Kind::burning,12500,1000));
        check(world.has_condition(owner,combat::conditions::Kind::burning,12500)==!extension);
        if(extension){
            check(world.apply_condition(owner,owner,combat::conditions::Kind::burning,14000,1000));
            check(world.has_condition(owner,combat::conditions::Kind::burning,14000));
        }
    }
    std::cout<<"PASS Frigid Armor paid cast, body369, base/20% duration and recharge\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
