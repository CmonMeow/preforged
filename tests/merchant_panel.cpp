#include "gw_merchant_panel.h"
#include <fstream>
#include <iostream>
#include <map>
int main(int argc,char** argv)try{
 namespace m=fw::gw::merchant::native;namespace inv=fw::gw::inventory;
 const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 check(argc==2,"original merchant panel fixtures required");
 std::ifstream input(argv[1]);std::map<std::string,std::string> fixtures;std::string key,hex;
 while(input>>key>>hex)fixtures[key]=hex;
 const auto wire=[](fw::gw::items::Packet packet){packet[1]|=0x80;std::string result;constexpr char digits[]="0123456789abcdef";
  for(auto value:packet){result+=digits[value>>4];result+=digits[value&15];}return result;};
 auto owned=inv::baseline();owned.gold=500;m::Binding menu;fw::gw::collectors::native::MenuIds ids;
 const m::Observation observed{10012,148,0,99,7,10,500,true};
 const auto panel=m::open_panel(menu,ids,observed,"Sanura [Merchant]",owned);
 check(panel&&panel->size()==11&&menu.stock.size()==7&&ids.next==60012,"merchant panel shape");
 check(wire(panel->at(0))==fixtures.at("owner")&&wire(panel->at(8))==fixtures.at("list")&&
       wire(panel->at(9))==fixtures.at("buy")&&wire(panel->at(10))==fixtures.at("sell"),"original merchant panel framing mismatch");
 for(unsigned i=0;i<7;++i)check(wire(panel->at(i+1))==fixtures.at("stock"+std::to_string(60005+i)),"captured stock definition mismatch");
 const auto old_buy=menu.stock.front().definition.id;
 check(m::open_panel(menu,ids,observed,"Sanura",owned).has_value(),"reopen unavailable");
 m::Request replay{75,1,100,0,{},{{old_buy,1}}};
 check(!m::authorize(menu,replay,owned,observed)&&!menu.actor,"old buy menu ID authorized after reopen");
 check(m::open_panel(menu,ids,observed,"Sanura",owned).has_value(),"current ID setup unavailable");
 replay.receive[0].item=menu.stock.front().definition.id;
 check(m::authorize(menu,replay,owned,observed).has_value(),"current buy ID control rejected");
 for(const auto& shop:m::shops){auto at=observed;at.map=shop.map;
  fw::gw::collectors::native::MenuIds fresh_ids;
  const auto shared=m::open_panel(menu,fresh_ids,at,std::string(shop.name)+" [Merchant]",owned);
  check(shared&&shared->size()==11,"table-bound merchant panel unavailable");
  for(unsigned i=0;i<7;++i)check(wire(shared->at(i+1))==fixtures.at("stock"+std::to_string(60005+i)),"table-bound merchant stock diverged");
  constexpr unsigned costs[]{100,0,50,40,100,200,4};
  for(unsigned i=0;i<7;++i){const auto quote=fw::gw::merchant::buy_quote(menu.stock[i].definition,menu.scale,1);
   check(quote&&quote->gold==costs[i],"shared merchant quote differs from wiki goods costs");}
  if(shop.map==164||shop.map==165)check(shop.evidence==m::StockEvidence::wiki_shared_stock,"wiki-only merchant mislabeled captured");
  auto wrong=at;wrong.map=145;
  check(!m::open_panel(menu,fresh_ids,wrong,shop.name,owned),"merchant name accepted on unbound map");}
 check(menu.sellable.empty(),"invented owned sell stock");
 inv::Item shield;shield.definition=m::sanura_stock()[0].definition;shield.definition.id=1;shield.bag=1;
 owned.items.emplace(1,shield);owned.next_item=2;
 check(m::open_panel(menu,ids,observed,"Sanura",owned)&&menu.sellable==std::vector<unsigned>{1},"exact nonstackable stock not sellable");
 auto changed=owned;changed.items.at(1).definition.price=999;
 check(m::open_panel(menu,ids,observed,"Sanura",changed)&&menu.sellable.empty(),"appearance-only item got sell authority");
 changed=owned;changed.items.at(1).customized=u"Tester";
 check(m::open_panel(menu,ids,observed,"Sanura",changed)&&menu.sellable.empty(),"customized stock admitted by bounded sell policy");
 const auto cursor=ids.next;auto invalid=observed;invalid.map=146;
 check(!m::open_panel(menu,ids,invalid,"Sanura",owned)&&!menu.actor&&ids.next==cursor,"wrong map panel admitted");
 invalid=observed;invalid.nearby=false;
 check(!m::open_panel(menu,ids,invalid,"Sanura",owned),"out of range panel admitted");
 check(!m::open_panel(menu,ids,observed,"Other Merchant",owned),"unsupported merchant swallowed fallback");
 const auto catalog=fw::gw::collectors::presearing_offers();
 for(unsigned model:{429u,433u,432u,428u,424u,427u,422u}){
  const auto source=std::find_if(catalog.begin(),catalog.end(),[&](const auto& offer){return offer.trophy.model==model;});
  check(source!=catalog.end(),"canonical trophy source missing");
  auto loot=inv::baseline();loot.next_item=2;inv::Item drop;drop.definition=source->trophy;
  drop.definition.id=1;drop.definition.quantity=5;drop.bag=1;loot.items.emplace(1,drop);
  check(m::canonical_trophy_sell(loot,loot.items.at(1),catalog),"captured canonical trophy rejected");
  check(m::open_panel(menu,ids,observed,"Sanura",loot)&&menu.sellable==std::vector<unsigned>{1},"canonical loot absent from sell authority");
  for(unsigned field=0;field<6;++field){auto forged=loot;
   auto& definition=forged.items.at(1).definition;
   switch(field){case 0:++definition.file;break;case 1:++definition.price;break;case 2:definition.interaction^=0x100;break;
    case 3:definition.encoded_name.push_back(1);break;case 4:definition.dye^=1;break;case 5:definition.modifiers.push_back(0x62c80500);break;}
   check(!m::canonical_trophy_sell(forged,forged.items.at(1),catalog),"changed trophy signature authorized sale");
  }
  auto restricted=loot;restricted.items.at(1).customized=u"Tester";
  check(!m::canonical_trophy_sell(restricted,restricted.items.at(1),catalog),"unsupported customized trophy admitted");
  restricted=loot;restricted.items.at(1).profession=6;
  check(!m::canonical_trophy_sell(restricted,restricted.items.at(1),catalog),"unexpected trophy profession admitted");
  restricted=loot;restricted.items.at(1).bag=2;
  check(!m::canonical_trophy_sell(restricted,restricted.items.at(1),catalog),"equipped trophy admitted");
  restricted=loot;restricted.bags.at(1).container=1;
  check(!m::canonical_trophy_sell(restricted,restricted.items.at(1),catalog),"installed container admitted");
  restricted=loot;restricted.items.at(1).definition.quantity=251;
  check(!m::canonical_trophy_sell(restricted,restricted.items.at(1),catalog),"unsupported trophy stack bound ignored");
  auto changed=loot;changed.items.at(1).definition.price=4;
  m::Request forged_quote{75,11,0,8,{{1,2}}, {}};
  check(!m::authorize(menu,forged_quote,changed,observed)&&!menu.actor,"changed trophy retained old opening authority");
  check(m::open_panel(menu,ids,observed,"Sanura",changed)&&menu.sellable.empty(),"changed trophy acquired new authority");
 }
 // Positive captured native value is not permission to sell this wiki-excluded item.
 for(unsigned row:{1u,6u}){auto excluded=fw::gw::inventory::baseline();fw::gw::inventory::Item item;
  item.definition=m::sanura_stock()[row].definition;item.definition.id=1;item.bag=1;
  item.profession=0;item.customized=u"";excluded.items.emplace(1,item);excluded.next_item=2;
  check(m::open_panel(menu,ids,observed,"Sanura",excluded)&&menu.sellable.empty(),"wiki-excluded Shield/Flute admitted for sale");
  m::Request sell{72,11,0,item.definition.price,{{1,0}}, {}};
  check(!m::authorize(menu,sell,excluded,observed),"positive-value Flute sale authorized");
 }
 std::cout<<"PASS original merchant owner/definitions/list/buy/sell fixtures and exact private stock policy\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
