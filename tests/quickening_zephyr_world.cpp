#include "gw_world_simulation.h"
#include <source_location>
#include <stdexcept>
#include <iostream>

using namespace fw::gw;
static void check(bool value,std::source_location at=std::source_location::current()){
    if(!value)throw std::runtime_error("Quickening Zephyr World line "+std::to_string(at.line()));
}

int main()try{
    const auto canonical=skills::effects::quickening_zephyr(0);
    check(canonical.has_value());
    const auto plan=*canonical;
    check(plan.level==1&&plan.lifetime==15000);
    check(skills::effects::quickening_zephyr(15)->level==10&&
          skills::effects::quickening_zephyr(15)->lifetime==45000);
    check(skills::effects::quickening_zephyr(20)->level==13&&
          skills::effects::quickening_zephyr(20)->lifetime==55000);
    check(!skills::effects::quickening_zephyr(21));
    for(unsigned mode=0;mode<6;++mode){
        world::Instance world;
        world::Instance::Npc npc;
        npc.id=npc.definition.id=100;
        npc.x=mode==1?2513.f:2512.f;
        npc.plane=mode==2?1:0;
        npc.definition.level=std::uint8_t(plan.level+(mode==5));
        npc.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition body;
        body.binding.map=146;
        body.binding.actor=body.binding.definition=100;
        body.binding.level=npc.definition.level;
        body.relation=combat::npcs::Relation::allied_to_players;
        body.spawn_health=combat::Health{100,100};
        body.damageable=true;
        body.creature_type=mode==3?"human":"spirit";
        body.creature_type_evidence="reviewed fixture";
        npc.combat_definition=body;
        world.configure_npcs({npc});
        world.join(1,"Caster",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1);
        world.set_vitals(1,{100,100,50,50},100);
        const auto owner=*world.effect_scope(1),spirit=*world.effect_scope(100);
        check(world.quickening_zephyr_affects_player(owner,200)==false);
        check(world.record_quickening_zephyr_spirit(owner,spirit,plan,200)==(mode!=3&&mode!=5));
        check(!world.record_quickening_zephyr_spirit(owner,spirit,plan,200));
        check(world.quickening_zephyr_affects_player(owner,250)==(mode==0||mode==4));
        check(world.quickening_zephyr_affects_player(owner,15200)==false);
        world.tick(15200);
        check(world.effect_scope(100)==spirit); // Static fixture remains placed.
        check(world.record_quickening_zephyr_spirit(owner,spirit,plan,15200)==(mode!=3&&mode!=5));
        check(world.quickening_zephyr_affects_player(owner,250)==false);
        auto stale=owner;++stale.generation;
        check(!world.quickening_zephyr_affects_player(stale,250));
        auto recycled=spirit;++recycled.generation;
        check(!world.record_quickening_zephyr_spirit(owner,recycled,plan,200));
        auto forged=plan;++forged.lifetime;
        check(!world.record_quickening_zephyr_spirit(owner,spirit,forged,200));
    }
    {
        world::Instance world;
        auto out=std::make_shared<world::Outbox>();
        world.join(1,"Ranger",0,0,out,0,{}, {},100,1);
        world.set_vitals(1,{100,100,50,50},100);
        const auto owner=*world.effect_scope(1);
        out->drain();
        world::Instance::Npc spirit;
        spirit.definition.id=2937; // Fixture only; capture identity unconfirmed.
        spirit.definition.model_file=1234;
        spirit.definition.level=std::uint8_t(plan.level);
        spirit.definition.visual_adjustment={0,0,0,100};
        combat::npcs::Definition body;
        body.binding.map=146;
        body.binding.definition=spirit.definition.id;
        body.binding.model_file=spirit.definition.model_file;
        body.binding.level=spirit.definition.level;
        body.relation=combat::npcs::Relation::allied_to_players;
        body.spawn_health=combat::Health{50,50};body.damageable=true;
        body.healing_allowed=false;body.healing_evidence="test reviewed template";
        body.fleshy=false;body.fleshy_evidence="test reviewed template";
        body.creature_type="spirit";
        body.creature_type_evidence="test reviewed template";
        body.holy_traits.summoned=true;
        body.summoned_evidence="test reviewed template";
        spirit.combat_definition=body;
        std::vector<world::Packet> packets;
        auto forged=plan;++forged.lifetime;
        check(!world.spawn_quickening_zephyr_spirit(owner,forged,spirit,1000,packets));
        auto bad=spirit;bad.combat_definition->creature_type_evidence.clear();
        check(!world.spawn_quickening_zephyr_spirit(owner,plan,bad,1000,packets));
        check(world.npc_packets().empty()&&packets.empty());
        const auto created=world.spawn_quickening_zephyr_spirit(owner,plan,spirit,1000,packets);
        check(created&&world.effect_scope(created->actor)==created);
        check(std::any_of(packets.begin(),packets.end(),[](const auto& packet){
            return packet.size()>=2&&packet[0]==32&&packet[1]==0;
        }));
        check(out->drain().empty()); // The cast host owns post-finish publication.
        check(world.npc_packets().empty()); // Entry snapshots cannot see a pending spirit.
        check(world.quickening_zephyr_affects_player(owner,1000)==false);
        check(world.publish_quickening_zephyr_spirit(*created,1000));
        check(!world.publish_quickening_zephyr_spirit(*created,1000));
        const auto first_publication=out->drain();
        check(std::any_of(first_publication.begin(),first_publication.end(),[&](const auto& packet){
            return packet.size()>=6&&packet[0]==32&&packet[1]==0&&
                (unsigned(packet[2])|(unsigned(packet[3])<<8)|(unsigned(packet[4])<<16)|(unsigned(packet[5])<<24))==created->actor;
        }));
        check(world.quickening_zephyr_affects_player(owner,1000)==true);
        auto conflicting=spirit;++conflicting.definition.model_file;
        ++conflicting.combat_definition->binding.model_file;
        check(!world.spawn_quickening_zephyr_spirit(owner,plan,conflicting,2000,packets));
        const auto second=world.spawn_quickening_zephyr_spirit(owner,plan,spirit,2000,packets);
        check(second&&second->actor!=created->actor);
        check(std::none_of(packets.begin(),packets.end(),[](const auto& packet){
            return packet.size()>=2&&(packet[0]==85||packet[0]==86)&&packet[1]==0;
        }));
        check(world.effect_scope(created->actor)==created); // Staging cannot replace.
        check(world.publish_quickening_zephyr_spirit(*second,2000));
        check(!world.effect_scope(created->actor));
        const auto replacement=out->drain();
        check(std::count_if(replacement.begin(),replacement.end(),[](const auto& packet){
            return packet.size()>=2&&packet[0]==33&&packet[1]==0;
        })==1);
        world.tick(15999);
        check(!world.effect_scope(created->actor));
        world.tick(16000);
        check(!world.effect_scope(created->actor));
        check(world.effect_scope(second->actor)==second);
        check(world.quickening_zephyr_affects_player(owner,16000)==true);
        world.tick(17000);
        check(!world.effect_scope(second->actor));
        check(world.quickening_zephyr_affects_player(owner,17000)==false);
        const auto removed=out->drain();
        check(std::count_if(removed.begin(),removed.end(),[](const auto& packet){
            return packet.size()>=2&&packet[0]==33&&packet[1]==0;
        })==1); // Replacement removal was already drained above.
        // A nearby allied owner replaces too; another team or a center
        // beyond spirit range leaves the original summoned spirit alive.
        for(unsigned mode=0;mode<3;++mode){
            world::Instance battle;
            battle.join(1,"First",0,0,std::make_shared<world::Outbox>(),0,{}, {},100,1,false);
            battle.join(2,"Second",mode==2?2513.f:2512.f,0,std::make_shared<world::Outbox>(),0,{}, {},100,mode==1?2u:1u,false);
            battle.set_vitals(1,{100,100,50,50},100);
            battle.set_vitals(2,{100,100,50,50},100);
            const auto a=*battle.effect_scope(1),b=*battle.effect_scope(2);
            const auto first=battle.spawn_quickening_zephyr_spirit(a,plan,spirit,1000,packets);
            check(first&&battle.publish_quickening_zephyr_spirit(*first,1000));
            const auto next=battle.spawn_quickening_zephyr_spirit(b,plan,spirit,2000,packets);
            check(next&&battle.effect_scope(first->actor)==first);
            check(battle.publish_quickening_zephyr_spirit(*next,2000));
            check(bool(battle.effect_scope(first->actor))==(mode!=0));
            check(battle.effect_scope(next->actor)==next);
        }
        world::Instance streamed;
        streamed.configure_npc_visibility(1000);
        auto near=std::make_shared<world::Outbox>();
        auto far=std::make_shared<world::Outbox>();
        streamed.join(1,"Ranger",0,0,near,0,{}, {},100,1,false);
        streamed.join(2,"Far",5000,0,far,0,{}, {},100,1,false);
        streamed.set_vitals(1,{100,100,50,50},100);
        streamed.set_vitals(2,{100,100,50,50},100);
        near->drain();far->drain();
        check(streamed.npc_entry_packets(1).empty());
        check(streamed.npc_entry_packets(2).empty());
        const auto scoped=*streamed.effect_scope(1);
        const auto pending=streamed.spawn_quickening_zephyr_spirit(scoped,plan,spirit,1000,packets);
        check(pending&&near->drain().empty()&&far->drain().empty());
        check(streamed.npc_packets().empty());
        check(streamed.publish_quickening_zephyr_spirit(*pending,1000));
        const auto nearby=near->drain(),distant=far->drain();
        check(std::any_of(nearby.begin(),nearby.end(),[&](const auto& packet){
            return packet.size()>=6&&packet[0]==32&&packet[1]==0&&
                (unsigned(packet[2])|(unsigned(packet[3])<<8)|(unsigned(packet[4])<<16)|(unsigned(packet[5])<<24))==pending->actor;
        }));
        check(std::none_of(distant.begin(),distant.end(),[](const auto& packet){
            return packet.size()>=2&&packet[0]==32&&packet[1]==0;
        }));
        check(!streamed.publish_quickening_zephyr_spirit(*pending,1000));
        const auto never_published=streamed.spawn_quickening_zephyr_spirit(scoped,plan,spirit,2000,packets);
        check(never_published&&streamed.quickening_zephyr_affects_player(scoped,16000)==false);
        streamed.tick(16000);near->drain();far->drain();
        streamed.tick(17000);
        check(!streamed.publish_quickening_zephyr_spirit(*never_published,17000));
        const auto late=near->drain();
        check(std::none_of(late.begin(),late.end(),[&](const auto& packet){
            return packet.size()>=6&&packet[0]==33&&packet[1]==0&&
                (unsigned(packet[2])|(unsigned(packet[3])<<8)|(unsigned(packet[4])<<16)|(unsigned(packet[5])<<24))==never_published->actor;
        }));
    }
    std::cout<<"PASS trusted Quickening Zephyr spirit scope, plane, range and expiry\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
