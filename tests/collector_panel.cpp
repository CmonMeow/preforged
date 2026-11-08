#include "gw_collector_panel.h"
#include "gw_presearing_collectors.h"
#include <fstream>
#include <iostream>
#include <map>

int main(int argc,char** argv)try{
 namespace n=fw::gw::collectors::native;namespace c=fw::gw::collectors;namespace inv=fw::gw::inventory;
 const auto check=[](bool ok,const char* why){if(!ok)throw std::runtime_error(why);};
 check(argc==2,"original collector panel fixture path required");
 std::ifstream input(argv[1]);std::map<std::string,std::string> fixtures;std::string key,hex;
 while(input>>key>>hex)fixtures.emplace(key,hex);check(fixtures.size()==6,"six native fixtures required");
 const auto wire=[](fw::gw::items::Packet packet){packet[1]|=0x80;std::string result;constexpr char digits[]="0123456789abcdef";
  for(auto value:packet){result+=digits[value>>4];result+=digits[value&15];}return result;};
 const auto offers=c::presearing_offers();n::MenuIds ids;n::Binding active;
 const n::Observation observed{10012,146,6,99,7,10,500,true};
 const auto panel=n::open_panel(active,ids,offers,observed,"Brownlow [Collector]");
 check(panel&&panel->packets.size()==6&&active.offers.size()==2&&ids.next==60008,"native panel shape/IDs");
 const char* labels[]{"owner","cost","pouch","egg","list","heading"};
 for(unsigned i=0;i<6;++i)check(wire(panel->packets[i])==fixtures.at(labels[i]),"collector panel differs from original serializer");
 check(panel->menu_items==std::vector<unsigned>{60005,60006,60007}&&
       active.offers[0]==std::pair{60006u,offers[0].key},"cost-first menu mapping");
 const auto again=n::open_panel(active,ids,offers,observed,"Brownlow");
 check(again&&again->menu_items.front()==60008&&ids.next==60011,"reopening reused private menu identities");
 auto profile=inv::baseline();inv::Item trophy;trophy.definition=offers[0].trophy;trophy.definition.id=1;
 trophy.definition.quantity=5;trophy.bag=1;profile.items.emplace(1,trophy);profile.next_item=2;
 n::Request request{72,2,0,0,{{1,0}},{{60006,0}}};const auto fresh=active;
 check(!n::authorize(active,request,offers,profile,observed),"old panel reward ID accepted after reopening");
 active=fresh;request.receive[0].item=60009;
 const auto plan=n::authorize(active,request,offers,profile,observed);
 const auto reward=plan?n::exchange_selected(*plan,profile,146,6,10,u"Tester"):std::nullopt;
 check(reward&&reward->profile.items.at(reward->reward).definition.file==offers[0].reward.definition.file,
       "native selected menu reward did not bind trusted collector recipe");
 n::MenuIds exhausted{65534};active=fresh;
 check(!n::open_panel(active,exhausted,offers,observed,"Brownlow")&&exhausted.next==65534&&!active.actor,
       "pool exhaustion allocated partial panel or retained stale menu");
 const auto cursor=ids.next;auto invalid=observed;invalid.nearby=false;
 check(!n::open_panel(active,ids,offers,invalid,"Brownlow")&&ids.next==cursor,"out-of-range panel opened");
 auto changed=offers;changed[0].cost=4;changed[1].cost=4;
 check(!n::open_panel(active,ids,changed,observed,"Brownlow")&&ids.next==cursor,"uncaptured heading/recipe mismatch accepted");
 for(const auto& [name,map]:{std::pair{"Jacobs",146u},{"Kaylee  [Collector]",146u},{"Walden",146u},
                            {"Savich",161u},{"Humphreys",146u}}){auto at=observed;at.map=map;
  const auto captured=n::open_panel(active,ids,offers,at,name);check(bool(captured),"captured collector panel unavailable");
  check(captured->menu_items.size()==active.offers.size()+1,"cost template included in selectable reward mapping");}
 auto other=observed;other.map=161;other.profession=1;
 check(!n::open_panel(active,ids,offers,other,"Savich"),"uncaptured Savich profession panel invented");
 std::cout<<"PASS original collector panel fixtures, private menu identities, trusted exchange and admission limits\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
