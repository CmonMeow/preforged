#include "gw_merchant_panel.h"
#include "gw_merchant_exchange.h"
#include "gw_storage.h"
#include "gw_equipment_metadata_provider.h"
#include <iostream>
int main()try{
 namespace m=fw::gw::merchant::native;namespace inv=fw::gw::inventory;
 const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 auto durable=inv::baseline();durable.gold=500;inv::identity::Session identities;
 auto wire=identities.hydrate(durable,fw::gw::storage::baseline());std::uint64_t revision=10;
 m::Observation observed{10012,148,0,99,7,10,500,true};
 fw::gw::collectors::native::MenuIds ids;m::Binding menu;
 check(m::open_panel(menu,ids,observed,"Sanura",wire.character).has_value(),"panel unavailable");
 auto stock=menu.stock;
 m::Request request{75,1,100,0,{},{{stock[0].definition.id,1}}};
 auto plan=m::authorize(menu,request,wire.character,observed);check(bool(plan),"buy admission failed");
 auto changed_stock=stock;changed_stock[0].definition.price=1;
 check(!m::propose(*plan,wire.character,10,changed_stock,u"Tester"),"changed stock after admission accepted");
 changed_stock=stock;changed_stock[0].reviewed_unrestricted=false;
 check(!m::propose(*plan,wire.character,10,changed_stock,u"Tester"),"changed issuance metadata after admission accepted");
 const auto before=inv::encode(wire.character),bank_before=inv::encode(wire.bank);const auto identity_revision=identities.revision();unsigned saves=0;
 auto result=m::commit(*plan,identities,wire.character,wire.bank,revision,stock,u"Tester",
  [&](const inv::Profile& proposed,std::uint64_t expected)->std::optional<std::uint64_t>{++saves;
   check(expected==10&&proposed.gold==400&&proposed.items.size()==1,"gold and item not in same durable proposal");return {};});
 check(!result&&saves==1&&revision==10&&identities.revision()==identity_revision&&inv::encode(wire.character)==before&&inv::encode(wire.bank)==bank_before,"failed CAS mutated live state");
 result=m::commit(*plan,identities,wire.character,wire.bank,revision,stock,u"Tester",
  [&](const inv::Profile& proposed,std::uint64_t expected)->std::optional<std::uint64_t>{++saves;
   check(expected==10&&proposed.gold==400&&proposed.items.size()==1,"successful durable proposal lost debit/reward");return 11;});
 check(result&&wire.character.gold==400&&wire.character.items.size()==1&&revision==11,"successful purchase not installed");
 check(!m::commit(*plan,identities,wire.character,wire.bank,revision,stock,u"Tester",
  [&](const inv::Profile&,std::uint64_t)->std::optional<std::uint64_t>{throw std::runtime_error("replay reached CAS");}),"purchase replay succeeded");
 const auto purchased=wire.character.items.begin()->first;
 auto persisted=inv::decode(inv::encode(wire.character));
 check(persisted.items.at(purchased).profession==0&&persisted.items.at(purchased).customized==u"","trusted purchase metadata absent after durable roundtrip");
 fw::gw::equipment::proposal::Policy equipment;equipment.allowed=true;equipment.primary=6;equipment.character="Tester";
 equipment.reviewed=[&](const fw::gw::items::Definition& d){return fw::gw::equipment::metadata::reviewed(persisted,d);};
 inv::Request equip;equip.change=inv::Change::equip;equip.item=purchased;
 const auto equipped=fw::gw::equipment::proposal::propose(persisted,equip,equipment);
 check(equipped.accepted(),"actual bought Focus cannot equip through strict metadata authority");
 persisted=inv::decode(inv::encode(equipped.profile));
 inv::Request unequip;unequip.change=inv::Change::unequip;unequip.equipment_slot=1;unequip.bag=1;unequip.slot=0;
 const auto removed=fw::gw::equipment::proposal::propose(persisted,unequip,equipment);
 check(removed.accepted(),"bought Focus cannot unequip after persistence");
 wire.character=inv::decode(inv::encode(removed.profile));observed.revision=revision;
 check(m::open_panel(menu,ids,observed,"Sanura",wire.character).has_value()&&menu.sellable==std::vector<unsigned>{purchased},"purchased exact item cannot be sold");
 stock=menu.stock;request={72,11,0,50,{{purchased,0}}, {}};
 plan=m::authorize(menu,request,wire.character,observed);check(bool(plan),"whole sell admission failed");
 result=m::commit(*plan,identities,wire.character,wire.bank,revision,stock,u"Tester",
  [&](const inv::Profile& proposed,std::uint64_t expected)->std::optional<std::uint64_t>{check(expected==11&&proposed.gold==450&&proposed.items.empty(),"sell item and gold not atomically persisted");return 12;});
 check(result&&wire.character.items.empty()&&wire.character.gold==450&&revision==12,"sell failed");
 // Both remaining no-requirement stock rows receive the same explicit
 // issuance facts; wiki sale restrictions do not prevent buying/equipping.
 for(unsigned row:{1u,6u}){auto owned=inv::baseline();owned.gold=100;
  observed.revision=revision;check(m::open_panel(menu,ids,observed,"Sanura",owned).has_value(),"offhand panel unavailable");
  const auto goods=menu.stock;const auto price=fw::gw::merchant::buy_quote(goods[row].definition,menu.scale,1);
  request={75,1,price->gold,0,{},{{goods[row].definition.id,1}}};
  const auto admitted=m::authorize(menu,request,owned,observed);check(bool(admitted),"offhand buy admission failed");
  const auto bought=m::propose(*admitted,owned,revision,goods,u"Tester");check(bool(bought),"offhand buy proposal failed");
  auto saved=inv::decode(inv::encode(bought->profile));const auto id=saved.items.begin()->first;
  equipment.reviewed=[&](const fw::gw::items::Definition& d){return fw::gw::equipment::metadata::reviewed(saved,d);};
  equip.item=id;check(fw::gw::equipment::proposal::propose(saved,equip,equipment).accepted(),"bought Shield/Flute cannot equip");
 }
 // Capacity failure must not debit any gold or invoke persistence.
 auto full=wire.character;full.bags.at(1).capacity=1;auto filler=m::sanura_stock()[0].definition;
 filler.id=full.next_item++;inv::Item item;item.definition=filler;item.bag=1;full.items.emplace(filler.id,item);
 observed.revision=12;check(m::open_panel(menu,ids,observed,"Sanura",full).has_value(),"full inventory panel failed");
 stock=menu.stock;request={75,1,100,0,{},{{stock[0].definition.id,1}}};plan=m::authorize(menu,request,full,observed);
 const auto full_before=inv::encode(full);
 check(plan&&!m::propose(*plan,full,12,stock,u"Tester")&&inv::encode(full)==full_before,"full inventory leaked gold debit");
 // A captured Spider Leg signature supplies trusted value3, independently of
 // the seven purchasable stock definitions. Exercise real selected loot sales.
 auto loot=inv::baseline();loot.gold=100;loot.next_item=2;inv::Item trophy;
 const auto catalog=fw::gw::collectors::presearing_offers();
 const auto spider=std::find_if(catalog.begin(),catalog.end(),[](const auto& offer){return offer.trophy.model==422;});
 check(spider!=catalog.end(),"captured Spider Leg source missing");
 trophy.definition=spider->trophy;trophy.definition.id=1;trophy.definition.quantity=5;trophy.bag=1;
 loot.items.emplace(1,trophy);inv::identity::Session loot_ids;
 auto projected=loot_ids.hydrate(loot,fw::gw::storage::baseline());std::uint64_t loot_revision=10;
 auto at=observed;at.revision=10;fw::gw::collectors::native::MenuIds loot_menu_ids;
 const auto open=[&]{at.revision=loot_revision;return m::open_panel(menu,loot_menu_ids,at,"Spence [Merchant]",projected.character);};
 at.map=164;check(open().has_value(),"loot merchant panel missing");stock=menu.stock;
 const auto wire_item=projected.character.items.begin()->first;
 request={75,11,0,6,{{wire_item,2}}, {}};
 plan=m::authorize(menu,request,projected.character,at);check(bool(plan),"partial canonical loot sale rejected");
 const auto loot_before=inv::encode(projected.character),loot_bank=inv::encode(projected.bank);
 const auto registry_before=loot_ids.revision();unsigned loot_saves=0;
 result=m::commit(*plan,loot_ids,projected.character,projected.bank,loot_revision,stock,u"Tester",
  [&](const inv::Profile& proposed,std::uint64_t expected)->std::optional<std::uint64_t>{++loot_saves;
   check(expected==10&&proposed.gold==106&&proposed.items.begin()->second.definition.quantity==3,"partial debit and gold separated");return {};});
 check(!result&&loot_saves==1&&loot_revision==10&&loot_ids.revision()==registry_before&&
       inv::encode(projected.character)==loot_before&&inv::encode(projected.bank)==loot_bank,"failed loot CAS changed projection");
 result=m::commit(*plan,loot_ids,projected.character,projected.bank,loot_revision,stock,u"Tester",
  [&](const inv::Profile&,std::uint64_t)->std::optional<std::uint64_t>{++loot_saves;return 11;});
 check(result&&projected.character.gold==106&&projected.character.items.at(wire_item).definition.quantity==3,"partial loot sale not installed");
 check(!m::commit(*plan,loot_ids,projected.character,projected.bank,loot_revision,stock,u"Tester",
  [&](const inv::Profile&,std::uint64_t)->std::optional<std::uint64_t>{throw std::runtime_error("old loot plan reached persistence");}),"loot Plan replay succeeded");
 // The wire has no sell nonce: the same partial request under a NEW opening
 // remains an intentional supported second sale. Do not claim global dedup.
 check(open().has_value()&&m::authorize(menu,request,projected.character,at).has_value(),
       "fresh authorization cannot repeat an intentional partial sale");
 check(open().has_value(),"reopen after loot sale missing");
 auto capped=projected.character;capped.gold=99999;
 check(m::open_panel(menu,loot_menu_ids,at,"Spence",capped).has_value(),"gold cap menu missing");
 check(!m::authorize(menu,request,capped,at)&&!menu.actor,"loot sale exceeded carried gold cap");
 check(open().has_value(),"cancel setup missing");
 check(!m::open_panel(menu,loot_menu_ids,at,"Other NPC",projected.character)&&!menu.actor&&
       !m::authorize(menu,request,projected.character,at),"NPC change retained loot sell authority");
 check(open().has_value(),"whole loot setup missing");stock=menu.stock;
 request={72,11,0,9,{{wire_item,0}}, {}};
 plan=m::authorize(menu,request,projected.character,at);check(bool(plan),"whole remaining loot sale rejected");
 result=m::commit(*plan,loot_ids,projected.character,projected.bank,loot_revision,stock,u"Tester",
  [&](const inv::Profile& proposed,std::uint64_t expected)->std::optional<std::uint64_t>{
   check(expected==11&&proposed.gold==115&&proposed.items.empty(),"whole loot durable projection incorrect");return 12;});
 check(result&&projected.character.gold==115&&projected.character.items.empty()&&inv::encode(projected.bank)==loot_bank,
       "whole loot sale did not remove exact item or preserve bank");
 std::cout<<"PASS merchant buy/sell durable item+gold CAS, failure rollback, replay and capacity guards\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}

