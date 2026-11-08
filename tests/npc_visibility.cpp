#include "gw_world_simulation.h"
#include "gw_skill_casting.h"
#include <iostream>
#include <stdexcept>
using namespace fw::gw;
unsigned id(const world::Packet& p,unsigned at=2){return p.at(at)|unsigned(p.at(at+1))<<8|unsigned(p.at(at+2))<<16|unsigned(p.at(at+3))<<24;}
bool has(const std::vector<world::Packet>& packets,unsigned opcode,unsigned actor){
    return std::any_of(packets.begin(),packets.end(),[&](const auto& p){return p.size()>=6&&p[0]==opcode&&p[1]==0&&id(p)==actor;});
}
world::Instance::Npc make_npc(unsigned actor,float x){
    world::Instance::Npc n;n.id=actor;n.x=x;
    n.definition={17,1234,0,{0,0,0,100},0,524,1,2,u"\x108\x107NPC\x1"};
    return n;
}
int main()try{
    auto check=[](bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);};
    world::Instance area;area.configure_npc_visibility(5020);
    area.configure_npcs({make_npc(100,4000),make_npc(101,9000),make_npc(102,5020),make_npc(103,5020.25f)});
    const auto old_scope=area.interaction_scope(100);
    check(old_scope.has_value()&&!area.effect_scope(100),"ordinary NPC interaction lifetime must not invent combat metadata");
    area.configure_npcs({make_npc(100,4000),make_npc(101,9000),make_npc(102,5020),make_npc(103,5020.25f)});
    check(area.interaction_scope(100)!=old_scope&&!area.interaction_scope(999),"NPC replacement must invalidate old menus");
    auto first=std::make_shared<world::Outbox>(),second=std::make_shared<world::Outbox>();
    area.join(1,"First",0,0,first,0,{},0,0,0,false);
    area.join(2,"Second",9000,0,second,0,{},0,0,0,false);first->drain();second->drain();
    auto entry=area.npc_entry_packets(1),other=area.npc_entry_packets(2);
    check(has(entry,32,100)&&has(entry,32,102)&&!has(entry,32,101)&&!has(entry,32,103),"entry respects exact5020 boundary");
    check(has(other,32,101)&&has(other,32,103),"NPC visibility is per client");
    check(!area.select_target(1,101,0)&&area.select_target(1,100,0),"far NPC selection rejected");
    first->enqueue({skills::particles::aftershock_screen_shake_2022(101)});
    check(first->drain().empty(),"camera shake must not reference an unpublished distant NPC");
    const auto nearby_shake=skills::particles::aftershock_screen_shake_2022(100);
    first->enqueue({nearby_shake});check(first->drain()==std::vector<world::Packet>{nearby_shake},"nearby NPC camera shake retained with direction flag");
    check(area.approach_npc(1,101,0)==world::Instance::NpcApproach::unavailable,"far NPC cannot initiate map-wide approach");
    first->enqueue({combat::detail::integer(42,101,120),combat::detail::fraction(101,{100,120}),agent::facing_2022(101,1)});
    check(first->drain().empty(),"unpublished NPC health and movement references are suppressed");
    first->enqueue({skills::casting::activation(1,101,194),skills::casting::activation(101,1,194)});
    check(first->drain().empty(),"skill activation must not reference a hidden caster or target");
    const auto visible_cast=skills::casting::activation(1,100,194);
    first->enqueue({visible_cast});check(first->drain()==std::vector<world::Packet>{visible_cast},"visible-target skill activation must remain intact");
    area.move(1,4500,0,0);first->drain();second->drain();area.tick(25000);
    auto entered=first->drain();check(has(entered,32,101)&&has(entered,32,103)&&!has(entered,32,100),"newly nearby NPCs spawn once");
    check(area.select_target(1,101,0),"nearby NPC is selectable after publication");
    area.tick(25100);check(!has(first->drain(),32,101),"unchanged visible set does not duplicate spawns");
    area.move(1,-1000,0,25100);first->drain();second->drain();area.tick(51000);
    auto removed=first->drain();check(has(removed,33,101)&&has(removed,33,102)&&has(removed,33,103)&&!has(removed,33,100),"distant NPCs removed while nearby NPC remains");
    check(!area.select_target(1,101,0),"removed NPC cannot remain a selection target");
    first->enqueue({combat::detail::fraction(101,{50,120})});check(first->drain().empty(),"late update does not reference removed NPC");
    area.move(1,4500,0,51000);first->drain();area.tick(77000);
    check(has(first->drain(),32,101),"returning to an NPC republishes its definition and agent");
    std::cout<<"PASS per-client NPC publication, removal, reference filtering and selection bounds\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}

