#include "../src/runtime/gw_crafter_panel.h"
#include <fstream>
#include <iostream>
#include <tuple>
int main(int argc,char** argv)try{
 const auto check=[](bool v,const char* why){if(!v)throw std::runtime_error(why);};check(argc==2,"expected original panel fixture");std::ifstream input(argv[1]);std::string key,hex;std::vector<fw::gw::items::Packet> expected;
 while(input>>key>>hex){fw::gw::items::Packet packet;check(hex.size()%2==0,"invalid fixture");for(std::size_t i=0;i<hex.size();i+=2)packet.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));expected.push_back(std::move(packet));}
 namespace c=fw::gw::crafter;namespace inv=fw::gw::inventory;auto owned=inv::baseline();owned.gold=500;c::Binding binding;fw::gw::collectors::native::MenuIds ids;const c::Observation at{10007,148,0,99,7,10,500,true};
 const auto panel=c::open_panel(binding,ids,at,"Arthur Ayala [Weapons]",owned);check(panel&&*panel==expected&&panel->size()==16&&binding.recipes.size()==13,"original native13recipe panel bytes differ");
 const auto old=binding.recipes.front().reward.definition.id;check(ids.next==60018,"menu cursor not persistent");
 for(const auto& row:binding.recipes)check(row.reward.profession==0&&row.reward.customized&&row.reward.customized->empty()&&c::valid_recipe(row),"wiki unrestricted issuance or recipe identity missing");
 check(c::open_panel(binding,ids,at,"Arthur Ayala",owned).has_value()&&binding.recipes.front().reward.definition.id!=old,"reopen reused IDs");
 c::Request replay{75,3,100,0,{{43,1}},{{old,1}}};check(!c::authorize(binding,replay,owned,at),"old catalog ID admitted");
 auto wrong=at;wrong.map=146;check(!c::open_panel(binding,ids,wrong,"Arthur Ayala",owned)&&!binding.observed.actor,"wrongmap crafter activated");
 check(!c::open_panel(binding,ids,at,"Elias [Weapons]",owned),"unknown crafter activated");
 check(c::open_panel(binding,ids,at,"Arthur Ayala",owned).has_value(),"reopen failed");
 check(!c::authorize_wire(binding,fw::gw::items::Packet{75,0},owned,at)&&!binding.observed.actor,"malformed native frame retained binding");
 // Exact native75 descriptor framing, selected stack marker1, and the
 // same trusted catalog used by the normal NPC path.
 for(auto [id,model,element,slot]:{std::tuple{43u,948u,1u,0u},std::tuple{44u,946u,6u,1u}}){inv::Item item;item.definition.id=id;item.definition.file=1234;item.definition.type=11;item.definition.model=model;item.definition.quantity=10;item.definition.interaction=0x80001;item.definition.modifiers={0x25080000u|(element<<8)};item.bag=1;item.slot=slot;owned.items.emplace(id,item);}owned.next_item=100;
 check(c::open_panel(binding,ids,at,"Arthur Ayala",owned).has_value(),"inventory-specific opening failed");
 fw::gw::items::Packet wire;const auto put=[&](unsigned value,unsigned width){fw::gw::items::put(wire,value,width);};
 put(75,2);put(3,1);put(100,4);put(2,2);put(43,4);put(44,4);put(2,2);put(1,1);put(1,1);put(0,4);put(1,2);put(binding.recipes.front().reward.definition.id,4);put(1,2);put(1,1);
 const auto admitted=c::authorize_wire(binding,wire,owned,at);check(bool(admitted)&&!binding.observed.actor,"native75 catalog selection rejected");const auto transaction=c::propose(*admitted,owned,10);
 check(transaction&&transaction->profile.gold==400&&transaction->profile.items.at(43).definition.quantity==6&&transaction->profile.items.at(44).definition.quantity==9,"wire request did not derive exact recipe debit");
 const auto reward=transaction->profile.items.at(100);check(reward.profession==0&&reward.customized&&reward.customized->empty(),"normal catalog lost explicit uncustomized metadata");
 std::cout<<"PASS original native13recipe panel bytes, wiki metadata, scope and freshIDs\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
