#include "gw_inventory_ground.h"
#include <iostream>
#include <stdexcept>

using namespace fw::gw;
namespace planner=inventory::ground;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static unsigned opcode(const items::Packet& packet){return packet[0]|unsigned(packet[1])<<8;}
static inventory::Item item(unsigned id,unsigned count=100,unsigned bag=1,unsigned slot=0){inventory::Item value;
    value.definition.id=id;value.definition.file=1234;value.definition.model=567;value.definition.type=11;
    value.definition.quantity=count;value.definition.interaction=0x80000;value.definition.modifiers={0x12345678};
    value.definition.encoded_name=u"Synthetic floor fixture";value.profession=6;value.customized=u"Fixture Owner";
    value.bag=static_cast<std::uint16_t>(bag);value.slot=static_cast<std::uint8_t>(slot);return value;
}
static inventory::Profile profile(){auto p=inventory::baseline();p.items.emplace(1,item(1));p.next_item=2;return p;}
static world::ground::Item floor_fixture(unsigned count=100){world::ground::Item result;result.scope={10,32768,9};result.source={10,5,6};result.source_item=1;
    result.definition=item(inventory::identity::ground_first,count).definition;result.profession=6;result.customized=u"Fixture Owner";
    result.position={10,20,3};result.created=1000;return result;
}
static unsigned units(const inventory::Profile& p){unsigned total=0;for(const auto& [id,value]:p.items)total+=value.definition.quantity;return total;}
static void unchanged(const inventory::Profile& original,const inventory::Profile& proposed,const inventory::Batch& packets,const char* message){check(inventory::encode(original)==inventory::encode(proposed)&&packets.empty(),message);}
int main(){try{
    const auto original=profile();
    auto bundle=floor_fixture(1);bundle.definition.type=6;
    const auto held_only=planner::credit(original,bundle,1,true);
    check(held_only.status==planner::Reason::bundle,"bundles require held ownership, never a bag credit");
    unchanged(original,held_only.profile,held_only.packets,"bundle rejection preserves inventory");
    const auto partial=planner::debit(original,1,25,true);check(partial.accepted()&&partial.quantity==25&&partial.profile.items.at(1).definition.quantity==75,"partial drop debits exact units");
    check(partial.packets.size()==1&&opcode(partial.packets[0])==312,"partial drop only updates inventory count");
    check(partial.source.definition.quantity==100&&partial.source.profession==6&&partial.source.customized==u"Fixture Owner","world drop receives original definition and exact metadata");
    check(units(partial.profile)+partial.quantity==units(original),"partial drop conserves units");
    auto full=planner::debit(original,1,100,true);check(full.accepted()&&full.profile.items.empty()&&full.packets.size()==2&&opcode(full.packets[0])==332&&opcode(full.packets[1])==348,"full drop detaches then deletes owned definition");
    for(unsigned quantity:{0u,101u,256u}){const auto denied=planner::debit(original,1,quantity,true);check(!denied.accepted(),"invalid drop quantity denied");unchanged(original,denied.profile,denied.packets,"invalid drop leaves profile unchanged");}
    check(planner::debit(original,1,1,false).status==planner::Reason::mode&&planner::debit(original,99,1,true).status==planner::Reason::missing,"mode and foreign ownership checked");
    auto equipped=original;equipped.items.at(1).bag=2;equipped.items.at(1).slot=5;check(planner::debit(equipped,1,1,true).status==planner::Reason::carrying,"equipped items cannot drop");
    auto container=original;container.items.at(1).definition.type=3;container.items.at(1).definition.quantity=1;container.items.at(1).bag=0;container.items.at(1).slot=0;container.bags.at(1).container=1;check(planner::debit(container,1,1,true).status==planner::Reason::container,"active bag container cannot drop");
    const auto installed_pickup=planner::credit(container,floor_fixture(),100,true);
    check(installed_pickup.accepted()&&installed_pickup.profile.items.at(1).bag==0&&installed_pickup.profile.items.at(1).definition.quantity==1,"pickup preserves installed container and uses real carrying slots");
    for(unsigned bit=0;bit<32;++bit)check(planner::drop_permitted(1u<<bit,true,false)==(bit!=8),"native drop predicate excludes only bit8");
    check(!planner::drop_permitted(0,false,false)&&!planner::drop_permitted(0,true,true),"outpost and cinematic prohibit native drop");
    auto restricted=original;restricted.items.at(1).definition.interaction|=0x100;
    auto denied_drop=planner::debit(restricted,1,1,true);check(denied_drop.status==planner::Reason::restricted,"server rejects restricted item even with admitted map");
    unchanged(restricted,denied_drop.profile,denied_drop.packets,"restricted drop leaves ownership untouched");
    auto weapon=original;weapon.items.at(1).definition.type=5;weapon.items.at(1).definition.quantity=1;weapon.items.at(1).definition.interaction=0;weapon.weapons.sets[0].weapon=1;weapon.weapons.sets[2].weapon=1;
    auto dropped_weapon=planner::debit(weapon,1,1,true);check(dropped_weapon.accepted()&&dropped_weapon.profile.weapons.sets[0].weapon==0&&dropped_weapon.profile.weapons.sets[2].weapon==0,"full drop clears active and inactive actual weapon references");
    check(dropped_weapon.packets.size()==7&&opcode(dropped_weapon.packets[2])==326&&opcode(dropped_weapon.packets.back())==327,"weapon references publish after drop removal");

    auto merged=planner::credit(original,floor_fixture(200),200,true);check(merged.accepted()&&merged.quantity==200&&merged.profile.items.at(1).definition.quantity==250&&merged.profile.items.at(2).definition.quantity==50,"pickup merges250 then creates exact remainder");
    check(units(merged.profile)==units(original)+200,"pickup conserves all requested units");
    check(merged.packets.size()==5&&opcode(merged.packets[0])==312&&opcode(merged.packets[1])==352&&opcode(merged.packets[2])==345&&opcode(merged.packets[3])==313&&opcode(merged.packets[4])==317,"new pickup publishes definition and metadata before placement");
    check(merged.profile.items.at(2).definition.id==2&&merged.profile.items.at(2).definition.id!=floor_fixture().definition.id&&merged.profile.items.at(2).definition.modifiers==floor_fixture().definition.modifiers,"new owned ID replaces global floor ID without altering unknown modifiers");
    auto selected=planner::credit(original,floor_fixture(100),25,true);check(selected.accepted()&&selected.profile.items.at(1).definition.quantity==125&&selected.quantity==25,"caller-selected partial pickup preserves unclaimed units for world");
    auto roomy=inventory::baseline();auto many=planner::credit(roomy,floor_fixture(600),600,true);check(many.accepted()&&many.profile.items.size()==3&&many.profile.items.at(1).definition.quantity==250&&many.profile.items.at(2).definition.quantity==250&&many.profile.items.at(3).definition.quantity==100,"large exact stack splits into bounded carried stacks");
    auto differs=floor_fixture(25);differs.customized=u"Different Owner";auto separate=planner::credit(original,differs,25,true);check(separate.accepted()&&separate.profile.items.at(1).definition.quantity==100&&separate.profile.items.at(2).customized==u"Different Owner","different customization never merges");
    differs=floor_fixture(25);differs.profession=7;separate=planner::credit(original,differs,25,true);check(separate.accepted()&&separate.profile.items.at(1).definition.quantity==100&&separate.profile.items.at(2).profession==7,"different explicit profession never merges");
    auto unstacked=floor_fixture(1);unstacked.definition.interaction=0;unstacked.definition.type=5;auto one=planner::credit(roomy,unstacked,1,true);check(one.accepted()&&one.profile.items.size()==1&&one.profile.items.at(1).definition.quantity==1,"nonstackable exact item credit");
    check(planner::credit(original,floor_fixture(),0,true).status==planner::Reason::quantity&&planner::credit(original,floor_fixture(),101,true).status==planner::Reason::quantity&&planner::credit(original,floor_fixture(),1,false).status==planner::Reason::mode,"pickup quantity and mode denied");
    auto invalid=floor_fixture();invalid.definition.file=0;check(planner::credit(original,invalid,1,true).status==planner::Reason::metadata,"missing actual asset rejected");
    invalid=floor_fixture();invalid.profession=12;check(planner::credit(original,invalid,1,true).status==planner::Reason::metadata,"malformed profession rejected");
    invalid=floor_fixture();invalid.customized=std::u16string(32,u'x');check(planner::credit(original,invalid,1,true).status==planner::Reason::metadata,"oversize customization rejected");
    invalid=floor_fixture();invalid.scope.generation=0;check(planner::credit(original,invalid,1,true).status==planner::Reason::metadata,"unknown floor lifetime rejected");

    auto limited=original;limited.bags.at(1).capacity=1;limited.items.at(1).definition.quantity=240;
    {
        auto packed=limited;packed.items.at(1).definition.quantity=250;packed.bags.emplace(42,inventory::Bag{42,1,4,5,0});
        auto denied=planner::credit(packed,floor_fixture(1),1,true,u"Fixture Owner");
        check(denied.status==planner::Reason::full,"empty equipment pack is not material pickup capacity");unchanged(packed,denied.profile,denied.packets,"rejected pack pickup publishes nothing");
        packed.items.emplace(2,item(2,100,42));packed.next_item=3;
        denied=planner::credit(packed,floor_fixture(1),1,true,u"Fixture Owner");
        check(denied.status==planner::Reason::full,"legacy material stack in pack cannot receive pickup merge");unchanged(packed,denied.profile,denied.packets,"pack merge denial preserves every owned count");
        packed.items.erase(2);
        auto gear=floor_fixture(1);gear.definition.type=5;gear.definition.interaction=0;
        const auto admitted=planner::credit(packed,gear,1,true,u"Fixture Owner");
        check(admitted.accepted()&&admitted.profile.items.at(3).bag==42,"owned weapon pickup can use eligible pack capacity");
        for(const auto owner:{u"Other Owner",u""}){
            denied=planner::credit(packed,gear,1,true,owner);check(denied.status==planner::Reason::full,"foreign or missing character cannot pick customized gear into pack");
            unchanged(packed,denied.profile,denied.packets,"owner rejection preserves inventory and packet batch");
        }
        gear.customized.reset();check(planner::credit(packed,gear,1,true,u"Fixture Owner").status==planner::Reason::full,"unknown customization cannot enter pack on pickup");
        gear.customized=u"";check(planner::credit(packed,gear,1,true).accepted(),"explicit uncustomized pickup remains eligible for pack");
        packed.bags.emplace(43,inventory::Bag{43,1,2,1,0});
        gear.customized=u"Foreign Owner";const auto ordinary=planner::credit(packed,gear,1,true,u"Fixture Owner");
        check(ordinary.accepted()&&ordinary.profile.items.at(3).bag==43,"pickup skips ineligible pack and uses ordinary bag by native index");
        check(inventory::decode(inventory::encode(admitted.profile)).items.at(3).bag==42,"pack pickup placement survives durable codec");
    }
    const auto no_room=planner::credit(limited,floor_fixture(25),25,true);check(no_room.status==planner::Reason::full,"insufficient full pickup space rejects");unchanged(limited,no_room.profile,no_room.packets,"failed capacity after possible merge leaves all units and packets untouched");
    auto no_ids=original;no_ids.next_item=inventory::identity::owned_last+1;no_ids.items.at(1).definition.quantity=240;
    auto exhausted=planner::credit(no_ids,floor_fixture(25),25,true);check(exhausted.status==planner::Reason::identity,"new owned ID exhaustion denied before publication");unchanged(no_ids,exhausted.profile,exhausted.packets,"ID exhaustion after possible merge rolls back whole pickup");
    auto fitting=planner::credit(no_ids,floor_fixture(10),10,true);check(fitting.accepted()&&fitting.profile.items.at(1).definition.quantity==250,"merge-only pickup needs no new identity");
    auto last_id=inventory::baseline();last_id.next_item=inventory::identity::owned_last;auto final=planner::credit(last_id,floor_fixture(1),1,true);check(final.accepted()&&final.profile.items.contains(inventory::identity::owned_last)&&final.profile.next_item==inventory::identity::ground_first,"last legal private wire ID accepted");
    auto only_equipped=inventory::baseline();only_equipped.bags.erase(1);const auto no_carry=planner::credit(only_equipped,floor_fixture(1),1,true);check(no_carry.status==planner::Reason::full,"equipment slots cannot substitute carrying capacity");
    auto ordering=inventory::baseline();ordering.bags.clear();ordering.bags={{1,{1,1,3,1,0}},{10,{10,1,0,1,0}}};auto ordered=planner::credit(ordering,floor_fixture(1),1,true);check(ordered.accepted()&&ordered.profile.items.at(1).bag==10,"vacant slots selected by native bag index, not wire order");

    // End-to-end planner plus identity projection: no floor IDs become durable
    // character IDs, and a discarded CAS proposal mutates neither registry nor
    // the caller's source profile. Full debit+credit preserves exact content.
    inventory::identity::Session identities;auto wire=identities.hydrate(original,storage::baseline());
    const auto before=identities.revision();const auto dropped=planner::debit(wire.character,*identities.wire_item(inventory::identity::Domain::character,1),100,true);
    auto debit_projection=identities.prepare(dropped.profile,wire.bank);
    check(identities.revision()==before&&identities.wire_item(inventory::identity::Domain::character,1).has_value(),"debit projection discarded on failed CAS preserves identity registry");
    check(identities.commit(std::move(debit_projection)),"drop identity projection commit");wire=std::move(debit_projection.wire);
    auto exact_floor=floor_fixture(100);exact_floor.definition=dropped.source.definition;exact_floor.definition.id=inventory::identity::ground_first;exact_floor.profession=dropped.source.profession;exact_floor.customized=dropped.source.customized;
    const auto picked=planner::credit(wire.character,exact_floor,100,true);auto credit_projection=identities.prepare(picked.profile,wire.bank);
    check(credit_projection.durable.character.items.at(2).definition.id==2&&credit_projection.durable.character.items.at(2).definition.quantity==100&&credit_projection.durable.character.items.at(2).profession==original.items.at(1).profession&&credit_projection.durable.character.items.at(2).customized==original.items.at(1).customized,"roundtrip gives fresh durable identity and preserves exact source metadata");
    check(units(credit_projection.durable.character)==units(original)&&credit_projection.durable.character.items.at(2).definition.modifiers==original.items.at(1).definition.modifiers,"drop/pickup roundtrip conserves units and opaque definition");
    auto armor_profile=inventory::baseline();auto armor=item(1,1);armor.definition.type=7;armor.definition.interaction=0;
    armor.reviewed_armor=inventory::ReviewedArmor{60,10,5};armor_profile.items.emplace(1,armor);armor_profile.next_item=2;
    const auto armor_drop=planner::debit(armor_profile,1,1,true);
    check(armor_drop.accepted(),"reviewed armor can be dropped from carrying inventory");
    auto armor_floor=floor_fixture(1);armor_floor.definition=armor_drop.source.definition;armor_floor.definition.id=inventory::identity::ground_first;
    armor_floor.profession=armor_drop.source.profession;armor_floor.customized=armor_drop.source.customized;armor_floor.reviewed_armor=armor_drop.source.reviewed_armor;
    world::ground::State armor_ground;check(armor_ground.add(armor_floor),"reviewed armor ground metadata accepted");
    const auto armor_pickup=planner::credit(armor_drop.profile,*armor_ground.find(armor_floor.scope),1,true);
    check(armor_pickup.accepted()&&armor_pickup.profile.items.at(2).reviewed_armor==armor.reviewed_armor&&
        inventory::decode(inventory::encode(armor_pickup.profile)).items.at(2).reviewed_armor==armor.reviewed_armor,
        "reviewed armor ratings survive drop, ground state, pickup and durable encoding");
    for(unsigned malformed=0;malformed<4;++malformed){auto bad=armor_floor;
        if(malformed==0)bad.profession.reset();else if(malformed==1)bad.definition.quantity=2;
        else if(malformed==2)bad.definition.type=5;else bad.reviewed_armor->base=201;
        world::ground::State rejected;check(!rejected.add(bad),"malformed reviewed armor never enters ground state");
        const auto denied=planner::credit(armor_drop.profile,bad,1,true);
        check(denied.status==planner::Reason::metadata,"pickup rejects malformed armor metadata before staging");
        unchanged(armor_drop.profile,denied.profile,denied.packets,"malformed armor pickup preserves profile");
    }
    std::cout<<"inventory ground tests passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
