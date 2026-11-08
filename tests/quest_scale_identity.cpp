#include "gw_presearing_items.h"
#include "gw_inventory_identity.h"
#include <iostream>
#include <stdexcept>

namespace inv=fw::gw::inventory;
namespace ids=inv::identity;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}

int main()try{
    auto character=inv::baseline();
    for(unsigned id=1;id<=2;++id){
        inv::Item scale;scale.definition=inv::presearing::shimmering_scale_definition();
        scale.definition.id=id;scale.bag=1;scale.slot=static_cast<std::uint8_t>(id-1);
        character.items.emplace(id,scale);
    }
    character.next_item=3;
    ids::Session session;auto views=session.hydrate(character,fw::gw::storage::baseline());
    const auto first=*session.wire_item(ids::Domain::character,1);
    const auto second=*session.wire_item(ids::Domain::character,2);
    const auto carried=*session.wire_bag(ids::Domain::character,1);
    const auto bank=*session.wire_bag(ids::Domain::bank,100);
    // The real ownership-transfer planner preserves a wire identity but assigns
    // a new durable identity on return. Item ordering is no longer equivalent.
    const auto deposit=fw::gw::storage::transfer(views.character,views.bank,
        {inv::Change::move,first,0,bank,0},{true,true,true});
    check(deposit.accepted(),"scale fixture deposit rejected");
    auto deposited=session.prepare(deposit.character,deposit.bank);
    check(session.commit(std::move(deposited)),"scale fixture deposit commit failed");
    views=std::move(deposited.wire);
    const auto withdrawal=fw::gw::storage::transfer(views.character,views.bank,
        {inv::Change::move,first,0,carried,0},{true,true,true});
    check(withdrawal.accepted(),"scale fixture withdrawal rejected");
    auto withdrawn=session.prepare(withdrawal.character,withdrawal.bank);
    const auto durable=withdrawn.durable.character;
    check(session.commit(std::move(withdrawn)),"scale fixture withdrawal commit failed");
    views=std::move(withdrawn.wire);
    check(first<second&&session.origin(first)->logical==3&&session.origin(second)->logical==2,
        "fixture did not reverse wire and durable scale order");
    const auto before=inv::encode(views.character);const auto revision=session.revision();
    const auto wire_consumed=inv::presearing::consume_shimmering_scale(views.character);
    const auto durable_consumed=inv::presearing::consume_shimmering_scale(durable);
    check(wire_consumed&&durable_consumed,"carried scales not recognized");
    auto prepared=session.prepare(*wire_consumed,views.bank);
    check(inv::encode(prepared.durable.character)==inv::encode(*durable_consumed),
        "quest turn-in consumed different wire and durable scales");
    check(!prepared.wire.character.items.contains(first)&&prepared.wire.character.items.contains(second),
        "turn-in did not select first carried slot");
    check(session.revision()==revision&&inv::encode(views.character)==before,
        "uncommitted reward proposal changed live state");
    check(session.commit(std::move(prepared)),"scale consumption commit failed");
    check(!session.origin(first)&&session.origin(second)->logical==2,
        "committed reward retired wrong native identity");
    ids::Session reconnect;
    const auto restored=reconnect.hydrate(inv::decode(inv::encode(*durable_consumed)),withdrawn.durable.bank);
    check(restored.character.items.size()==1&&restored.character.items.begin()->second.slot==1,
        "reconnect restored a different surviving scale");
    std::cout<<"PASS scale turn-in selects the same carried item before and after identity projection\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
