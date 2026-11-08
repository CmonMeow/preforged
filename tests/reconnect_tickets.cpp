#include "gw_reconnect_tickets.h"
#include "gw_world_simulation.h"
#include <iostream>
#include <memory>
#include <stdexcept>
struct Grant {unsigned owner=0,map=0;std::uint64_t expires=0;std::shared_ptr<int> reservation;};
int main()try{
    auto check=[](bool value,const char* message){if(!value)throw std::runtime_error(message);};
    fw::gw::ReconnectTickets<Grant> tickets;using Key=decltype(tickets)::Key;Key key{42,1};
    // A successful issuance must publish a consumable pending ticket. Reject
    // already expired grants immediately and release their reservations.
    for(const auto expiry:{std::uint64_t{99},std::uint64_t{100}}){
        auto expired_lease=std::make_shared<int>(1);std::weak_ptr<int> expired_weak=expired_lease;
        check(!tickets.issue(key,{7,160,expiry,std::move(expired_lease)},100),"expired single ticket rejected at issuance");
        check(expired_weak.expired(),"rejected ticket releases reservation immediately");
        check(!tickets.consume(key,100,[](const auto&){return true;}),"expired issuance publishes no ticket");
    }
    auto lease=std::make_shared<int>(1);std::weak_ptr<int> weak=lease;
    check(tickets.issue(key,{7,160,1000,lease},0),"ticket issue");lease.reset();
    auto owned=[](const Grant& g){return g.owner==7&&g.map==160;};
    check(!tickets.consume(key,1,[](const auto&){return false;}),"wrong ownership rejected");
    auto first=tickets.consume(key,2,owned);check(bool(first),"wrong owner must not revoke rightful ticket");
    check(!tickets.consume(key,3,owned),"parallel connection rejected");
    tickets.prune(100000);check(!weak.expired(),"active ticket retains reservation beyond pending expiry");
    first.reset();tickets.disconnect(key,100000,true);
    auto retry=tickets.consume(key,100001,owned);check(bool(retry),"disconnected client reconnects to same world/actor");
    check(retry->map==160&&retry->owner==7,"reconnect retains owned destination");
    retry.reset();tickets.disconnect(key,100002,true);
    check(!tickets.consume(key,130002,owned),"reconnect window expires exactly");
    check(weak.expired(),"expired reconnect releases reserved player and world");
    check(tickets.issue(key,{7,160,150000,{}},140000),"issue after expiry");
    check(bool(tickets.consume(key,140001,owned)),"second active ticket");
    tickets.disconnect(key,140002,false);check(!tickets.consume(key,140003,owned),"transferred session cannot reconnect into old map");
    check(tickets.issue(key,{7,160,140005,{}},140004),"pending ticket");
    check(!tickets.consume(key,140005,owned),"pending transfer expiration");
    const std::array<Key,2> pair{{{90,1},{90,2}}};
    check(tickets.issue_pair(pair,{{{7,2,160000,{}},{8,2,160000,{}}}},150000),"paired match tickets");
    check(!tickets.issue_pair({pair[0],Key{90,3}},{{{9,2,160000,{}},{9,2,160000,{}}}},150001),"pair collision rejected");
    check(!tickets.consume({90,3},150002,[](const auto&){return true;}),"collision leaves no partial second ticket");
    auto player1=tickets.consume(pair[0],150003,[](const Grant& g){return g.owner==7;});
    auto player2=tickets.consume(pair[1],150003,[](const Grant& g){return g.owner==8;});
    check(player1&&player2,"rejected pair does not overwrite original owners");
    check(!tickets.issue_pair({Key{91,1},Key{91,2}},{{{7,2,150003,{}},{8,2,160000,{}}}},150003),"expired pair rejected");
    check(!tickets.consume({91,2},150003,[](const auto&){return true;}),"expired pair publishes neither member");
    fw::gw::ReconnectTickets<Grant> capacity;
    for(unsigned i=1;i<=4095;++i)check(capacity.issue({100,i},{i,2,1000,{}},0),"fill ticket capacity");
    check(!capacity.issue_pair({Key{101,1},Key{101,2}},{{{7,2,1000,{}},{8,2,1000,{}}}},0),"one free slot cannot admit a pair");
    check(!capacity.consume({101,1},1,[](const auto&){return true;}),"capacity failure publishes neither ticket");
    // A solo connection must retain the actual instance, not merely its IDs.
    // Declare Instances first: ticket-owned leases release before their owner.
    fw::gw::world::Instances worlds;
    fw::gw::ReconnectTickets<Grant> solo;
    unsigned initialized=0;
    const auto acquire=[&]{return std::make_shared<fw::gw::world::Instances::Lease>(worlds.acquire(200,[&](auto&){++initialized;}));};
    const Key solo_key{200,1};
    check(solo.issue(solo_key,{7,160,1000,{}},0),"solo ticket issued");
    auto connection=acquire();std::weak_ptr<fw::gw::world::Instances::Lease> retained=connection;
    check(!solo.retain(solo_key,connection),"pending ticket cannot retain a world");
    check(bool(solo.consume(solo_key,1,owned))&&solo.retain(solo_key,connection),"active solo world retained");
    fw::gw::world::Instance::Npc npc;npc.id=npc.definition.id=100;npc.definition.visual_adjustment={0,0,0,100};
    fw::gw::combat::npcs::Definition definition;definition.binding.map=160;definition.binding.actor=definition.binding.definition=100;
    definition.relation=fw::gw::combat::npcs::Relation::hostile_to_players;definition.damageable=true;definition.spawn_health=fw::gw::combat::Health{123,500};npc.combat_definition=definition;
    connection->instance().configure_npcs({npc});
    const auto npc_scope=*connection->instance().effect_scope(100);
    connection.reset();solo.disconnect(solo_key,100,true);
    check(!retained.expired(),"solo disconnect preserves world during reconnect window");
    check(bool(solo.consume(solo_key,101,owned)),"solo reconnect admitted");
    connection=acquire();
    const auto snapshot=connection->instance().npc_combat_snapshot(npc_scope);
    check(initialized==1&&snapshot&&snapshot->health&&snapshot->health->current==123,"solo reconnect preserves live NPC state without reinitialization");
    check(solo.retain(solo_key,connection),"reconnect replaces retained lease");
    retained=connection;connection.reset();solo.disconnect(solo_key,200,true);
    solo.prune(30199);check(!retained.expired(),"world survives until exact reconnect expiry");
    solo.prune(30200);check(retained.expired(),"expiry releases idle world without another transfer");
    connection=acquire();check(initialized==2,"expired solo world is reclaimed");
    check(solo.issue(solo_key,{7,160,40000,{}},30201)&&bool(solo.consume(solo_key,30202,owned))&&solo.retain(solo_key,connection),"new world retained");
    retained=connection;connection.reset();solo.disconnect(solo_key,30203,false);
    check(retained.expired(),"non-resumable disconnect releases retained world");
    connection=acquire();
    check(solo.issue(solo_key,{7,160,40000,{}},30204)&&bool(solo.consume(solo_key,30205,owned))&&solo.retain(solo_key,connection),"revocable world retained");
    retained=connection;connection.reset();solo.erase(solo_key);
    check(retained.expired(),"explicit revocation releases retained world");
    std::cout<<"PASS exclusive reconnect, ownership, expiry and world reservation cleanup\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
