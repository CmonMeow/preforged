#include "gw_inventory_identity.h"
#include <iostream>
#include <stdexcept>

using namespace fw::gw;
namespace ids=inventory::identity;
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class Function> static void rejects(Function&& fn,const char* message){bool rejected=false;try{fn();}catch(const std::exception&){rejected=true;}check(rejected,message);}
static inventory::Item item(unsigned logical,unsigned model,unsigned bag,unsigned slot,unsigned type=11,unsigned count=100){
    inventory::Item result;result.definition.id=logical;result.definition.file=1234;result.definition.model=model;
    result.definition.type=static_cast<std::uint8_t>(type);result.definition.quantity=count;result.definition.interaction=count==1?0:0x80000;
    result.definition.encoded_name=u"Synthetic identity fixture";result.bag=static_cast<std::uint16_t>(bag);result.slot=static_cast<std::uint8_t>(slot);
    result.profession=6;result.customized=u"Fixture Owner";return result;
}
static inventory::Profile character(){
    inventory::Profile p;p.inventory=17;p.bags={{100,{100,1,0,20,3}},{200,{200,2,21,9,0}}};
    p.items.emplace(1,item(1,101,100,0));p.items.emplace(2,item(2,102,100,1,5,1));p.items.emplace(3,item(3,103,0,0,3,1));
    p.next_item=4;p.weapons.sets[0].weapon=2;p.weapons.sets[2].weapon=2;inventory::validate(p);return p;
}
static inventory::Profile bank(){auto p=storage::baseline();p.inventory=23;p.items.emplace(1,item(1,201,100,0));p.items.emplace(2,item(2,202,100,1));p.next_item=3;return p;}
int main(){try{
    const auto durable_character=character(),durable_bank=bank();
    ids::Session registry;auto views=registry.hydrate(durable_character,durable_bank);
    const auto char1=*registry.wire_item(ids::Domain::character,1),bank1=*registry.wire_item(ids::Domain::bank,1);
    const auto charbag=*registry.wire_bag(ids::Domain::character,100),bankbag=*registry.wire_bag(ids::Domain::bank,100);
    check(char1!=bank1&&charbag!=bankbag,"independent logical item1 and bag100 hydrate distinctly");
    check(views.character.inventory==1&&views.bank.inventory==1,"one native local inventory identity");
    check(views.character.items.at(char1).definition.model==101&&views.bank.items.at(bank1).definition.model==201,"colliding logical definitions never overwrite each other");
    check(views.character.bags.at(charbag).container==registry.wire_item(ids::Domain::character,3),"bag container item translated");
    const auto container_wire=*registry.wire_item(ids::Domain::character,3);
    check(views.character.items.at(container_wire).bag==0&&views.character.items.at(container_wire).slot==0&&
        registry.item(container_wire)==ids::Origin{ids::Domain::character,3},"installed container retains zero placement and real owner through hydration");
    check(views.character.weapons.sets[0].weapon==registry.wire_item(ids::Domain::character,2)&&views.character.weapons.sets[2].weapon==views.character.weapons.sets[0].weapon,"all active and inactive weapon references translated");
    check(views.character.next_item==views.bank.next_item&&views.character.next_item==registry.next_item(),"operational allocation cursor synchronized");
    const auto roundtrip=registry.prepare(views.character,views.bank);
    check(inventory::encode(roundtrip.durable.character)==inventory::encode(durable_character)&&inventory::encode(roundtrip.durable.bank)==inventory::encode(durable_bank),"unmodified hydration roundtrips durable IDs and inventory metadata exactly");
    auto attempted_container=storage::transfer(views.character,views.bank,{inventory::Change::move,container_wire,0,bankbag,2},{true,true,true});
    check(!attempted_container.accepted()&&attempted_container.status==inventory::Reason::unsupported&&
        inventory::encode(attempted_container.character)==inventory::encode(views.character),"installed container cannot become a bank item or lose its bag reference");
    auto forged_container=views.character;forged_container.items.at(container_wire).bag=charbag;forged_container.items.at(container_wire).slot=2;
    const auto installed_revision=registry.revision();
    rejects([&]{(void)registry.prepare(forged_container,views.bank);},"forged installed-container placement is rejected before CAS");
    check(registry.revision()==installed_revision&&registry.item(container_wire)==ids::Origin{ids::Domain::character,3},"failed installed-container projection leaves identity registry unchanged");

    auto transfer=storage::transfer(views.character,views.bank,{inventory::Change::move,char1,0,bankbag,2},{true,true,true});
    check(transfer.accepted(),"real transfer planner works with colliding durable namespaces");
    const auto before_revision=registry.revision();
    auto prepared=registry.prepare(transfer.character,transfer.bank);
    check(!prepared.durable.character.items.contains(1)&&prepared.durable.bank.items.contains(1)&&prepared.durable.bank.items.contains(2)&&prepared.durable.bank.items.contains(3),"deposit gets fresh destination logical3 without losing bank logical1 or2");
    check(prepared.durable.bank.items.at(3).definition.model==101&&prepared.durable.bank.items.at(3).profession==6&&prepared.durable.bank.items.at(3).customized==u"Fixture Owner","cross-owner opaque definition and metadata preserved");
    check(prepared.durable.character.next_item==4&&prepared.durable.bank.next_item==4,"independent durable cursors preserved and destination advanced");
    check(registry.revision()==before_revision&&registry.item(char1)==ids::Origin{ids::Domain::character,1},"failed durable CAS can discard preparation without registry mutation");
    check(prepared.durable.character.items.at(3).bag==0&&prepared.durable.character.bags.at(100).container==3&&
        prepared.durable.character.items.at(3).customized==durable_character.items.at(3).customized,"storage CAS projection preserves installed container identity and metadata");
    check(registry.commit(std::move(prepared)),"successful CAS commits prepared registry");
    views=std::move(prepared.wire);
    check(registry.item(char1)==ids::Origin{ids::Domain::bank,3}&&views.bank.items.contains(char1),"full deposit retains visible wire ID while rebinding durable owner");
    auto reloaded=registry.hydrate(prepared.durable.character,prepared.durable.bank);
    check(registry.wire_item(ids::Domain::bank,3)==char1&&registry.wire_item(ids::Domain::bank,1)==bank1,"authoritative reload preserves surviving wire identities");
    check(registry.wire_item(ids::Domain::character,3)==container_wire&&reloaded.character.items.at(container_wire).bag==0&&
        reloaded.character.bags.at(charbag).container==container_wire,"successful transfer and reconnect preserve installed container backlink identity");
    views=std::move(reloaded);

    auto withdrawal=storage::transfer(views.character,views.bank,{inventory::Change::move,bank1,0,charbag,0},{true,true,true});
    check(withdrawal.accepted(),"real alternate-owner withdrawal planner");
    auto withdrawn=registry.prepare(withdrawal.character,withdrawal.bank);
    check(withdrawn.durable.character.items.contains(4)&&withdrawn.durable.character.items.at(4).definition.model==201&&!withdrawn.durable.bank.items.contains(1),"withdrawal allocates fresh character logical4");
    check(registry.commit(std::move(withdrawn)),"withdrawal CAS commits mapping");views=std::move(withdrawn.wire);
    check(registry.item(bank1)==ids::Origin{ids::Domain::character,4},"wire reference remains stable through withdrawal");

    const auto created_wire=registry.next_item();
    auto split=inventory::propose(views.character,{inventory::Change::split,bank1,25,charbag,3},{true,true});
    check(split.accepted()&&split.profile.items.contains(created_wire),"ordinary split allocates from common session cursor");
    auto split_prepared=registry.prepare(split.profile,views.bank);
    check(split_prepared.durable.character.items.at(5).definition.quantity==25&&split_prepared.durable.character.items.at(4).definition.quantity==75,"new split wire identity gets new owner-local logical5");
    check(registry.commit(std::move(split_prepared)),"split commit succeeds");views=std::move(split_prepared.wire);
    check(views.bank.next_item==views.character.next_item&&registry.item(created_wire)==ids::Origin{ids::Domain::character,5},"split synchronizes both views without moving bank durable cursor");

    const auto cross_wire=registry.next_item();
    auto cross_split=storage::transfer(views.character,views.bank,{inventory::Change::split,bank1,25,bankbag,3},{true,true,true});
    check(cross_split.accepted(),"real cross-owner split with colliding original namespaces");
    auto cross_prepared=registry.prepare(cross_split.character,cross_split.bank);
    check(cross_prepared.durable.bank.items.at(4).definition.quantity==25&&cross_prepared.durable.character.items.at(4).definition.quantity==50,"cross-owner split gets bank logical4 and retains original character logical4 independently");
    check(registry.commit(std::move(cross_prepared)),"cross-owner split commits");views=std::move(cross_prepared.wire);
    check(registry.item(cross_wire)==ids::Origin{ids::Domain::bank,4},"new cross-owner split wire identity resolves actual bank owner");

    auto destroyed=inventory::propose(views.character,{inventory::Change::destroy,created_wire},{true,true});check(destroyed.accepted(),"delete owned split");
    auto deleted=registry.prepare(destroyed.profile,views.bank);check(registry.commit(std::move(deleted)),"delete commit retires identity");views=std::move(deleted.wire);
    check(!registry.item(created_wire),"removed identity cannot authorize a stale request");
    auto forged=views.character;auto copy=forged.items.at(bank1);copy.definition.id=created_wire;copy.slot=4;forged.items.emplace(created_wire,copy);
    rejects([&]{(void)registry.prepare(forged,views.bank);},"retired wire ID cannot be reused by a forged profile");
    forged=views.character;copy.definition.id=ids::ground_first;copy.slot=4;forged.items.emplace(copy.definition.id,copy);forged.next_item=ids::ground_first+1;
    rejects([&]{(void)registry.prepare(forged,views.bank);},"owned item cannot overlap ground pool");
    forged=views.character;copy.definition.id=ids::placeholder_first;copy.slot=4;forged.items.emplace(copy.definition.id,copy);forged.next_item=ids::placeholder_first+1;
    rejects([&]{(void)registry.prepare(forged,views.bank);},"placeholder never becomes an owned item");
    forged=views.character;copy.definition.id=registry.next_item()+1;copy.slot=4;forged.items.emplace(copy.definition.id,copy);forged.next_item=copy.definition.id+1;
    rejects([&]{(void)registry.prepare(forged,views.bank);},"new wire IDs cannot skip the allocator");
    auto duplicate_bank=views.bank;copy=views.character.items.at(bank1);copy.bag=bankbag;copy.slot=5;duplicate_bank.items.emplace(bank1,copy);
    rejects([&]{(void)registry.prepare(views.character,duplicate_bank);},"same wire item cannot be in two ownership domains");
    auto forged_bag=views.character;forged_bag.bags.at(charbag).id=bankbag;auto bagvalue=forged_bag.bags.at(charbag);forged_bag.bags.erase(charbag);forged_bag.bags.emplace(bankbag,bagvalue);for(auto& [id,value]:forged_bag.items)if(value.bag==charbag)value.bag=bankbag;
    rejects([&]{(void)registry.prepare(forged_bag,views.bank);},"bag cannot steal another domain's wire identity");

    auto stable=registry.prepare(views.character,views.bank),stale=registry.prepare(views.character,views.bank);
    check(registry.commit(std::move(stable))&&!registry.commit(std::move(stale)),"obsolete prepared revision cannot overwrite registry");
    ids::Session foreign;const auto foreign_views=foreign.hydrate(character(),bank());auto other=foreign.prepare(foreign_views.character,foreign_views.bank);
    check(!registry.commit(std::move(other)),"foreign session preparation cannot commit");
    const auto stable_revision=registry.revision();const auto stable_wire=registry.wire_item(ids::Domain::character,4);
    auto malformed=withdrawn.durable.character;malformed.items.begin()->second.definition.id=0;
    rejects([&]{registry.hydrate(malformed,withdrawn.durable.bank);},"malformed reload is rejected");
    check(registry.revision()==stable_revision&&registry.wire_item(ids::Domain::character,4)==stable_wire,"failed reload is atomic");

    ids::Session reconnect;const auto reconnect_views=reconnect.hydrate(cross_prepared.durable.character,cross_prepared.durable.bank);
    const auto reconnect_projection=reconnect.prepare(reconnect_views.character,reconnect_views.bank);
    check(inventory::encode(reconnect_projection.durable.character)==inventory::encode(cross_prepared.durable.character)&&inventory::encode(reconnect_projection.durable.bank)==inventory::encode(cross_prepared.durable.bank),"new connection restores durable transferred identities without persisting old wire IDs");
    auto large_logical=inventory::baseline();large_logical.items.emplace(ids::placeholder_first,item(ids::placeholder_first,401,1,0));large_logical.next_item=ids::placeholder_first+1;
    ids::Session large_session;const auto large_wire=large_session.hydrate(large_logical,storage::baseline());
    check(large_wire.character.items.begin()->first==ids::owned_first,"durable numeric placeholder collision is mapped to safe owned wire ID");
    check(large_session.prepare(large_wire.character,large_wire.bank).durable.character.items.contains(ids::placeholder_first),"durable high ID remains unchanged behind mapping");

    // Retiring logical identities on reload must not recycle client IDs. This
    // also exercises the final legal slot and failure before any publication.
    ids::Session exhaustion;auto logical=inventory::baseline(),empty_bank=storage::baseline();logical.items.emplace(1,item(1,301,1,0));logical.next_item=2;
    auto last=exhaustion.hydrate(logical,empty_bank);
    for(unsigned id=2;id<=ids::owned_last;++id){logical.items.clear();logical.items.emplace(id,item(id,301,1,0));logical.next_item=id+1;last=exhaustion.hydrate(logical,empty_bank);}
    check(last.character.items.begin()->first==ids::owned_last&&last.character.next_item==ids::owned_last+1,"last owned wire ID remains legal and disjoint from floor pool");
    const auto exhausted_revision=exhaustion.revision();logical.items.clear();logical.items.emplace(ids::owned_last+1,item(ids::owned_last+1,301,1,0));logical.next_item=ids::owned_last+2;
    rejects([&]{exhaustion.hydrate(logical,empty_bank);},"wire exhaustion rejects authoritative reload atomically");
    check(exhaustion.revision()==exhausted_revision&&exhaustion.wire_item(ids::Domain::character,ids::owned_last)==ids::owned_last,"exhaustion preserves old live mapping");
    auto exhausted_split=inventory::propose(last.character,{inventory::Change::split,ids::owned_last,25,*exhaustion.wire_bag(ids::Domain::character,1),1},{true,true});
    check(exhausted_split.accepted(),"logical planner alone cannot know session reserved range");
    rejects([&]{(void)exhaustion.prepare(exhausted_split.profile,last.bank);},"identity layer rejects exhausted split before durable CAS/publication");
    auto gold_character=inventory::baseline();auto gold_bank=fw::gw::storage::baseline();gold_character.gold=99999;gold_bank.gold=999999;
    ids::Session gold_session;auto gold_wire=gold_session.hydrate(gold_character,gold_bank);
    check(gold_wire.character.gold==99999&&gold_wire.bank.gold==999999,"wire projection preserves both ownership gold balances");
    --gold_wire.character.gold;++gold_wire.bank.gold;auto gold_prepared=gold_session.prepare(gold_wire.character,gold_wire.bank);
    check(gold_prepared.durable.character.gold==99998&&gold_prepared.durable.bank.gold==1000000,"reverse projection preserves conserved bank transfer balances");
    check(inventory::decode(inventory::encode(gold_prepared.durable.bank)).gold==1000000,"FWI4 maximum bank gold durable roundtrip");
    gold_wire.bank.gold=1000001;rejects([&]{(void)gold_session.prepare(gold_wire.character,gold_wire.bank);},"bank cap rejects before commit");
    gold_character.gold=100001;rejects([&]{(void)inventory::encode(gold_character);},"carried gold cap rejects persistence");
    // Real material merge/withdraw proposals use the same session projection
    // and independent logical namespaces as ordinary item transfers.
    auto material_character=character(),material_bank=bank();material_character.gold=1234;material_bank.gold=567890;material_bank.material_quota=2500;
    auto& carried_material=material_character.items.at(1);carried_material.definition.model=921;carried_material.definition.quantity=250;carried_material.definition.modifiers={0x25080000};
    auto& stored_material=material_bank.items.at(1);stored_material.definition.model=921;stored_material.definition.quantity=2250;stored_material.definition.modifiers={0x25080000};stored_material.bag=101;stored_material.slot=0;
    ids::Session material_ids;auto material_wire=material_ids.hydrate(material_character,material_bank);
    const auto source_wire=*material_ids.wire_item(ids::Domain::character,1),target_wire=*material_ids.wire_item(ids::Domain::bank,1);const auto material_bag=*material_ids.wire_bag(ids::Domain::bank,101);
    check(source_wire!=target_wire&&material_wire.bank.material_quota==2500,"same logical material1 remains distinct and reviewed quota survives hydration");
    auto forged_quota=material_wire.bank;forged_quota.material_quota=2250;const auto material_revision=material_ids.revision();
    rejects([&]{(void)material_ids.prepare(material_wire.character,forged_quota);},"item request cannot alter otherwise valid account material quota");
    check(material_ids.revision()==material_revision&&material_ids.item(source_wire)==ids::Origin{ids::Domain::character,1},"forged quota does not change live owner registry");
    const auto material_deposit=storage::transfer(material_wire.character,material_wire.bank,{inventory::Change::move,source_wire,0,material_bag,0},{true,true,true});check(material_deposit.accepted(),"material merge succeeds on translated session IDs");
    auto material_prepared=material_ids.prepare(material_deposit.character,material_deposit.bank);
    check(!material_prepared.durable.character.items.contains(1)&&material_prepared.durable.bank.items.at(1).definition.quantity==2500&&material_prepared.durable.bank.items.at(1).bag==101&&material_prepared.durable.bank.material_quota==2500,"merge keeps actual bank logical1 and exact material owner/slot/quota");
    check(material_prepared.durable.character.items.at(3).bag==0&&material_prepared.durable.character.bags.at(100).container==3&&material_prepared.durable.character.weapons==material_character.weapons&&material_prepared.durable.character.gold==1234&&material_prepared.durable.bank.gold==567890,"material transaction preserves installed container/weapon sets and both gold fields");
    check(material_ids.revision()==material_revision&&material_ids.item(source_wire)==ids::Origin{ids::Domain::character,1},"discarded material preparation after failed CAS preserves both origin mappings");
    check(material_ids.commit(std::move(material_prepared))&&!material_ids.item(source_wire)&&material_ids.item(target_wire)==ids::Origin{ids::Domain::bank,1},"committed full merge retires consumed wire ID and keeps survivor");material_wire=std::move(material_prepared.wire);
    const auto fresh_wire=material_ids.next_item();const auto carry_bag=*material_ids.wire_bag(ids::Domain::character,100);
    const auto material_withdrawal=storage::transfer(material_wire.character,material_wire.bank,{inventory::Change::move,target_wire,0,carry_bag,0},{true,true,true});check(material_withdrawal.accepted()&&material_withdrawal.character.items.at(fresh_wire).definition.quantity==250,"material2500 withdrawal allocates new250-unit wire stack");
    auto material_withdrawn=material_ids.prepare(material_withdrawal.character,material_withdrawal.bank);
    check(material_withdrawn.durable.character.items.at(4).definition.quantity==250&&material_withdrawn.durable.character.items.at(4).profession==carried_material.profession&&material_withdrawn.durable.character.items.at(4).customized==carried_material.customized&&material_withdrawn.durable.bank.items.at(1).definition.quantity==2250&&material_withdrawn.durable.bank.material_quota==2500,"partial withdrawal assigns fresh owner logical4 and retains exact metadata/quota/counts");
    check(material_ids.commit(std::move(material_withdrawn))&&material_ids.item(fresh_wire)==ids::Origin{ids::Domain::character,4}&&!material_ids.item(source_wire),"retired merged wire ID never reused by withdrawal");
    auto rehydrate_material=material_ids.hydrate(material_withdrawn.durable.character,material_withdrawn.durable.bank);check(material_ids.wire_item(ids::Domain::bank,1)==target_wire&&material_ids.wire_item(ids::Domain::character,4)==fresh_wire&&rehydrate_material.bank.material_quota==2500,"reload preserves surviving material identities and quota");
    ids::Session material_reconnect;const auto recovered_material=material_reconnect.hydrate(inventory::decode(inventory::encode(material_withdrawn.durable.character)),inventory::decode(inventory::encode(material_withdrawn.durable.bank)));const auto restored_material=material_reconnect.prepare(recovered_material.character,recovered_material.bank);
    check(inventory::encode(restored_material.durable.character)==inventory::encode(material_withdrawn.durable.character)&&inventory::encode(restored_material.durable.bank)==inventory::encode(material_withdrawn.durable.bank),"FWI5 reconnect roundtrips material contents/quota, currency, installed containers and weapon references");
    auto wrong_character=material_character;wrong_character.material_quota=500;rejects([&]{material_reconnect.hydrate(wrong_character,material_bank);},"account material quota cannot be attached to character ownership domain");
    std::cout<<"inventory identity tests passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
