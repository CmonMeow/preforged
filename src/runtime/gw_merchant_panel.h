#pragma once
#include "gw_merchant_transaction.h"
#include "gw_collector_panel.h"
#include "gw_merchant_sell_policy.h"
#include "gw_presearing_collectors.h"

namespace fw::gw::merchant::native {
// Sanura map148: captured modern353 definitions10556..10562, packet source
// pre-searing-side-quests-2026-10-12/checkpoint-1815/packets-43292-168891734.jsonl.
// Exact field/serializer fixtures: merchant-panel-2022.txt. Complete52044 lists
// independently prove the same order for Riley/Vencel/Gram, with all21 stock
// serializers matching. Sanura's older132 list itself remains incomplete.
// Owned files omit preview bit31.
inline std::vector<Stock> sanura_stock(){
 std::vector<Stock> out;
 {items::Definition d;d.id=1u;d.file=41031u;d.type=12u;d.unknown1=0u;d.dye=0u;d.materials=0u;d.unknown2=0u;d.interaction=536871425u;d.price=50u;d.model=957u;d.quantity=1u;d.encoded_name={8948,59277,37384,3232};d.modifiers={1657275136u};out.push_back({d,(d.interaction&0x80000)?250u:1u});}
 {items::Definition d;d.id=2u;d.file=15449u;d.type=24u;d.unknown1=3u;d.dye=0u;d.materials=0u;d.unknown2=0u;d.interaction=536875009u;d.price=0u;d.model=2822u;d.quantity=1u;d.encoded_name={9814,38279,61274,607};d.modifiers={2747794176u};out.push_back({d,(d.interaction&0x80000)?250u:1u});}
 {items::Definition d;d.id=3u;d.file=9383u;d.type=10u;d.unknown1=0u;d.dye=11u;d.materials=0u;d.unknown2=0u;d.interaction=554172929u;d.price=25u;d.model=918u;d.quantity=1u;d.encoded_name={8910,62060,62927,13357};d.modifiers={618138369u,619184139u};out.push_back({d,(d.interaction&0x80000)?250u:1u});}
 {items::Definition d;d.id=4u;d.file=79229u;d.type=29u;d.unknown1=0u;d.dye=0u;d.materials=0u;d.unknown2=0u;d.interaction=553648641u;d.price=20u;d.model=2993u;d.quantity=1u;d.encoded_name={10013,44808,49094,26605};d.modifiers={635962112u,609746954u};out.push_back({d,(d.interaction&0x80000)?250u:1u});}
 {items::Definition d;d.id=5u;d.file=130581u;d.type=29u;d.unknown1=0u;d.dye=0u;d.materials=0u;d.unknown2=0u;d.interaction=553648641u;d.price=50u;d.model=2989u;d.quantity=1u;d.encoded_name={10008,53131,41023,6395};d.modifiers={635961600u,609746969u};out.push_back({d,(d.interaction&0x80000)?250u:1u});}
 {items::Definition d;d.id=6u;d.file=112010u;d.type=21u;d.unknown1=0u;d.dye=0u;d.materials=0u;d.unknown2=0u;d.interaction=537395713u;d.price=100u;d.model=2110u;d.quantity=1u;d.encoded_name={9401,53264,55986,21848};d.modifiers={};out.push_back({d,(d.interaction&0x80000)?250u:1u});}
 {items::Definition d;d.id=7u;d.file=111997u;d.type=12u;d.unknown1=0u;d.dye=0u;d.materials=0u;d.unknown2=0u;d.interaction=536870913u;d.price=2u;d.model=1101u;d.quantity=1u;d.encoded_name={9049,46336,60056,8468};d.modifiers={1657274624u};out.push_back({d,(d.interaction&0x80000)?250u:1u});}
 // These three fixed, reviewed stock rows have no profession/attribute requirement.
 for(auto& row:out)if(row.definition.type==12||row.definition.type==24)row.reviewed_unrestricted=true;
 return out;
}
enum class StockEvidence {captured_definitions,captured_list,wiki_shared_stock};
struct Shop {unsigned map;std::string_view name;StockEvidence evidence;};
// Spence/Laren use identical wiki goods tables, not an individually captured
// panel. The native panel/transaction implementation is shared for all rows.
inline constexpr Shop shops[]={{148,"Sanura",StockEvidence::captured_definitions},
 {163,"Riley",StockEvidence::captured_list},{162,"Vencel",StockEvidence::captured_list},
 {166,"Gram",StockEvidence::captured_list},{164,"Spence",StockEvidence::wiki_shared_stock},
 {165,"Laren",StockEvidence::wiki_shared_stock}};
inline bool supported(unsigned map,std::string_view name){
 constexpr std::string_view suffix=" [Merchant]";
 if(name.ends_with(suffix))name.remove_suffix(suffix.size());
 return std::any_of(std::begin(shops),std::end(shops),[&](const auto& row){return row.map==map&&row.name==name;});
}
inline bool same_stock(items::Definition a,items::Definition b){
 a.id=b.id=1;a.quantity=b.quantity=1;
 return items::definition_2022(a)==items::definition_2022(b);
}
inline std::optional<inventory::Batch> open_panel(Binding& active,collectors::native::MenuIds& identities,
 const Observation& observed,std::string_view displayed,const inventory::Profile& owned){
 active={};
 if(!supported(observed.map,displayed)||!observed.actor||observed.actor>65535||!observed.world||
    !observed.npc_generation||!observed.nearby||observed.now>UINT64_MAX-120000)return {};
 auto stock=sanura_stock();
 if(identities.next<collectors::native::menu_first||identities.next>collectors::native::menu_last||
    stock.size()>collectors::native::menu_last-identities.next+1)return {};
 Binding pending;pending.actor=observed.actor;pending.map=observed.map;pending.world=observed.world;
 pending.npc_generation=observed.npc_generation;pending.revision=observed.revision;
 pending.deadline=observed.now+120000;pending.scale={1};
 pending.inventory_at_open=inventory::encode(owned);
 // Exact stock and seven canonical trophy signatures only. Do not value
 // arbitrary quest/customized/installed/equipped items from appearance.
 const auto trophies=collectors::presearing_offers();
 for(const auto& [id,item]:owned.items){const auto bag=owned.bags.find(item.bag);
  if(canonical_trophy_sell(owned,item,trophies)){pending.sellable.push_back(id);continue;}
  if(bag==owned.bags.end()||bag->second.type!=1||(item.customized&&!item.customized->empty())||(item.profession&&*item.profession)||item.reviewed_armor)continue;
  // Wiki explicitly excludes Flute and Starter Shield from sales, even
  // though the captured Flute definition has positive native value2.
  for(const auto& row:stock)if(row.definition.model!=1101&&row.definition.model!=2822&&same_stock(item.definition,row.definition)){
   pending.sellable.push_back(id);break;}}
 inventory::Batch packets;items::Packet owner;items::put(owner,195,2);items::put(owner,observed.actor,4);packets.push_back(std::move(owner));
 unsigned next=identities.next;items::Packet list;items::put(list,131,2);items::put(list,static_cast<unsigned>(stock.size()),2);
 for(auto& row:stock){row.definition.id=next++;auto preview=row.definition;preview.file|=0x80000000u;
  packets.push_back(items::definition_2022(preview));items::put(list,row.definition.id,4);}
 packets.push_back(std::move(list));
 items::Packet buy;items::put(buy,201,2);items::put(buy,1,1);items::put(buy,0x3f800000,4);packets.push_back(std::move(buy));
 items::Packet sell;items::put(sell,194,2);items::put(sell,11,1);items::put(sell,0,4);packets.push_back(std::move(sell));
 pending.stock=std::move(stock);identities.next=next;active=std::move(pending);return packets;
}
}
